picp: picp.cpp linux.hpp
	$(CXX) -std=c++11 -Wall -Wextra -g -O2 -o $@ picp.cpp

picp.exe: picp.cpp windows.hpp
	i686-w64-mingw32-g++ -std=c++11 -static -O2 -o $@ picp.cpp
	i686-w64-mingw32-strip -s $@
