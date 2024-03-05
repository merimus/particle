

benchmark : benchmark.cpp Makefile
	clang++ -g -std=c++17 -Wall -O3 -o benchmark benchmark.cpp -ltbb

