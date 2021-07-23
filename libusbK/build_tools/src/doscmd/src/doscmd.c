/* doscmd.c
 * Copyright (c) 2010 Travis Robinson <libusbdotnet@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <conio.h>
#include <Shlwapi.h>
#include "doscmd.h"
#include "tokenizer.h"

#define _VER_ONLY
#include "doscmd_rc.rc"
#include "user_debug.h"

#define SCRBUF_MAX 16384
#define FMTBUF_MAX 4096

static char SCRBUF[SCRBUF_MAX + 1];
static char FMTBUF[FMTBUF_MAX + 1];


LPCSTR version_keys_strings[] =
{
	"Comments",
	"CompanyName",
	"FileDescription",
	"FileVersion",
	"InternalName",
	"LegalCopyright",
	"LegalTrademarks",
	"OriginalFilename",
	"PrivateBuild",
	"ProductName",
	"ProductVersion",
	NULL
};
#define DCMD_IS_OUTPUT(Filename, SpecialFilename) \
	((tok_string_cmp(Filename, SpecialFilename, sizeof(SpecialFilename) - 1, TRUE)) == 0)

#define FPRINTF_COPY_FMT_TO_TEMP(DCmdPtr, Length) \
{ \
	if ((Length) > 0) \
	{ \
		strncpy(&(DCmdPtr)->buf.data[(DCmdPtr)->buf.index], &(DCmdPtr)->src.fmt[(DCmdPtr)->src.index], (Length)); \
		(DCmdPtr)->buf.index += (Length); \
		(DCmdPtr)->src.index += (Length); \
		(DCmdPtr)->buf.data[(DCmdPtr)->buf.index] = '\0'; \
	} \
}

/* shell32.dll exports */

typedef LPWSTR* (WINAPI* commandline_to_argvw_t)(LPCWSTR, int*);

static HINSTANCE shell32_dll = NULL;
static commandline_to_argvw_t commandline_to_argvw = NULL;
#define INIT_COMMANDLINE_TO_ARGVW() \
if (commandline_to_argvw == NULL) \
{ \
	if (shell32_dll == NULL) shell32_dll = LoadLibrary("shell32"); \
	if (shell32_dll) \
	{ \
		commandline_to_argvw = (commandline_to_argvw_t) GetProcAddress(shell32_dll, "CommandLineToArgvW"); \
	} \
}

struct LANGANDCODEPAGE
{
	WORD wLanguage;
	WORD wCodePage;
}* lpTranslate;

typedef enum _PARAM_TYPE
{
    PARAM_TYPE_INVALID,
    PARAM_TYPE_COMMAND,
    PARAM_TYPE_STRING,
    PARAM_TYPE_PAIR,
} PARAM_TYPE;

extern FILE* file_create_temp(__deref_out_opt PCHAR* filename);
extern LONG fset_length(FILE* file, LONG new_length);
extern LONG fget_length(FILE* file);
extern LONG file_write_remaining(FILE* src_file, FILE* output_file);

static void show_help();

static BOOL char_is_white(CHAR c);
static PCHAR string_ltrim(PCHAR src);
static PCHAR string_rtrim(PCHAR src);
static PCHAR string_trim(PCHAR src);
static PCHAR main_parse_argv(PCHAR src, const PCHAR short_name, const PCHAR long_name);
static PCHAR dcmd_parse_env_entity(__inout PCHAR line,
                                   __deref_out_opt PCHAR* param,
                                   __deref_out_opt PCHAR* value,
                                   __deref_out_opt PARAM_TYPE* param_type);

static INT dcmd_fprintf_internal(__in FILE* out,
                                 __in HANDLE std,
                                 __in PCHAR format,
                                 __in PDCMD_FPRINTF_OPTIONS_T options,
                                 __deref_in PCHAR* arg_list,
                                 __in INT arg_count);

static INT fprintf_parse_and_print(PDCMD_FPRINTF_T dcmd);
static INT fprintf_format_escape(PDCMD_FPRINTF_T dcmd);
static INT fprintf_format_type(PDCMD_FPRINTF_T dcmd);

static PCHAR dcmd_parse_option(__inout PCHAR src,
                               __deref_out_opt PCHAR* param,
                               __deref_out_opt PCHAR* value,
                               __deref_out_opt PARAM_TYPE* param_type,
                               __in_opt CONST PCHAR stop_chars);

static PCHAR get_param(__inout PCHAR param,
                       __in PCHAR line,
                       __in CONST CHAR separator,
                       __in CONST CHAR comment,
                       __in BOOL is_sep_required);

static INT version_control_get_item(__in PCHAR line,
                                    __deref_out PVERSION_CONTROL_ITEM_T* item);

static LPCSTR version_info_get_item(LPCSTR InfoItem, LPCSTR szFullPath);
static BOOL OutputToConsole(__in HANDLE stdHandle, FILE* fileHandle, __in LPCSTR format, ...);
INT main(INT argc, PCHAR argv[]);

VOID output_debug_string(LPCSTR fmt, ...)
{
	CHAR buf[256];
	INT len;
	va_list args;

	va_start(args, fmt);
#if __STDC_WANT_SECURE_LIB__
	len = _vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, fmt, args);
#else
	len = _vsnprintf(buf, sizeof(buf) - 1, fmt, args);
#endif
	va_end(args);

	if (len > 0)
		buf[len - 1] = '\0';

	buf[sizeof(buf) - 1] = '\0';

	OutputDebugStringA(buf);
}

static BOOL OutputToConsole(__in HANDLE stdHandle, FILE* fileHandle, __in LPCSTR format, ...)
{
	INT len;
	va_list args;

	va_start(args, format);
#if __STDC_WANT_SECURE_LIB__
	len = _vsnprintf_s(SCRBUF, SCRBUF_MAX, SCRBUF_MAX - 1, format, args);
#else
	len = _vsnprintf(SCRBUF,  SCRBUF_MAX - 1, format, args);
#endif
	va_end(args);

	if (len > 0)
	{
		DWORD charsWritten;
		SCRBUF[len] = '\0';
		if (!stdHandle || stdHandle == INVALID_HANDLE_VALUE)
		{
			if (!fileHandle)
				return FALSE;
			return (fputs(SCRBUF, fileHandle) >= 0);
		}
		else
		{
			CHAR* pbuf = SCRBUF;
			while (WriteConsoleA(stdHandle, pbuf, (DWORD)len, &charsWritten, NULL))
			{
				len -= charsWritten;
				pbuf += charsWritten;
				if (len <= 0)
					break;
			}

			return (len == 0);
		}
	}
	return FALSE;
}


/* DLL main entry point */
BOOL WINAPI DllMain(HANDLE module, DWORD reason, LPVOID reserved)
{
	UNREFERENCED_PARAMETER(reserved);
	UNREFERENCED_PARAMETER(module);

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}

int CALLBACK dcmd_rundll(HWND wnd, HINSTANCE instance, LPSTR cmd_line, int cmd_show)
{
	WCHAR* cmd_line_w;
	size_t length;
	int ret;
	int arg;
	int argc = 0;
	LPWSTR* argvw = NULL;
	LPSTR* argva = NULL;
	LPSTR arga;
	LPWSTR argw;
	int char_pos;

	UNREFERENCED_PARAMETER(wnd);
	UNREFERENCED_PARAMETER(instance);
	UNREFERENCED_PARAMETER(cmd_show);

	INIT_COMMANDLINE_TO_ARGVW();
	if (!cmd_line || !strlen(cmd_line) || !commandline_to_argvw)
	{
		return -1;
	}

	length = (strlen(cmd_line) * sizeof(WCHAR)) + sizeof(WCHAR);
	cmd_line_w = malloc(length);
	if (!cmd_line_w)
		return -1;

	memset(cmd_line_w, 0, length);
	if ((length = mbstowcs(cmd_line_w, cmd_line, length / sizeof(WCHAR))) < 1)
	{
		free(cmd_line_w);
		return -1;
	}
	cmd_line_w[length] = 0;
	if ((argvw = commandline_to_argvw(cmd_line_w, &argc)) == NULL)
	{
		free(cmd_line_w);
		return -1;
	}

	argva = (LPSTR*)argvw;
	arga = *argva;
	argw = *argvw;
	arg = 0;
	char_pos = 0;

	while (arg < argc)
	{
		arga[char_pos] = (CHAR)argw[char_pos];
		if (!argw[char_pos])
			arg++;

		char_pos++;
	}
	arga[char_pos] = '\0';

	ret = main(argc, argva);

	// TODO: execute main.

	LocalFree(argvw);
	free(cmd_line_w);

	return ret;
}

static BOOL char_is_white(CHAR c)
{
	if (c == ' ' || c == '\t' || c == '\r'  || c == '\n'  || c == '\f'  || c == '\v')
		return TRUE;
	return FALSE;
}

static PCHAR string_ltrim(PCHAR src)
{
	PCHAR p;
	PCHAR start;
	p = start = src;

	while(char_is_white(*p))
		p++;

	if (p != start)
		*(p - 1) = '\0';

	return p;
}

static PCHAR string_rtrim(PCHAR src)
{
	PCHAR p;
	PCHAR start;
	p = start = src + strlen(src) - 1;

	while(char_is_white(*p))
	{
		*p = '\0';
		p--;
	}

	return src;
}

static PCHAR string_trim(PCHAR src)
{
	return(string_ltrim(string_rtrim(src)));
}


