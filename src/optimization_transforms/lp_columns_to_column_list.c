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

#include "octo.h"
#include "octo_types.h"
#include "logical_plan.h"

SqlColumnListAlias *lp_columns_to_column_list(SqlColumn *column, SqlTableAlias *table_alias) {
	SqlColumnList *cur;
	SqlColumnListAlias *ret = NULL, *t_column_list_alias, *cur_column_list_alias;
	SqlStatement *stmt;
	SqlColumnAlias *alias;
	SqlColumn *cur_column, *start_column;

	if(column == NULL)
		return NULL;

	cur_column = start_column = column;
	do {
		cur = (SqlColumnList*)octo_cmalloc(memory_chunks, sizeof(SqlColumnList));
		memset(cur, 0, sizeof(SqlColumnList));
		dqinit(cur);
		SQL_STATEMENT(stmt, column_alias_STATEMENT);
		MALLOC_STATEMENT(stmt, column_alias, SqlColumnAlias);
		cur->value = stmt;

		alias = stmt->v.column_alias;
		PACK_SQL_STATEMENT(alias->column, cur_column, column);
		PACK_SQL_STATEMENT(alias->table_alias, table_alias, table_alias);

		cur_column_list_alias = (SqlColumnListAlias*)octo_cmalloc(memory_chunks, sizeof(SqlColumnListAlias));
		cur_column_list_alias->alias = cur_column->columnName;
		PACK_SQL_STATEMENT(cur_column_list_alias->column_list, cur, column_list);
		dqinit(cur_column_list_alias);
		PACK_SQL_STATEMENT(alias->table_alias, table_alias, table_alias);
		if(ret == NULL) {
			ret = cur_column_list_alias;
		} else {
			dqinsert(ret, cur_column_list_alias, t_column_list_alias);
		}
		cur_column = cur_column->next;
	} while(cur_column != start_column);
	assert(ret != NULL);
	return ret;
}
