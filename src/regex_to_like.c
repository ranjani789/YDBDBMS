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

char *regex_to_like(const char *src) {
	char *ret, *d, *end;
	const char *c;

	ret = octo_cmalloc(memory_chunks, MAX_STR_CONST);
	end = ret + MAX_STR_CONST;
	d = ret;
	c = src;

	*d++ = '^';

	while(*c != '\0' && d < end) {
		if(*c == '%') {
			*d++ = '.';
			*d++ = '*';
		} else if(*c == '_') {
			*d++ = '.';
		} else {
			*d++ = *c;
		}
		c++;
	}

	*d++ = '$';
	*d = '\0';

	return ret;
}
