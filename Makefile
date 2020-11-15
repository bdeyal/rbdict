
DEPS=Makefile rbdict.h
CFLAGS=-D_GNU_SOURCE -DNDEBUG -O2 -Wall -Wextra -Wno-unused-parameter
LFLAGS=-s

RBDICT_O=rbdict.o kernel-rbtree.o

EXES=rbdict wcnt

all: $(EXES) $(DEPS)

rbdict: rbdict_test.o $(RBDICT_O)
	$(CC) -o $@ rbdict_test.o $(RBDICT_O) $(LFLAGS)

wcnt: word_count.o $(RBDICT_O)
	$(CC) -o $@ word_count.o $(RBDICT_O) $(LFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c $(CFLAGS) -o $@ $<

memtest: all
	@for prog in $(EXES) ; do \
	echo -e "\nTesting" $$prog... ; \
	valgrind --quiet --leak-check=full ./$$prog 1>/dev/null; \
	done

test: all
	./$(EXES)

clean:
	rm -f *~ *.o $(EXES)
