/* tokenizer.h
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

#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "user_debug.h"


#define TOK_ALLOC_SIZE (64)
#define TOK_MAX_ALLOC_SIZE (512000)


#define __ref_out		__deref_out_ecount_full(1)

enum TOK_ENUM_ACTION_RETURN
{
    TOK_ENUM_STOP = 0x01,
    TOK_ENUM_CONTINUE = 0x02,
    TOK_ENUM_DEL_TOKEN = 0x04,

    TOK_ENUM_MASK = 0xff,
    TOK_ENUM_USER_MASK = 0xffffff00
};

typedef struct _tok_entity_t
{
	PCHAR match;
	PCHAR replace;
} tok_entity_t, *PTOK_ENTITY;

typedef PVOID PTOKENIZER_CONTEXT;

typedef INT (CALLBACK* PTOK_ENUM_CALLBACK) (PTOKENIZER_CONTEXT tokenizer, CONST PTOK_ENTITY token, PVOID user_context);

PTOKENIZER_CONTEXT tok_create(__in CONST PCHAR start_delim,
                              __in CONST PCHAR end_delim,
                              __in BOOL ignore_case,
                              __in BOOL recursive);

INT tok_destroy(__deref_in PTOKENIZER_CONTEXT* tokenizer);

INT tok_set_value(__in PTOKENIZER_CONTEXT tokenizer,
                  __in CONST PCHAR match,
                  __in CONST PCHAR replace);

INT tok_set_value_ex(__in PTOKENIZER_CONTEXT tokenizer,
                     __in CONST PCHAR match,
                     __in INT match_length,
                     __in CONST PCHAR replace,
                     __in INT replace_length);

INT tok_get_token(__in PTOKENIZER_CONTEXT tokenizer,
                  __in CONST PCHAR match,
                  __ref_out PTOK_ENTITY* tok_entity_ref);

INT tok_get_value(__in PTOKENIZER_CONTEXT tokenizer,
                  __in CONST PCHAR match,
                  __out_opt PCHAR replace,
                  __inout PINT replace_length);

INT tok_text(__in PTOKENIZER_CONTEXT tokenizer,
             __in LPCSTR src,
             __in INT src_count,
             __deref_out_opt PCHAR* buffer_ref);

INT tok_file(__in PTOKENIZER_CONTEXT tokenizer,
             __in FILE* src_file,
             __in FILE* dst_file);

INT tok_resource(__in PTOKENIZER_CONTEXT tokenizer,
                 __in LPCSTR resource_name,
                 __in LPCSTR resource_type,
                 __deref_opt_out LPSTR* buffer_ref);

INT tok_enum(__in PTOKENIZER_CONTEXT tokenizer,
             __in PTOK_ENUM_CALLBACK callback,
             __in_opt PVOID user_context);

INT verify_and_allocate_buffer(__deref_inout PVOID* memory_ref,
                               __inout PLONG current_alloc_size,
                               __in LONG required_alloc_size);

FORCEINLINE VOID tok_memory_zero(__in PVOID memory,
                                 __in SIZE_T size)
{
	if (memory != NULL)
		memset(memory, 0, size);
}

LONG file_read_line(__in FILE* src_file,
                    __deref_inout PCHAR* current_line_buffer,
                    __inout PLONG current_alloc_size);

// All memory allocation ends up in this function.
//
FORCEINLINE PVOID tok_memory_realloc(__in_opt PVOID memory,
                                     __in SIZE_T new_size)
{
	BOOL is_new = (memory == NULL);
	PVOID p = realloc(memory, new_size);

	if (p != NULL && is_new)
		tok_memory_zero(p, new_size);

	return p;
}

// All memory de-allocation ends up in this function.
//
FORCEINLINE VOID tok_memory_free(__deref_inout_opt PVOID* memory)
{
	if (memory && *memory)
	{
		free(*memory);
		*memory = NULL;
	}
}

FORCEINLINE PVOID tok_memory_alloc(__in SIZE_T alloc_size)
{
	return tok_memory_realloc(NULL, alloc_size);
}

FORCEINLINE VOID tok_free_match_replace(__inout PTOK_ENTITY token)
{
	tok_memory_free(&(token->match));
	tok_memory_free(&(token->replace));
}

//
// NOTE: This uses the strcat_s safe function. size_in_bytes is always the
//       total size of the dst buffer, not the number of bytes remaining.
//
FORCEINLINE errno_t tok_string_cat(__out PCHAR dst,
                                   __in SIZE_T size_in_bytes,
                                   __in CONST PCHAR src)
{
	return strcat_s(dst, size_in_bytes, src);
}

FORCEINLINE errno_t tok_string_cpy(__out PCHAR dst,
                                   __in SIZE_T size_in_bytes,
                                   __in CONST PCHAR src)
{
	return strcpy_s(dst, size_in_bytes, src);
}

FORCEINLINE errno_t tok_string_ncpy(__out PCHAR dst,
                                    __in SIZE_T size_in_bytes,
                                    __in CONST PCHAR src,
                                    __in SIZE_T max_count)
{
	return strncpy_s(dst, size_in_bytes, src, max_count);
}

FORCEINLINE PCHAR tok_string_dup(__in CONST PCHAR src)
{
	if (src)
	{
		SIZE_T alloc_size = strlen(src) + 1;
		PCHAR new_string = tok_memory_alloc(alloc_size);
		if (new_string)
		{
			if (tok_string_cpy(new_string, alloc_size, src) == EINVAL)
			{
				DOSERR("tok_string_cpy: alloc_size=%d, src pointer=%Xh\n",
				       alloc_size, src);

				// tok_memory_free(&new_string);
			}
			return new_string;
		}
	}
	return NULL;
}

FORCEINLINE INT tok_string_cmp(__in CONST PCHAR src1,
                               __in CONST PCHAR src2,
                               __in INT required_length,
                               __in BOOL ignore_case)
{
	SIZE_T src1_length;
	SIZE_T src2_length;

	if (!src1 || !src2)
		return -EINVAL;

	src1_length = strlen(src1);
	src2_length = strlen(src2);

	if (required_length == -1)
		required_length = (INT)max(src1_length, src2_length);

	if (required_length > (INT)src1_length || required_length > (INT)src2_length)
		return -EINVAL;

	if (ignore_case)
		return _memicmp(src1, src2, required_length);
	else
		return memcmp(src1, src2, required_length);
}

#endif
