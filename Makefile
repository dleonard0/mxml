
CFLAGS += -Wall
CFLAGS += -ggdb
CFLAGS += -O -pedantic
LDFLAGS += -ggdb

OBJS  = mxml.o
OBJS += mxml_cache.o
OBJS += mxml_cursor.o
OBJS += mxml_ekey.o
OBJS += mxml_find.o

check: t-mxml
	./t-mxml
t-mxml: t-mxml.o $(OBJS)
	$(LINK.c) -o $@ $^
clean:
	-rm -f *.o t-mxml TAGS tags

