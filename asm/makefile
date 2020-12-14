CC=g++
SDL=`sdl-config --cflags --libs`
LIB=-lSDL
CONF=-O3 -Wall -Wno-unused-value -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result
OBJ=nesasm.o asm.o

all: $(OBJ)
	$(CC) $(OBJ) $(LIB) $(SDL) -o asm
	./asm

%.o: %.cc asm.h
	$(CC) $(SDL) $(LIB) $(CONF) -c $< -o $@

clean:
	rm -f *.o asm
