FLAGS = -g -Wall -std=c99 -Werror -o
LEAKFLAGS = --leak-check=full --show-leak-kinds=all
TEST1 = ./myChannels 0 1 1 1 1 1

all: myChannels

myChannels: myChannels.c
	gcc $(FLAGS) myChannels myChannels.c -lm

clean: 
	rm -rf *.o myChannels

debug:
	gdb $(TEST1)

leak:
	valgrind $(LEAKFLAGS) $(TEST1)

test:
	$(TEST1)
