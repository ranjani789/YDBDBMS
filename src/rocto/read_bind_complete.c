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

// Used to convert between network and host endian
#include <arpa/inet.h>

#include "rocto.h"
#include "message_formats.h"

BindComplete *read_bind_complete(BaseMessage *message, ErrorResponse **err) {
	BindComplete *ret;
	unsigned int remaining_length = 0;

	UNUSED(err);

	remaining_length = ntohl(message->length);
	ret = (BindComplete*)malloc(sizeof(BindComplete));

	ret->type = message->type;
	ret->length = remaining_length;

	return ret;
}
