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
	// Test a single startup message
	unsigned int message_length = 0;
	message_length += sizeof(unsigned int);		// count length
	message_length += 1;				// count item
	char *message = "SELECT * FROM names;";
	message_length += strlen(message) + 1;		// count null
	char *c = NULL;
	ErrorResponse *err = NULL;

	// Populate base message
        BaseMessage *test_data = (BaseMessage*)malloc(message_length + sizeof(BaseMessage) - sizeof(unsigned int));
	test_data->type = PSQL_Describe;
	test_data->length = htonl(message_length);
	// Set item field
	c = test_data->data;
	*c++ = 'S';
	// Set name field (exclude length and item)
	strncpy(c, message, message_length - sizeof(unsigned int) - 1);

	// The actual test
	Describe *describe = read_describe(test_data, &err);

	// Standard checks
	assert_non_null(describe);
	assert_null(err);
	assert_string_equal(message, describe->name);

	free(describe);
	free(test_data);
}

static void test_non_terminated_input(void **state) {
	// Test a single startup message
	unsigned int message_length = 0;
	message_length += sizeof(unsigned int);		// count length
	message_length += 1;				// count item
	char *message = "SELECT * FROM names;";
	message_length += strlen(message);		// exclude null
	char *c = NULL;
	ErrorResponse *err = NULL;
	ErrorBuffer err_buff;
	err_buff.offset = 0;
	const char *error_message;

	// Populate base message
        BaseMessage *test_data = (BaseMessage*)malloc(message_length + sizeof(BaseMessage) - sizeof(unsigned int));
	test_data->type = PSQL_Describe;
	test_data->length = htonl(message_length - 2);
	// Set item field
	c = test_data->data;
	*c++ = 'S';
	// Set name field (exclude length, item, and null)
	strncpy(c, message, message_length - sizeof(unsigned int) - 1 - 1);

	// The actual test
	Describe *describe = read_describe(test_data, &err);

	// Standard checks
	assert_null(describe);
	assert_non_null(err);

	// Ensure correct error message
	error_message = format_error_string(&err_buff, ERR_ROCTO_MISSING_NULL, "Describe", "name");
	assert_string_equal(error_message, err->args[2].value + 1);

	free(describe);
	free(test_data);
	free_error_response(err);
}

static void test_invalid_type(void **state) {
	// Test a single startup message
	unsigned int message_length = 0;
	message_length += sizeof(unsigned int);		// count length
	message_length += 1;				// count item
	char *message = "SELECT * FROM names;";
	message_length += strlen(message) + 1;		// count null
	char *c = NULL;
	ErrorResponse *err = NULL;
	ErrorBuffer err_buff;
	err_buff.offset = 0;
	const char *error_message;

	// Populate base message
        BaseMessage *test_data = (BaseMessage*)malloc(message_length + sizeof(BaseMessage) - sizeof(unsigned int));
	test_data->type = 'X';
	test_data->length = htonl(message_length);
	// Set bad item field
	c = test_data->data;
	*c++ = 'S';
	// Set name field (exclude length and item)
	strncpy(c, message, message_length - sizeof(unsigned int) - 1);

	// The actual test
	Describe *describe = read_describe(test_data, &err);

	// Standard checks
	assert_null(describe);
	assert_non_null(err);

	// Ensure correct error message
	error_message = format_error_string(&err_buff, ERR_ROCTO_INVALID_TYPE, "Describe", test_data->type, PSQL_Describe);
	assert_string_equal(error_message, err->args[2].value + 1);

	free(describe);
	free(test_data);
	free_error_response(err);
}

static void test_invalid_item(void **state) {
	// Test a single startup message
	unsigned int message_length = 0;
	message_length += sizeof(unsigned int);		// count length
	message_length += 1;				// count item
	char *message = "SELECT * FROM names;";
	message_length += strlen(message) + 1;		// count null
	char *c = NULL;
	ErrorResponse *err = NULL;
	ErrorBuffer err_buff;
	err_buff.offset = 0;
	const char *error_message;

	// Populate base message
        BaseMessage *test_data = (BaseMessage*)malloc(message_length + sizeof(BaseMessage) - sizeof(unsigned int));
	test_data->type = PSQL_Describe;
	test_data->length = htonl(message_length);
	// Set bad item field
	c = test_data->data;
	*c++ = 'X';
	// Set name field (exclude length and item)
	strncpy(c, message, message_length - sizeof(unsigned int) - 1);

	// The actual test
	Describe *describe = read_describe(test_data, &err);

	// Standard checks
	assert_null(describe);
	assert_non_null(err);

	// Ensure correct error message
	error_message = format_error_string(&err_buff, ERR_ROCTO_INVALID_CHAR_VALUE, "Describe", "item",
			*(char*)test_data->data, "'S' or 'P'");
	assert_string_equal(error_message, err->args[2].value + 1);

	free(describe);
	free(test_data);
	free_error_response(err);
}

static void test_unexpectedly_terminated_input(void **state) {
	// Test a single startup message
	unsigned int message_length = 0;
	message_length += sizeof(unsigned int);		// count length
	message_length += 1;				// count item
	char *message = "SELECT * FROM names\0;";
	message_length += strlen(message) + 2;		// expecting extra character after null
	char *c = NULL;
	ErrorResponse *err = NULL;
	ErrorBuffer err_buff;
	err_buff.offset = 0;
	const char *error_message;

	// Populate base message
        BaseMessage *test_data = (BaseMessage*)malloc(message_length + sizeof(BaseMessage) - sizeof(unsigned int));
	test_data->type = PSQL_Describe;
	test_data->length = htonl(message_length);
	// Set item field
	c = test_data->data;
	*c++ = 'S';
	// Set name field (exclude length and item)
	strncpy(c, message, message_length - sizeof(unsigned int) - 1);

	// The actual test
	Describe *describe = read_describe(test_data, &err);

	// Standard checks
	assert_null(describe);
	assert_non_null(err);

	// Ensure correct error message
	error_message = format_error_string(&err_buff, ERR_ROCTO_TRAILING_CHARS, "Describe");
	assert_string_equal(error_message, err->args[2].value + 1);

	free(describe);
	free(test_data);
	free_error_response(err);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		   cmocka_unit_test(test_valid_input),
		   cmocka_unit_test(test_non_terminated_input),
		   cmocka_unit_test(test_invalid_type),
		   cmocka_unit_test(test_invalid_item),
		   cmocka_unit_test(test_unexpectedly_terminated_input),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