static PCHAR main_parse_argv(PCHAR src, const PCHAR short_name, const PCHAR long_name)
{
	if (src)
	{
		if (short_name)
		{
			if (tok_string_cmp(src, short_name, (INT)strlen(short_name), TRUE) == 0)
				return src + strlen(short_name);
		}
		if (long_name)
		{
			if (tok_string_cmp(src, long_name, (INT) strlen(long_name), TRUE) == 0)
				return src + strlen(long_name);
		}
	}
	return NULL;
}
static PCHAR dcmd_parse_next_option(__inout PCHAR src,
                                    __deref_out_opt PCHAR* param,
                                    __deref_out_opt PCHAR* value,
                                    __in CONST CHAR entity_separator,
                                    __in CONST CHAR option_separator,
                                    __in CONST CHAR string_quote_char)
{
	BOOL quoted = FALSE;
	BOOL found_entity_separator = FALSE;
	PCHAR src_next = NULL;

	*param = *value = NULL;
	if (!src || !src[0]) return NULL;

	src = string_ltrim(src);

	src_next = src;
	while(src_next[0])
	{
		if (quoted)
		{
			if (src_next[0] == string_quote_char)
			{
				if (entity_separator)
					src_next[0] = '\0';

				quoted = FALSE;
			}
		}
		else if (src_next[0] == string_quote_char)
		{
			quoted = TRUE;
			if (entity_separator)
			{
				src_next[0] = '\0';
				if (found_entity_separator == FALSE && *param == NULL)
				{
					*param = src_next + 1;
				}
				else
				{
					*value = src_next + 1;
				}
			}
		}
		else if (entity_separator && src_next[0] == entity_separator)
		{
			src_next[0] = '\0';
			*value = src_next + 1;
			found_entity_separator = TRUE;
		}
		else if (src_next[0] == option_separator)
		{
			src_next[0] = '\0';
			src_next++;
			break;
		}

		if (!*param)
			*param = src_next;
		src_next++;
	}

	return src_next;
}

static PCHAR dcmd_parse_option(__inout PCHAR src,
                               __deref_out_opt PCHAR* param,
                               __deref_out_opt PCHAR* value,
                               __deref_out_opt PARAM_TYPE* param_type,
                               __in_opt CONST PCHAR stop_chars)
{
	*param = NULL;
	*value = NULL;
	*param_type = PARAM_TYPE_INVALID;

	if (src != NULL)
	{
		PCHAR sep;

		// the last one was a final terminating sep.
		if (!src[0])
			return NULL;

		if (stop_chars)
			sep = strpbrk(src, stop_chars);
		else
			sep = strpbrk(src, ";");

		// found an option terminator; null it out and get ready for the next option.
		// we will parse up to this point for the current option.
		if (sep)
		{
			// use this sep as a terminator and advance.
			*sep = '\0';
			sep++;
		}
		else
		{
			sep = src + strlen(src);
		}

		dcmd_parse_env_entity(src, param, value, param_type);

		// return a pointer to the next option (if any)
		return sep;
	}

	return NULL;
}

static PCHAR dcmd_parse_env_entity(__inout PCHAR line,
                                   __deref_out_opt PCHAR* param,
                                   __deref_out_opt PCHAR* value,
                                   __deref_out_opt PARAM_TYPE* param_type)
{
	PCHAR ch = line;
	BOOL in_quotes = FALSE;
	BOOL was_quoted = FALSE;
	*param = NULL;
	*value = NULL;

	if (strlen(line) == 0) return NULL;

	while(*(ch))
	{
		if (in_quotes)
		{
			if (*ch == '\"')
				in_quotes = FALSE;
			ch++;
		}
		else
		{
			switch(*ch)
			{
			case '\"':
				in_quotes = TRUE;
				was_quoted = TRUE;
				ch++;
				break;
			case ' ':
			case '\t':
				// end of param or param=value
				goto EndOfArgOrLine;
			case '=':
				*ch = '\0';
				ch++;
				*param = line;
				*value = ch;
				continue;
			default:
				ch++;
			}
		}
	};

EndOfArgOrLine:
	if (*param && *value)
	{
		if (tok_string_cmp(*value, *param, (INT)strlen(*param), TRUE) == 0)
		{
			// if the value and param match then this it is considered a command.
			// i.e. fre=fre or mycommand=mycommand
			*param_type = PARAM_TYPE_COMMAND;
		}
		else
		{
			*param_type = PARAM_TYPE_PAIR;
		}
	}
	else if (was_quoted)
	{
		*param_type = PARAM_TYPE_STRING;
		string_trim(line);
		*param = line;
		*value = line;
	}
	else
	{
		*param_type = PARAM_TYPE_COMMAND;
		string_trim(line);
		*param = line;
		*value = line;
	}
	if (*ch)
	{
		*ch = '\0';
		ch++;
		return string_ltrim(ch);
	}
	return ch;
}

static INT fprintf_format_type(PDCMD_FPRINTF_T dcmd)
{
	INT ret = ERROR_SUCCESS;
	PCHAR format_type;
	PCHAR format_arg;
	INT format_type_length;

	// we should always find one of these at this point.
	// a valid (unescaped) % already exists on the line.
	format_type = strpbrk(&dcmd->src.fmt[dcmd->src.index], "diouxXeEfgGaAs");
	if (!format_type)
	{
		DOSERR("invalid format type near %s.\n",
		       &dcmd->src.fmt[dcmd->src.index]);

		DOS_E_RET(ERROR_BAD_ARGUMENTS);
	}

	// copy everything from the source fmt string into the temp buffer.
	format_type_length = (INT)(format_type - (&dcmd->src.fmt[dcmd->src.index])) + 1;

	if (dcmd->arg.index >= dcmd->arg.count)
	{
		DOSERR("invalid format type near %s.\n",
		       &dcmd->src.fmt[dcmd->src.index]);

		// There aren't any more args for this one! try and keep going anyways..
		// rewind one char and null the percent back out.
		dcmd->buf.data[--dcmd->buf.index] = '\0';
		dcmd->src.index += format_type_length;
		return 0;
	}

	FPRINTF_COPY_FMT_TO_TEMP(dcmd, format_type_length);


	format_arg = dcmd->arg.list[dcmd->arg.index++];

	// There are really only three basic types; int, double, and string.
	// we already have string leaving only two to convert.  Once we get
	// our string into one of these 2 number formats, printf will do the
	// rest.
	switch (*format_type)
	{
	case 'd':
	case 'i':
	case 'o':
	case 'u':
	case 'x':
	case 'X':
		// int.
		OutputToConsole(dcmd->std, dcmd->out, dcmd->buf.data, atoi(format_arg));
		break;

	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G':
	case 'a':
	case 'A':
		// double.
		OutputToConsole(dcmd->std, dcmd->out, dcmd->buf.data, atof(format_arg));
		break;

	case 's':
		// string.
		OutputToConsole(dcmd->std, dcmd->out, dcmd->buf.data, format_arg);
		break;
	}

	// reset the temp buffer; it was written to output.
	tok_memory_zero(dcmd->buf.data, dcmd->buf.index);
	dcmd->buf.index = 0;

	return ret;
}

static INT fprintf_format_escape(PDCMD_FPRINTF_T dcmd)
{
	INT ret = ERROR_SUCCESS;
	CHAR hex_byte[5];
	CHAR fmtchar = dcmd->src.fmt[dcmd->src.index++];

	switch (fmtchar)
	{
	case 'b':			// Backspace.
		dcmd->buf.data[dcmd->buf.index++] = '\b';
		break;
	case 't':			// (Horizontal) Tab.
		dcmd->buf.data[dcmd->buf.index++] = '\t';
		break;
	case 'n':			// New Line Feed (LF).
		dcmd->buf.data[dcmd->buf.index++] = '\n';
		break;
	case 'v':			// Vertical Tab.
		dcmd->buf.data[dcmd->buf.index++] = '\v';
		break;
	case 'f':			// Form Feed.
		dcmd->buf.data[dcmd->buf.index++] = '\f';
		break;
	case 'r':			// Carriage Return (CR).
		dcmd->buf.data[dcmd->buf.index++] = '\r';
		break;

		// Quotes don't need unescaped from dos. instead we will make
		// the escaped single quote appear as a quote.  This will make
		// it easy to embed a dblquote into a dblquoted string.
	case '\'':			// Single quote.
		dcmd->buf.data[dcmd->buf.index++] = '\"';
		break;

		// An escaped double quote will come out as a literal single quote.
	case '\"':			// Double quote.
		dcmd->buf.data[dcmd->buf.index++] = '\'';
		break;
	case 'k':			// Color.
	case 'x':			// Hex digits.
		hex_byte[0] = '0';
		hex_byte[1] = 'x';
		hex_byte[2] = dcmd->src.fmt[dcmd->src.index++];
		hex_byte[3] = dcmd->src.fmt[dcmd->src.index++];
		hex_byte[4] = '\0';
		if (fmtchar == 'x')
		{
			dcmd->buf.data[dcmd->buf.index++] = (CHAR)strtoul(hex_byte, NULL, 0);
			break;
		}

		////////////////////////////////////////
		// output flush and color change.
		////////////////////////////////////////
		if (dcmd->std)
		{
			if (dcmd->buf.index > 0)
			{
				//fprintf(dcmd->out, dcmd->buf.data);
				OutputToConsole(dcmd->std, dcmd->out, "%s", dcmd->buf.data);
				tok_memory_zero(dcmd->buf.data, dcmd->buf.index);
				dcmd->buf.index = 0;
			}

			// set the text color.
			SetConsoleTextAttribute(dcmd->std,  (WORD)(strtoul(hex_byte, NULL, 0)));
		}
		break;
	default:
		if (fmtchar == dcmd->options.escape)
		{
			// This will make a literal escape char; i.e. "\\" --> "\"
			dcmd->buf.data[dcmd->buf.index++] = dcmd->options.escape;
			break;
		}
		else if (fmtchar == dcmd->options.mark)
		{
			// This will make a literal mark char; i.e. "\@" --> "@"
			dcmd->buf.data[dcmd->buf.index++] = dcmd->options.mark;
			break;
		}

		DOSERR("unsupported escape %c%c at char index %i.\n",
		       dcmd->options.escape, fmtchar, dcmd->src.index - 1);
		ret = -ERROR_BAD_ARGUMENTS;
		break;
	}

	return ret;
}

