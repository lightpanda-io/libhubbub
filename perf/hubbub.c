#define _GNU_SOURCE

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

#include <hubbub/hubbub.h>
#include <hubbub/parser.h>
#include <hubbub/tree.h>

#define UNUSED(x) ((x) = (x))

typedef struct attr_t attr_t;
typedef struct node_t node_t;
typedef struct buf_t buf_t;

struct attr_t {
	hubbub_ns ns;
	char *name;
	char *value;
};

struct node_t {
	enum { DOCTYPE, COMMENT, ELEMENT, CHARACTER } type;

	union {
		struct {
			char *name;
			char *public_id;
			char *system_id;
		} doctype;

		struct {
			hubbub_ns ns;
			char *name;
			attr_t *attrs;
			size_t n_attrs;
		} element;

		char *content;		/**< For comments, characters **/
	} data;

	node_t *next;
	node_t *prev;

	node_t *child;
	node_t *parent;
};

struct buf_t {
	char *buf;
	size_t len;
	size_t pos;
};


#define NUM_NAMESPACES 7
const char *ns_names[NUM_NAMESPACES] =
		{ NULL, NULL /*html*/, "math", "svg", "xlink", "xml", "xmlns" };


node_t *Document;



static hubbub_error create_comment(void *ctx, const hubbub_string *data, void **result);
static hubbub_error create_doctype(void *ctx, const hubbub_doctype *doctype,
		void **result);
static hubbub_error create_element(void *ctx, const hubbub_tag *tag, void **result);
static hubbub_error create_text(void *ctx, const hubbub_string *data, void **result);
static hubbub_error ref_node(void *ctx, void *node);
static hubbub_error unref_node(void *ctx, void *node);
static hubbub_error append_child(void *ctx, void *parent, void *child, void **result);
static hubbub_error insert_before(void *ctx, void *parent, void *child, void *ref_child,
		void **result);
static hubbub_error remove_child(void *ctx, void *parent, void *child, void **result);
static hubbub_error clone_node(void *ctx, void *node, bool deep, void **result);
static hubbub_error reparent_children(void *ctx, void *node, void *new_parent);
static hubbub_error get_parent(void *ctx, void *node, bool element_only, void **result);
static hubbub_error has_children(void *ctx, void *node, bool *result);
static hubbub_error form_associate(void *ctx, void *form, void *node);
static hubbub_error add_attributes(void *ctx, void *node,
		const hubbub_attribute *attributes, uint32_t n_attributes);
static hubbub_error set_quirks_mode(void *ctx, hubbub_quirks_mode mode);
static hubbub_error change_encoding(void *ctx, const char *charset);

static hubbub_tree_handler tree_handler = {
	create_comment,
	create_doctype,
	create_element,
	create_text,
	ref_node,
	unref_node,
	append_child,
	insert_before,
	remove_child,
	clone_node,
	reparent_children,
	get_parent,
	has_children,
	form_associate,
	add_attributes,
	set_quirks_mode,
	change_encoding,
	NULL,
	NULL
};

static void *myrealloc(void *ptr, size_t len, void *pw)
{
	UNUSED(pw);

	return realloc(ptr, len);
}



int main(int argc, char **argv)
{
	hubbub_parser *parser;
	hubbub_parser_optparams params;

	struct stat info;
	int fd;
	uint8_t *file;

	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	assert(hubbub_parser_create("UTF-8", false, myrealloc, NULL, &parser) ==
			HUBBUB_OK);

	params.tree_handler = &tree_handler;
	assert(hubbub_parser_setopt(parser, HUBBUB_PARSER_TREE_HANDLER,
			&params) == HUBBUB_OK);

	params.document_node = (void *)1;
	assert(hubbub_parser_setopt(parser, HUBBUB_PARSER_DOCUMENT_NODE,
			&params) == HUBBUB_OK);

	stat(argv[1], &info);
	fd = open(argv[1], 0);
	file = mmap(NULL, info.st_size, PROT_READ, MAP_SHARED, fd, 0);

	assert(hubbub_parser_parse_chunk(parser, file, info.st_size)
			== HUBBUB_OK);

	return HUBBUB_OK;
}


/*** Tree construction functions ***/

