FLAGS = -g -Wall -std=c11 -Werror -pthread -o
LEAKFLAGS = --leak-check=full --show-leak-kinds=all
EDTEST1 = ./myChannels 2 2 /home/vboxuser/cmpt300/a3/metadata.txt 1 1 /home/vboxuser/cmpt300/a3/output_file.txt

all: myChannels

myChannels: myChannels.c
	gcc $(FLAGS) myChannels myChannels.c -lm

clean: 
	rm -rf *.o myChannels output_file.txt

debug:
	gdb $(EDTEST1)

leak:
	valgrind $(LEAKFLAGS) $(EDTEST1)

test:
	$(EDTEST1)
