#include <windows.h>
#include <io.h>

struct Port {

private:
	HANDLE handle;

	Port(Port const &);
	Port & operator = (Port const &);

public:

	Port(char const * f) {
		std::string name = "\\\\.\\"; name += f;
		handle = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (handle == INVALID_HANDLE_VALUE) throw std::runtime_error(std::string("Unable to open ") + f + ".");
		COMMTIMEOUTS t;
		t.ReadIntervalTimeout = 0;
		t.ReadTotalTimeoutConstant = 100;
		t.ReadTotalTimeoutMultiplier = 0;
		t.WriteTotalTimeoutMultiplier = 0;
		t.WriteTotalTimeoutConstant = 0;
		SetCommTimeouts(handle, &t);
	}

	void write(uint8_t b) {
		DWORD written = 0;
		if (!WriteFile(handle, &b, 1, &written, 0) || written != 1) throw std::runtime_error("Unable to write data.");
	}

	uint8_t read() {
		uint8_t b;
		DWORD read = 0;
		if (!ReadFile(handle, &b , 1, &read, 0) || read != 1) throw std::runtime_error("Unable to read data.");
		return b;
	}

	~Port() {
		CloseHandle(handle);
	}

};

inline void usleep(unsigned int microseconds) {
	Sleep((microseconds + 999) / 1000);
}

char const * default_port = "COM5";

inline bool is_port(char const * p) {
	return p[0] == 'C' && p[1] == 'O' & p[2] == 'M';
}
