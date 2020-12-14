CC=g++
SDL=`sdl-config --cflags --libs`
LIB=-lSDL
CONF=-O3 -Wall -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result
OBJ=main.o cpu.o disasm.o nes.o

all: $(OBJ)
	$(CC) $(OBJ) $(LIB) $(SDL) -o nes
	./nes ../rom/ducktales2.nes

%.o: %.cc nes.h cpu.h
	$(CC) $(SDL) $(LIB) $(CONF) -c $< -o $@

clean:
	rm -f *.o main
