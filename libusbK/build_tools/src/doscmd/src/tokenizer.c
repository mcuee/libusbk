/* tokenizer.c
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

#include "tokenizer.h"
#include <io.h>
#include <Winerror.h>

#define _VER_ONLY
#include "doscmd_rc.rc"
#include "user_debug.h"

#define AquireContext(InternalContextPtr, PublicContextPtr) \
	{ \
	InternalContextPtr = (PTOKENIZER_CONTEXT_INTERNAL)PublicContextPtr; \
	if (lock_aquire(InternalContextPtr, INFINITE) == FALSE)	DOS_E_RET(ERROR_LOCK_FAILED); \
	}

#define ReturnReleaseContext(InternalContextPtr, ErrorCode) \
	{ lock_release(InternalContextPtr); return(ErrorCode); 	}

#define AllocateContext(ContextPtr) \
	ContextPtr = (PTOKENIZER_CONTEXT_INTERNAL) tok_memory_alloc(sizeof(tokenizer_context_internal))

#define AllocateToken(TokenPtr) \
	TokenPtr = (PTOK_ENTITY_INTERNAL) tok_memory_alloc(sizeof(tok_entity_internal))

#define FreeAllTokens(TokenizerContext) \
	tok_enum(TokenizerContext, FreeAllTokensFn, NULL)

typedef struct _tok_entity_internal
{
	tok_entity_t user;
	PVOID next;

	PCHAR match_text_cache;
	LONG match_text_cache_alloc_size;
	INT match_text_cache_length;
	ULONG token_id;
} tok_entity_internal, *PTOK_ENTITY_INTERNAL;


typedef struct _tokenizer_context_internal
{
	PTOK_ENTITY_INTERNAL tokens;
	PCHAR prefix;
	PCHAR suffix;

	BOOL recursive;
	BOOL ignore_case;

	PCHAR temp_line;
	LONG temp_line_alloc_size;

	PCHAR temp_match;
	LONG  temp_match_alloc_size;

	HANDLE tok_context_lock;
	ULONG token_count;
} tokenizer_context_internal, *PTOKENIZER_CONTEXT_INTERNAL;

typedef union _ENUM_TOKEN_PARAM_T
{
	struct
	{
		PCHAR match;
		INT match_length;
		PCHAR replace;
		INT replace_length;
	} set;

	struct
	{
		PCHAR match;
		INT match_length;
		PTOK_ENTITY tok_entity;
	} get;

} enum_token_param_t, *PENUM_TOKEN_PARAM_T;


////////////////////////////////////////////////////////////////////////////
/* File functions headers */////////////////////////////////////////////////
FILE* file_create_temp(__deref_out_opt PCHAR* filename);
static LONG fget_position(FILE* file);
LONG fget_length(FILE* file);
static LONG fget_remaining(FILE* file);
static LONG file_write(FILE* output_file, PCHAR data, LONG offset, LONG length);
LONG file_write_remaining(FILE* src_file, FILE* output_file);

static LONG realloc_buffer(PVOID* memory, SIZE_T new_size);

/////////////////////////////////////////////////////////////////////////////
/* Mutex locking functions */////////////////////////////////////////////////
static BOOL lock_create(HANDLE* mutex_handle_out, BOOL own);
static BOOL lock_aquire(PTOKENIZER_CONTEXT_INTERNAL tok, DWORD dwMilliseconds);
static BOOL lock_release(PTOKENIZER_CONTEXT_INTERNAL tok);
static BOOL lock_destroy(HANDLE* mutex_handle_in);


static INT fill_token(__inout PTOK_ENTITY token,
                      __in PCHAR match,
                      __in INT match_length,
                      __in PCHAR replace,
                      __in INT replace_length);

static INT free_context(PTOKENIZER_CONTEXT* tokenizer);
static VOID free_token(__deref_inout PTOK_ENTITY_INTERNAL* token);

