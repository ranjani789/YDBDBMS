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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "octo.h"
#include "octo_types.h"
#include "helpers.h"

void cleanup_tables() {
	int status;
	char buffer[MAX_STR_CONST];
	ydb_buffer_t *loaded_schemas_b = make_buffers("%ydboctoloadedschemas", 2, "", "chunk");
	ydb_buffer_t result_b;
	result_b.buf_addr = buffer;
	result_b.len_alloc = result_b.len_used = sizeof(buffer);
	ydb_buffer_t z_status, z_status_value;
	YDB_MALLOC_BUFFER(&loaded_schemas_b[1], MAX_STR_CONST);

	while(TRUE) {
		status = ydb_subscript_next_s(loaded_schemas_b, 1, &loaded_schemas_b[1], &loaded_schemas_b[1]);
		if(status == YDB_ERR_NODEEND) {
			break;
		}
		YDB_ERROR_CHECK(status, &z_status, &z_status_value);
		status = ydb_get_s(loaded_schemas_b, 2, &loaded_schemas_b[1], &result_b);
		YDB_ERROR_CHECK(status, &z_status, &z_status_value);
		octo_cfree(*((MemoryChunk**)result_b.buf_addr));
	}

	free(loaded_schemas_b[1].buf_addr);
	free(loaded_schemas_b);
}