//
// FPRINTF_PARSE_AND_PRINT
//
// 1) Finds the next format string and pairs it with it's argument (if any)
// 2) Writes one format specifier section entry to out and returns.
//    For example: ("Hello %s", "Travis")
//
// RETURNS:
// If > 0 there are more arguments left to parse and print.
// If = 0 The operation is done and has completed successfully.
// If < 0 an error occured and the output is unpredictable.
//
static INT fprintf_parse_and_print(PDCMD_FPRINTF_T dcmd)
{
	INT ret = ERROR_SUCCESS;
	INT length;
	PCHAR  next_specifier;

	while ((next_specifier = strpbrk(&dcmd->src.fmt[dcmd->src.index], dcmd->options.all)) != NULL)
	{
		length = (INT)(next_specifier - (&dcmd->src.fmt[dcmd->src.index]));
		FPRINTF_COPY_FMT_TO_TEMP(dcmd, length);

		if (*(next_specifier) == dcmd->options.mark)
		{
			// this is a '%'.  If the next char is a mark then this
			// is an escaped mark char.
			if (dcmd->src.fmt[dcmd->src.index + 1] == dcmd->options.mark)
			{
				dcmd->buf.data[dcmd->buf.index++] = '%';
				dcmd->buf.data[dcmd->buf.index++] = '%';
				dcmd->src.index += 2;
				continue;
			}
			else
			{
				// this is the start of a format sequence.
				dcmd->buf.data[dcmd->buf.index++] = '%';
				dcmd->src.index++;

				if ((ret = fprintf_format_type(dcmd)) < 0)
				{
					DOSERR0("failed format type.\n");
					goto Done;
				}
			}

		}
		else if (*(next_specifier) == dcmd->options.escape)
		{
			// skip over the escape '\' char.
			dcmd->src.index++;

			// process escape codes; such as \n \r \t \x22.
			if ((ret = fprintf_format_escape(dcmd)) < 0)
			{
				DOSERR0("failed format escape.\n");
				goto Done;
			}
		}
		else
		{
			DOSERR("not implemented. near %s.\n", next_specifier);
			ret = -ERROR_NOT_SUPPORTED;
			goto Done;
		}
	}

	// No more formatting or escape chars exists.  write all remaining data from
	// the src fmt string to the temp buffer.
	if (dcmd->src.index < dcmd->src.length)
	{
		length = (dcmd->src.length - dcmd->src.index);
		FPRINTF_COPY_FMT_TO_TEMP(dcmd, length);
	}
Done:
	if (ret >= 0 && dcmd->buf.index > 0)
	{
		// the index is always the next char to be read or written.
		// In this respect it's also the data length.
		dcmd->buf.data[dcmd->buf.index] = '\0';

		//fprintf(dcmd->out, dcmd->buf.data);
		OutputToConsole(dcmd->std, dcmd->out, "%s", dcmd->buf.data);

		// all done at this point, but keep everything in sync
		// incase we want to add more code.
		tok_memory_zero(dcmd->buf.data, dcmd->buf.index);
		dcmd->buf.index = 0;
	}
	return ret;
}

INT dcmd_version_control(PDCMD_VERSION_CONTROL_OPTIONS_T options)
{
	INT ret = ERROR_SUCCESS;
	FILE* version_file = NULL;
	FILE* temp_file = NULL;
	CHAR version_line[MAX_PATH * 4];
	PCHAR line;
	PVERSION_CONTROL_ITEM_T found = NULL;
	PVERSION_CONTROL_ITEM_T head = NULL;
	PVERSION_CONTROL_ITEM_T prev_item = NULL;
	PVERSION_CONTROL_ITEM_T* next_item = &head;
	INT line_index = -1;


	// open the version database file
	if ((version_file = fopen(options->versions_file, "r+")) == NULL)
	{
		ret = 0;
		goto Done;
	}
	// build the list.
	while ( (line = fgets(version_line, sizeof(version_line), version_file)) != NULL )
	{
		line_index++;
		if (version_control_get_item(line, next_item) == ERROR_SUCCESS)
		{
			PVERSION_CONTROL_ITEM_T check = *next_item;

			// double linking here will make it easy to go back and find linked targets.
			if (prev_item)
				check->prev = prev_item;
			prev_item = check;

			// If/when the versions file is updated, we will only modify this line.
			check->line_index = line_index;

			// reference the next pointer we will use if there are more lines.
			next_item = (PVERSION_CONTROL_ITEM_T*)(&check->next);

			if (tok_string_cmp(check->target_name, options->target_name, -1, TRUE) == 0)
			{
				// when the target matches we are done looking ahead.  If this is a linked target,
				// the target which it's linked to must aleady have been defined.
				found = check;
				if (found->is_linked)
				{
					PVERSION_CONTROL_ITEM_T real_version;

					// a linked version has 'readonly' access to the version string.
					// it is only allowed to use it, but an attempt to change it should not cause an error.
					if (options->action != VCA_TAG)
					{
						ret = ERROR_SUCCESS;
						break;
					}
					real_version = found;
					while (((real_version = (PVERSION_CONTROL_ITEM_T)real_version->prev)) != NULL)
					{
						if (tok_string_cmp(real_version->target_name, found->linked_name, -1, TRUE) == 0)
						{
							if (real_version->is_linked)
							{
								// probably should not link to links, but it should work.
								tok_string_cpy(found->linked_name, MAX_PATH, real_version->linked_name);
							}
							else
							{
								memcpy(&found->version, &real_version->version, sizeof(found->version));
								break;
							}
						}
					}
					if (!real_version)
					{
						ret = ERROR_SUCCESS;
						break;
					}
				}

				// TODO:
				// If tagging we only need add the version fields to the env and call tokenizer on the template file.
				// If changing, we first need to modify found, then rewind the version file and write it to a temp file
				// until we get to found->line_index.  The new version line will be completely re-written and formatted.
				//
				if (options->action == VCA_TAG)
				{
					sprintf_s(version_line, sizeof(version_line), "%u", found->version.major);
					SetEnvironmentVariableA("VERSION_MAJOR", version_line);

					sprintf_s(version_line, sizeof(version_line), "%u", found->version.minor);
					SetEnvironmentVariableA("VERSION_MINOR", version_line);

					sprintf_s(version_line, sizeof(version_line), "%u", found->version.micro);
					SetEnvironmentVariableA("VERSION_MICRO", version_line);

					sprintf_s(version_line, sizeof(version_line), "%u", found->version.nano);
					SetEnvironmentVariableA("VERSION_NANO", version_line);

					// DEBUGGING:
					SetEnvironmentVariableA("TOKVAR_LTAG", "@");
					SetEnvironmentVariableA("TOKVAR_RTAG", "@");
					DeleteFile(found->version_output_filepath);

					ret = dcmd_env_tag(found->version_template_filepath, found->version_output_filepath, NULL);
				}
				else
				{
					// This is a version inc/change.
					// NOTE: If it made it here, it was not linked.

					// This will store the 0-3 (nano, micro, etc..)
					INT ver_part = options->action & 3;

					if (options->action & VCA_INC)
					{
						found->ver[ver_part]++;

						// NOTE: This should be an option. I perfer this strategy but others may not.
						while (ver_part)
							found->ver[--ver_part] = 0;

					}
					else if (options->action & VCA_DEC)
					{
						// don't go under zero
						if (found->ver[ver_part] > 0)
							found->ver[ver_part]--;
					}
					else
					{
						DOSERR("Invalid action for version control item %s\n", found->target_name);
						ret = -1;
						goto Done;
					}

					rewind(version_file);
					temp_file = file_create_temp(NULL);

					line_index = 0;
					while ( (line = fgets(version_line, sizeof(version_line), version_file)) != NULL )
					{
						if (line_index++ == found->line_index)
						{
							sprintf_s(version_line, sizeof(version_line), "%s = %u.%u.%u.%u; %s; %s;\n",
							          found->target_name,
							          found->version.major, found->version.minor, found->version.micro, found->version.nano,
							          found->version_template_filepath,
							          found->version_output_filepath);
						}

						fputs(version_line, temp_file);
					}

					rewind(temp_file);
					rewind(version_file);
					fset_length(version_file, fget_length(temp_file));
					file_write_remaining(temp_file, version_file);
					fflush(version_file);

				}
				break;
			}
		}
	}

Done:
	if (version_file)
		fclose(version_file);

	if (temp_file)
		fclose(temp_file);

	// Free the list from head to tail.
	while (head)
	{
		PVERSION_CONTROL_ITEM_T p = head->next;
		free(head);
		head = p;
	}
	return ret;
}

