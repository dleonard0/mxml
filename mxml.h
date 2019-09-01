
/** Lightweight, in-memory XML parser */
struct mxml;

/**
 * Creates an XML parser/editor context
 * Assumes that the XML source:
 * <ul><li>Is well-formed, and tags balance
 *     <li>Does not use "<tag/>"-style empty tags
 *     <li>Does not use attributes
 *     <li>Is encoded in UTF-8 or ASCII
 *     <li>Only uses entities &lt; &amp; &gt;
 *     <li>Only has text in leaf elements
 * </ul>
 * @param base Pointer to read-only, in-memory XML source.
 * @param len Length of the XML source in bytes.
 * @returns a context structure. Free it with #mxml_close().
 * @retval NULL on error, sets #errno.
 */
struct mxml *mxml_new(const char *base, unsigned int len);

/**
 * Closes the XML file opened by #xml_open().
 * All edits made will be lost
 */
void mxml_free(struct mxml *m);

/**
 * Gets the text value of an XML element.
 * Prior edits are honoured.
 * @param key The search key for the leaf element.
 *            The key is a '.'-delimited sequence of child tag names.  The
 *            first matching tag is found.
 *	      The subkey "tag[5]" is converted to "tags.tag5".
 *	      The special key "tag[#]" is converted to "tags.total".
 *	      The special key "tag[$]" expands to "tags.tagN" where "N"
 *	      is the value of "tags.total".
 *	      The following chars are not permitted in tag names: . # % [
 * @returns the unescaped content in a string allocated by malloc
 * @retval NULL [ENOENT] the key does not exist
 * @retval NULL [EINVAL] the key was malformed
 * @retval NULL [ENOMEM] the key was too long
 * @retval NULL [ENOMEM] not enough memory to copy the value
 */
char *mxml_get(struct mxml *m, const char *key);

/**
 * Tests if the tag described by the key exists.
 * @retval 0  the key does not exist
 * @retval 1  the key does exist
 */
int mxml_exists(struct mxml *m, const char *key);

/**
 * Deletes the element (and its children) from the document.
 * @retval 0 on success
 * @retval -1 [ENOENT] the element was deleted or does not exist.
 */
int mxml_delete(struct mxml *m, const char *key);

/**
 * Sets the text value of an existing element.
 * @retval 0  success
 * @retval -1 [ENOENT] the element has been deleted or never existed.
 */
int mxml_set(struct mxml *m, const char *key, const char *value);

/**
 * Appends a new tag to its parent, creating parents as needed.
 * @param key the tag to append.
 *            If the key ends with "[+]" then the next integer
 *            is used, and the total is updated. The caller
 *            can then use "[$]" to access the last key.
 * @param value (optional) the value of the tag; or NULL
 * @retval 0  success
 * @retval -1 [EXIST] the key already exists, and was not changed.
 */
int mxml_append(struct mxml *m, const char *key, const char *value);

/**
 * Writes the XML document with edits to a file descriptor.
 * @param fd Output file to write.
 * @returns the non-negative number of bytes written
 * @retval -1 on errors, sets #errno
 */
int mxml_write(int fd, const struct mxml *m);
