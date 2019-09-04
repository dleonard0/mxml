
PREFIX ?= /usr
libdir ?= $(PREFIX)/lib
incdir ?= $(PREFIX)/include
INSTALL ?= install

#CFLAGS += -Wall -ggdb -O -pedantic
#LDFLAGS += -ggdb
PICFLAGS = -fPIC
PICFLAGS += -fvisibility=hidden

OBJS  = mxml.o
OBJS += mxml_cache.o
OBJS += mxml_cursor.o
OBJS += mxml_ekey.o
OBJS += mxml_find.o
OBJS += mxml_write.o
OBJS += mxml_flatten.o
OBJS += mxml_keys.o

default: libmxml.so libmxml.a
libmxml.a: libmxml.a($(OBJS))
libmxml.so: $(OBJS:.o=.po)
	$(LINK.c) -shared -o $@ $(OBJS:.o=.po)
libmxml.so: LDFLAGS += -Wl,-soname,libmxml.so

%.po: %.c
	$(COMPILE.c) -o $@ $(PICFLAGS) $<

# Unit tests
check: t-mxml
	./t-mxml
t-mxml: t-mxml.o $(OBJS)
	$(LINK.c) -o $@ $^

clean:
	-rm -f *.o t-mxml TAGS tags
	-rm -f *.po libmxml.a libmxml.so

install:
	-$(INSTALL) -d $(DESTDIR)$(libdir)
	-$(INSTALL) -d $(DESTDIR)$(incdir)
	$(INSTALL) -m 444 libmxml.a $(DESTDIR)$(libdir)/libmxml.a
	$(INSTALL) -m 444 libmxml.so $(DESTDIR)$(libdir)/libmxml.so
	$(INSTALL) -m 444 mxml.h $(DESTDIR)$(incdir)/mxml.h

show-unused-visible: $(OBJS)
	nm -go $(OBJS) | sed 's,:................,,' | \
	awk '$$2 == "T" { def[$$3] = $$1 } \
	     $$2 == "U" { use[$$3] = 1 } \
	     END { for (sym in def) \
	            if (!use[sym]) \
		     print def[sym] ": unused symbol, " sym; }'

.PHONY: default check clean install show-unused-visible
