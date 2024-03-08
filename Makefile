

benchmark : benchmark.cpp Makefile
	clang++ -gdwarf-4 -std=c++17 -Wall -Og -o benchmark benchmark.cpp -ltbb
