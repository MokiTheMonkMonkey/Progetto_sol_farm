CC			=  gcc
CFLAGS		+= -pedantic -pthread -Wall -std=c99 -g
INCLUDES	= -I ./includes
TARGETS		= farm
OBJS        = ./source/main.o ./source/utils.o ./source/ThreadsPool.o ./source/collector.o ./source/masterWorker.o

.PHONY: all clearAll clear clearExe test
.SUFFIXES: .c .h

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

all: $(TARGETS)


test: 		generafile farm
	./test.sh

generafile: ./source/generafile.c
	gcc -std=c99 -o $@ $^

farm : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

./source/generafile.o : ./source/generafile.c

./source/masterWorker.o : ./source/masterWorker.c

./source/collector.o : ./source/collector.c

./source/ThreadsPool.o : ./source/ThreadsPool.c

./source/main.o : ./source/main.c

./source/utils.o : ./source/utils.c

clearAll: clear clearExe

clear :
	\rm -f ./source/*.o
	\rm -f *.sck

clearExe :
	\rm  -f *.dat
	\rm -f generafile
	\rm -f farm
	\rm -f *.txt
	\rm -r testdir
