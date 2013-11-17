picp: picp.cpp
	$(CXX) -std=c++11 -Wall -Wextra -g -O2 -o $@ $^

picp.exe: picp.cpp
	i686-w64-mingw32-g++ -std=c++11 -static -O2 -o $@ $^
	i686-w64-mingw32-strip -s $@