INT dcmd_make_args(PCHAR output_file, INT start_offset)
{
	FILE* fileOut			= NULL;
	INT ret					= -1;
	INT string_index		= 0;
	LPWSTR cmdline_w		= GetCommandLineW();
	LPSTR cmdline_a			= NULL;
	INT cmdline_w_length	= (cmdline_w != NULL ? ((INT)wcslen(cmdline_w)) : 0);
	INT cmdline_a_length;
	PCHAR line;
	PCHAR param;
	PCHAR value;
	PARAM_TYPE param_type;
	INT matches = 0;

	if (!cmdline_w || cmdline_w_length < 1)
	{
		DOSERR0("usage: --make-args output_batch_file [param param=value]+.\n");
		goto Done;
	}
	if ((cmdline_a = tok_memory_alloc(cmdline_w_length + 1)) == NULL)
	{
		DOSERR0("tok_memory_alloc failed.\n");
		goto Done;
	}

	cmdline_a_length = (INT)wcstombs(cmdline_a, cmdline_w, cmdline_w_length);
	if (!(cmdline_a_length))
	{
		DOSERR0("wctomb failed.\n");
		goto Done;
	}

	cmdline_a[cmdline_a_length] = '\0';

	fileOut = fopen(output_file, "w");
	if (!fileOut)
	{
		DOSERR("unable to open %s for writing.\n", output_file);
		goto Done;
	}


	line = cmdline_a;
	while((line = dcmd_parse_env_entity(line, &param, &value, &param_type)) != NULL)
	{
		matches++;
		if (matches < start_offset)
			continue;

		switch(param_type)
		{
		case PARAM_TYPE_COMMAND:
			fprintf(fileOut, "SET G_%s=%s\n", param, param);
			break;
		case PARAM_TYPE_STRING:
			fprintf(fileOut, "SET S_%d=%s\n", ++string_index, param);
			break;
		case PARAM_TYPE_PAIR:
			while (value[0] == '\"' && value[strlen(value) - 1] == '\"')
			{
				// remove surrounding double quotes.
				value++;
				value[strlen(value) - 1] = '\0';
			}

			fprintf(fileOut, "SET G_%s=%s\n", param, value);
			break;
		}

	}

	ret = 0;
	/*
		if ((argc-start_offset) < 4)
	    {
	        DOSERR0("usage: --make-args output_batch_file [param param=value]+.\n");
	        goto Done;
	    }
	*/
Done:
	if (fileOut)
	{
		fflush(fileOut);
		fclose(fileOut);
	}

	if (cmdline_a)
	{
		free(cmdline_a);
	}
	return ret;
}

INT dcmd_update_cfg(__in PCHAR cfg_filename,
                    __in PCHAR setting_name,
                    __in PCHAR setting_value)
{
	FILE* cfg_file = NULL;
	FILE* tmp_file = NULL;
	INT ret = -ERROR_BAD_ARGUMENTS;
	CHAR cfg_line[8192];

	BOOL modified = FALSE;

	cfg_file = fopen(cfg_filename, "r+");
	if (!cfg_file)
	{
		cfg_file = fopen(cfg_filename, "w+");
		if (!cfg_file)
		{
			DOSERR("failed opening %s\n", cfg_filename);
			goto Done;
		}
	}

	tmp_file = file_create_temp(NULL);
	if (!tmp_file)
	{
		goto Done;
	}

	while( (fgets(cfg_line, sizeof(cfg_line) - 1, cfg_file)) != NULL)
	{
		PCHAR sep = NULL;
		PCHAR p = cfg_line;

		// skip leading white
		while(char_is_white(*p))
			p++;

		if ((*p == '\0') ||						// if this is a blank line
		        (*p == ';') ||						// if this is a comment line
		        ((sep = strchr(p, '=')) == NULL))	// find the first equals
		{
			// write the ORIGINAL line.
			fputs(cfg_line, tmp_file);
		}
		else
		{
			if (tok_string_cmp(p, setting_name, (INT)strlen(setting_name), TRUE) == 0)
			{
				// only white spaces allowed after the name and before the equals.
				p += strlen(setting_name);
				while(*p)
				{
					// This is what we are looking for.
					if (*p == '=')
						break;

					if (!char_is_white(*p))
						break;

					p++;
				}

				if (*p == '=')
				{
					// matched a setting.
					// write the MODIFIED line.
					fprintf(tmp_file, "%s=%s\n", setting_name, setting_value);
					modified = TRUE;
				}
				else
				{
					// write the ORIGINAL line.
					fputs(cfg_line, tmp_file);
				}
			}
			else
			{
				// write the ORIGINAL line.
				fputs(cfg_line, tmp_file);
			}
		}
	}

	if (!modified)
	{
		// the setting was not found; put it at the end.
		// write the NEW line.
		fprintf(cfg_file, "%s=%s\n", setting_name, setting_value);
		ret = 1;
		goto Done;


	}

	// the setting was found and the line was updated in tmp_file.
	// write tmp_file back to cfg_file.
	rewind(tmp_file);
	rewind(cfg_file);
	fset_length(cfg_file, fget_length(tmp_file));

	file_write_remaining(tmp_file, cfg_file);

	ret = ERROR_SUCCESS;

Done:
	if (cfg_file)
	{
		fflush(cfg_file);
		fclose(cfg_file);
	}
	if (tmp_file)
	{
		fclose(tmp_file);
	}
	return ret;
}

INT dcmd_make_tokens(__in CONST PCHAR input_filename,
                     __in CONST PCHAR output_filename,
                     __in CONST PDCMD_MAKE_TOKEN_OPTIONS_T options)
{
	FILE* fileIn = NULL;
	FILE* fileOut = NULL;
	INT ret = -1;
	CHAR line[8192];
	DWORD fileInSize = 0;
	INT continuation_line_count = 0;

	fileIn = fopen(input_filename, "r");
	if (!fileIn)
	{
		DOSERR("unable to open %s for reading.\n", input_filename);
		goto Done;
	}
	fileInSize = _filelength(_fileno(fileIn));

	fileOut = fopen(output_filename, "w");
	if (!fileOut)
	{
		DOSERR("unable to open %s for writing.\n", output_filename);
		goto Done;
	}

	while (fgets(line, sizeof(line), fileIn) != NULL)
	{
		PCHAR p = line;
		PCHAR sep = NULL;
		PCHAR name = NULL;
		PCHAR value = NULL;
		PCHAR end_of_line;

		// skip leading white
		p = string_ltrim(p);

		// if this is a blank line
		if (*p == '\0')
			continue;

		// if this is a comment line
		if (*p == ';')
			continue;

		if (options->is_ddk_sources_file)
		{
			end_of_line = &p[strlen(p) - 1];
			while (*end_of_line && (char_is_white(*end_of_line)))
				end_of_line--;

			if (*end_of_line == '\\' && (char_is_white(*(end_of_line - 1)) || *(end_of_line - 1) == '='))
			{
				INT new_length;

				continuation_line_count++;

				// null out the continuation '\'
				*end_of_line = ' ';
				string_rtrim(p);

				new_length = (INT)strlen(p);
				if (new_length == 0)
				{
					// this was a blank continuation line.
					continue;
				}
				end_of_line = &p[new_length];

				// add the "path style" ';' sep
				*end_of_line = ';';

				if (continuation_line_count == 1)
				{
					// we will let the first continuation line fall through.
					// whitespaces have been removed and a ';' appended.
				}
				else
				{
					fputs(p,  fileOut);
					continue;
				}
			}
			else if (continuation_line_count > 0)
			{
				continuation_line_count = 0;
				fputs(p,  fileOut);
				continue;
			}
		}



		// find the first equals
		if ((sep = strchr(p, '=')) != NULL)
		{
			BOOL no_write_line = FALSE;
			// set the value to null
			*sep = '\0';

			// names is on the left, value is on the right.
			name = string_trim(string_rtrim(p));
			value = sep + 1;
			if (options->get_prefix[0])
			{
				if ((tok_string_cmp(name, options->get_prefix, (INT) strlen(options->get_prefix), TRUE)) != 0)
					no_write_line = TRUE;
			}
			if (!no_write_line)
			{
				if (options->is_ddk_sources_file)
				{
					value = string_ltrim(value);
				}

				if (options->set_prefix[0])
				{
					fprintf(fileOut, "SET %s%s=%s", options->set_prefix, name, value);
				}
				else
				{
					fprintf(fileOut, "SET %s=%s", name, value);
				}
			}
		}
	}

	ret = 0;

Done:
	if (fileIn)
	{
		fclose(fileIn);
	}

	if (fileOut)
	{
		fflush(fileOut);
		fclose(fileOut);
	}
	return ret;
}

INT dcmd_env_tag(PCHAR input_filename, PCHAR output_filename, PCHAR env_prefix)
{
	FILE* fileIn = NULL;
	FILE* fileOut = NULL;
	INT ret = -1;
	LPCH env_list	= NULL;
	LPCH env_string = NULL;
	LPCH next_env_string = NULL;
	PTOKENIZER_CONTEXT tokenizer = NULL;
	PCHAR pv_sep;
	INT length_ltag;
	INT length_rtag;
	CHAR string_ltag[128];
	CHAR string_rtag[128];

	length_ltag = GetEnvironmentVariable("TOKVAR_LTAG", string_ltag, sizeof(string_ltag));
	if (length_ltag <= 0 || length_ltag >= sizeof(string_ltag))
		tok_string_cpy(string_ltag, sizeof(string_ltag), "$(");
	else
		string_ltag[length_ltag] = '\0';  // don't trust windows to self terminate.


	length_rtag = GetEnvironmentVariable("TOKVAR_RTAG", string_rtag, sizeof(string_rtag));
	if (length_rtag <= 0 || length_rtag >= sizeof(string_rtag))
		tok_string_cpy(string_rtag, sizeof(string_rtag), ")");
	else
		string_rtag[length_rtag] = '\0';  // don't trust windows to self terminate.

	tokenizer = tok_create(string_ltag, string_rtag, TRUE, TRUE);

	next_env_string = env_string = env_list = GetEnvironmentStringsA();
	if (!env_list)
	{
		DOSERR0("GetEnvironmentStringsA failed.\n");
		goto Done;
	}

	while (*next_env_string)
	{
		LONG param_len;
		LONG value_len;
		PCHAR env_param;
		PCHAR env_value;

		env_string = _strdup(next_env_string);
		next_env_string = next_env_string + strlen(next_env_string) + 1;

		pv_sep = strchr(env_string, '=');
		if (!pv_sep)
			goto NextEnvString;

		env_param = (PCHAR)(env_string);
		env_value = (PCHAR)(pv_sep + 1);
		if (!env_param || !env_value)
			goto NextEnvString;

		value_len = (LONG)strlen(env_value);
		param_len = (LONG)(strlen(env_param) - value_len) - 1;
		if (value_len <= 0 || param_len <= 0)
			goto NextEnvString;

		if (param_len == 1 && env_param[0] == '=')
			goto NextEnvString;

		if (env_prefix)
		{
			if (tok_string_cmp(env_param, env_prefix, (INT)strlen(env_prefix), TRUE) == 0)
			{
				DOSOFF("adding %s=%s\n", env_param, env_value);
				tok_set_value_ex(tokenizer, env_param, param_len, env_value, value_len);
			}
		}
		else
		{
			DOSOFF("adding %s=%s\n", env_param, env_value);
			tok_set_value_ex(tokenizer, env_param, param_len, env_value, value_len);
		}
NextEnvString:
		if (env_string)
		{
			free(env_string);
			env_string = NULL;
		}
	}

	fileIn = fopen(input_filename, "r");
	if (!fileIn)
	{
		DOSERR("unable to open %s for reading.\n", input_filename);
		goto Done;
	}
	fileOut = fopen(output_filename, "a");
	if (!fileOut)
	{
		DOSERR("unable to open %s for writing.\n", output_filename);
		goto Done;
	}

	ret = tok_file(tokenizer, fileIn, fileOut);
	if (ret < 0)
	{
		DOSERR("failed tokenizing file %s to %s.\n",
		       input_filename, output_filename);
	}
	else
	{
		DOSDBG("tokenized file %s to %s (%d bytes).\n",
		       input_filename, output_filename, ret);
		ret = 0;
	}

Done:

	if (env_list)
		FreeEnvironmentStringsA(env_list);

	if (fileIn)
	{
		fclose(fileIn);
	}

	if (fileOut)
	{
		fflush(fileOut);
		fclose(fileOut);
	}

	if (tokenizer)
	{
		tok_destroy(&tokenizer);
	}

	if (env_list)
	{
		FreeEnvironmentStrings(env_list);
	}
	return ret;
}

