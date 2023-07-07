FLAGS = -g -Wall -std=c99 -Werror -o
TEST1 = ./myChannels 1 1 1 1 1 1

all: myChannels

myChannels: myChannels.c
	gcc $(FLAGS) myChannels myChannels.c -lm

clean: 
	rm -rf *.o myChannels

leakCheck:
	valgrind $(TEST1)

test:
	$(TEST1)