INT CALLBACK FreeAllTokensFn(PTOKENIZER_CONTEXT tokenizer, CONST PTOK_ENTITY token, PVOID user_context);
INT CALLBACK SetTokenFn(PTOKENIZER_CONTEXT tokenizer, CONST PTOK_ENTITY token, PVOID user_context);
INT CALLBACK GetTokenFn(PTOKENIZER_CONTEXT tokenizer, CONST PTOK_ENTITY token, PVOID user_context);

/////////////////////////////////////////////////////////////////////////////
/* Mutex locking functions */////////////////////////////////////////////////
static BOOL lock_create(HANDLE* mutex_handle_out, BOOL own)
{
	*mutex_handle_out = CreateMutexA(NULL, own, NULL);
	return (*(mutex_handle_out) != NULL);
}
static BOOL lock_destroy(HANDLE* mutex_handle_in)
{
	if (!mutex_handle_in || !*mutex_handle_in)
		return TRUE;

	if(CloseHandle(*mutex_handle_in))
	{
		*mutex_handle_in = NULL;
		return TRUE;
	}
	return FALSE;

}
static BOOL lock_release(PTOKENIZER_CONTEXT_INTERNAL tok)
{
	if (!tok) return FALSE;
	if (!tok->tok_context_lock) return TRUE;
	return ReleaseMutex(tok->tok_context_lock);
}
static BOOL lock_aquire(PTOKENIZER_CONTEXT_INTERNAL tok, DWORD dwMilliseconds)
{
	if (!tok) return FALSE;
	if (!tok->tok_context_lock) return TRUE;
	return (WaitForSingleObject(tok->tok_context_lock, dwMilliseconds) == WAIT_OBJECT_0);
}

static LONG realloc_buffer(PVOID* memory, SIZE_T new_size)
{
	void* new_memory = NULL;

	if (!memory)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	if ((new_memory = tok_memory_realloc(*memory, new_size)) != NULL)
	{
		*memory = new_memory;
		return (LONG)new_size;
	}

	if (*memory)
	{
		free(*memory);
		*memory = NULL;
	}

	DOS_E_RET(ERROR_NOT_ENOUGH_MEMORY);
}

/* verify_and_allocate_buffer()

   This function checks that a buffer is big enough. If it's not,
   re-allocate it.

   IMPORTANT: A buffer is considered "too small" if required_alloc_size
              is LESS THAN the current_alloc_size.  i.e. There will always
			  be atleast one more byte available in memory_ref than requested
			  in required_alloc_size.
*/
INT verify_and_allocate_buffer(__deref_inout PVOID* memory_ref,
                               __inout PLONG current_alloc_size,
                               __in LONG required_alloc_size)
{
	INT alloc_size = *current_alloc_size;

	// if line buffer is not big enough.
	if ((required_alloc_size) < alloc_size)
		return 0;

	// 512K is a hard limit for safety; this can be turned up if necessary.
	if (required_alloc_size > TOK_MAX_ALLOC_SIZE)
		DOS_E_RET(ERROR_NOT_ENOUGH_MEMORY);

	// we didn't have a big enough line buffer; add somemore.
	alloc_size = required_alloc_size + TOK_ALLOC_SIZE;
#if 0
	DOSMSG("verify_and_allocate_buffer: alloc_size=%d\n", alloc_size);
#endif
	// since we are dealing with strings, always allocate an extra byte here to store a null terminator.
	if ((realloc_buffer(memory_ref, alloc_size + 1)) < (alloc_size + 1))
	{
		*current_alloc_size = 0;
		DOS_E_RET(ERROR_NOT_ENOUGH_MEMORY);
	}

	*current_alloc_size = alloc_size;
	((PCHAR)*memory_ref)[alloc_size] = '\0'; // this is our extra byte; it should always be null.
	return alloc_size;
}

static LONG fget_position(FILE* file)
{
	fpos_t pos;

	if (fgetpos(file, &pos) == 0)
		return (LONG)(pos);

	DOS_E_RET(ERROR_FILE_INVALID);
}
static LONG fset_position(FILE* file, LONG pos)
{
	fpos_t fpos = (fpos_t)pos;

	return (LONG) fsetpos(file, &fpos);
}

