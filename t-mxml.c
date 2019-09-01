#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "mxml.h"

#define MXML_NEW(s) mxml_new(s, sizeof s - 1)

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

int main() {

	struct mxml *m;
	char *s;

	/* With a trivial document */
	m = MXML_NEW("<a>b</a>");
	assert(m);
	/*  The key "a" exists in <a>b</a>  */
	assert(mxml_exists(m, "a"));
	/*  The value of key "a" in <a>b</a> is "b" */
	assert_streq(s = mxml_get(m, "a"), "b"); free(s);
	/*  Many keys do not exist in <a>b</a>  */
	assert(!mxml_exists(m, "aa"));
	assert(!mxml_exists(m, "a.a"));
	assert(!mxml_exists(m, "b"));
	/* Getting a non-existent key returns NULL */
	errno = 0;
	assert_streq(mxml_get(m, "b"), NULL);
	assert_inteq(errno, ENOENT, "d");
	/* Seeting a non-existent key fails */
	assert_errno(mxml_set(m, "a.x", "foo"), ENOENT);
	/* Creating a new key succeeds */
	assert0(mxml_append(m, "a.x", "foo"));
	/* Creating an existing key fails */
	assert_errno(mxml_append(m, "a.x", "foo"), EEXIST);
	/* Creating the root again fails */
	assert_errno(mxml_append(m, "a", "foo"), EEXIST);
	/* Can access a recently created key */
	assert_streq(s = mxml_get(m, "a.x"), "foo"); free(s);
	/* Created keys don't disrupt a previous parent value */
	assert_streq(s = mxml_get(m, "a"), "b"); free(s);
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
	assert_streq(s=mxml_get(m, "config.version"), "1"); free(s);
	assert_streq(s=mxml_get(m, "config.system.name"), "localhost"); free(s);
	/* Entity decoding works */
	assert_streq(s=mxml_get(m, "config.system.motd"), "Ben&Jerry's < Oak >"); free(s);
	/* Can change a key's value */
	assert0(mxml_set(m, "config.system.name", "fred"));
	assert_streq(s=mxml_get(m, "config.system.name"), "fred"); free(s);
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
	assert_streq(s=mxml_get(m, "top.dog[1].name"), "Fido"); free(s);
	assert_streq(s=mxml_get(m, "top.dog[2].colour"), "Spotty"); free(s);
	assert_null_errno(mxml_get(m, "top.dog[3].name"), ENOENT);
	assert_null_errno(mxml_get(m, "top.dog[0].name"), EINVAL);
	/* Some things shoould not exist */
	assert_null_errno(mxml_get(m, "top.rhinoceros[1].horn"), ENOENT);
	assert_null_errno(mxml_get(m, "top.unicorn[1].magic"), ENOENT);
	/* [#] expands to the total count */
	assert_streq(s=mxml_get(m, "top.dog[#]"), "2"); free(s);
	/* [#] expands to 0 if the list doesn't exist */
	assert_streq(s=mxml_get(m, "top.unicorn[#]"), "0"); free(s);
	/* [$] expands to the last item */
	assert_streq(s=mxml_get(m, "top.dog[$].name"), "Spot"); free(s);
	assert_null_errno(mxml_get(m, "top.unicorn[$].magic"), ENOENT);
	/* [#] must be last */
	assert_null_errno(mxml_get(m, "top.unicorn[#].magic"), EINVAL);
	/* can't write to [#] */
	assert_errno(mxml_set(m, "top.dog[#]", "9"), EPERM);

	/* Can insert a new unicorn */
	assert0(mxml_append(m, "top.unicorn[+].name", "Charlie"));
	assert_streq(s=mxml_get(m, "top.unicorn[$].name"), "Charlie"); free(s);
	assert_streq(s=mxml_get(m, "top.unicorn[#]"), "1"); free(s);
	/* Can delete an entire tree */
	assert0(mxml_delete(m, "top.cat[*]"));
	assert_streq(s=mxml_get(m, "top.cat[#]"), "0"); free(s);

}