static INT dcmd_fprintf_internal(__in FILE* out,
                                 __in HANDLE std,
                                 __in PCHAR format,
                                 __in PDCMD_FPRINTF_OPTIONS_T options,
                                 __deref_in PCHAR* arg_list,
                                 __in INT arg_count)
{
	INT ret = -ERROR_BAD_ARGUMENTS;
	dcmd_fprintf_t dcmd;

	tok_memory_zero(&dcmd, sizeof(dcmd));

	dcmd.out = out;
	dcmd.std = std;

	dcmd.src.fmt = format;
	dcmd.src.length = (INT)strlen(format);

	memcpy(&dcmd.options, options, sizeof(dcmd.options));

	dcmd.arg.list = arg_list;
	dcmd.arg.count = arg_count;

	if (dcmd.std)
	{
		if (!GetConsoleScreenBufferInfo(dcmd.std, &dcmd.con_info))
		{
			DOSWRN0("unable to get console screen buffer info.\n");
			std = NULL;
		}
	}
	// This buffer needs to be large enough to hold a single format pair.
	dcmd.buf.max = FMTBUF_MAX;
	dcmd.buf.data = FMTBUF;
	/*
	if ((dcmd.buf.data = tok_memory_alloc(dcmd.buf.max)) == NULL)
	{
		ret = -ERROR_NOT_ENOUGH_MEMORY;
		goto Done;
	}
	*/
	// parse the doscmd format string into a 'C' format string and write
	// the output.
	ret = fprintf_parse_and_print(&dcmd);

	if (ret < 0)
	{
		DOSERR0("failed parsing fprintf statement.\n");
		goto Done;
	}

	// TODO: SUCCESS
	// All done.  Anything we want to do after work
	// has been successfully completed goes here.

Done:
	if (ret < 0)
	{
		// TODO: ERROR
		// We are finished but an error occured.
	}

	////////////////////////////////////////
	// resource cleanup on success or error.
	////////////////////////////////////////

	// free the temp buffer
	//if (dcmd.buf.data)
	//	tok_memory_free(&dcmd.buf.data);

	// close the output file (if it really was one)
	if (dcmd.out && !dcmd.std)
	{
		// flush the cache and close the file.
		fflush(dcmd.out);
		fclose(dcmd.out);
	}

	return ret;
}

INT dcmd_fprintf(__in PCHAR out_fname,
                 __in PCHAR format,
                 __in PDCMD_FPRINTF_OPTIONS_T options,
                 __deref_in PCHAR* arg_list,
                 __in INT arg_count)
{
	FILE* out = NULL;
	HANDLE std = NULL;

	// configure the output
	//
	if (DCMD_IS_OUTPUT(out_fname, "stderr"))
	{
		// if writing to std we can support colors.
		out = stderr;
		std = GetStdHandle(STD_ERROR_HANDLE);
	}
	else if(DCMD_IS_OUTPUT(out_fname, "stdout") || DCMD_IS_OUTPUT(out_fname, "con"))
	{
		// if writing to std we can support colors.
		out = stdout;
		std = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else
	{
		// open the file.
		if ((out = fopen(out_fname, "a")) == NULL)
		{
			DOSERR("failed opening file %s.\n", out_fname);
			DOS_E_RET(ERROR_OPEN_FAILED);
		}
	}

	if (std == INVALID_HANDLE_VALUE)
	{
		std = NULL;
		DOSWRN0("unable to get console handle.\n");
	}
	else
	{

	}

	return dcmd_fprintf_internal(out, std, format, options, arg_list, arg_count);
}


INT make_tokens_parse_options(PCHAR src_arg,  PDCMD_MAKE_TOKEN_OPTIONS_T options)
{
	PCHAR param, value;
	PCHAR more_options;
	PARAM_TYPE param_type;

	if (!options || !src_arg)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	// start with the default options
	tok_memory_zero(options, sizeof(*options));

	if (!src_arg[0])
		return ERROR_SUCCESS;

	more_options = src_arg;
	while((more_options = dcmd_parse_option(more_options, &param, &value, &param_type, ";")) != NULL)
	{
		if (!param || !value)
			return ERROR_SUCCESS;

		if (tok_string_cmp(param, "set-prefix", 10, TRUE) == 0)
		{
			tok_string_cpy(options->set_prefix, sizeof(options->set_prefix), value);
		}
		else if (tok_string_cmp(param, "get-prefix", 10, TRUE) == 0)
		{
			tok_string_cpy(options->get_prefix, sizeof(options->get_prefix), value);
		}
		else if (tok_string_cmp(param, "sources", 7, TRUE) == 0)
		{
			options->is_ddk_sources_file = TRUE;
		}
		else
		{
			DOSERR("invalid make-token option %s.\n", param);
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

	}

	return ERROR_SUCCESS;
}


// parses the options that come after --fprintf=
//
INT fprintf_parse_options(PCHAR src_arg,  PDCMD_FPRINTF_OPTIONS_T options)
{
	PCHAR sep, sep_last;
	PCHAR param, value;
	PARAM_TYPE param_type;

	if (!options || !src_arg)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	// start with the default options
	options->mark = '@';
	options->escape = '\\';

	options->all[0] = options->mark;
	options->all[1] = options->escape;
	options->all[2] = '\0';

	if (!src_arg[0])
		return ERROR_SUCCESS;

	sep = sep_last = src_arg;
	while (sep_last != NULL)
	{
		sep = strpbrk(sep, ";");

		// found an option terminator; parse it.
		if (sep)
		{
			// use this sep as a terminator and advance.
			*sep = '\0';
			sep++;
		}
		else
		{
			// this one was a final terminating sep.
			if (!strlen(sep_last))
				break;
		}

		dcmd_parse_env_entity(sep_last, &param, &value, &param_type);
		sep_last = sep;

		// these are requre a pair
		if (!param[0] || !value[0] || param_type != PARAM_TYPE_PAIR)
			goto Error;

		{
			if ((tok_string_cmp(param, "mrk", 3, TRUE)) == 0)
			{
				options->mark = value[0];
			}
			else if ((tok_string_cmp(param, "esc", 3, TRUE)) == 0)
			{
				options->escape = value[0];
			}
			else
			{
				goto Error;
			}
		}

		if (!sep) break;
	}

	options->all[0] = options->mark;
	options->all[1] = options->escape;
	options->all[2] = '\0';

	return ERROR_SUCCESS;

Error:
	DOSERR("unknown fprintf option %s.\n",
	       sep_last);

	DOS_E_RET(ERROR_BAD_ARGUMENTS);

}

static PCHAR get_param(__inout PCHAR param,
                       __in PCHAR line,
                       __in CONST CHAR separator,
                       __in CONST CHAR comment,
                       __in BOOL is_sep_required)
{
	PCHAR sep;
	INT param_length;
	PCHAR value_start = NULL;

	while(char_is_white(*line))
		line++;

	// if empty or comment line
	if (*line == '\0' || *line == comment)
		return NULL;

	// if invalid line
	if ((sep = strchr(line, separator)) == NULL && is_sep_required)
		return NULL;

	if (sep == NULL)
	{
		// is_sep_required must have been false, and there wasn't one.
		// this means if it's not found, the end of line is used.
		sep = line + strlen(line);
		value_start  = sep;
	}
	else
	{
		value_start = sep + 1;
	}

	// get rid of trailing white.
	while (char_is_white(*(--sep)));

	// sep is now sitting on the last char in the param; we must inc++ for our length calculation
	sep++;
	param_length = (INT)(sep - line);

	// All of the version string params are limited to MAX_PATH
	if (tok_string_ncpy(param, MAX_PATH, line, param_length) != 0)
	{
		DOSERR0("string to long.\n");
		return NULL;
	}

	// return a pointer to the next param
	return value_start;
}

static INT version_control_get_item(__in PCHAR line,
                                    __deref_out PVERSION_CONTROL_ITEM_T* item)
{
	INT ret = ERROR_SUCCESS;
	PVERSION_CONTROL_ITEM_T pItem = NULL;
	CHAR strings[4][MAX_PATH];

	/*
	doscmd.exe=0.0.0.1; .\doscmd_version_h.in; .\src\doscmd_version.h;
	doscmd.dll=doscmd.exe; .\doscmd_version_h.in; .\src\doscmd_version.h;
	*/

	// target name
	if ((line = get_param(strings[0], line, '=', ';', TRUE)) == NULL)
		return -1;

	// version or linked target name
	if ((line = get_param(strings[1], line, ';', '\0', TRUE)) == NULL)
		return -1;

	// version template file path
	if ((line = get_param(strings[2], line, ';', '\0', TRUE)) == NULL)
		return -1;

	// tagged version output file path
	if ((line = get_param(strings[3], line, ';', '\0', FALSE)) == NULL)
		return -1;

	pItem = *item = malloc(sizeof(**item));
	memset(pItem, 0, sizeof(**item));

	//copy in the target name
	tok_string_cpy(pItem->target_name, MAX_PATH, strings[0]);

	ret = sscanf_s(strings[1], "%u.%u.%u.%u",
	               &pItem->version.major,
	               &pItem->version.minor,
	               &pItem->version.micro,
	               &pItem->version.nano);

	if (ret != 4)
	{
		//copy in the linked target name
		tok_string_cpy(pItem->linked_name, MAX_PATH, strings[1]);
		pItem->is_linked = TRUE;
	}

	//copy in the version template file path
	tok_string_cpy(pItem->version_template_filepath, MAX_PATH, strings[2]);

	//copy in the version output file path
	tok_string_cpy(pItem->version_output_filepath, MAX_PATH, strings[3]);


	return ERROR_SUCCESS;
}

// parses the options that come after --fprintf=
//
INT version_control_parse_options(PCHAR src_arg,  PDCMD_VERSION_CONTROL_OPTIONS_T options)
{
	PCHAR sep, sep_last;
	PCHAR param, value;
	PARAM_TYPE param_type;

	if (!options || !src_arg)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	// start with the default options
	memset(options, 0, sizeof(*options));
	options->action = VCA_TAG;

	if (!src_arg[0])
		return ERROR_SUCCESS;

	sep = sep_last = src_arg;
	while (sep_last != NULL)
	{
		sep = strpbrk(sep, ";");

		// found an option terminator; parse it.
		if (sep)
		{
			// use this sep as a terminator and advance.
			*sep = '\0';
			sep++;
		}
		else
		{
			// this one was a final terminating sep.
			if (!strlen(sep_last))
				break;
		}

		dcmd_parse_env_entity(sep_last, &param, &value, &param_type);
		sep_last = sep;

		// these are requre a pair
		if (!param[0] || !value[0])
			goto Error;

		if ((tok_string_cmp(param, "tag", 3, TRUE)) == 0)
		{
			options->action = VCA_TAG;
		}
		else if ((tok_string_cmp(param, "inc", 3, TRUE)) == 0)
		{
			options->action = VCA_INC;
		}
		else if ((tok_string_cmp(param, "dec", 3, TRUE)) == 0)
		{
			options->action = VCA_DEC;
		}
		else
		{
			goto Error;
		}
		if (options->action & VCA_INC || options->action & VCA_DEC)
		{
			if ((tok_string_cmp(value, "nano", -1, TRUE)) == 0)
				options->action |= VCA_NANO;
			else if ((tok_string_cmp(value, "micro", -1, TRUE)) == 0)
				options->action |= VCA_MICRO;
			else if ((tok_string_cmp(value, "minor", -1, TRUE)) == 0)
				options->action |= VCA_MINOR;
			else if ((tok_string_cmp(value, "major", -1, TRUE)) == 0)
				options->action |= VCA_MAJOR;
			else
				goto Error;
		}

		if (!sep) break;
	}

	return ERROR_SUCCESS;

Error:
	DOSERR("unknown version_control option %s.\n",
	       sep_last);

	DOS_E_RET(ERROR_BAD_ARGUMENTS);

}

static LPCSTR version_info_get_item(LPCSTR InfoItem, LPCSTR szFullPath)
{
	static     char    szResult[256] = {0};
	char    szGetName[256];
	LPSTR   lpVersion;        // String pointer to Item text
	DWORD   dwVerInfoSize;    // Size of version information block
	DWORD   dwVerHnd = 0;      // An 'ignored' parameter, always '0'
	UINT    uVersionLen;
	BOOL    bRetCode;

	dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
	if (dwVerInfoSize)
	{
		LPSTR   lpstrVffInfo;
		HANDLE  hMem;
		hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
		lpstrVffInfo  =  (LPSTR)GlobalLock(hMem);
		GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);

		// Get a codepage from base_file_info_sctructure
		lstrcpy(szGetName, "\\VarFileInfo\\Translation");

		uVersionLen   = 0;
		lpVersion     = NULL;
		bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
		                         (LPSTR)szGetName,
		                         (void**)&lpVersion,
		                         (UINT*)&uVersionLen);
		if ( bRetCode && uVersionLen && lpVersion)
		{
			sprintf(szResult, "%04x%04x", (WORD)(*((DWORD*)lpVersion)),
			        (WORD)(*((DWORD*)lpVersion) >> 16));
//            lstrcpy(szResult, lpVersion);
		}
		else
		{
			// 041904b0 is a very common one, because it means:
			//   US English/Russia, Windows MultiLingual characterset
			// Or to pull it all apart:
			// 04------        = SUBLANG_ENGLISH_USA
			// --09----        = LANG_ENGLISH
			// --19----        = LANG_RUSSIA
			// ----04b0 = 1200 = Codepage for Windows:Multilingual
			lstrcpy(szResult, "041904b0");
		}

		// Add a codepage to base_file_info_sctructure
		sprintf (szGetName, "\\StringFileInfo\\%s\\", szResult);
		// Get a specific item
		lstrcat (szGetName, InfoItem);

		uVersionLen   = 0;
		lpVersion     = NULL;
		bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
		                         (LPSTR)szGetName,
		                         (void**)&lpVersion,
		                         (UINT*)&uVersionLen);
		if ( bRetCode && uVersionLen && lpVersion)
		{
			lstrcpy(szResult, lpVersion);
		}
		else
		{
			lstrcpy(szResult, "");
		}

	}
	return szResult;
}



