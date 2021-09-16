#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "mxml.h"

#define MXML_NEW(s) mxml_new(s, sizeof s - 1)

/**
 * Compares two XML documents for equivalence, tolerant of some spaces.
 * Ignores leading and trailing whitespace and
 * whitespace before '<' and after '>'.
 * This removes fragility introduced by tag indentation changes.
 * @returns 1 if the two XML strings are equivalent with space equivalence.
 */
static int
xml_streq(const char *a, const char *b)
{

#	define EATSPACE() do { \
		while (*a && isspace(*a)) ++a; \
		while (*b && isspace(*b)) ++b; \
	    } while (0)

	if (!a || !b)
		return !a && !b;
	EATSPACE();
	while (*a && *b) {
		if (*a == *b && *a == '>') {
			a++, b++;
			EATSPACE();
		} else if (*a == *b) {
			a++, b++;
		} else {
			EATSPACE();
			if (*a == '>' && *b == '>')
				a++, b++;
			else
				return !*a && !*b;
		}
	}
	EATSPACE();
	return !*a && !*b;
#undef EATSPACE
}

#define assert_streq(a,b) do { \
	const char *_a = (a); \
	const char *_b = (b); \
	if (_a == NULL ? _b != NULL : _b == NULL || strcmp(_a,_b) != 0) { \
		fprintf(stderr, "%s:%d failed assert_streq(%s, %s)\n", \
			__FILE__, __LINE__, #a, #b); \
		fprintf(stderr, "%s != %s\n", _a, _b); \
		abort(); \
	} \
    } while (0)

#define assert_xml_streq(a,b) do { \
	const char *_a = (a); \
	const char *_b = (b); \
	if (!xml_streq(_a,_b)) { \
		fprintf(stderr, "%s:%d failed assert_xml_streq(%s, %s)\n", \
			__FILE__, __LINE__, #a, #b); \
		fprintf(stderr, "%s != %s\n", _a, _b); \
		abort(); \
	} \
    } while (0)

#define assert_inteq(a,b,fmt) do { \
	typeof(a) _a = (a); \
	typeof(b) _b = (b); \
	if (_a != _b) { \
		fprintf(stderr, "%s:%d failed assert_inteq(%s, %s)\n", \
			__FILE__, __LINE__, #a, #b); \
		fprintf(stderr, "%" fmt " != %" fmt "\n", _a, _b); \
		abort(); \
	} \
    } while (0)

#define assert0(e) do { \
	int _ret; \
	errno = 0; \
	if ((_ret = (e))) { \
		fprintf(stderr, \
			"%s:%d failed assert0(%s)\nret %d, errno %d %s\n", \
			__FILE__, __LINE__, #e, \
			_ret, errno, strerror(errno)); \
		abort(); \
	} \
    } while (0)

#define assert_errno(e, expected_errno) do { \
	int _ret; \
	errno = 0; \
	if ((_ret = (e)) != -1 || errno != (expected_errno)) { \
		fprintf(stderr, \
			"%s:%d failed assert_errno(%s, %s)\nret %d, errno %d %s\n", \
			__FILE__, __LINE__, #e, #expected_errno, \
			_ret, errno, strerror(errno)); \
		abort(); \
	} \
    } while (0)

#define assert_null_errno(e, expected_errno) do { \
	const void * _ret; \
	errno = 0; \
	if ((_ret = (e)) != NULL || errno != (expected_errno)) { \
		fprintf(stderr, \
			"%s:%d failed assert_errno(%s, %s)\nret %p, errno %d %s\n", \
			__FILE__, __LINE__, #e, #expected_errno, \
			_ret, errno, strerror(errno)); \
		abort(); \
	} \
    } while (0)

struct buf {
	char *data;
	size_t alloc;
	size_t len;
};
static int
buf_write(void *context, const char *d, unsigned int len) {
	struct buf *b = context;
	while (b->len + len + 1 > b->alloc) {
		b->data = realloc(b->data, b->alloc += 512);
		if (!b->data) return -1;
	}
	memcpy(b->data + b->len, d, len);
	b->len += len;
	b->data[b->len] = '\0';
	return len;
}
static void buf_init(struct buf *b) { memset(b, 0, sizeof *b); }
static void buf_clear(struct buf *b) { b->len = 0; buf_write(b, "", 0); }
static void buf_release(struct buf *b) { free(b->data); buf_init(b); }