hubbub_error create_comment(void *ctx, const hubbub_string *data, void **result)
{
	node_t *node = calloc(1, sizeof *node);

	UNUSED(ctx);

	node->type = COMMENT;
	node->data.content = strndup((const char *) data->ptr, data->len);

	*result = node;

	return HUBBUB_OK;
}

hubbub_error create_doctype(void *ctx, const hubbub_doctype *doctype, void **result)
{
	node_t *node = calloc(1, sizeof *node);

	UNUSED(ctx);

	node->type = DOCTYPE;
	node->data.doctype.name = strndup(
			(const char *) doctype->name.ptr,
			doctype->name.len);

	if (!doctype->public_missing) {
		node->data.doctype.public_id = strndup(
				(const char *) doctype->public_id.ptr,
				doctype->public_id.len);
	}

	if (!doctype->system_missing) {
		node->data.doctype.system_id = strndup(
				(const char *) doctype->system_id.ptr,
				doctype->system_id.len);
	}

	*result = node;

	return HUBBUB_OK;
}

hubbub_error create_element(void *ctx, const hubbub_tag *tag, void **result)
{
	node_t *node = calloc(1, sizeof *node);

	UNUSED(ctx);

	assert(tag->ns < NUM_NAMESPACES);

	node->type = ELEMENT;
	node->data.element.ns = tag->ns;
	node->data.element.name = strndup(
			(const char *) tag->name.ptr,
			tag->name.len);
	node->data.element.n_attrs = tag->n_attributes;

	node->data.element.attrs = calloc(node->data.element.n_attrs,
			sizeof *node->data.element.attrs);

	for (size_t i = 0; i < tag->n_attributes; i++) {
		attr_t *attr = &node->data.element.attrs[i];

		assert(tag->attributes[i].ns < NUM_NAMESPACES);

		attr->ns = tag->attributes[i].ns;

		attr->name = strndup(
				(const char *) tag->attributes[i].name.ptr,
				tag->attributes[i].name.len);

		attr->value = strndup(
				(const char *) tag->attributes[i].value.ptr,
				tag->attributes[i].value.len);
	}

	*result = node;

	return HUBBUB_OK;
}

hubbub_error create_text(void *ctx, const hubbub_string *data, void **result)
{
	node_t *node = calloc(1, sizeof *node);

	UNUSED(ctx);

	node->type = CHARACTER;
	node->data.content = strndup((const char *) data->ptr, data->len);

	*result = node;

	return HUBBUB_OK;
}

hubbub_error ref_node(void *ctx, void *node)
{
	UNUSED(ctx);
	UNUSED(node);

	return HUBBUB_OK;
}

hubbub_error unref_node(void *ctx, void *node)
{
	UNUSED(ctx);
	UNUSED(node);

	return HUBBUB_OK;
}

hubbub_error append_child(void *ctx, void *parent, void *child, void **result)
{
	node_t *tparent = parent;
	node_t *tchild = child;

	UNUSED(ctx);

	node_t *insert = NULL;

	tchild->parent = tparent;
	tchild->next = tchild->prev = NULL;

	*result = child;

	if (parent == (void *)1) {
		if (Document) {
			insert = Document;
		} else {
			Document = tchild;
		}
	} else {
		if (tparent->child == NULL) {
			tparent->child = tchild;
		} else {
			insert = tparent->child;
		}
	}

	if (insert) {
		while (insert->next != NULL) {
			insert = insert->next;
		}

		if (tchild->type == CHARACTER && insert->type == CHARACTER) {
			insert->data.content = realloc(insert->data.content,
					strlen(insert->data.content) +
					strlen(tchild->data.content) + 1);
			strcat(insert->data.content, tchild->data.content);
			*result = insert;
		} else {
			insert->next = tchild;
			tchild->prev = insert;
		}
	}

	return HUBBUB_OK;
}

/* insert 'child' before 'ref_child', under 'parent' */
hubbub_error insert_before(void *ctx, void *parent, void *child, void *ref_child,
		void **result)
{
	node_t *tparent = parent;
	node_t *tchild = child;
	node_t *tref = ref_child;

	UNUSED(ctx);

	if (tchild->type == CHARACTER && tref->prev &&
			tref->prev->type == CHARACTER) {
		node_t *insert = tref->prev;

		insert->data.content = realloc(insert->data.content,
				strlen(insert->data.content) +
				strlen(tchild->data.content) + 1);
		strcat(insert->data.content, tchild->data.content);

		*result = insert;
	} else {
		tchild->parent = parent;

		tchild->prev = tref->prev;
		tchild->next = tref;
		tref->prev = tchild;

		if (tchild->prev)
			tchild->prev->next = tchild;
		else
			tparent->child = tchild;

		*result = child;
	}

	return HUBBUB_OK;
}

