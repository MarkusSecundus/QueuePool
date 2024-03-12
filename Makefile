all: src/main.cpp src/*.h src/tests/*.cpp src/tests/*.h
	g++ -std=c++20 src/main.cpp src/tests/*.cpp && ./a.out

clean:
	rm -f src/*.o a.out