LONG fset_length(FILE* file, LONG new_length)
{
	INT file_no;

	if ((file_no = _fileno(file)) == -1)
		DOS_E_RET(ERROR_FILE_INVALID);

	if (_chsize(file_no, new_length) != 0)
		DOS_E_RET(ERROR_FILE_INVALID);

	return new_length;
}

LONG fget_length(FILE* file)
{
	INT file_no;
	__int64 file_length;

	if ((file_no = _fileno(file)) == -1)
		DOS_E_RET(ERROR_FILE_INVALID);

	if ((file_length = _filelengthi64(file_no)) == -1)
		DOS_E_RET(ERROR_FILE_INVALID);

	return (LONG)(file_length);
}

static LONG fget_remaining(FILE* file)
{
	INT file_no;
	__int64 file_length;
	fpos_t pos;

	if ((file_no = _fileno(file)) == -1)
		DOS_E_RET(ERROR_FILE_INVALID);

	if ((file_length = _filelengthi64(file_no)) == -1)
		DOS_E_RET(ERROR_FILE_INVALID);

	if (fgetpos(file, &pos) == 0)
		return (LONG)(file_length - pos);

	DOS_E_RET(ERROR_FILE_INVALID);
}

FILE* file_create_temp(__deref_out_opt PCHAR* filename)
{
	FILE* file = NULL;
	PCHAR dst_filename;

	if ((dst_filename = _tempnam(NULL, "tok_")) == NULL)
		return tmpfile();

	if ((file = fopen(dst_filename, "w+D")) == NULL)
		return tmpfile();

	if (filename != NULL)
		*filename = dst_filename;

	return file;
}

static LONG file_write(FILE* output_file, PCHAR data, LONG offset, LONG length)
{
	if (!output_file || !data)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	return (LONG)fwrite(&data[offset], 1, (SIZE_T)length, output_file);
}

LONG file_write_remaining(FILE* src_file, FILE* output_file)
{
	LONG total_written = 0;
	LONG read;
	CHAR temp_buffer[1024];


	if (!src_file || !output_file)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	while((read = (LONG)fread(temp_buffer, 1, sizeof(temp_buffer), src_file)) > 0)
	{
		if (ferror(src_file))
			DOS_E_RET(ERROR_FILE_INVALID);

		fwrite(temp_buffer, 1, read, output_file);

		if (ferror(output_file))
			DOS_E_RET(ERROR_FILE_INVALID);

		total_written += read;

		if (feof(src_file))
			break;
	}
	return total_written;
}

LONG file_read_line(__in FILE* src_file,
                    __deref_inout PCHAR* current_line_buffer,
                    __inout PLONG current_alloc_size)
{
	INT ret;
	size_t line_length;
	size_t total_line_length;
	PCHAR line;

	if (!current_line_buffer || !current_alloc_size)
		DOS_E_RET(ERROR_INVALID_PARAMETER);

	if (!(*current_line_buffer))
	{
		// line buffer not allocated
		ret = verify_and_allocate_buffer(current_line_buffer,
		                                 current_alloc_size,
		                                 *current_alloc_size + TOK_ALLOC_SIZE);

		if (ret < 0)
			DOS_E_RET(ERROR_NOT_ENOUGH_MEMORY);
	}

	total_line_length = line_length = 0;
	line = *current_line_buffer;
	while ((line = fgets(line, *current_alloc_size - (INT)total_line_length, src_file)) != NULL)
	{
		line_length = strlen(line);
		total_line_length += line_length;
		if (line_length && line[line_length - 1] != '\n')
		{
			// line buffer not large enough; reallocate it
			ret = verify_and_allocate_buffer(current_line_buffer,
			                                 current_alloc_size,
			                                 *current_alloc_size + TOK_ALLOC_SIZE);

			if (ret < 0)
				DOS_E_RET(ERROR_NOT_ENOUGH_MEMORY);

			line = *current_line_buffer;
			line += total_line_length;
			continue;
		}

		// line complete
		break;
	}

	return (INT)total_line_length;
}

