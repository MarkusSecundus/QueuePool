all: src/main.cpp
	g++ src/main.cpp && ./a.out

clean:
	rm -f src/*.o a.out