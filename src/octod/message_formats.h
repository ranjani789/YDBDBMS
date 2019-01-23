/* Copyright (C) 2018 YottaDB, LLC
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
#ifndef MESSAGE_FORMATS_H
#define MESSAGE_FORMATS_H

// Source: https://www.postgresql.org/docs/current/protocol-message-formats.html
// Structs annotated with // B are "backend" messages (sent from the backend)
// // F are "frontend" messages (send from the frontend, read by us)
// Several items are just "stubs"; copy-paste the name, but no implementation
// These start with //#
// Some structures containe variable-length strings; we declare pointers
//  to those (and other items) in the first part of the struct. All network
//  data should start storing/reading at offset(type)

enum PSQL_MessageTypes {
	PSQL_Authenication = 'R',
	PSQL_Bind = 'B',
	PSQL_ErrorResponse = 'E'
};

typedef struct __attribute__((packed)) {
		char type;
		int length;
		char data[];
} BaseMessage;

// B
typedef struct __attribute__((packed)) {
	char type;
	unsigned int length;
	int result;
} AuthenticationOk;

//# AuthenticationKerberosV5;

// B
typedef struct __attribute__((packed)) {
	char type;
	unsigned int length;
	int _b;
} AuthenticationCleartextPassword;

//# AuthenticationMD5Password
//# AuthenticationSCMCredential
//# AuthenticationGSS
//# AuthenticationSSPI
//# AuthenticationGSSContinue
//# AuthenticationSASL
//# AuthenticationSASLContinue
//# AuthenticationSASLFinal

//# BackendKeyData

typedef struct {
	void *value;
	int length;
} BindParm;

typedef struct __attribute__((packed)) {
	char *dest;
	char *source;
	short int num_parm_format_codes;
	short int *parm_format_codes;
	short int num_parms;
	BindParm *parms;
	short int num_result_col_format_codes;
	short int *result_col_format_codes;

	char type;
	unsigned int length;
	char data[];
} Bind;

typedef struct {
	char type;
	char *value;
} ErrorResponseArg;

// B
typedef struct __attribute__((packed)) {
	ErrorResponseArg *args;

	char type;
	unsigned int length;
	char data[];
} ErrorResponse;

typedef enum {
	PSQL_Error_SEVERITY = 'S',
	PSQL_Error_Code = 'C',
	PSQL_Error_Message = 'M',
	PSQL_Error_Detail = 'D',
	PSQL_Error_Hint = 'H',
	PSQL_Error_Position = 'P',
	PSQL_Error_Internal_Position = 'p',
	PSQL_Error_Internal_Query = 'q',
	PSQL_Error_Where = 'W',
	PSQL_Error_Schema_Name = 's',
	PSQL_Error_Table_Name = 't',
	PSQL_Error_Column_Name = 'c',
	PSQL_Error_Data_Type_Name = 'd',
	PSQL_Error_Constraint_Name = 'n',
	PSQL_Error_File = 'F',
	PSQL_Error_Line = 'L',
	PSQL_Error_Routine = 'R'
} PSQL_ErrorResponseArgType;

#define ERROR_DEF(name, format_string) name,
#define ERROR_END(name, format_string) name
typedef enum {
  #include "error_severity.hd"
} PSQL_ErrorSeverity;
#undef ERROR_DEF
#undef ERROR_END

#define ERROR_DEF(name, format_string) format_string,
#define ERROR_END(name, format_string) format_string
static const char *psql_error_severity_str[] = {
  #include "error_severity.hd"
};
#undef ERROR_DEF
#undef ERROR_END

#define ERROR_DEF(name, format_string) name,
#define ERROR_END(name, format_string) name
typedef enum {
  #include "error_codes.hd"
} PSQL_SQLSTATECode;
#undef ERROR_DEF
#undef ERROR_END

#define ERROR_DEF(name, format_string) format_string,
#define ERROR_END(name, format_string) format_string
static const char *psql_sqlstate_codes_str[] = {
  #include "error_codes.hd"
};
#undef ERROR_DEF
#undef ERROR_END

// varargs should be of type ErrorResponseArg
ErrorResponse *make_error_response(PSQL_ErrorSeverity severity, PSQL_SQLSTATECode code, char *message, size_t num_args, ...);

Bind *read_bind(BaseMessage *message, ErrorResponse **err);

#endif