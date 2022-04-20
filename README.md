
# mxml

mxml is a C library that accesses an XML document as a key-value store.

It is designed for fast and lightweight use of the
simple XML config file format used by Opengear console server firmware.

```c
struct mxml;
struct mxml *mxml_new(const char *xml, size_t xml_len);
void         mxml_free(struct mxml *m);

const char * mxml_get(struct mxml *m, const char *key);
int          mxml_exists(struct mxml *m, const char *key);

int          mxml_delete(struct mxml *m, const char *key);
int          mxml_update(struct mxml *m, const char *key, const char *value);
int          mxml_append(struct mxml *m, const char *key, const char *value);
int          mxml_set(struct mxml *m, const char *key, const char *value);

int          mxml_write(const struct mxml *m,
                   size_t (*writefn)(const void *p, size_t size, size_t nmemb, void *context),
                   void *context);

char *       mxml_expand_key(struct mxml *m, const char *key);
char **      mxml_keys(const struct mxml *m, unsigned int *nkeys_return);
void         mxml_free_keys(char **keys, unsigned int nkeys);
```

## Features

The mxml library has the following design properties:

 - Fast initialisation
 - Low memory use when reading
 - Low memory use when making changes to the XML
 - Support for Opengear's list convention (`foo[1]` -> `foos.foo1`)

## File format

Here is an example XML file that contains two key-value pairs,
`a.b.c=hello` and `a.x=meow`:

```xml
    <?xml?>
    <a>
      <b>
        <c>hello</c>
      </b>
      <x>meow</x>
    </a>
```

We call `a.b.c` a "key path". Here it names the string value `hello`.
All values are UTF-8 strings and will not include NULs.

## Reading keys

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

	/* Read XML from stdin into memory */
	getdelim(&data, &datalen, 0, stdin);

	db = mxml_new(data, datalen);

	printf("a.b.c = %s\n", mxml_get(db, "a.b.c"));
	printf("a.x   = %s\n", mxml_get(db, "a.x"));

	mxml_free(db);
	free(data);
}
```

This program, given the previous XML example as input, will output:

    a.b.c = hello
    a.x   = meow

### Operational notes

The library does not construct a DOM when the file is opened.
Instead, the entire XML document is assumed to be in memory in
its on-disk form. Ideally the file is accessed using `mmap()`.

The `mxml_get()` traverses the XML document, locates the value,
expands any XML entities and appends a NUL terminator.
The function returns a pointer to a private buffer
that will be invalidated by the next call to `mxml_get()`
or `mxml_free()`.

## Lists

The library supports th Opengear config list convention.
In this convention, a "list-typed" value is encoded as a
subtree of 1-indexed elements.

For example, here is a list of two cats under the key path `a.cat`:

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

List elements can be accessed explicitly, e.g. `a.cats.cat1` and `a.cats.cat2`,
but this library provides a convenience syntax that uses square brackets.
Such convenience syntax is expanded as follows:

	a.cat[1].name	->  a.cats.cat1.name
	a.cat[2].name	->  a.cats.cat2.name
	a.cat[#]	->  a.cats.total
	a.cat[$]	->  a.cats.cat2          because a.cats.total = 2
	a.cat[+]	->  a.cats.cat3          because a.cats.total + 1 = 3

## Editing

In-memory edits can be made to a loaded XML document, even if the
document memory is read-only.

To achieve this, the library keeps track of building an "edit journal".
Subsequent read operations consult the active edit journal first so
the updates will be immediately visible.

The API uses a callback function to receive the byte stream generated
from applying the edit list.

This example program demonstrates manipulating a simple XML document
and then writing the result to standard output:

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

	getdelim(&data, &datalen, 0, stdin);
	db = mxml_new(data, datalen);

	mxml_set(db, "a.title", "Famous cats");
	mxml_set(db, "a.cat[+].name", "Hello");
	mxml_set(db, "a.cat[$].colour", "pink");
	mxml_delete(db, "a.cat[1].name");

	mxml_write(db, fwrite, stdout);

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

## Error codes

If an error occurs, functions return -1 or NULL, and always set `errno`.
See the [`<mxml.h>`](mxml.h) header file for details.

## Limitations

This library has the following limitations:

 - XML processing instructions, comments and tag attributes are ignored
 - Namespaces are not understood
 - Character encoding is ignored (UTF-8 can be assumed)
 - Element names should be unique within their parent
 - Only the XML entities `&lt; &gt; &amp;` are converted
 - Only leaf-element values are supported

## Author

David Leonard, 2019
