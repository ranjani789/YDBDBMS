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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Used to convert between network and host endian
#include <arpa/inet.h>

#include "rocto.h"
#include "message_formats.h"

static void test_valid_input(void **state) {
	RoctoSession session;
	ydb_buffer_t session_id;
	YDB_STRING_TO_BUFFER("0", &session_id);
	session.session_id = &session_id;
	int expected_length = 12;
	char salt1[4];
	char salt2[4];

	AuthenticationMD5Password *response = make_authentication_md5_password(&session, salt1);
	// Standard checks
	assert_non_null(response);
	assert_int_equal(response->length, htonl(expected_length));

	AuthenticationMD5Password *response2 = make_authentication_md5_password(&session, salt2);
	// Standard checks
	assert_non_null(response2);
	assert_int_equal(response2->length, htonl(expected_length));
	// Confirm different salt produced on each call
	assert_int_not_equal(salt1, salt2);

	free(response);
	free(response2);
}

static void test_invalid_input_null_pointer(void **state) {
	int expected_length = 12;
	char salt[4];

	AuthenticationMD5Password *response = make_authentication_md5_password(NULL, salt);
	assert_null(response);
}

int main(void) {
	octo_init(0, NULL);
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_valid_input),
		cmocka_unit_test(test_invalid_input_null_pointer)
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