hubbub_error remove_child(void *ctx, void *parent, void *child, void **result)
{
	node_t *tparent = parent;
	node_t *tchild = child;

	UNUSED(ctx);

	assert(tparent->child);
	assert(tchild->parent == tparent);

	if (tchild->parent->child == tchild) {
		tchild->parent->child = tchild->next;
	}

	if (tchild->prev)
		tchild->prev->next = tchild->next;

	if (tchild->next)
		tchild->next->prev = tchild->prev;

	/* now reset all the child's pointers */
	tchild->next = tchild->prev = tchild->parent = NULL;

	*result = child;

	return HUBBUB_OK;
}

hubbub_error clone_node(void *ctx, void *node, bool deep, void **result)
{
	node_t *old_node = node;
	node_t *new_node = calloc(1, sizeof *new_node);

	UNUSED(ctx);

	*new_node = *old_node;
	*result = new_node;

	new_node->child = new_node->parent =
			new_node->next = new_node->prev =
			NULL;

	if (deep == false)
		return HUBBUB_OK;

	if (old_node->next) {
		void *n;

		clone_node(ctx, old_node->next, true, &n);

		new_node->next = n;
		new_node->next->prev = new_node;
	}

	if (old_node->child) {
		void *n;

		clone_node(ctx, old_node->child, true, &n);

		new_node->child = n;
		new_node->child->parent = new_node;
	}

	return HUBBUB_OK;
}

/* Take all of the child nodes of "node" and append them to "new_parent" */
hubbub_error reparent_children(void *ctx, void *node, void *new_parent)
{
	node_t *parent = new_parent;
	node_t *old_parent = node;

	node_t *insert;
	node_t *kids;

	UNUSED(ctx);

	kids = old_parent->child;
	if (!kids) return HUBBUB_OK;

	old_parent->child = NULL;

	insert = parent->child;
	if (!insert) {
		parent->child = kids;
	} else {
		while (insert->next != NULL) {
			insert = insert->next;
		}

		insert->next = kids;
		kids->prev = insert;
	}

	while (kids) {
		kids->parent = parent;
		kids = kids->next;
	}

	return HUBBUB_OK;
}

hubbub_error get_parent(void *ctx, void *node, bool element_only, void **result)
{
	UNUSED(ctx);
	UNUSED(element_only);

	*result = ((node_t *)node)->parent;

	return HUBBUB_OK;
}

hubbub_error has_children(void *ctx, void *node, bool *result)
{
	UNUSED(ctx);

	*result = ((node_t *)node)->child ? true : false;

	return HUBBUB_OK;
}

hubbub_error form_associate(void *ctx, void *form, void *node)
{
	UNUSED(ctx);
	UNUSED(form);
	UNUSED(node);

	return HUBBUB_OK;
}

hubbub_error add_attributes(void *ctx, void *vnode,
		const hubbub_attribute *attributes, uint32_t n_attributes)
{
	node_t *node = vnode;
	size_t old_elems = node->data.element.n_attrs;

	UNUSED(ctx);

	node->data.element.n_attrs += n_attributes;

	node->data.element.attrs = realloc(node->data.element.attrs,
			node->data.element.n_attrs *
				sizeof *node->data.element.attrs);

	for (size_t i = 0; i < n_attributes; i++) {
		attr_t *attr = &node->data.element.attrs[old_elems + i];

		assert(attributes[i].ns < NUM_NAMESPACES);

		attr->ns = attributes[i].ns;

		attr->name = strndup(
				(const char *) attributes[i].name.ptr,
				attributes[i].name.len);

		attr->value = strndup(
				(const char *) attributes[i].value.ptr,
				attributes[i].value.len);
	}


	return HUBBUB_OK;
}

hubbub_error set_quirks_mode(void *ctx, hubbub_quirks_mode mode)
{
	UNUSED(ctx);
	UNUSED(mode);

	return HUBBUB_OK;
}

hubbub_error change_encoding(void *ctx, const char *charset)
{
	UNUSED(ctx);
	UNUSED(charset);

	return HUBBUB_OK;
}
