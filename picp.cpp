#include <iostream>
#include <sstream>
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <sys/select.h>

struct Port {

private:
	int fd;

	Port(Port const &);
	Port & operator = (Port const &);

	termios old_termstate;

public:

	Port(char const * f) {
		fd = open(f, O_RDWR);
		if (fd < 0) throw std::runtime_error("Unable to open file.");
		termios termstate;
		tcgetattr(fd, &termstate);
		old_termstate = termstate;
		termstate.c_iflag = IGNBRK | IGNPAR;
		termstate.c_oflag = 0;
		termstate.c_cflag = B115200 | CS8 | CREAD | CLOCAL;
		termstate.c_lflag = 0;
		tcsetattr(fd, TCSANOW, &termstate);
	}

	void write(uint8_t b) {
		if (::write(fd, &b, 1) < 0) throw std::runtime_error(std::string("Unable to write: ") + strerror(errno));
	}

	uint8_t read() {
		{
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 10000;
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			int r = select(fd + 1, &fds, 0, 0, &timeout);
			if (r < 0) throw std::runtime_error(std::string("Unable to read. select(): ") + strerror(errno));
			if (r == 0) throw std::runtime_error("Unable to read: Timeout.");
		}
		{
			uint8_t b;
			ssize_t r = ::read(fd, &b, 1);
			if (r != 1) throw std::runtime_error("Unable to read a byte.");
			if (r < 0) throw std::runtime_error(std::string("Unable to read: ") + strerror(errno));
			return b;
		}
	}

	~Port() {
		tcsetattr(fd, TCSANOW, &old_termstate);
		close(fd);
	}

};

struct Icsp {

	Port & p;

	Icsp(Port & p) : p(p) {
		p.write(' ');
	}

	std::string version() {
		p.write('V');
		std::string version;
		while (true) {
			char c = p.read();
			if (c == '\n') break;
			version += c;
		}
		return version;
	}

	void write_value(uint16_t value) {
		p.write(0x80 | (value >> 7));
		p.write(0x80 | (value & 0x7F));
	}

	void test() {
		p.write('T');
		if (p.read() != 'Y') throw std::runtime_error("Got invalid reply.");
	}

	uint16_t read_value() {
		uint8_t a = p.read();
		uint8_t b = p.read();
		if (!(a & b & 0x80)) throw std::runtime_error("Invalid data received.");
		return (a & 0x7F) << 7 | (b & 0x7F);
	}

	void begin() { p.write('B'); }
	void end() { p.write('E'); }
	void load_configuration(uint16_t v) { p.write('C'); write_value(v); }
	void load_data(uint16_t v) { p.write('L'); write_value(v); }
	uint16_t read_data() { p.write('R'); return read_value(); }
	void increment_address() { p.write('I'); }
	void reset_address() { p.write('A'); }
	void begin_programming() { p.write('P'); }
	void begin_externally_timed_programming() { p.write('Q'); }
	void end_externally_timed_programming() { p.write('S'); }
	void bulk_erase() { p.write('X'); }
	void row_erase() { p.write('Y'); }

};

int main(int argc, char * * argv) try {

	if (argc < 2 || argv[1] == std::string("help")) {
		std::clog << "Usage: \n\n";
		std::clog << '\t' << argv[0] << " /dev/tty...\n\t\tReset device and check connection.\n\n";
		std::clog << '\t' << argv[0] << " /dev/tty... read [file]\n\t\tTODO\n\n";
		// TODO
		return 1;
	}

	Port p(argv[1]);
	Icsp d(p);

	std::clog << d.version() << std::endl;
	std::clog << "Connected to programmer." << std::endl;

	std::clog << "Resetting target..." << std::endl;
	d.end();
	usleep(250000);
	d.begin();

	{
		d.load_configuration(0);
		for (size_t i = 0; i < 6; ++i) d.increment_address();
		uint16_t dev_id = d.read_data();
		std::string device_name = "unknown device";
		if      (dev_id == 0x3020) device_name = "PIC16F1454";
		else if (dev_id == 0x3024) device_name = "PIC16LF1454";
		else if (dev_id == 0x3021) device_name = "PIC16F1455";
		else if (dev_id == 0x3025) device_name = "PIC16LF1455";
		else if (dev_id == 0x3023) device_name = "PIC16F1459";
		else if (dev_id == 0x3027) device_name = "PIC16LF1459";
		else if (dev_id == 0x3FFF) {
			std::clog << "No target found." << std::endl;
			d.end();
			return 1;
		}
		std::clog << "Connected to " << device_name << "." << std::endl;
	}

	if (argc == 3 && argv[2] == std::string("config")) {
		d.load_configuration(0);
		char const *names[] = {
			"User ID 0", "User ID 1", "User ID 2", "User ID 3",
			"Reserved",
			"Revision ID", "Device ID",
			"Configuration Word 1", "Configuration Word 2",
			"Calibration Word 1", "Calibration Word 2"
		};
		for (unsigned int i = 0; i < 11; ++i) {
			uint16_t r = d.read_data();
			printf("%04X: 0x%04X\t%s \n", 0x8000 + i, r, names[i]);
			d.increment_address();
		}

	} else if (argc == 3 && argv[2] == std::string("dump")) {
		bool showprogress = isatty(fileno(stderr)) && !isatty(fileno(stdout));
		std::clog << "Downloading program memory..." << std::endl;
		d.reset_address();
		for (unsigned int a = 0; a < 0x4000; a += 2) {
			uint16_t v = d.read_data();
			d.increment_address();
			uint8_t checksum = 0x100 - 0x02 - (a & 0xFF) - (a >> 8) - (v & 0xFF) - (v >> 8);
			printf(":02%04X00%02X%02X%02X\n", a, v & 0xFF, v >> 8, checksum);
			if (showprogress) {
				std::stringstream s;
				s << "\r[";
				size_t x = (a * 73 / 0x4000);
				for (size_t i = 0; i < x; ++i) s << '=';
				if (x != 72) { s << '>'; ++x; }
				for (; x < 72; ++x) s << ' ';
				s << "] " << (a * 100 / (0x4000 - 1)) << '%';
				std::cerr << s.str();
			}
		}
		if (showprogress) std::clog << std::endl;
		std::clog << "Downloading configuration..." << std::endl;
		printf(":020000040001F9\n");
		d.load_configuration(0);
		for (unsigned int a = 0; a < 21; a += 2) {
			uint16_t v = d.read_data();
			d.increment_address();
			uint8_t checksum = 0x100 - 0x02 - (a & 0xFF) - (a >> 8) - (v & 0xFF) - (v >> 8);
			printf(":02%04X00%02X%02X%02X\n", a, v & 0xFF, v >> 8, checksum);
		}
		printf(":00000001FF\n");
		std::clog << "Done." << std::endl;

	//} else if (argc == 3 && argv[2] == std::string("program")) {
		// TODO
		// d.bulk_erase();
		// usleep(5000);
		// write progrm memory
		// write user ids
		// verify program memory
		// verify user ids
		// write config words
		// verify config words

	} else {
		std::clog << "Unknown command '" << argv[2] << "'." << std::endl;
		std::clog << "Run '" << argv[0] << " help' for help." << std::endl;
		return 1;
	}

	d.end();

} catch (std::exception & e) {
	std::clog << e.what() << std::endl;
	return 1;
}