INT tok_file(__in PTOKENIZER_CONTEXT tokenizer,
             __in FILE* src_file,
             __in FILE* dst_file)
{
	INT ret;
	INT tok_prefix_length;
	INT tok_suffix_length;
	INT src_count_left;
	PCHAR match_text;
	PTOK_ENTITY_INTERNAL current;
	PTOK_ENTITY_INTERNAL next;
	PTOKENIZER_CONTEXT_INTERNAL tok;
	size_t line_length;


	if (!src_file || !dst_file)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	AquireContext(tok, tokenizer);

	tok_prefix_length = (INT)strlen(tok->prefix);
	tok_suffix_length = (INT)strlen(tok->suffix);

	src_count_left = fget_remaining(src_file);

	// 512K is a hard limit for safety; this can be turned up if necessary.
	if ( src_count_left > TOK_MAX_ALLOC_SIZE)
	{
		ReturnReleaseContext(tok, -ERROR_BAD_ENVIRONMENT);
	}

	// error or nothing to do.
	if (src_count_left == 0)
	{
		ReturnReleaseContext(tok, ERROR_SUCCESS);
	}
	// should never happen.
	if (src_count_left < 0)
	{
		ReturnReleaseContext(tok, -ERROR_FILE_INVALID);
	}

	if (verify_and_allocate_buffer(&tok->temp_line, &tok->temp_line_alloc_size, TOK_ALLOC_SIZE) < 0)
	{
		ReturnReleaseContext(tok, -ERROR_NOT_ENOUGH_MEMORY);
	}

	while ((line_length = file_read_line(src_file, &tok->temp_line, &tok->temp_line_alloc_size)) > 0)
	{
		INT write;
		INT cmp;
		PCHAR line = tok->temp_line;
		LONG line_pos = 0;
		INT max_line_advance_length = 1;

		while(line[line_pos])
		{
			BOOL match_found = FALSE;
			next = current = tok->tokens;
			if (!current)
			{
				ReturnReleaseContext(tok, -ERROR_BAD_ENVIRONMENT);
			}

			// iterate the tokens; find matches for all the tokens at the current linepos.
			while((current = next) != NULL)
			{
				next = current->next;

				// the match and replace fields must both be set
				if (!current->user.match || !current->user.replace)
				{
					break;
				}

				if (!current->match_text_cache)
				{
					INT match_name_length = (INT)strlen(current->user.match);
					current->match_text_cache_alloc_size = 0;

					// cache the full match string.
					current->match_text_cache_length =  match_name_length + tok_prefix_length + tok_suffix_length;
					ret = verify_and_allocate_buffer(
					          &current->match_text_cache,
					          &current->match_text_cache_alloc_size,
					          current->match_text_cache_length);

					if (ret < 0)
						ReturnReleaseContext(tok, -ERROR_NOT_ENOUGH_MEMORY);


					// create the whole match string.
					current->match_text_cache[0] = '\0';
					tok_string_cat(current->match_text_cache, current->match_text_cache_length + 1, tok->prefix);
					tok_string_cat(current->match_text_cache, current->match_text_cache_length + 1, current->user.match);
					tok_string_cat(current->match_text_cache, current->match_text_cache_length + 1, tok->suffix);
				}

				// use the cached data.
				match_text = current->match_text_cache;

				cmp = tok_string_cmp(&line[line_pos],
				                     match_text,
				                     current->match_text_cache_length,
				                     tok->ignore_case);

				// if it didn't match go on to the next token.
				if (cmp !=  0)
					continue;

				// this one matched; write the replace string
				write = (INT)strlen(current->user.replace);
				if ((ret = file_write(dst_file, current->user.replace, 0, write)) != write)
				{
					ReturnReleaseContext(tok, -ERROR_FILE_INVALID);
				}

				// no need to check; just add it.
				line_pos += current->match_text_cache_length;
				match_found = TRUE;
				break;
			}

			// we found a match and replaced it above; below is the old text which was already skipped.
			if (match_found == TRUE)
				continue;

			max_line_advance_length = 1;
			/*
			if (tok_prefix_length > 0)
			{
				// try and match the prefix to go faster.
				// this will make a prefix case-sensitive!
				PCHAR next_match_start = strstr(&line[line_pos + 1], tok->prefix);
				if (next_match_start)
				{
					// got to the start of the prefix.
					max_line_advance_length = (LONG)(next_match_start - (&line[line_pos]));
				}
				else
				{
					// no prefix was found; go to the end.
					max_line_advance_length = (INT)strlen(line) - line_pos;

				}
			}
			*/
			if ((ret = file_write(dst_file, &line[line_pos], 0, max_line_advance_length)) != max_line_advance_length)
			{
				ReturnReleaseContext(tok, -ERROR_FILE_INVALID);
			}

			line_pos += max_line_advance_length;
		}
	}

	ReturnReleaseContext(tok, fget_position(dst_file));
}

