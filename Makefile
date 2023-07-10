CC=gcc
LINK=gcc

MEM=memtest
SIMPLE=simpletest
LIBRARY=memory.so
DEBUG=memdebug

MEMOBJS=memtest.o memory.o
SIMPLEOBJS=simpletest.o memory.o
LIBRARYOBJS=memory.o
DEBUGOBJS=simpletest.o memory_debug.o

LIBS=-lm
CFLAGS=-fPIC -fanalyzer -D WINDOWS=0 -D LOGGING=1 -ggdb3 -Og -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option
LFLAGS=-ggdb3 -Og
OFLAGS=-fPIC -ggdb3 -Og -rdynamic

all:${MEM} ${SIMPLE} ${LIBRARY}

debug:${DEBUG}

${MEM}: ${MEMOBJS}
	${CC} ${LFLAGS} -o $@ $^

${SIMPLE}: ${SIMPLEOBJS}
	${CC} ${LFLAGS} -o $@ $^

${LIBRARY}: ${LIBRARYOBJS}
	${CC} ${OFLAGS} -shared -Lstatic -o $@ $^

${DEBUG}: ${DEBUGOBJS}
	${CC} ${LFLAGS} -o $@ $^

memory.o: memory.c memory.h

memory_debug.o: memory_debug.c memory.h

memtest.o: memtest.c

simpletest.o: simpletest.c

.PHONY : clean

clean:
	rm -rf ${MEM} core*
	rm -rf ${SIMPLE} core*
	rm -rf ${LIBRARY} core*
	rm -rf ${DEBUG} core*
	rm -rf memory_debug.c core*
	rm -rf *.o core*