INT file_line_parse_options(PCHAR src_arg,  PDCMD_FILE_LINE_OPTIONS_T options)
{
	PCHAR next_option_string, param, value;

	if (!options || !src_arg)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	// start with the default options
	memset(options, 0, sizeof(*options));
	options->action = LINE_UPDATE;
	options->start_after = TRUE;

	if (src_arg[0] == '=')
		src_arg++;

	if (!src_arg[0])
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	options->line_index = strtoul(src_arg, &src_arg, 0);
	if (src_arg[0] == ';')
		src_arg++;

	next_option_string = src_arg;
	while ((next_option_string = dcmd_parse_next_option(next_option_string, &param, &value, '=', ';', '\"')) != NULL)
	{
		if (!strlen(param)) continue;

		if ((tok_string_cmp(param, "insert", -1, TRUE)) == 0)
		{
			options->action = LINE_INSERT;
		}
		else if ((tok_string_cmp(param, "update", -1, TRUE)) == 0)
		{
			options->action = LINE_UPDATE;
		}
		else if ((tok_string_cmp(param, "delete", -1, TRUE)) == 0)
		{
			options->action = LINE_DELETE;
		}
		else if ((tok_string_cmp(param, "char-start-index", -1, TRUE)) == 0)
		{
			options->char_start_index = (INT)strtol(value, NULL, 0);
		}
		else if ((tok_string_cmp(param, "start", -1, TRUE)) == 0 ||
		         (tok_string_cmp(param, "start-after", -1, TRUE)) == 0)
		{
			options->start_string = value;
			options->start_after = TRUE;
		}
		else if ((tok_string_cmp(param, "start-before", -1, TRUE)) == 0)
		{
			options->start_string = value;
			options->start_after = FALSE;
		}
		else
		{
			DOSERR("Invalid option %s\n", param);
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}
	}

	return ERROR_SUCCESS;
}





INT get_commandline_parse_options(PCHAR src_arg,  PDCMD_GET_COMMANDLINE_OPTIONS_T options)
{
	PCHAR next_option_string, param, value;

	if (!options || !src_arg)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	// start with the default options
	memset(options, 0, sizeof(*options));
	options->arg_start_offset = 2;

	if (src_arg[0] == '=')
		src_arg++;

	if (!src_arg[0])
		return ERROR_SUCCESS;

	next_option_string = src_arg;
	while ((next_option_string = dcmd_parse_next_option(next_option_string, &param, &value, '=', ';', '\"')) != NULL)
	{
		if ((tok_string_cmp(param, "split", -1, TRUE)) == 0)
		{
			if ((tok_string_cmp(value, "0x", 2, TRUE)) == 0)
				options->split_char = (CHAR)strtoul(value, NULL, 0);
			else
				options->split_char = value[0];
		}
		else if ((tok_string_cmp(param, "escaped", -1, TRUE)) == 0)
		{
			CHAR hex_string[5];
			PCHAR next_src_escape = value;
			PCHAR next_dst_escape = options->escaped;

			memset(hex_string, 0, sizeof(hex_string));
			hex_string[0] = '0';
			hex_string[1] = 'x';

			while (next_src_escape && strlen(next_src_escape) > 1)
			{
				CHAR ch_value;
				hex_string[2] = next_src_escape[0];
				hex_string[3] = next_src_escape[1];
				ch_value = (CHAR)strtoul(hex_string, NULL, 0);
				next_src_escape = strpbrk(next_src_escape, ",; ");

				if (next_src_escape)
					next_src_escape++;

				if (ch_value)
				{
					if (!options->escape_char)
						options->escape_char = ch_value;

					*next_dst_escape = ch_value;
					next_dst_escape++;
				}
			}
		}
		else if ((tok_string_cmp(param, "notrim", -1, TRUE)) == 0)
			options->no_trimming = TRUE;

		else if ((tok_string_cmp(param, "strip_quotes", -1, TRUE)) == 0)
			options->strip_quotes = TRUE;

		else if ((tok_string_cmp(param, "prefix", -1, TRUE)) == 0)
			strcpy_s(options->arg_prefix, sizeof(options->arg_prefix), value);

		else if ((tok_string_cmp(param, "suffix", -1, TRUE)) == 0)
			strcpy_s(options->arg_suffix, sizeof(options->arg_suffix), value);

		else if ((tok_string_cmp(param, "header", -1, TRUE)) == 0)
			strcpy_s(options->header, sizeof(options->header), value);

		else if ((tok_string_cmp(param, "footer", -1, TRUE)) == 0)
			strcpy_s(options->footer, sizeof(options->footer), value);

		else if ((tok_string_cmp(param, "start", -1, TRUE)) == 0)
			options->arg_start_offset += atoi(value);

		else
		{
			DOSERR("Invalid get_commandline %s\n", param);
			return -1;
		}
	}

	return ERROR_SUCCESS;
}