INT tok_resource(__in PTOKENIZER_CONTEXT tokenizer,
                 __in LPCSTR resource_name,
                 __in LPCSTR resource_type,
                 __deref_opt_out LPSTR* buffer_ref)
{
	INT ret;
	PCHAR src;
	INT src_count;
	HGLOBAL res_data;
	PTOKENIZER_CONTEXT_INTERNAL tok = (PTOKENIZER_CONTEXT_INTERNAL)tokenizer;

	HRSRC hSrc = FindResourceA(NULL, resource_name, resource_type);

	FILE* fsrc =  NULL;
	FILE* fdst =  NULL;

	if (!tokenizer || !resource_name || !resource_type || !buffer_ref)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	*(buffer_ref) = NULL;

	if (!hSrc)
		DOS_E_RET(ERROR_RESOURCE_DATA_NOT_FOUND);

	src_count = SizeofResource(NULL, hSrc);
	res_data = LoadResource(NULL, hSrc);
	if (!res_data)
		DOS_E_RET(ERROR_RESOURCE_DATA_NOT_FOUND);

	src = (PCHAR) LockResource(res_data);
	if (!src)
		DOS_E_RET(ERROR_RESOURCE_DATA_NOT_FOUND);

	fsrc = file_create_temp(NULL);
	fdst = file_create_temp(NULL);

	if (!fsrc || !fdst)
	{
		ret = -ERROR_FILE_INVALID;
		goto Done;
	}

	if (file_write(fsrc, src, 0, src_count) != src_count)
	{
		ret = -ERROR_FILE_INVALID;
		goto Done;
	}

	fseek(fsrc, 0, SEEK_SET);

	ret = tok_file(tok, fsrc, fdst);
	if (ret > 0)
	{
		*buffer_ref = tok_memory_alloc(ret + 1);
		fseek(fdst, 0, SEEK_SET);
		if (fread(*buffer_ref, 1, ret, fdst) != (SIZE_T)ret)
		{
			ret = -ERROR_FILE_INVALID;
			goto Done;
		}
		(*buffer_ref)[ret] = '\0';

	}
Done:
	if (fsrc)
	{
		fclose(fsrc);
		fsrc = NULL;
	}
	if (fdst)
	{
		fclose(fdst);
		fdst = NULL;
	}

	if (ret < 0)
		tok_memory_free(buffer_ref);

	return ret;
}

PTOKENIZER_CONTEXT tok_create(__in CONST PCHAR start_delim,
                              __in CONST PCHAR end_delim,
                              __in BOOL ignore_case,
                              __in BOOL recursive)
{
	PTOKENIZER_CONTEXT_INTERNAL tok;

	AllocateContext(tok);
	if (!tok) return NULL;

	tok->prefix = tok_string_dup(start_delim);
	tok->suffix = tok_string_dup(end_delim);
	tok->ignore_case = ignore_case;
	tok->recursive = recursive;

#ifdef TOK_ENABLE_CONTEXT_LOCKING
	if (lock_create(&tok->tok_context_lock, FALSE) == FALSE)
	{
		free_context(&tok);
		return NULL;
	}
#endif

	return tok;
}


