#include <stdlib.h>	/* size_t */

#define KEY_MAX		256	/* maximum length of expanded key */

/* In-memory XML parser and editor */
struct mxml {
	const char *start;	/* First char of XML document */
	size_t size;		/* Length of XML document */
	struct edit *edits;	/* Reverse list of edits */
#if HAVE_CACHE
	/* A small cache of previously-found key prefixes
	 * within the XML body start[0..size-1] */
	struct cache {
		char key[CACHE_KEY_MAX];
		int keylen;
		const char *data; /* First byte after <tag> */
		size_t size;
	} cache[CACHE_MAX];
	unsigned int cache_next;
#endif
};

/* An edit record. These are always held unintegrated */
struct edit {
	struct edit *next;
	char *key;
	char *value;	/* Must be NULL when op=EDIT_DELETE */
	enum edit_op { EDIT_DELETE, EDIT_SET, EDIT_APPEND } op;
};

struct cursor {
	const char *pos;
	const char *end;
};

/* mxml_cursor.c */
int cursor_is_at_eof(const struct cursor *c);
int cursor_is_at(const struct cursor *c, const char *s);
int cursor_eatn(struct cursor *c, const char *s, unsigned int slen);
int cursor_eatch(struct cursor *c, char ch);
int cursor_eat_white(struct cursor *c);
void cursor_skip_to_ch(struct cursor *c, char ch);
void cursor_skip_content(struct cursor *c);
void cursor_skip_to_close(struct cursor *c);

/* mxml_cache.c */
#if HAVE_CACHE
void cache_init(struct mxml *m);
const char *cache_get(struct mxml *m, const char *ekey, int ekeylen,
		      size_t *sz_return);
void cache_set(struct mxml *m, const char *ekey, int ekeylen,
	       const char *data, size_t size)
#endif

/* mxml_ekey.c */
int expand_key(struct mxml *m, char *outbuf, size_t outbufsz, const char *key);

/* mxml_find.c */
const char *find_key(struct mxml *m, const char *ekey,
	int ekeylen, size_t *sz_return);


