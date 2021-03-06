/* Copyright (C) 2018-2019 YottaDB, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
	// Test a single SSLRequest message
	// most significant 16 bits: 1234, least significant 16 bits: 5679
	unsigned int request_code = 80877103;
	unsigned int message_length = sizeof(SSLRequest);
	ErrorResponse *err = NULL;

	// Length + extra stuff - already counted (length, protocol version)
	SSLRequest *test_data = (SSLRequest*)malloc(sizeof(SSLRequest));
	test_data->length = htonl(message_length);
	test_data->request_code = htonl(request_code);

	// The actual test
	SSLRequest *ssl = read_ssl_request(NULL, (char*)(&test_data->length), message_length, &err);

	// Standard checks
	assert_non_null(ssl);
	assert_null(err);
	assert_int_equal(message_length, ssl->length);
	assert_int_equal(request_code, ssl->request_code);

	free(test_data);
	free(ssl);
}

static void test_invalid_length(void **state) {
	// Test a single SSLRequest message
	// most significant 16 bits: 1234, least significant 16 bits: 5679
	unsigned int request_code = 80877103;
	unsigned int message_length = 9;
	ErrorResponse *err = NULL;
	ErrorBuffer err_buff;
	err_buff.offset = 0;
	const char *error_message;

	// Length + extra stuff - already counted (length, protocol version)
	SSLRequest *test_data = (SSLRequest*)malloc(sizeof(SSLRequest));
	test_data->length = htonl(message_length);
	test_data->request_code = htonl(request_code);

	// The actual test
	SSLRequest *ssl = read_ssl_request(NULL, (char*)(&test_data->length), message_length, &err);

	// Standard checks
	assert_non_null(err);
	assert_null(ssl);

	// Ensure correct error message
	error_message = format_error_string(&err_buff, ERR_ROCTO_INVALID_INT_VALUE,
			"SSLRequest", "length", message_length, "8");
	assert_string_equal(error_message, err->args[2].value + 1);

	free(test_data);
	free_error_response(err);
}

static void test_invalid_request_code(void **state) {
	// Test a single SSLRequest message
	// most significant 16 bits: 1234, least significant 16 bits: 5679
	unsigned int request_code = 0x43219765;
	unsigned int message_length = sizeof(SSLRequest);
	ErrorResponse *err = NULL;
	ErrorBuffer err_buff;
	err_buff.offset = 0;
	const char *error_message;

	// Length + extra stuff - already counted (length, protocol version)
	SSLRequest *test_data = (SSLRequest*)malloc(sizeof(SSLRequest));
	test_data->length = htonl(message_length);
	test_data->request_code = htonl(request_code);

	// The actual test
	SSLRequest *ssl = read_ssl_request(NULL, (char*)(&test_data->length), message_length, &err);

	// Standard checks
	assert_non_null(err);
	assert_null(ssl);

	// Ensure correct error message
	error_message = format_error_string(&err_buff, ERR_ROCTO_INVALID_INT_VALUE,
			"SSLRequest", "request code", request_code, "80877103");
	assert_string_equal(error_message, err->args[2].value + 1);

	free(test_data);
	free_error_response(err);
}
int main(void) {
	const struct CMUnitTest tests[] = {
			cmocka_unit_test(test_valid_input),
			cmocka_unit_test(test_invalid_length),
			cmocka_unit_test(test_invalid_request_code),
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
