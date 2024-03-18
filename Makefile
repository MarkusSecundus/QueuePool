all: src/*.cpp src/*.h src/tests/*.cpp src/tests/*.h
	g++ -std=c++20 -Wall -Wextra -Wno-unknown-pragmas src/*.cpp src/tests/*.cpp && ./a.out

clean:
	rm -f src/*.o a.out