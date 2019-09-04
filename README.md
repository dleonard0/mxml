
# mxml

mxml is a C library that accesses an XML document as a key-value store.

It is designed for fast and lightweight use of the
simple XML config file format used by Opengear's 5000 and 7000 series
console server firmware.

## Features

The mxml library has the following design properties:

 - Fast with low memory use, especially in the common case of
   reading proximate keys
 - Small memory use when making changes to the XML
 - Support for Opengear's list key convention

## Reading

Here is an XML file containing two key-value pairs, `a.b.c` and `a.x`:

```xml
    <?xml?>
    <a>
      <b>
        <c>hello</c>
      </b>
      <x>meow</x>
    </a>
```

It can be accessed with this small demo program:

```c
#include <stdio.h>
#include <stdlib.h>
#include <mxml.h>

int
main()
{
	char *data = NULL;
	size_t datalen;
	struct mxml *db;

	/* Read XML from stdin into memory (You can use mmap) */
	getdelim(&data, &datalen, 0, stdin);

	/* Construct database handle */
	db = mxml_new(data, datalen);

	printf("a.b.c = %s\n", mxml_get(db, "a.b.c"));
	printf("a.x   = %s\n", mxml_get(db, "a.x"));

	mxml_free(db);
	free(data);
}
```

This would print, if given the previous XML example as input, the following output:

    a.b.c = hello
    a.x   = meow

The library avoids building the whole DOM in memory.
Instead, it scans the XML document
(usually mmap'd read-only into virtual memory)
to find the value requested.
A small prefix cache is used to remember where elements begin.

Values from `mxml_get()` are always returned as C strings allocated by _malloc()_,
and with XML entities decoded.

## Reading lists

Opengear-style config lists are supported.
The following demonstrates the 1-based list encoding of two cats.

```xml
    <?xml?>
    <a>
        <cats>
            <cat1>
                <name>Felix</name>
                <colour>black</colour>
            </cat1>
            <cat2>
                <name>Tom</name>
            </cat2>
            <total>2</total>
        </cats>
    </a>
```

Keys containing `[…]` list references are expanded into "plain" keys as follows:

	a.cat[1].name	->  a.cats.cat1.name
	a.cat[2].name	->  a.cats.cat2.name
	a.cat[#]	->  a.cats.total
	a.cat[$]	->  a.cats.cat2          because total = 2
	a.cat[+]	->  a.cats.cat3          because total = 2

## Editing

The library keeps track of "edits" made to the document.
Subsequent reads of keys are aware of the edits made and will return
the new values.

When writing out the document, the edit list is "flattened" against
the original XML source and a byte stream is generated.
You must supply a callback function to receive the byte stream.

Here is a program that demonstrates manipulating an XML document
and then writing it to standard output.

```c
#include <stdio.h>
#include <stdlib.h>
#include <mxml.h>

static int
writefn(void *context, const char *text, unsigned int len)
{
	return fwrite(text, 1, len, (FILE *)context);
}

int
main()
{
	char *data = NULL;
	size_t datalen;
	struct mxml *db;

	getdelim(&data, &datalen, 0, stdin);
	db = mxml_new(data, datalen);

	mxml_set(db, "a.title", "Famous cats");
	mxml_set(db, "a.cat[+].name", "Hello");
	mxml_set(db, "a.cat[$].colour", "pink");
	mxml_delete(db, "a.cat[1].name");

	mxml_write(db, writefn, stdout);

	mxml_free(db);
	free(data);
}
```

Adding a key containing `[+]` automatically increments the `.total` element.
Deleting a key ending in `[$]` automatically decrements `.total`.

## Time and memory complexity

This implementation has been designed primarily for size, then speed.

Key access time after edits is linearly proportional to the number of edits made.

The flattening process uses memory proportional to the number of edits,
but an execution time proportional to the product of the document size
and the number of edits.

## Other things

Other functions provided by the library are:

```c
struct mxml *mxml_new(const char *base, unsigned int len);
void         mxml_free(struct mxml *m);

char *       mxml_get(struct mxml *m, const char *key);
int          mxml_set(struct mxml *m, const char *key, const char *value);

int          mxml_exists(struct mxml *m, const char *key);
int          mxml_delete(struct mxml *m, const char *key);
int          mxml_update(struct mxml *m, const char *key, const char *value);
int          mxml_append(struct mxml *m, const char *key, const char *value);

char *       mxml_expand_key(struct mxml *m, const char *key);

int          mxml_write(const struct mxml *m,
                        int (*writefn)(void *context, const char *text, unsigned int len),
                        void *context);

char **      mxml_keys(const struct mxml *m, unsigned int *nkeys_return);
void         mxml_free_keys(char **keys, unsigned int nkeys);
```

If an error occurs, functions generally return -1 or NULL, and set `errno`.
See the [`<mxml.h>`](mxml.h) header file for details.

## Limitations

This library has the following limitations:

 - XML processing instructions, comments and tag attributes are ignored
 - Namespaces are not understood
 - CDATA is not (yet) understood
 - Character encoding is ignored (UTF-8 can be assumed)
 - Element names should be unique within their parent
 - Only the XML entities `&lt; &gt; &amp;` are converted
 - Only leaf-element values are supported

## Author

David Leonard, 2019