INT tok_destroy(__deref_in PTOKENIZER_CONTEXT* tokenizer)
{
	PTOKENIZER_CONTEXT_INTERNAL tok;
	HANDLE lock_handle;

	AquireContext(tok, (*tokenizer));
	lock_handle = tok->tok_context_lock;

	FreeAllTokens(tok);
	free_context(tokenizer);

	*tokenizer = NULL;

	lock_destroy(&lock_handle);

	return ERROR_SUCCESS;
}

INT CALLBACK SetTokenFn(PTOKENIZER_CONTEXT tokenizer, CONST PTOK_ENTITY token, PVOID user_context)
{
	PENUM_TOKEN_PARAM_T set_token_param = (PENUM_TOKEN_PARAM_T)user_context;
	PTOKENIZER_CONTEXT_INTERNAL tok = (PTOKENIZER_CONTEXT_INTERNAL)tokenizer;

	if (!tok || !set_token_param || !token)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	if (token->match && token->replace)
	{
		BOOL removing = ( set_token_param->set.replace == NULL );
		BOOL matching = ( tok_string_cmp(token->match, set_token_param->set.match, set_token_param->set.match_length, tok->ignore_case) == 0 );

		if (matching) matching = ( (INT)strlen(token->match) == set_token_param->set.match_length );

		if (matching && removing)
		{
			// remove it.
			return (TOK_ENUM_DEL_TOKEN | TOK_ENUM_STOP);
		}
		else if (matching)
		{
			// update it.
			fill_token(token,
			           set_token_param->set.match,
			           set_token_param->set.match_length,
			           set_token_param->set.replace,
			           set_token_param->set.replace_length);

			return TOK_ENUM_STOP;
		}
		return (TOK_ENUM_CONTINUE);

	}
	else
	{
		// no more tokens..
		// add it.
		fill_token(token,
		           set_token_param->set.match,
		           set_token_param->set.match_length,
		           set_token_param->set.replace,
		           set_token_param->set.replace_length);

		return (TOK_ENUM_STOP);

	}
}

INT tok_set_value_ex(__in PTOKENIZER_CONTEXT tokenizer,
                     __in CONST PCHAR match,
                     __in INT match_length,
                     __in CONST PCHAR replace,
                     __in INT replace_length)
{
	enum_token_param_t set_token_param;

	if ((match) && match_length < 1)
		match_length = (INT)strlen(match);

	if ((replace) && replace_length < 1)
		replace_length = (INT)strlen(replace);

	tok_memory_zero(&set_token_param, sizeof(set_token_param));
	set_token_param.set.match			= match;
	set_token_param.set.match_length	= match_length;
	set_token_param.set.replace			= replace;
	set_token_param.set.replace_length	= replace_length;

	return tok_enum(tokenizer, SetTokenFn, &set_token_param);


}

static INT fill_token(__inout PTOK_ENTITY token,
                      __in PCHAR match,
                      __in INT match_length,
                      __in PCHAR replace,
                      __in INT replace_length)
{
	tok_free_match_replace(token);

	token->match = tok_memory_alloc(match_length + 1);
	tok_string_ncpy(token->match, match_length + 1, match, match_length);
	token->match[match_length] = '\0';

	token->replace = tok_memory_alloc(replace_length + 1);
	tok_string_ncpy(token->replace, replace_length + 1, replace, replace_length);
	token->replace[replace_length] = '\0';

	return ERROR_SUCCESS;
}

INT tok_set_value(__in PTOKENIZER_CONTEXT tokenizer,
                  __in CONST PCHAR match,
                  __in CONST PCHAR replace)
{

	if (!tokenizer || !match || !replace)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	return tok_set_value_ex(tokenizer, match, (INT)strlen(match), replace, (INT)strlen(replace));
}

