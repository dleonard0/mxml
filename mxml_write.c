#include <string.h>

#include "mxml.h"
#include "mxml_int.h"

struct write_token_context {
	size_t (*writefn)(const void *ptr, size_t size, size_t nmemb, void *context);
	void *context;
};

static size_t
write_token(void *context, const struct token *token)
{
	struct write_token_context *c = context;
	const char *first;
	const char *entity;
	const char *text;
	size_t ret = 0;

	if (token->valuelen > 0) {
		return c->writefn(token->value, 1, token->valuelen, c->context);
	}
	if (token->valuelen == 0)
		return 0;

#define OUT(s, len) do { \
		int _len = (len); \
		if (_len) { \
			const char *_s = (s); \
			size_t _n = c->writefn(_s, 1, _len, c->context); \
			if (_n == -1) return -1; \
			ret += _n; \
		} \
	} while (0)

	text = token->value;
	while ((text = strpbrk(first = text, "<>&"))) {
		OUT(first, text - first);
		if (*text == '<')
			entity = "&lt;";
		else if (*text == '>')
			entity = "&gt;";
		else /* if (*text == '&') */
			entity = "&amp;";
		OUT(entity, strlen(entity));
		text++;
	}
	OUT(first, strlen(first));
	return ret;
#undef OUT
}

size_t
mxml_write(const struct mxml *m,
	   size_t (*writefn)(const void *ptr, size_t size, size_t nmemb, void *context),
	   void *context)
{
	struct write_token_context c = {
		.writefn = writefn,
		.context = context
	};
	return flatten_edits(m, write_token, &c);
}
