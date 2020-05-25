DEBUG = true
CC = clang
CFLAGS = -std=c99 -pedantic -O0 -g3 ## -O3 # -O0 -g3
CWARNINGS = -Wall -Wextra -Wshadow
CSANITIZERS = -fsanitize=undefined
CPLATFORM = -D_POSIX_C_SOURCE=200809L

.PHONY : all clean

all : dbar

clean :
	@-rm -f obj/*.o dbar

dbar : $(shell ls *.c *.h 2>/dev/null)
	${CC} ${CFLAGS} ${CPLATFORM} ${CWARNINGS} ${CSANITIZERS} dbar.c -o dbar