INT CALLBACK GetTokenFn(PTOKENIZER_CONTEXT tokenizer, CONST PTOK_ENTITY token, PVOID user_context)
{
	PENUM_TOKEN_PARAM_T token_param	= (PENUM_TOKEN_PARAM_T)user_context;
	PTOKENIZER_CONTEXT_INTERNAL tok	= (PTOKENIZER_CONTEXT_INTERNAL)tokenizer;

	if (!token->match)
		DOS_E_RET(ERROR_NO_MORE_ITEMS);

	if ((tok_string_cmp(token_param->get.match, token->match, token_param->get.match_length, tok->ignore_case)) == 0)
	{
		token_param->get.tok_entity = (PTOK_ENTITY)token;
		return TOK_ENUM_STOP;
	}
	return TOK_ENUM_CONTINUE;
}

INT tok_get_token(__in PTOKENIZER_CONTEXT tokenizer,
                  __in CONST PCHAR match,
                  __ref_out PTOK_ENTITY* tok_entity_ref)
{
	INT ret;
	enum_token_param_t get_token_param;

	if (!tokenizer || !match || !tok_entity_ref)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	*tok_entity_ref = NULL;

	tok_memory_zero(&get_token_param, sizeof(get_token_param));
	get_token_param.get.match = match;
	get_token_param.get.match_length = (INT)strlen(match);
	get_token_param.get.tok_entity = NULL;

	if ((ret = tok_enum(tokenizer, GetTokenFn, &get_token_param)) == TOK_ENUM_STOP)
	{
		*tok_entity_ref = get_token_param.get.tok_entity;
	}

	return ret;
}

INT tok_get_value(__in PTOKENIZER_CONTEXT tokenizer,
                  __in CONST PCHAR match,
                  __out_opt PCHAR replace,
                  __inout PINT replace_length)
{
	PTOKENIZER_CONTEXT_INTERNAL tok;
	PTOK_ENTITY token = NULL;
	int ret;

	if (!tokenizer || !match || !replace_length)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	AquireContext(tok, tokenizer);

	ret = tok_get_token(tokenizer, match, &token);
	if (ret == TOK_ENUM_STOP)
	{
		int actual_length = (INT)strlen(token->replace) + 1;

		if (!replace)
		{
			*replace_length = actual_length;
			ReturnReleaseContext(tok, -ERROR_BUFFER_OVERFLOW);
		}
		if (*replace_length < actual_length)
		{
			*replace_length = actual_length;
			ReturnReleaseContext(tok, -ERROR_BUFFER_OVERFLOW);
		}

		tok_string_cpy(replace, actual_length, token->replace);
		ReturnReleaseContext(tok, ERROR_SUCCESS);

	}

	ReturnReleaseContext(tok, ret);
}

INT tok_text(__in PTOKENIZER_CONTEXT tokenizer,
             __in LPCSTR src,
             __in INT src_count,
             __deref_out_opt PCHAR* buffer_ref)
{
	INT ret;
	FILE* fsrc = NULL;
	FILE* fdst = NULL;
	PTOKENIZER_CONTEXT_INTERNAL tok;

	AquireContext(tok, tokenizer);

	fsrc = file_create_temp(NULL);
	fdst = file_create_temp(NULL);

	if (!fsrc || !fdst || !buffer_ref)
	{
		ret = -ERROR_BAD_ARGUMENTS;
		goto Done;
	}

	if (file_write(fsrc, (PCHAR)src, 0, src_count) != src_count)
	{
		ret = -ERROR_FILE_INVALID;
		goto Done;
	}

	fseek(fsrc, 0, SEEK_SET);

	ret = tok_file(tok, fsrc, fdst);

	// if everything went okay, allocate the user buffer and fill it from
	// fdst.
	if (ret > 0)
	{
		INT read;
		if ((*buffer_ref = tok_memory_alloc(ret + 1)) == NULL)
		{
			ret = -ERROR_NOT_ENOUGH_MEMORY;
			goto Done;
		}

		// rewind the de-tokenized temp file.
		rewind(fdst);
		if ((read = (INT)fread(*buffer_ref, 1, ret, fdst)) != ret)
		{
			ret = -ERROR_FILE_INVALID;
			goto Done;
		}
		// self-terminate.
		(*buffer_ref)[read] = '\0';
	}

Done:
	if (fsrc)
	{
		fclose(fsrc);
		fsrc = NULL;
	}

	if (fdst)
	{
		fclose(fdst);
		fdst = NULL;
	}

	if (ret < 0)
		tok_memory_free(buffer_ref);

	ReturnReleaseContext(tok, ret);
}

