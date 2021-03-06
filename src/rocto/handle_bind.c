/****************************************************************
 *								*
 * Copyright (c) 2019 YottaDB LLC and/or its subsidiaries.	*
 * All rights reserved.						*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libyottadb.h>

#include "octo.h"
#include "octo_types.h"
#include "rocto.h"
#include "message_formats.h"
#include "helpers.h"

int handle_bind(Bind *bind, RoctoSession *session) {
	// At the moment, we don't have "bound function"
	// This feature should be implemented before 1.0
	// For now, just a search-and-replace of anything starting with a '$'
	// This is not super great because it means one could have a SQLI attack
	ydb_buffer_t *src_subs, *dest_subs, sql_expression;
	ydb_buffer_t z_status, z_status_value;
	int status, bind_parm;
	char *ptr, *end_ptr, new_query[MAX_STR_CONST];
	char *int_start, *new_query_ptr, *end_new_query_ptr;
	char c;
	BindComplete *response;
	ErrorResponse *err;

	TRACE(ERR_ENTERING_FUNCTION, "handle_bind");

	// zstatus buffers
	YDB_LITERAL_TO_BUFFER("$ZSTATUS", &z_status);

	// Fetch the named SQL query from the session ^session(id, "prepared", <name>)
	src_subs = make_buffers(config->global_names.session, 3, session->session_id->buf_addr, "prepared", bind->source);
	YDB_MALLOC_BUFFER(&sql_expression, MAX_STR_CONST);
	sql_expression.len_alloc -= 1;		// Leave space for null terminator

	status = ydb_get_s(&src_subs[0], 3, &src_subs[1], &sql_expression);
	if(status == YDB_ERR_GVUNDEF) {
		err = make_error_response(PSQL_Error_ERROR,
				    PSQL_Code_Invalid_Sql_Statement_Name,
				    "Bind to unknown query attempted", 0);
		send_message(session, (BaseMessage*)(&err->type));
		free_error_response(err);
		free(sql_expression.buf_addr);
		return -1;
	}
	// Handle other errors; this will crash the process with an error, which should be OK
	YDB_ERROR_CHECK(status, &z_status, &z_status_value);

	// Scan through and calculate a new length for the query
	ptr = sql_expression.buf_addr;
	end_ptr = ptr + sql_expression.len_used;
	new_query_ptr = new_query;
	end_new_query_ptr = new_query + MAX_STR_CONST;
	while(ptr < end_ptr) {
		if(*ptr == '$') {
			// Extract the parameter number, then paste in the new value
			int_start = ++ptr;
			while(ptr < end_ptr && *ptr >= 48 && *ptr <= 57) {
				ptr++;
			}
			c = *ptr;
			*ptr = '\0';
			bind_parm = atoi(int_start);
			*ptr = c;
			if(bind_parm == -1) {
				// Special case of a "NULL" value, which we represent as an empty string.
				// This is technically incorrect, but life goes on
				*new_query_ptr++ = '"';
				*new_query_ptr++ = '"';
			} else {
				// Copy in the value; if the type is a string, wrap it in
				//  quotes, otherwise just place it
				if(bind_parm >= bind->num_parms || bind_parm < 0) {
					err = make_error_response(PSQL_Error_ERROR,
								  PSQL_Code_Syntax_Error,
								  "wrong number of parameters for prepared statement",
								  0);
					send_message(session, (BaseMessage*)(&err->type));
					free_error_response(err);
					free(sql_expression.buf_addr);
					free(z_status_value.buf_addr);
					return -1;
				}
				assert(bind_parm < bind->num_parms);
				// Init prepared statement
				*new_query_ptr++ = '"';
				// Copy value
				if (0 == bind->num_parm_format_codes) {
					new_query_ptr = copy_text_parameter(session, bind, bind_parm, new_query_ptr, end_new_query_ptr);
				} else if (1 == bind->num_parm_format_codes) {
					if (0 == bind->parm_format_codes[0]) {
						new_query_ptr = copy_text_parameter(session, bind, bind_parm, new_query_ptr, end_new_query_ptr);
					} else {
						new_query_ptr = copy_binary_parameter(session, bind, bind_parm, new_query_ptr, end_new_query_ptr);
					}
				} else {
					if (0 == bind->parm_format_codes[bind_parm]) {
						new_query_ptr = copy_text_parameter(session, bind, bind_parm, new_query_ptr, end_new_query_ptr);
					} else {
						new_query_ptr = copy_binary_parameter(session, bind, bind_parm, new_query_ptr, end_new_query_ptr);
					}
				}
				if (NULL == new_query_ptr) {
					free(sql_expression.buf_addr);
					return -1;
				}
				// End copy value
				*new_query_ptr++ = '"';
				// End prepared statement
			}
		} else {
			*new_query_ptr++ = *ptr++;
		}
		assert(new_query_ptr < end_new_query_ptr);
	}

	// Now we store the bound statement in a global to execute ^session(session_id, "bound", <bound name>)
	dest_subs = make_buffers(config->global_names.session, 3, session->session_id->buf_addr, "bound", bind->dest);
	free(sql_expression.buf_addr);

	sql_expression.buf_addr = new_query;
	sql_expression.len_alloc = MAX_STR_CONST;
	sql_expression.len_used = new_query_ptr - new_query;

	// status = ydb_set_s(&session_global, 3, dest_subs, &sql_expression);
	status = ydb_set_s(&dest_subs[0], 3, &dest_subs[1], &sql_expression);
	YDB_ERROR_CHECK(status, &z_status, &z_status_value);

	// Construct the response message
	response = make_bind_complete();
	send_message(session, (BaseMessage*)response);
	free(response);

	return 0;
}
