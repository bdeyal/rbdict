
DEPS=Makefile rbdict.h
CFLAGS=-D_GNU_SOURCE -DNDEBUG -O2 -Wall -Wextra -Wno-unused-parameter -fno-exceptions
LFLAGS=-s

COMP=gcc
RBDICT_O=rbdict.o rbdict_test.o kernel-rbtree.o

EXES=rbdict

all: $(EXES) $(DEPS)

rbdict: $(RBDICT_O)
	$(COMP) -o $@ $(RBDICT_O) $(LFLAGS)

%.o: %.c $(DEPS)
	$(COMP) -c $(CFLAGS) -o $@ $<

memtest: all
	@for prog in $(EXES) ; do \
	echo -e "\nTesting" $$prog... ; \
	valgrind --quiet --leak-check=full ./$$prog 1>/dev/null; \
	done

clean:
	rm -f *~ $(RBDICT_O) $(EXES)
