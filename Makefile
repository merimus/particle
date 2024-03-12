

benchmark : benchmark.cpp Makefile bhqt.hpp geom.hpp
	clang++ -gdwarf-4 -std=c++17 -Wall -O2 -o benchmark benchmark.cpp -ltbb
