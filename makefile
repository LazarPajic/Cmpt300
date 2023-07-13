FLAGS = -g -Wall -std=c11 -Werror -pthread -o

all: myChannels

myChannels: myChannels.c
	gcc $(FLAGS) myChannels myChannels.c -lm

clean: 
	rm -rf *.o myChannels output_file.txt
