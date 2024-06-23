/* lomoji.h - Last Outpost Emoji Translator Library*/
/* Created: Wed May 29 10:35:37 AM EDT 2024 malakai */
/* Copyright © 2024 Jeffrika Heavy Industries */
/* $Id: lomoji.h,v 1.4 2024/06/22 21:40:05 malakai Exp $ */

/* Copyright © 2024 Jeff Jahr <malakai@jeffrika.com>
 *
 * This file is part of liblomoji - Last Outpost Emoji Translation Library
 *
 * liblomoji is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * liblomoji is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with liblomoji.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JHI_LOMOJI_H
#define JHI_LOMOJI_H

#include <glib.h>

/*---- Global Defines ----*/
#ifndef LOMOJI_VERSION
#define LOMOJI_VERSION "0.0"
#endif

/*--- Lomoji Basic API ----*/

/* This is the 'basic set' of lomoji functions, that use the builtin
 * lomoji_default_ctx context, default file path list for annotations, and a
 * default filter set. They take a minimum of arguments, and work from a shared
 * context that must be set up with lomoji_init() before use.
 */

/* lomoji_init() - Sets up the default context.
 * 
 * This function should be called first, before using any of the other
 * functions in the basic set.  It sets up a default context from the default
 * annotations file locations, with default operating parameters. 
 *
 * The annotation files are merged in order from default.xml, derived.xml, and
 * extra.xml out of the $(SHARE_PREFIX)/lomoji directory, then from the
 * $(HOME)/.lomoji directory.  These files may be symlinks to localized CLDR
 * annotations files, custom annotation files, or simply not present.
 * 
 * lomoji_init() Will not return an error, but will set errno to ENOENT and
 * send a message to stderr if no annotation files can be found whatsoever, as
 * this is likely a misconfiguration.
 */
void lomoji_init(void);

/* lomoji_done(done) - Deallocates the default context.
 *
 * This should be called after lomoji_init(), to deallocate any resources used
 * by the default context.  It's always a good idea to clean up your mess.  
 */
void lomoji_done(void);

/* lomoji_to_ascii() - Translates src to ASCII.
 *
 * This function works like strdup(), but the dup'd string will contain ASCII,
 * translated from the UTF-8 encoded src using the default ASCII translation
 * filter lomoji_toascii.  
 *
 * Return Value - A null terminated string that the caller must free.  Function
 * may return an empty string on error, but never returns NULL.
 */
char *lomoji_to_ascii(char *src);

/* lomoji_from_ascii() - Translates src to UTF-8 encoded Unicode.
 *
 * This function works like strdup(), but the dup'd string will contain UTF-8
 * encoded Unicode graphemes translated from the encoded src (which may be
 * UTF-8 or ASCII) using the default translation filter lomoji_fromascii.
 * Strings beginning with LOMOJI_PREFIX and ending with LOMOJI_SUFFIX are used
 * to look up a corresponding UTF-8 sequence.
 *
 * Return Value - A null terminated string that the caller must free.  Function
 * may return an empty string on error, but never returns NULL.
 */
char *lomoji_from_ascii(char *src);

/* lomoji_suggest() - Search for completions to a localized emoji name.
 *
 * This function works like strdup(), but the dup'd string will contain a space
 * separated list of less than or equal to max possible completions for the
 * string in src.  (Note that max=0 means 'no limit'.)  If the pointer 'found'
 * is non-null, then the integer it points to is incremented by the number of
 * suggestions offered.
 *
 * Note that the string in src MUST start with the configured LOMOJI_PREFIX
 * string, which can be obtained programmatically with a call to
 * lomoji_get_param(LOMOJI_PREFIX).  The prefix to be searched may end with
 * either '\0', or the LOMOJI_SUFFIX string.
 *
 * Return Value - A null terminated string that the caller must free, or NULL
 * to indicate there were no matches found.  If found is non-null, (*found) is
 * incremented by the number of completions offered in the string, which may be
 * less than or equal to max.
 */
char *lomoji_suggest(char *src, int max, int *found);

/* lomoji_param - An enum of settable operating parameters. */
typedef enum {
	LOMOJI_NONE,
	LOMOJI_PREFIX,	/* Defaults to ":" */
	LOMOJI_SUFFIX,	/* Defaults to ":" */
	LOMOJI_UNKNOWN,	/* Defaults to "?" */
	LOMOJI_MAX
} lomoji_param;

/* lomoji_get_param() - Get operating parameter from default context.
 *
 * Used to look up the value of different operating parameters, such as the
 * configured LOMOJI_PREFIX, which may be configured differently from the
 * expected default values.  The 'which' parameter is from the lomoji_param
 * enum.
 *
 * Return Value - a const null terminated string, pointing to the configured
 * value, or NULL in case of error.  The caller does NOT own this string, and
 * should NOT free the returned value.
 */
const char *lomoji_get_param(lomoji_param which); 

