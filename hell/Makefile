CSRC	= $(wildcard src/*.c)
CPPSRC	= $(wildcard src/*.cpp)
OBJ		= $(patsubst src/%.c,bin/%.o,$(CSRC)) $(patsubst src/%.cpp,bin/%.o,$(CPPSRC))


FLAGS = $(shell pkg-config --cflags --libs QtGui) -flto


all: hell


hell: $(OBJ) Makefile
	g++ -Wall -O2 -lSDL -lportmidi $(OBJ) -o hell $(FLAGS)


moc/main.moc: src/main.cpp
	@mkdir -p moc
	moc $< -o $@


bin/main.o: moc/main.moc


bin/%.o: src/%.c src/*.h Makefile
	@mkdir -p bin
	gcc --std=c99 -O3 $< -c -o $@ $(FLAGS)


bin/%.o: src/%.cpp src/*.h Makefile
	@mkdir -p bin
	g++ --std=c++11 -O2 $< -c -o $@ $(FLAGS)


clean:
	rm -rf bin moc hell
