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

LogicalPlan *lp_replace_helper(LogicalPlan *where, SqlTableAlias *table_alias, SqlKey *key);

/**
 * Goes through plan replaces LP_COLUMN references to the given table alias
 *  with a LP_DERIVED_COLUMN and a LP_PIECE_NUMBER/LP_KEY combination
 */
LogicalPlan *lp_replace_derived_table_references(LogicalPlan *root, LogicalPlan *new_plan, SqlTableAlias *table_alias) {
	SqlKey *key;
	LogicalPlan *key_lp, *t;

	key_lp = lp_get_output_key(new_plan);
	key = key_lp->v.key;
	t = lp_get_select_where(root);
	t->v.operand[0] = lp_replace_helper(t->v.operand[0], table_alias, key);
	t = lp_get_project(root);
	t->v.operand[0] = lp_replace_helper(t->v.operand[0], table_alias, key);
	// Make sure to update table references in order by clauses
	t = root->v.operand[1]->v.operand[1];
	if(t != NULL) {
		t->v.operand[0] = lp_replace_helper(t->v.operand[0], table_alias, key);
	}

	return root;
}

LogicalPlan *lp_replace_helper(LogicalPlan *where, SqlTableAlias *table_alias, SqlKey *key) {
	SqlColumnAlias *alias;
	LogicalPlan *ret = where;
	int part = 1;

	if(where == NULL)
		return NULL;

	switch(where->type) {
	case LP_COLUMN_ALIAS:
		alias = where->v.column_alias;
		if(alias->table_alias->v.table_alias->unique_id == table_alias->unique_id) {
			MALLOC_LP(ret, LP_DERIVED_COLUMN);
			MALLOC_LP(ret->v.operand[0], LP_KEY);
			ret->v.operand[0]->v.key = key;
			part = get_column_piece_number(alias, table_alias);
			MALLOC_LP(ret->v.operand[1], LP_PIECE_NUMBER);
			ret->v.operand[1]->v.piece_number = part;
		}
		break;
	case LP_COLUMN_LIST:
	case LP_FUNCTION_CALL:
		ret->v.operand[0] = lp_replace_helper(where->v.operand[0], table_alias, key);
		ret->v.operand[1] = lp_replace_helper(where->v.operand[1], table_alias, key);
		break;
	case LP_WHERE:
		ret->v.operand[0] = lp_replace_helper(where->v.operand[0], table_alias, key);
		break;
	case LP_SET_OPERATION:
		// I don't expect this to happen, but seems like a possible path
		assert(FALSE);
		break;
	case LP_VALUE:
	case LP_DERIVED_COLUMN:
		// Nothing to do
		break;
	default:
		assert(where->type > LP_ADDITION);
		ret->v.operand[0] = lp_replace_helper(where->v.operand[0], table_alias, key);
		ret->v.operand[1] = lp_replace_helper(where->v.operand[1], table_alias, key);
		break;
	}
	return ret;
}
