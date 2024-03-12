all: src/main.cpp
	g++ -std=c++20 src/main.cpp && ./a.out

clean:
	rm -f src/*.o a.out