/* lomoji_set_param() - Set operating parameter in default context.
 *
 * Used programmatically to change the value of different operating parameters,
 * such as the configured LOMOJI_PREFIX.  The 'which' parameter is from the
 * lomoji_param enum.  The 'to' paramter is a pointer to a null terminated
 * string.  The 'to' parameter is strdup'ed into the context, and ownership of
 * 'to' stays with the caller.
 *
 * Return Value - a const char *, pointing to the configured value, or NULL in
 * case of error.  The caller does NOT own this string, and should NOT free the
 * returned value.
 */
const char *lomoji_set_param(lomoji_param which, const char *to);

/*--- END ---- Lomoji Basic API ----*/


/*--- Lomoji Advanced API ----*/

/* This is the 'advanced set' of lomoji functions and definitions, that can be
 * used with individual context setups with with their own annotation files,
 * custom filters, and filter lists.  */

/*---- Structs and typedefs ----*/

/* lomoji_ctx_t is 'context' that lomoji uses for its transformations.  The
 * context contains the annotation and codepoint equivalent mappings, as well
 * as operating parameters like prefix, suffix, and unknown substitution
 * strings. */
typedef struct lomoji_ctx_s lomoji_ctx_t;

/* codepoint filter functions take this form. */
typedef int lomoji_filter(lomoji_ctx_t *ctx, gchar *check, GString **out);

/*--- exported global variable declarations ---*/

/* lomoji_default_ctx - the context created by lomoji_init, and is used by the
 * 'basic' lomoji functions. It is exposed here so that, after calling
 * lomoji_init(), caller can specify it with param commands. */
extern lomoji_ctx_t *lomoji_default_ctx;

/* lomoji_default_filepaths - a list of filepath locations that is initialized
 * by lomoji_init(), OR by lomoji_init_filepaths if the default context is not
 * being used at all. */
extern char **lomoji_default_filepaths;

/* lomoji_ctx_new() - create a new context
 *
 * Creates a new context pointer with default values.  If annotations is NULL,
 * no annotations files are loaded.  Annotations can be merged later with
 * lomoji_add_annotations().  lomoji_default_filepaths may be used for the
 * initial annotations as long as lomoji_init() or lomoji_init_filepaths() has
 * been called first.
 *
 * Return Value - a new context pointer.  Caller must free with
 * lomoji_ctx_free().
 */
lomoji_ctx_t *lomoji_ctx_new(char **annotations);

/* lomoji_ctx_free() - deallocates a context pointer.
 *
 * Deallocates the entirety of a lomoji_ctx_t context pointer.
 *
 */
void lomoji_ctx_free(lomoji_ctx_t *f);

/* Calls to initialize and dispose of lomoji_default_filepaths.  Only required
 * if user is NOT calling lomoji_init() and lomoji_done(), but still wishes to
 * use lomoji_default_filepaths in a lomoji_new() or lomoji_add_annotations()
 * call. */
void lomoji_init_filepaths(void);
void lomoji_done_filepaths(void);

/* These are the 'extended set' of lomoji functions, that use a supplied
 * context, annotations, and filters list. They work the same as the basic set
 * (described above) but with more arguments. */
char *lomoji_to_ascii_ext(lomoji_ctx_t *ctx, char *src, lomoji_filter **filters);
char *lomoji_from_ascii_ext(lomoji_ctx_t *ctx, char *src, lomoji_filter **filters);
char *lomoji_suggest_ext(lomoji_ctx_t *ctx, char *in, int max, int *found);
int lomoji_add_annotations(lomoji_ctx_t *ctx, char **annotations);
const char *lomoji_get_param_ext(lomoji_ctx_t *ctx, lomoji_param which);
const char *lomoji_set_param_ext(lomoji_ctx_t *ctx, lomoji_param which, const char *to);

/* These are some useful predefined filter lists for handing to lomoji_X_ascii_ext() */
extern lomoji_filter *lomoji_toascii[];  	/* the basic default */
extern lomoji_filter *lomoji_fromascii[];	/* the basic default */
extern lomoji_filter *lomoji_none[];     	/* do nothing. */
extern lomoji_filter *lomoji_iconv[];    	/* simply g_str_to_ascii() */
extern lomoji_filter *lomoji_namesonly[];	/* no cp equivs or unknown subs. */
extern lomoji_filter *lomoji_nameuplus[];	/* names, then uplus */
extern lomoji_filter *lomoji_uplusonly[];	/* UTF-8 to uplus*/

/* some filter primitives, for creating custom lists of lomoji_filter *x[]'s */
lomoji_filter filter_equiv;
lomoji_filter filter_toname;
lomoji_filter filter_fromname;
lomoji_filter filter_iconv;
lomoji_filter filter_unknown;
lomoji_filter filter_uplus;
lomoji_filter filter_decompose;

#endif /* JHI_LOMOJI_H */
