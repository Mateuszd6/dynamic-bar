CC = clang
CFLAGS = -std=c99 -O0 -g3 -pedantic
CWARNINGS = -Wall -Wextra -Wshadow
CSANITIZERS = -fsanitize=undefined
CPLATFORM = -D_POSIX_C_SOURCE=200809L

.PHONY : all clean

all : main

clean :
	@-rm -f obj/*.o main

main : $(shell ls *.c *.h 2>/dev/null)
	${CC} ${CFLAGS} ${CPLATFORM} ${CWARNINGS} ${CSANITIZERS} main.c -o main