int main() {
	struct buf buf;
	struct mxml *m;
	char **keys;
	unsigned int nkeys;

	buf_init(&buf);

	/* With a trivial document */
	m = MXML_NEW("<a>b</a>");
	assert(m);
	/*  The key "a" exists in <a>b</a>  */
	assert(mxml_exists(m, "a"));
	/*  The value of key "a" in <a>b</a> is "b" */
	assert_streq(mxml_get(m, "a"), "b");
	/*  Many keys do not exist in <a>b</a>  */
	assert(!mxml_exists(m, "aa"));
	assert(!mxml_exists(m, "a.a"));
	assert(!mxml_exists(m, "b"));
	/* Getting a non-existent key returns NULL */
	errno = 0;
	assert_streq(mxml_get(m, "b"), NULL);
	assert_inteq(errno, ENOENT, "d");
	/* Seeting a non-existent key fails */
	assert_errno(mxml_update(m, "a.x", "foo"), ENOENT);
	/* Creating a new key succeeds */
	assert0(mxml_append(m, "a.x", "foo"));
	/* Creating an existing key fails */
	assert_errno(mxml_append(m, "a.x", "foo"), EEXIST);
	/* Creating the root again fails */
	assert_errno(mxml_append(m, "a", "foo"), EEXIST);
	/* Can access a recently created key */
	assert_streq(mxml_get(m, "a.x"), "foo");
	/* Created keys don't disrupt a previous parent value */
	assert_streq(mxml_get(m, "a"), "b");
	mxml_free(m);

	/* With a more complicated document */
	m = MXML_NEW(
		"<?xml version=\"1.0\"?>\n"
		"  <!-- this is some stuff -->\n"
		"<config>\n"
		"  <version>1</version>\n"
		"  <system>\n"
		"    <name>localhost</name>\n"
		"    <motd>Ben&amp;Jerry's &lt; Oak &gt;</motd>\n"
		"  </system>\n"
		"</config>\n");
	assert_streq(mxml_get(m, "config.version"), "1");
	assert_streq(mxml_get(m, "config.system.name"), "localhost");
	/* Entity decoding works */
	assert_streq(mxml_get(m, "config.system.motd"), "Ben&Jerry's < Oak >");
	/* Can change a key's value */
	assert0(mxml_update(m, "config.system.name", "fred"));
	assert_streq(mxml_get(m, "config.system.name"), "fred");

	/* Can set an existing key */
	assert0(mxml_set(m, "config.system.name", "barney"));
	assert_streq(mxml_get(m, "config.system.name"), "barney");

	/* Can set a non-existing key */
	assert0(mxml_set(m, "config.system.model", "SD4002"));
	assert_streq(mxml_get(m, "config.system.model"), "SD4002");

	/* Can set a key to NULL and delete it */
	assert0(mxml_set(m, "config.system.model", NULL));
	assert(!mxml_exists(m, "config.system.model"));

	mxml_free(m);

	/* With a document containing lists: */
	m = MXML_NEW(
		"<top>"
		  "<dogs>"
		     "<dog1>"
		        "<name>Fido</name>"
			"<colour>Tan</colour>"
		     "</dog1>"
		     "<dog2>"
		        "<name>Spot</name>"
			"<colour>Spotty</colour>"
		     "</dog2>"
		     "<total>2</total>"
		  "</dogs>"
		  "<cats>"
		     "<cat1>"
		        "<name>Felix</name>"
			"<colour>Black</colour>"
			"<lives>3</lives>"
		     "</cat1>"
		     "<total>1</total>"
		  "</cats>"
		"</top>");
	assert_streq(mxml_get(m, "top.dog[1].name"), "Fido");
	assert_streq(mxml_get(m, "top.dog[2].colour"), "Spotty");
	assert_null_errno(mxml_get(m, "top.dog[3].name"), ENOENT);
	assert_null_errno(mxml_get(m, "top.dog[0].name"), EINVAL);
	/* Accessing non-existent list entries returnes ENOENT */
	assert_null_errno(mxml_get(m, "top.rhinoceros[1].horn"), ENOENT);
	assert_null_errno(mxml_get(m, "top.unicorn[1].magic"), ENOENT);
	/* [#] expands to the total count of a valid list */
	assert_streq(mxml_get(m, "top.dog[#]"), "2");
	/* [#] expands to 0 if the list doesn't exist */
	assert_streq(mxml_get(m, "top.unicorn[#]"), "0");
	/* [$] expands to the last item */
	assert_null_errno(mxml_get(m, "top.unicorn[$].magic"), ENOENT);
	assert_streq(mxml_get(m, "top.cat[$].lives"), "3");
	assert_streq(mxml_expand_key(m, "top.dog[$]"), "top.dog[2]");
	assert_streq(mxml_expand_key(m, "top.unicorn[$].magic"), "top.unicorn[0].magic");
	/* [#] is invalid when used in the middle of a key pattern */
	assert_null_errno(mxml_get(m, "top.unicorn[#].magic"), EINVAL);
	/* can't write to the [#] */
	assert_errno(mxml_update(m, "top.dog[#]", "9"), EPERM);

	/* Can insert a new unicorn */
	assert0(mxml_append(m, "top.unicorn[+].name", "Charlie"));
	assert_streq(mxml_get(m, "top.unicorn[$].name"), "Charlie");
	assert_streq(mxml_get(m, "top.unicorn[#]"), "1");
	/* Can delete an entire tree */
	assert0(mxml_delete(m, "top.cat[*]"));
	assert_streq(mxml_get(m, "top.cat[#]"), "0");

	/* Deleting the last element [$] updates .total [#] */
	assert_streq(mxml_get(m, "top.dog[#]"), "2");
	assert0(mxml_delete(m, "top.dog[$]"));
	assert_streq(mxml_get(m, "top.dog[#]"), "1");
	assert0(mxml_delete(m, "top.dog[$]"));
	assert_streq(mxml_get(m, "top.dog[#]"), "0");
	assert0(mxml_delete(m, "top.dog[$]"));
	assert_streq(mxml_get(m, "top.dog[#]"), "0");

	mxml_free(m);

	/* Writing an unchanged XML document yields an identical output */
	m = MXML_NEW("<?xml?>\n"
		"<top>\n"
		"  <foo>123</foo>\n"
		"</top>\n");		/* The \n is outside the doc */
	buf_clear(&buf);
	assert(mxml_write(m, buf_write, &buf) > 0);
	assert_streq("<?xml?>\n<top>\n  <foo>123</foo>\n</top>\n", buf.data);

	/* Changing a value works */
	assert0(mxml_update(m, "top.foo", "45678"));
	buf_clear(&buf);
	assert(mxml_write(m, buf_write, &buf) > 0);
	assert_streq("<?xml?>\n<top>\n  <foo>45678</foo>\n</top>\n", buf.data);

	/* A newly-added value appears in the output */
	assert0(mxml_append(m, "top.bar", "BAR"));
	buf_clear(&buf);
	assert(mxml_write(m, buf_write, &buf) > 0);
	assert_xml_streq("<?xml?>"
	    "<top>"
	      "<foo>45678</foo>"
	      "<bar>BAR</bar>"
	    "</top>", buf.data);

	/* Adding a list of cats now. */
	assert0(mxml_append(m, "top.cat[+].name", "Meow"));
	assert0(mxml_append(m, "top.cat[$].colour", "white"));
	assert0(mxml_append(m, "top.cat[+].name", "Kitty"));
	assert0(mxml_append(m, "top.cat[$].colour", "pink"));
	assert0(mxml_delete(m, "top.foo"));
	buf_clear(&buf);
	assert(mxml_write(m, buf_write, &buf) > 0);
	assert_xml_streq("<?xml?>"
	    "<top>"
	      "<bar>BAR</bar>"
	      "<cats>"
	        "<cat1>"
		  "<name>Meow</name>"
		  "<colour>white</colour>"
	        "</cat1>"
		"<total>2</total>"	/* may move around */
	        "<cat2>"
		  "<name>Kitty</name>"
		  "<colour>pink</colour>"
	        "</cat2>"
	      "</cats>"
	    "</top>", buf.data);

	buf_release(&buf);

	/* We can extract the expanded keys */
	assert((keys = mxml_keys(m, &nkeys)) != NULL);
	assert(nkeys == 10);
	assert_streq(keys[0], "top");
	assert_streq(keys[1], "top.bar");
	assert_streq(keys[2], "top.cats");
	assert_streq(keys[3], "top.cats.cat1");
	assert_streq(keys[4], "top.cats.cat1.name");
	assert_streq(keys[5], "top.cats.cat1.colour");
	assert_streq(keys[6], "top.cats.total");
	assert_streq(keys[7], "top.cats.cat2");
	assert_streq(keys[8], "top.cats.cat2.name");
	assert_streq(keys[9], "top.cats.cat2.colour");
	mxml_free_keys(keys, nkeys);

	mxml_free(m);
}
