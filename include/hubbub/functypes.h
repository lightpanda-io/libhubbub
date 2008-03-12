/*
 * This file is part of Hubbub.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2007-8 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef hubbub_functypes_h_
#define hubbub_functypes_h_

#include <stdbool.h>
#include <stdlib.h>

#include <hubbub/types.h>

/* Type of allocation function for hubbub */
typedef void *(*hubbub_alloc)(void *ptr, size_t size, void *pw);

/**
 * Type of token handling function
 */
typedef void (*hubbub_token_handler)(const hubbub_token *token, void *pw);

/**
 * Type of document buffer handling function
 */
typedef void (*hubbub_buffer_handler)(const uint8_t *data,
		size_t len, void *pw);

/**
 * Type of parse error handling function
 */
typedef void (*hubbub_error_handler)(uint32_t line, uint32_t col,
		const char *message, void *pw);

/**
 * Type of tree comment node creation function
 */
typedef int (*hubbub_tree_create_comment)(void *ctx, const hubbub_string *data,
		void **result);

/**
 * Type of tree doctype node creation function
 */
typedef int (*hubbub_tree_create_doctype)(void *ctx, const hubbub_string *qname,
		const hubbub_string *public_id, const hubbub_string *system_id,
		void **result);

/**
 * Type of tree element node creation function
 */
typedef int (*hubbub_tree_create_element)(void *ctx, const hubbub_tag *tag, 
		void **result);

/**
 * Type of tree element node creation function (verbatim name)
 */
typedef int (*hubbub_tree_create_element_verbatim)(void *ctx, 
		const uint8_t *name, size_t name_len, void **result);

/**
 * Type of tree text node creation function
 */
typedef int (*hubbub_tree_create_text)(void *ctx, const hubbub_string *data,
		void **result);

/**
 * Type of tree node reference function
 */
typedef int (*hubbub_tree_ref_node)(void *ctx, void *node);

/**
 * Type of tree node dereference function
 */
typedef int (*hubbub_tree_unref_node)(void *ctx, void *node);

/**
 * Type of tree node appending function
 */
typedef int (*hubbub_tree_append_child)(void *ctx, void *parent, void *child,
		void **result);

/**
 * Type of tree node insertion function
 */
typedef int (*hubbub_tree_insert_before)(void *ctx, void *parent, void *child,
		void *ref_child, void **result);

/**
 * Type of tree node removal function
 */
typedef int (*hubbub_tree_remove_child)(void *ctx, void *parent, void *child,
		void **result);

/**
 * Type of tree node cloning function
 */
typedef int (*hubbub_tree_clone_node)(void *ctx, void *node, bool deep,
		void **result);

/**
 * Type of tree quirks mode notification function
 */
typedef int (*hubbub_tree_set_quirks_mode)(void *ctx, hubbub_quirks_mode mode);

#endif