PCHAR string_escape(CONST PCHAR src, CHAR escape_char, CONST PCHAR chars_to_escape)
{
	size_t alloc_size;
	PCHAR dst;
	PCHAR src_next, src_last;
	PCHAR dst_next;

	alloc_size = (strlen(src) * 2) + 5;
	dst = dst_next = LocalAlloc(LPTR, alloc_size);
	//memset(dst, 0, alloc_size);

	src_next = src_last = src;
	while ((src_next = strpbrk(src_next, chars_to_escape)) != NULL)
	{
		INT src_span = (INT) (src_next - src_last);
		if (src_span > 0 && alloc_size > 0)
		{
			strncpy(dst_next, src_last, src_span);
			dst_next += src_span;
			alloc_size -= src_span;
		}
		dst_next[2] = '\0';
		dst_next[0] = escape_char;
		dst_next[1] = *src_next;
		dst_next += 2;
		src_next += 1;
		src_last = src_next;
	}
	if (src_last && strlen(src_last))
		strcpy(dst_next, src_last);

	return dst;
}

INT dcmd_get_commandline()
{
	INT ret = ERROR_SUCCESS;
	LPSTR cmd_line = GetCommandLineA();
	INT arg_index = 0;
	PCHAR param, value;
	INT arg_command_offset = 1;
	dcmd_get_commandline_options_t options = {0};
	CHAR split_string[2] = {0};

	while (cmd_line)
	{
		if ((cmd_line = dcmd_parse_next_option(cmd_line, &param, &value, '\0', ' ', '\"')) == NULL)
			break;

		if (arg_index < arg_command_offset)
		{
			arg_index++;
			continue;
		}
		else if (arg_index == arg_command_offset)
		{
			PCHAR param_value = main_parse_argv(param, "-gc", "--get-commandline");
			if ((ret = get_commandline_parse_options(param_value, &options)) < 0)
				return ret;

			arg_index++;
			continue;
		}
		else if (arg_index < options.arg_start_offset)
		{
			arg_index++;
			continue;
		}
		else
		{
			PCHAR escaped_param = NULL;
			INT param_length;

			if (arg_index == options.arg_start_offset && options.header[0])
				printf("%s", options.header);
			arg_index++;

			if (options.escape_char)
			{
				param = escaped_param = string_escape(param, options.escape_char, options.escaped);
			}

			if (!options.no_trimming)
			{
				param = string_ltrim(param);
				param = string_rtrim(param);
			}
			param_length = (INT)strlen(param);

			if (param[0] == '\"' && param[param_length - 1] == '\"')
			{
				param++;
				param_length = (INT)strlen(param);
				if (param_length > 0)
					param[param_length - 1] = '\0';

				if (options.strip_quotes)
				{
					printf("%s%s%s%s", split_string, options.arg_prefix, param, options.arg_suffix);
				}
				else
				{
					printf("%s\"%s%s%s\"", split_string, options.arg_prefix, param, options.arg_suffix);
				}
			}
			else
			{
				printf("%s%s%s%s", split_string, options.arg_prefix, param, options.arg_suffix);
			}

			if (escaped_param)
				LocalFree(escaped_param);
			escaped_param = NULL;

			split_string[0] = (options.split_char ? options.split_char : ' ');
			split_string[1] = '\0';
		}
	}
	if (options.footer[0])
		printf("%s", options.footer);

	if (options.split_char != 0xA)
		printf("\n");

	return ret;
}

static INT file_line_upate_line(FILE* output_file, FILE* line_file, PDCMD_FILE_LINE_OPTIONS_T options)
{
	INT ret = ERROR_SUCCESS;
	CHAR line[8192];
	PCHAR new_line;

	while ((new_line = fgets(line, sizeof(line) - 1, line_file)) != NULL)
	{
		INT new_line_length = (INT)strlen(new_line);
		if (options->char_start_index)
		{
			if (options->char_start_index < new_line_length)
				new_line += options->char_start_index;
		}
		else if (options->start_string)
		{
			new_line = strstr(new_line, options->start_string);
			if (!new_line)
				new_line = line;
			else if (options->start_after)
				new_line += strlen(options->start_string);
		}

		// WRITE NEW LINE
		fputs(new_line, output_file);
	}

	return ret;
}

INT dcmd_file_line(PDCMD_FILE_LINE_OPTIONS_T options)
{
	CHAR line[4192];
	INT ret = ERROR_SUCCESS;
	INT index = 0;
	FILE* output_file = fopen(options->out_fname, "a");
	FILE* in_file = fopen(options->in_fname, "r");
	FILE* line_file = fopen(options->line_fname, "r");
	FILE* line_temp_file = file_create_temp(NULL);

	if (!output_file || !in_file || !line_file)
	{
		DOSERR0("failed opening file.\n");
		ret = -ERROR_FILE_INVALID;
		goto Done;
	}

	file_write_remaining(line_file, line_temp_file);
	rewind(line_temp_file);

	switch (options->action)
	{
	case LINE_DELETE:
		ret = -1;
		while((fgets(line, sizeof(line) - 1, in_file)) != NULL)
		{
			if (options->line_index != index)
				fputs(line, output_file);
			else
				ret = ERROR_SUCCESS;
			index++;
		}
		break;
	case LINE_INSERT:
	case LINE_UPDATE:
		index = -1;
		while((fgets(line, sizeof(line) - 1, in_file)) != NULL)
		{
			index++;

			if (options->line_index != index)
			{
				fputs(line, output_file);
				continue;
			}

			if (options->action == LINE_INSERT)
				fputs(line, output_file);

			if ((ret = file_line_upate_line(output_file, line_temp_file, options)) != ERROR_SUCCESS)
				goto Done;
		}
		if (options->line_index == -1 && options->action == LINE_INSERT)
			if ((ret = file_line_upate_line(output_file, line_temp_file, options)) != ERROR_SUCCESS)
				goto Done;
		break;
	}

Done:
	if (output_file)
	{
		fflush(output_file);
		fclose(output_file);
	}
	if (in_file)
		fclose(in_file);

	if (line_file)
		fclose(line_file);

	if (line_temp_file)
		fclose(line_temp_file);


	return ret;
}

