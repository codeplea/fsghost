cc ?= gcc

ccflags = -Wall -Wshadow -O2
lflags =


all: fsghost


fsghost: main.o
	$(cc) $(ccflags) -o $@ $^ $(lflags)


.c.o:
	$(cc) -c $(ccflags) $< -o $@


clean:
	rm *.o
	rm *.exe
