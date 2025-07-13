all: fs str list map grep bench

fs: fs_test.c
	gcc fs_test.c -o fs_test -Wall

str: str_test.c
	gcc str_test.c -o str_test -Wall

list: list_test.c
	gcc list_test.c -o list_test -Wall

map: map_test.c
	gcc map_test.c -o map_test -Wall

deque: deque_test.c
	gcc deque_test.c -o deque_test -Wall

grep: grep.c
	gcc grep.c -o grep -Wall

bench: bench.c
	gcc bench.c -o bench