INT tok_enum(__in PTOKENIZER_CONTEXT tokenizer,
             __in PTOK_ENUM_CALLBACK callback,
             __in_opt PVOID user_context)
{
	PTOK_ENTITY_INTERNAL current = NULL;
	PTOK_ENTITY_INTERNAL next;
	PTOK_ENTITY_INTERNAL parent = NULL;

	PTOKENIZER_CONTEXT_INTERNAL tok;
	INT action = -ERROR_NO_MORE_ITEMS;

	if (!tokenizer || !callback)
		DOS_E_RET(ERROR_BAD_ARGUMENTS);

	AquireContext(tok, tokenizer);

	if (!tok->tokens)
	{
		DOSDBG0("allocating primary token\n");
		AllocateToken(tok->tokens);
		if (!tok->tokens)
		{
			DOSERR0("failed allocating primary token\n");
			DOS_E_RET(ERROR_OUTOFMEMORY);
		}
	}

	next = tok->tokens;
	while (next)
	{
		parent = current;
		current = next;

		if (!current->next && current->user.match)
		{
			tok->token_count++;
			DOSOFF("allocating token #%u\n", tok->token_count);
			AllocateToken(current->next);
			if (!current->next)
			{
				DOSERR("failed allocating token #%u\n", tok->token_count);
				DOS_E_RET(ERROR_OUTOFMEMORY);
			}
			((PTOK_ENTITY_INTERNAL)current->next)->token_id = tok->token_count;
		}

		next = next->next;

		action = callback(tokenizer, (PTOK_ENTITY)current, user_context);
		if (action < 0) break;

		if (action & TOK_ENUM_DEL_TOKEN)
		{
			free_token(&current);

			// The parent token was matched; re-link.
			if (next && !parent)
				tok->tokens = next;
			else if (next)
				parent->next = next;

			current = NULL;
		}
		if (action & TOK_ENUM_STOP)
			break;

		if (!next)
			action = -ERROR_NO_MORE_ITEMS;
	}

	ReturnReleaseContext(tok, action);
}

static VOID free_token(__deref_inout PTOK_ENTITY_INTERNAL* token)
{
	if (*token)
	{
		DOSOFF("freeing token #%u\n", (*token)->token_id);
		tok_free_match_replace((PTOK_ENTITY)*token);
		tok_memory_free(&((*token)->match_text_cache));
		tok_memory_free(token);
	}
}

INT CALLBACK FreeAllTokensFn(PTOKENIZER_CONTEXT tokenizer, CONST PTOK_ENTITY token, PVOID user_context)
{
	UNREFERENCED_PARAMETER(tokenizer);
	UNREFERENCED_PARAMETER(token);
	UNREFERENCED_PARAMETER(user_context);

	return TOK_ENUM_DEL_TOKEN | TOK_ENUM_CONTINUE;
}

static INT free_context(PTOKENIZER_CONTEXT* tokenizer)
{
	PTOKENIZER_CONTEXT_INTERNAL tok;
	HANDLE lock_handle;

	if (!tokenizer || !(*tokenizer))
		return ERROR_SUCCESS;

	AquireContext(tok, (*tokenizer));
	lock_handle = tok->tok_context_lock;

	tok_memory_free(&tok->prefix);
	tok_memory_free(&tok->suffix);

	tok_memory_free(&tok->temp_match);
	tok_memory_free(&tok->temp_line);

	free(tok);

	*tokenizer = NULL;

	ReturnReleaseContext(tok, ERROR_SUCCESS);
}
