/* doscmd.h
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

#ifndef __DOS_CMD_H
#define __DOS_CMD_H

#include <windows.h>

enum VERSION_CONTROL_ACTION_E
{
    VCA_NANO = 0,
    VCA_MICRO,
    VCA_MINOR,
    VCA_MAJOR,

    VCA_INC = 0x10,
    VCA_DEC = 0x20,
    VCA_TAG = 0x40,

    VCA_INC_NANO = VCA_NANO | VCA_INC,
    VCA_INC_MICRO,
    VCA_INC_MINOR,
    VCA_INC_MAJOR,

    VCA_DEC_NANO = VCA_NANO | VCA_DEC,
    VCA_DEC_MICRO,
    VCA_DEC_MINOR,
    VCA_DEC_MAJOR,

};
typedef INT VERSION_CONTROL_ACTION;
enum FILE_LINE_ACTION_E
{
    LINE_INVALID,
    LINE_DELETE,
    LINE_INSERT,
    LINE_UPDATE,

};
typedef INT FILE_LINE_ACTION;

enum MATCH_ON_TYPE_E
{
    MATCH_ON_INVALID,
    MATCH_ON_EQUAL,
    MATCH_ON_NOT_EQUAL,
    MATCH_ON_STARTSWITH,
    MATCH_ON_ENDSWITH,
};
typedef INT MATCH_ON_TYPE;

typedef struct _dcmd_make_token_options_t
{
	CHAR	get_prefix[MAX_PATH];
	CHAR	set_prefix[MAX_PATH];
	BOOLEAN is_ddk_sources_file;
} dcmd_make_token_options_t, *PDCMD_MAKE_TOKEN_OPTIONS_T;

typedef struct _dcmd_get_commandline_options_t
{
	INT		arg_start_offset;
	CHAR	split_char;
	BOOL	no_trimming;
	BOOL	strip_quotes;
	CHAR	header[MAX_PATH];
	CHAR	footer[MAX_PATH];
	CHAR	arg_prefix[MAX_PATH];
	CHAR	arg_suffix[MAX_PATH];
	CHAR	escaped[MAX_PATH];
	CHAR	escape_char;
} dcmd_get_commandline_options_t, *PDCMD_GET_COMMANDLINE_OPTIONS_T;

typedef struct _dcmd_file_line_options_t
{
	FILE_LINE_ACTION action;

	PCHAR	in_fname;
	PCHAR	out_fname;
	PCHAR	line_fname;

	INT		line_index;
	INT		char_start_index;
	PCHAR	start_string;
	BOOL	start_after;

} dcmd_file_line_options_t, *PDCMD_FILE_LINE_OPTIONS_T;

typedef struct _dcmd_fprintf_options_t
{
	CHAR	mark;		// char used as a '%'. default is '@'.
	CHAR	escape;		// char used as a '\'.  default is '\'.
	CHAR	all[3];
} dcmd_fprintf_options_t, *PDCMD_FPRINTF_OPTIONS_T;

typedef struct _dcmd_version_control_options_t
{
	VERSION_CONTROL_ACTION action;
	PCHAR versions_file;
	PCHAR target_name;
} dcmd_version_control_options_t, *PDCMD_VERSION_CONTROL_OPTIONS_T;

typedef struct _version_control_item_t
{

#pragma warning(disable:4201)
	union
	{
		INT ver[4];
		struct
		{
			INT	nano;
			INT	micro;
			INT	minor;
			INT	major;
		} version;
	};
#pragma warning(default:4201)

	CHAR target_name[MAX_PATH];
	CHAR linked_name[MAX_PATH];
	CHAR version_template_filepath[MAX_PATH];
	CHAR version_output_filepath[MAX_PATH];

	BOOL is_linked;
	INT line_index;
	PVOID prev;
	PVOID next;
} version_control_item_t, *PVERSION_CONTROL_ITEM_T;


// This is our fprintf context.
typedef struct _dcmd_fprintf_t
{
	//PCHAR	filename;		// filename that was passed in on the command line.
	FILE*	out;			// permanent output, (stdout/stderr/file) handle.
	HANDLE	std;			// handle to the console when output is to std; required for setting console text attributes.

	dcmd_fprintf_options_t		options;
	CONSOLE_SCREEN_BUFFER_INFO	con_info;

	struct
	{
		INT max;			// total allocation size of data.
		INT index;			// next write position in data.
		PCHAR data;			// temp buffer for storing a single format specifier section; flushed to out when necessary.
	} buf;

	struct
	{
		INT		index;		// fmt index to be processed next.
		INT		length;		// number of chars left to process (starting from fmt[index])
		PCHAR	fmt;		// string that is converted to a standard 'C' format string.
	} src;

	struct
	{
		INT		index;		// argument index to be processed next.
		INT		count;		// number of argument strings in 'list'.
		PCHAR*	list;		// arguments from the command line.
	} arg;
} dcmd_fprintf_t, *PDCMD_FPRINTF_T;

#ifdef __cplusplus
extern "C"
{
#endif

	/* ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
	   ENV_TAG - Uses environment variables as match/replace strings to replace
	             text in a file.
	PARAMS:
	    input_filename   - The file to read. This file is the template. It
	                       contains the text with the tokens you want to replace.

	    output_filename  - This file to write.  The finished file with tokens
	                       replaced by their associated replace string.

	    env_prefix       - If given, only environment variables with names that
	                       begin with this string will be included as a token.
	REMARKS:
	    The environment tagger uses dos environment variables as a way of
	    replacing text in a file.  The match name is made up of the variable
	    name with a prefix and a suffix string. For Example, $(MY_VAR) would
	    represent a token with a prefix string of "$(", an environment variable
	    of "MY_VAR"	and a suffix string of ")".

	    Note that "$(" and ")" are the default left and right tags (respectively)
	    used by dcmd_env_tag().  This is	also the standard tag set used my Microsoft.

	RETURNS:
	    0 on success.
	    A negative windows error code if a failure occurs.	(see WinError.h)

	USES THE FOLLOWING ENVIRONMENT VARIABLES (IF SET):
	    TOKVAR_LTAG - The prefix string that is prepended to the match token.
	                  The default is: $(
	    TOKVAR_RTAG - The suffix string that is appended to the match token.
	                  The default is: )
	*/
	INT dcmd_env_tag(__in PCHAR input_filename,
	                 __in PCHAR output_filename,
	                 __in_opt PCHAR env_prefix);

	INT dcmd_make_args(__in PCHAR output_file,
	                   __in INT start_offset);

	INT dcmd_make_tokens(__in CONST PCHAR input_filename,
	                     __in CONST PCHAR output_filename,
	                     __in CONST PDCMD_MAKE_TOKEN_OPTIONS_T options);

	INT dcmd_update_cfg(__in PCHAR cfg_filename,
	                    __in PCHAR setting_name,
	                    __in PCHAR setting_value);

	INT dcmd_fprintf(__in PCHAR out_fname,
	                 __in PCHAR format,
	                 __in PDCMD_FPRINTF_OPTIONS_T options,
	                 __deref_in PCHAR* arg_list,
	                 __in INT arg_count);

	INT dcmd_version_control(PDCMD_VERSION_CONTROL_OPTIONS_T options);

	int CALLBACK dcmd_rundll(HWND wnd, HINSTANCE instance, LPSTR cmd_line, int cmd_show);

#ifdef __cplusplus
}
#endif

#endif