INT main(INT argc, PCHAR argv[])
{

	PCHAR param_value;
	PCHAR in_fname = NULL;
	PCHAR out_fname = NULL;

	INT ret = -1;
	INT argIndex;

	for (argIndex = 1; argIndex < argc; argIndex++)
	{
		DOSDBG("ARG%u=%s\n", argIndex, argv[argIndex]);
	}

	if (argc < 2)
	{
		show_help();
		goto Done;
	}

	//DOSDBG("doscmd-action = %s\n", argv[1]);
	if ((param_value = main_parse_argv(argv[1], "-et", "--env-tag")) != NULL)
	{
		DOSDBG("param_value=%s\n", param_value);

		if (argc < 4)
		{
			DOSERR0("usage: --env-tag[=EnvVariablePrefix] <in-file-name> <out-file-name>.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		in_fname =  argv[2];
		out_fname =  argv[3];

		if ((strlen(param_value) > 1) && tok_string_cmp(param_value, "=", 1, FALSE) == 0)
		{
			param_value += 1;
			DOSDBG("Using only env vars starting with %s\n", param_value);
			ret = dcmd_env_tag(in_fname, out_fname, param_value);

		}
		else
		{
			DOSDBG0("Using all env vars\n");
			ret = dcmd_env_tag(in_fname, out_fname, NULL);
		}
	}
	else if ((param_value = main_parse_argv(argv[1], "-rp", "--relative-path")) != NULL)
	{
		char rel_path[MAX_PATH];
		char* from_attr = argv[2];
		char* from_path = argv[3];
		char* to_attr = argv[4];
		char* to_path = argv[5];

		DOSDBG("param_value=%s\n", param_value);

		memset(&rel_path, 0, sizeof(rel_path));
		if (PathRelativePathToA(rel_path,
		                        from_path,
		                        (from_attr[0] != 'd' ? FILE_ATTRIBUTE_DIRECTORY : 0),
		                        to_path,
		                        (to_attr[0] != 'd' ? FILE_ATTRIBUTE_DIRECTORY : 0)))
		{
			printf("%s\n", rel_path);
		}
		else
		{
			printf("%s\n", to_path);
		}
	}
	else if ((param_value = main_parse_argv(argv[1], "-mt", "--make-tokens")) != NULL)
	{
		dcmd_make_token_options_t options;
		DOSDBG("options=%s\n", param_value);

		if (argc < 4)
		{
			DOSERR0("usage: --make-tokens[=options] <in-file-name> <out-file-name>.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}
		if (param_value[0] == '=')
			param_value++;

		if ((ret = make_tokens_parse_options(param_value, &options)) < 0)
		{
			DOSERR0("invalid options argument.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		in_fname = argv[2];
		out_fname = argv[3];
		ret = dcmd_make_tokens(in_fname, out_fname, &options);

	}
	else if ((param_value = main_parse_argv(argv[1], "-fl", "--file-line")) != NULL)
	{
		dcmd_file_line_options_t options;
		DOSDBG("options=%s\n", param_value);

		if (argc < 4 || strlen(param_value) < 2)
		{
			DOSERR0("usage: --file-line=<line-index>[;add/update/delete] <in-file-name> <out-file-name> [<line-file-name>].\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		if ((ret = file_line_parse_options(param_value, &options)) < 0)
			return ret;

		options.in_fname = argv[2];
		options.out_fname = argv[3];
		if (argc > 4)
			options.line_fname = argv[4];

		ret = dcmd_file_line(&options);
	}
	else if ((param_value = main_parse_argv(argv[1], "-ma", "--make-args")) != NULL)
	{
		if (strlen(param_value) > 1 && *param_value == '=')
		{
			param_value += 1;
			DOSDBG("making tokens args starting with %i\n", 2 + atoi(param_value));
			ret = dcmd_make_args(argv[2], 4 + atoi(param_value));
		}
		else
		{
			if (argc < 4)
			{
			}
			DOSDBG0("Using all args\n");
			ret = dcmd_make_args(argv[2], 4);
		}
	}
	else if ((param_value = main_parse_argv(argv[1], "-uc", "--update-cfg")) != NULL)
	{
		PCHAR setting_name;
		PCHAR cfg_filename;
		PCHAR setting_value;

		if ((argc < 4) || (strlen(param_value) < 2 || *param_value != '='))
		{
			DOSERR0("usage: --update-cfg=<SettingName> <cfg-file-name> <NewValue>\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}
		setting_name = param_value + 1;
		cfg_filename = argv[2];
		setting_value =  argv[3];

		DOSDBG("SettingName=%s cfg-file-name=%s NewValue\n",
		       setting_name, cfg_filename, setting_value);

		if ((ret = dcmd_update_cfg(cfg_filename, setting_name, setting_value)) < 0)
		{
			DOSERR("failed updating setting %s in config file %s\n",
			       setting_name, cfg_filename);
		}
	}
	else if ((param_value = main_parse_argv(argv[1], "-ff", "--fprintf")) != NULL)
	{
		PCHAR format;
		dcmd_fprintf_options_t options;

		if (argc < 4)
		{
			DOSERR0("usage: --fprintf[=FormatOptions] <out-file-name> <format> <params>+.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		if (param_value[0] == '=')
			param_value++;

		if ((ret = fprintf_parse_options(param_value, &options)) < 0)
		{
			DOSERR0("invalid options argument.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		// get output filename
		out_fname = argv[2];
		format = argv[3];

		ret = dcmd_fprintf(out_fname, format, &options, &argv[4], argc - 4);
	}
	else if ((param_value = main_parse_argv(argv[1], "-vc", "--version-control")) != NULL)
	{
		dcmd_version_control_options_t options;

		if (argc < 4)
		{
			DOSERR0("usage: --version-control=tag/incnano/incmicro <version-file> <target-name>\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		if (param_value[0] == '=')
			param_value++;

		if ((ret = version_control_parse_options(param_value, &options)) < 0)
		{
			DOSERR0("invalid options argument.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}
		options.versions_file = argv[2];
		options.target_name = argv[3];

		ret = dcmd_version_control(&options);
	}
	else if ((param_value = main_parse_argv(argv[1], "-gc", "--get-commandline")) != NULL)
	{

		if (argc < 3)
		{
			DOSERR0("usage: --get-commandline=[start=<ArgStartIndex>;prefix=<ArgPrefixToAdd>;suffix=<ArgSuffixToAdd>;split=(<ArgSepChar>|<0xArgSepCharHexByte>);] [arg]+\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}


		ret = dcmd_get_commandline();
	}
	else if ((param_value = main_parse_argv(argv[1], "-sp", "--split")) != NULL)
	{
		PCHAR split, last_split;

		if (param_value[0] == '=')
			param_value++;

		if (argc < 3 || strlen(param_value) < 1)
		{
			DOSERR0("usage: --split=SplitOnChar String\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}
		split = last_split = argv[2];
		while((split = strpbrk(last_split, param_value)) != NULL)
		{
			*split = '\0';

			if (strlen(last_split))
				printf("%s\n", last_split);
			last_split = split + 1;
		}
		if (strlen(last_split))
			printf("%s\n", last_split);
		ret = 0;
	}
	else if ((param_value = main_parse_argv(argv[1], "-v", "--version")) != NULL)
	{
		dcmd_fprintf_options_t options;
		PCHAR format;

		if (param_value[0] == '=')
			param_value++;

		if ((ret = fprintf_parse_options(param_value, &options)) < 0)
		{
			DOSERR0("invalid options argument.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		// get output filename
		out_fname = strdup("stdout");
		format = strdup("\\k0EDOSCMD\\k06:\\k0B v" RC_VERSION_STR " \\k03(\\k0B" VERSION_DATE_STR "\\k03)\\k07\\n");

		dcmd_fprintf(out_fname, format, &options, &argv[2], 0);
		free(out_fname);
		free(format);
		ret = 0;
	}
	else if ((param_value = main_parse_argv(argv[1], "-cw", "--clean-wdklog")) != NULL)
	{
		FILE* in_file;
		FILE* out_file;
		CHAR line[8192];
		PTOKENIZER_CONTEXT tokenizer;

		if (argc < 3 || param_value[0] != '=' || strlen(param_value) < 4)
		{
			DOSERR0("usage: --clean-wdklog=<err/wrn/log> <in-file-name>.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}
		param_value += 1;
		in_fname =  argv[2];

		out_file = stdout;
		if ((in_file = fopen(in_fname, "r")) == NULL)
		{
			ret = -ERROR_ACCESS_DENIED;
			DOSERR("failed opening file %s for reading.\n", in_fname);
			goto Done;
		}

		GetCurrentDirectoryA(sizeof(line) - 1, line);
		tokenizer = tok_create("", "", TRUE, FALSE);
		tok_set_value(tokenizer, line, ".");
		tok_set_value(tokenizer, "1>", "");
		tok_set_value(tokenizer, "2>", "");
		tok_set_value(tokenizer, "3>", "");
		tok_set_value(tokenizer, "4>", "");
		tok_set_value(tokenizer, "5>", "");
		tok_set_value(tokenizer, "6>", "");
		tok_set_value(tokenizer, "7>", "");
		tok_set_value(tokenizer, "8>", "");

		tok_file(tokenizer, in_file, out_file);
		tok_destroy(&tokenizer);

		/*		while((pLine=fgets(line, sizeof(line) - 1, in_file)) !=NULL)
				{
					fputs(pLine,out_file);
				}
		*/
		fclose(in_file);
		ret = 0;

	}
	else if ((param_value = main_parse_argv(argv[1], "-dvi", "--dump-version-info")) != NULL)
	{
		CHAR CONST* item;
		FILE* out_file = stdout;
		LPCSTR version_keys_string;
		INT version_key_index = 0;
		if (argc < 3)
		{
			DOSERR0("usage: --dump-version-info <in-file-name>.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		if (param_value[0] != '\0')
			param_value += 1;

		in_fname =  argv[2];
		while ((version_keys_string = version_keys_strings[version_key_index++]) != NULL)
		{
			item = version_info_get_item(version_keys_string, in_fname);
			if (item)
			{
				fprintf(out_file, "%s=%s\n", version_keys_string, item);
			}
		}

		ret = 0;

	}
	else if ((param_value = main_parse_argv(argv[1], "-gk", "--getkey")) != NULL)
	{
		PCHAR format =  argv[2];
		dcmd_fprintf_options_t options;
		CONST INT command_arg_count = 3;

		if (argc < command_arg_count)
		{
			DOSERR0("usage: --getkey[=Options] <format> <params>+.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		if (param_value[0] == '=')
			param_value++;

		if ((ret = fprintf_parse_options(param_value, &options)) < 0)
		{
			DOSERR0("invalid options argument.\n");
			DOS_E_RET(ERROR_BAD_ARGUMENTS);
		}

		// prompt output goes to stdout
		ret = dcmd_fprintf("stdout", format, &options, &argv[command_arg_count], argc - command_arg_count);
		if (ret >= 0)
		{
			INT anwser_arg_pos;
			CHAR response_str[2];

			// response goes to stderr.
			FILE* response_file = stderr;
			ret = _getch();

			response_str[0] = (CHAR)ret;
			response_str[1] = '\0';
			strupr(response_str);

			if (response_str[0] < '\x30' || response_str[0] > '\x5A')
				fprintf(response_file, "0x%02X\n", ret);
			else
				fprintf(response_file, "%s\n", response_str);

			for (anwser_arg_pos = command_arg_count; anwser_arg_pos < argc; anwser_arg_pos++)
			{
				PCHAR answer_arg = argv[anwser_arg_pos];
				if (_strnicmp(response_str, answer_arg, 1) == 0)
				{
					ret = (anwser_arg_pos - command_arg_count) + 1;
					goto Done;
				}
			}
		}
	}
Done:
	if (ret != 0)
	{
		DOSDBG("ret=%d\n", ret);
	}
	return ret;
}

void show_help()
{
	CONST CHAR* src;
	DWORD src_count, charsWritten;
	HGLOBAL res_data;
	HANDLE handle;
	HRSRC hSrc;

	hSrc = FindResourceA(NULL, MAKEINTRESOURCEA(ID_HELP_TEXT), MAKEINTRESOURCEA(ID_DOS_TEXT));
	if (!hSrc)	return;

	src_count = SizeofResource(NULL, hSrc);

	res_data = LoadResource(NULL, hSrc);
	if (!res_data)	return;

	src = (char*) LockResource(res_data);
	if (!src) return;

	if ((handle = GetStdHandle(STD_ERROR_HANDLE)) != INVALID_HANDLE_VALUE)
		WriteConsoleA(handle, src, src_count, &charsWritten, NULL);
}
