
CFLAGS += -Wall
CFLAGS += -ggdb
CFLAGS += -O -pedantic
LDFLAGS += -ggdb

OBJS  = mxml.o
OBJS += mxml_cache.o
OBJS += mxml_cursor.o
OBJS += mxml_ekey.o
OBJS += mxml_find.o
OBJS += mxml_write.o
OBJS += mxml_flatten.o
OBJS += mxml_keys.o

check: t-mxml
	./t-mxml
t-mxml: t-mxml.o $(OBJS)
	$(LINK.c) -o $@ $^
clean:
	-rm -f *.o t-mxml TAGS tags

show-unused-visible:
	nm -go $(OBJS) | sed 's,:................,,' | \
	awk '$$2 == "T" { def[$$3] = $$1 } \
	     $$2 == "U" { use[$$3] = 1 } \
	     END { for (sym in def) \
	            if (!use[sym]) \
		     print def[sym] ": unused symbol, " sym; }'
