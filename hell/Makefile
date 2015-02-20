CSRC	= $(wildcard src/*.c)
CPPSRC	= $(wildcard src/*.cpp)
OBJ		= $(patsubst src/%.c,bin/%.o,$(CSRC)) $(patsubst src/%.cpp,bin/%.o,$(CPPSRC))


QT = $(shell pkg-config --cflags --libs QtGui)


all: hell


hell: $(OBJ) Makefile
	g++ -Wall -O2 -lSDL -flto -lportmidi $(OBJ) -o hell $(QT)


moc/main.moc: src/main.cpp
	@mkdir -p moc
	moc $< -o $@


bin/main.o: moc/main.moc


bin/%.o: src/%.c src/*.h Makefile
	@mkdir -p bin
	gcc --std=c99 -O3 -lfo $< -c -o $@ $(QT)


bin/%.o: src/%.cpp src/*.h Makefile
	@mkdir -p bin
	g++ --std=c++11 -O2 -flto $< -c -o $@ $(QT)


clean:
	rm -rf bin moc hell
