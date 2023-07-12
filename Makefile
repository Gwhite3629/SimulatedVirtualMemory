CC=gcc
LINK=gcc

MEM=build/memtest
SIMPLE=build/simpletest
LIBRARY=build/memory.so
SIMPLEDEBUG=debug/simpledebug
MEMDEBUG=debug/memdebug

MEMOBJS=memtest.o memory.o
SIMPLEOBJS=simpletest.o memory.o
LIBRARYOBJS=memory.o
DEBUGOBJS=simpletest.o memory_debug.o
MEMDEBUGOBJS=memtest.o memory_debug.o

LIBS=-lm
CFLAGS=-fPIC -D WINDOWS=0 -D LOGGING=1 -Werror -Wall -Wextra -pedantic
DFLAGS=-fPIC -fanalyzer -D WINDOWS=0 -D LOGGING=1 -ggdb3 -Og -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option
BFLAGS=-O3
LFLAGS=-ggdb3 -Og
OFLAGS=-fPIC -O3 -rdynamic

all:${MEM} ${SIMPLE} ${LIBRARY}

all : CFLAGS += ${BFLAGS}

debug : CFLAGS += ${DFLAGS}

COMPdebug:${MEMDEBUG} ${SIMPLEDEBUG}

.PHONY : debug COMPdebug

${MEM}: ${MEMOBJS}
	mkdir -p build
	${CC} -o $@ $^

${SIMPLE}: ${SIMPLEOBJS}
	mkdir -p build
	${CC} -o $@ $^

${LIBRARY}: ${LIBRARYOBJS}
	mkdir -p build
	${CC} ${OFLAGS} -shared -Lstatic -o $@ $^

debug:
	make clean
	mkdir -p debug
	cp memory.c temp.c
	cp memory.h temp.h
	cp utils.h temp.u
	sed -e '1,6d' < memory.c > temp && mv temp memory.c
	sed -e '4,8d' < memory.h > temp && mv temp memory.h
	sed -e '4,7d' < utils.h > temp && mv temp utils.h
	gcc -E -P -C -D LOGGING=1 -D WINDOWS=0 memory.c > memory_debug.c
	mv temp.c memory.c
	mv temp.h memory.h
	mv temp.u utils.h
	sed -i '1s/^/#include <stdlib.h>\n /' memory_debug.c
	sed -i '1s/^/#include <stdbool.h>\n /' memory_debug.c
	sed -i '1s/^/#include <string.h>\n /' memory_debug.c
	sed -i '1s/^/#include <stdio.h>\n /' memory_debug.c
	sed -i '1s/^/#include <stdint.h>\n /' memory_debug.c
	sed -i '1s/^/#include <errno.h>\n /' memory_debug.c
	make COMPdebug

${SIMPLEDEBUG}: ${DEBUGOBJS}
	${CC} ${LFLAGS} -o $@ $^

${MEMDEBUG}: ${MEMDEBUGOBJS}
	${CC} ${LFLAGS} -o $@ $^

memtest.o: memtest.c
simpletest.o: simpletest.c

.PHONY : clean

clean:
	rm -rf ${MEM} core*
	rm -rf ${SIMPLE} core*
	rm -rf ${LIBRARY} core*
	rm -rf ${SIMPLEDEBUG} core*
	rm -rf ${MEMDEBUG} core*
	rm -rf memory_debug.c core*
	rm -rf *.o core*
	rm -rf build debug
