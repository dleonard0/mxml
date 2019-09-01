/*
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
*/

#include <string.h>
#include <errno.h>

#include "mxml.h"
#include "mxml_int.h"

/**
 * Copies/calculates unencoded form of XML.
 * @param out (optional) buffer to copy unencoded data to.
 * @returns number of bytes copied into @a out.
 */
static size_t
unencode_xml_into(const struct cursor *init_curs, char *out)
{
	size_t n = 0;
	struct cursor c = *init_curs;

#define OUT(ch) do { if (out) out[n] = (ch); n++; } while (0)

	while (!cursor_is_at_eof(&c)) {
		char ch = *c.pos++;
		if (ch == '&' && !cursor_is_at_eof(&c)) {
			switch (*c.pos) {
			case 'l': OUT('<'); break; /* &lt; */
			case 'g': OUT('>'); break; /* &gt; */
			case 'a': OUT('&'); break; /* &amp; */
			}
			cursor_skip_to_ch(&c, ';');
			if (!cursor_is_at_eof(&c))
				c.pos++;
		} else if (ch == '<') {
			break;
		} else {
			OUT(ch);
		}
	}
	return n;
#undef OUT
}

/**
 * Unencodes XML into a new string.
 * Expands the XML entities, (&lt; &gt; &amp;)
 * @returns NUL-terminated string allocated by #malloc.
 * @retval NULL [ENOMEM] could not allocate memory.
 */
static char *
unencode_xml(const char *content, size_t contentsz)
{
	size_t retsz = 0;
	struct cursor c;
	char *ret;

	c.pos = content;
	c.end = content + contentsz;
	retsz = unencode_xml_into(&c, NULL);
	ret = malloc(retsz + 1);
	if (ret) {
		unencode_xml_into(&c, ret);
		ret[retsz] = '\0';
	}
	return ret;
}

struct mxml *
mxml_new(const char *start, unsigned int size)
{
	struct mxml *m = malloc(sizeof *m);

	if (!m)
		return NULL;
	m->start = start;
	m->size = size;
	m->edits = NULL;
#if HAVE_CACHE
	cache_init(m);
#endif
	return m;
}

void
mxml_free(struct mxml *m)
{
	struct edit *e;

	if (!m)
		return;
	while ((e = m->edits)) {
		m->edits = e->next;
		free(e->key);
		free(e->value);
		free(e);
	}
	free(m);
}

char *
mxml_get(struct mxml *m, const char *key)
{
	char ekey[KEY_MAX];
	int ekeylen;
	const char *content;
	size_t contentsz;

	ekeylen = expand_key(m, ekey, sizeof ekey, key);
	if (ekeylen < 0)
		return NULL;
	content = find_key(m, ekey, ekeylen, &contentsz);
	if (!content)
		return NULL;
	return unencode_xml(content, contentsz);
}

/**
 * Create a new edit record, inserted at the head
 * of the edit list.
 * @retval NULL [ENOMEM] no memory
 */
static struct edit *
edit_new(struct mxml *m, enum edit_op op)
{
	struct edit *e = calloc(1, sizeof *e);
	if (e) {
		e->next = m->edits;
		m->edits = e;
		e->op = op;
	}
	return e;
}

int
mxml_delete(struct mxml *m, const char *key)
{
	struct edit *edit = edit_new(m, EDIT_DELETE);
	if (!edit)
		return -1;
	edit->op = EDIT_DELETE;
	edit->key = strdup(key);
	edit->value = NULL;
	return 0;
}

int
mxml_set(struct mxml *m, const char *key, const char *value)
{
	struct edit *edit;
	const char *text;
	size_t sz;

	text = find_key(m, key, strlen(key), &sz);
	if (!text)
		return -1;
	edit = edit_new(m, EDIT_SET);
	if (!edit)
		return -1;
	edit->key = strdup(key);
	edit->value = strdup(value);
	return 0;
}

int
mxml_exists(struct mxml *m, const char *key)
{
	char ekey[KEY_MAX];
	int ekeylen;
	const char *content;
	size_t contentsz;

	ekeylen = expand_key(m, ekey, sizeof ekey, key);
	if (ekeylen < 0)
		return 0;
	content = find_key(m, ekey, ekeylen, &contentsz);
	return content != NULL;
}

int
mxml_append(struct mxml *m, const char *key, const char *value)
{
	struct edit *edit;

	if (mxml_exists(m, key)) {
		errno = EEXIST;
		return -1;
	}
	/* We don't have to worry about parents right now,
	 * they will be created later */
	edit = edit_new(m, EDIT_APPEND);
	if (!edit)
		return -1;
	edit->key = strdup(key);
	edit->value = strdup(value);
	return 0;
}
