#include <iostream>
#include <iomanip>
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
			timeout.tv_usec = 100000;
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

uint8_t hex_value(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	throw std::runtime_error(std::string("Not a valid hexadecimal digit: '") + c + "'.");
}

uint8_t hex_byte(std::string & s, size_t i = 0) {
	if (i + 1 >= s.size() || s[i+1] == '\n') throw std::runtime_error("Unexpected end of line.");
	return hex_value(s[i]) << 4 | hex_value(s[i+1]);
}

struct memory_dump {

	uint16_t memory[0x2000];
	uint16_t user_id[4];
	uint16_t revision_id;
	uint16_t device_id;
	uint16_t configuration[2];

	size_t memory_used = 0;
	bool user_id_set = false;
	bool revision_id_set = false;
	bool device_id_set = false;
	bool configuration_set = false;

	memory_dump() {
		std::fill(std::begin(memory), std::end(memory), 0x3FFF);
	}

	void load_ihex(std::istream & in) {
		std::string line;
		size_t address_offset = 0;
		while (std::getline(in, line)) {
			if (line[0] != ':') continue;
			size_t size = hex_byte(line, 1);
			size_t address = hex_byte(line, 3) << 8 | hex_byte(line, 5);
			uint8_t type = hex_byte(line, 7);
			if (type == 0x00) {
				if (size % 2 != 0 || address % 2 != 0) throw std::runtime_error("Only data records with an even number of bytes on even addresses are supported.");
				for (size_t i = 0; i < size / 2; ++i) {
					size_t a = (address_offset + address) / 2 + i;
					uint16_t value = hex_byte(line, 9 + i * 4 + 2) << 8 | hex_byte(line, 9 + i * 4);
					if (a < 0x2000) {
						memory[a] = value;
						memory_used = std::max(a + 1, memory_used);
					} else if (a >= 0x8000 && a < 0x8004) {
						user_id[a - 0x8000] = value;
						user_id_set = true;
					} else if (a == 0x8005) {
						revision_id = value;
						revision_id_set = true;
					} else if (a == 0x8006) {
						device_id = value;
						device_id_set = true;
					} else if (a >= 0x8007 && a < 0x8009) {
						configuration[a - 0x8007] = value;
						configuration_set = true;
					} else if (a >= 0x8000 && a <= 0x800A) {
						// ignore
					} else {
						std::stringstream s;
						s << "Got data for invalid address " << std::hex << a << ".";
						throw std::runtime_error(s.str());
					}
				}
			} else if (type == 0x01) {
				break;
			} else if (type == 0x02 && size == 2) {
				address_offset = hex_byte(line, 9) << 12 | hex_byte(line, 11) << 4;
			} else if (type == 0x04 && size == 2) {
				address_offset = hex_byte(line, 9) << 24 | hex_byte(line, 11) << 16;
			} else {
				std::stringstream s;
				s << "Unknown record type 0x" << std::hex << type << " (size " << std::dec << size << ").";
				throw std::runtime_error(s.str());
			}
		}
	}


};

void print_progress(unsigned int now, unsigned int limit) {
	std::stringstream s;
	s << "\r[";
	size_t x = now * 72 / limit;
	for (size_t i = 0; i < x; ++i) s << '=';
	if (x != 72) { s << '>'; ++x; }
	for (; x < 72; ++x) s << ' ';
	s << "] " << (now * 100 / limit) << '%';
	std::cerr << s.str();
}

std::string hex_word(uint16_t v) {
	std::stringstream s;
	s << std::hex << std::setfill('0') << std::setw(4) << v;
	return s.str();
}

void verify_failure(char const * part, uint16_t good, uint16_t bad) {
	std::stringstream s;
	s << "Verification failure in " << part << ": ";
	s << "0x" << hex_word(good) << " was written, but ";
	s << "0x" << hex_word(bad) << " was read.";
	throw std::runtime_error(s.str());
}

int main(int argc, char * * argv) try {

	if (argc < 2) {
		std::clog << "Usage: \n";
		std::clog << '\t' << argv[0] << " /dev/tty...\n\t\tCheck connection with programmer.\n\n";
		std::clog << '\t' << argv[0] << " /dev/tty... reset\n\t\tReset target.\n\n";
		std::clog << '\t' << argv[0] << " /dev/tty... config\n\t\tShow the configuration words.\n\n";
		std::clog << '\t' << argv[0] << " /dev/tty... dump [> file]\n\t\tRead the program and configuration memory, and dump it in Intel HEX format.\n\n";
		std::clog << '\t' << argv[0] << " /dev/tty... program [< file]\n\t\tFlash the given program (and optionally, configuration and user id words) (in Intel HEX format) to the connected chip.\n\n";
		std::clog << '\t' << argv[0] << " /dev/tty... erase\n\t\tErase the program and configuration memory, excluding the four user id words.\n\n";
		std::clog << '\t' << argv[0] << " /dev/tty... eraseall\n\t\tErase the program and configuration memory, including the four user id words.\n\n";
		return 1;
	}

	Port p(argv[1]);
	Icsp d(p);

	std::clog << d.version() << std::endl;
	std::clog << "Connected to programmer." << std::endl;

	uint16_t revision_id = 0;
	uint16_t device_id = 0;

	auto connect = [&] {
		std::clog << "Resetting target..." << std::endl;
		d.end();
		usleep(250000);
		d.begin();

		d.load_configuration(0);
		for (size_t i = 0; i < 5; ++i) d.increment_address();
		revision_id = d.read_data();
		d.increment_address();
		device_id = d.read_data();
		{
			std::string device_name = "unknown device";
			if      (device_id == 0x3020) device_name = "PIC16F1454";
			else if (device_id == 0x3024) device_name = "PIC16LF1454";
			else if (device_id == 0x3021) device_name = "PIC16F1455";
			else if (device_id == 0x3025) device_name = "PIC16LF1455";
			else if (device_id == 0x3023) device_name = "PIC16F1459";
			else if (device_id == 0x3027) device_name = "PIC16LF1459";
			else if (device_id == 0x3FFF) throw std::runtime_error("No target found.");
			std::clog << "Connected to " << device_name << "." << std::endl;
		}
	};

	bool showprogress = isatty(fileno(stderr));

	if (argc == 3 && argv[2] == std::string("reset")) {
		connect();

	} else if (argc == 3 && argv[2] == std::string("config")) {
		connect();
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
		connect();
		showprogress &= !isatty(fileno(stdout));
		std::clog << "Downloading program memory..." << std::endl;
		d.reset_address();
		for (unsigned int a = 0; a <= 0x3FFE; a += 2) {
			uint16_t v = d.read_data();
			d.increment_address();
			uint8_t checksum = 0x100 - 0x02 - (a & 0xFF) - (a >> 8) - (v & 0xFF) - (v >> 8);
			printf(":02%04X00%02X%02X%02X\n", a, v & 0xFF, v >> 8, checksum);
			if (showprogress) print_progress(a, 0x3FFE);
		}
		if (showprogress) std::clog << std::endl;
		std::clog << "Downloading configuration..." << std::endl;
		printf(":020000040001F9\n");
		d.load_configuration(0);
		for (unsigned int a = 0; a <= 20; a += 2) {
			uint16_t v = d.read_data();
			d.increment_address();
			uint8_t checksum = 0x100 - 0x02 - (a & 0xFF) - (a >> 8) - (v & 0xFF) - (v >> 8);
			printf(":02%04X00%02X%02X%02X\n", a, v & 0xFF, v >> 8, checksum);
			if (showprogress) print_progress(a, 20);
		}
		if (showprogress) std::clog << std::endl;
		printf(":00000001FF\n");
		std::clog << "Done." << std::endl;

	} else if (argc == 3 && argv[2] == std::string("erase")) {
		connect();
		std::clog << "Erasing..." << std::endl;
		d.reset_address();
		d.bulk_erase();
		usleep(5000);
		d.test();
		std::clog << "Done." << std::endl;

	} else if (argc == 3 && argv[2] == std::string("eraseall")) {
		connect();
		std::clog << "Erasing..." << std::endl;
		d.load_configuration(0);
		d.bulk_erase();
		usleep(5000);
		d.test();
		std::clog << "Done." << std::endl;

	} else if (argc == 3 && argv[2] == std::string("program")) {
		memory_dump m;
		std::clog << "Reading Intel HEX formatted data..." << std::endl;
		m.load_ihex(std::cin);
		connect();
		if (m.device_id_set) {
			if (m.device_id == device_id) {
				std::clog << "Device ID matches." << std::endl;
			} else {
				throw std::runtime_error("Device ID does not match (hex file: " + hex_word(m.device_id) + ", device: " + hex_word(device_id) + ").");
			}
		}
		if (m.revision_id_set) {
			if (m.revision_id == revision_id) {
				std::clog << "Revision ID matches." << std::endl;
			} else {
				throw std::runtime_error("Revision ID does not match (hex file: " + hex_word(m.revision_id) + ", device: " + hex_word(revision_id) + ").");
			}
		}
		if (!m.configuration_set) {
			std::clog << "Warning: No configuration bits are given. The configuration bits will be erased but not programmed, thus left at all bits set." << std::endl;
		}
		std::clog << "Erasing..." << std::endl;
		if (m.user_id_set) {
			d.load_configuration(0);
		} else {
			d.reset_address();
		}
		d.bulk_erase();
		usleep(5000);
		std::clog << "Writing " << m.memory_used << " words to program memory..." << std::endl;
		d.reset_address();
		size_t latches_filled = 0;
		for (size_t a = 0; a < m.memory_used; ++a) {
			d.load_data(m.memory[a]);
			if (++latches_filled == 32 || a == m.memory_used - 1) {
				d.begin_programming();
				d.test();
				usleep(2500);
				latches_filled = 0;
				if (showprogress) print_progress(a, m.memory_used - 1);
			}
			d.increment_address();
		}
		if (showprogress) std::clog << std::endl;
		std::clog << "Verifying program memory..." << std::endl;
		d.reset_address();
		for (size_t a = 0; a < m.memory_used; ++a) {
			uint16_t v = d.read_data();
			if (v != m.memory[a]) verify_failure("program memory", m.memory[a], v);
			if (showprogress) print_progress(a, m.memory_used - 1);
			d.increment_address();
		}
		if (showprogress) std::clog << std::endl;
		if (m.configuration_set || m.user_id_set) {
			d.load_configuration(0);
			if (m.user_id_set) {
				std::clog << "Writing and verifying user id..." << std::endl;
				for (size_t i = 0; i < 4; ++i) {
					d.load_data(m.user_id[i]);
					d.begin_programming();
					usleep(5000);
					uint16_t v = d.read_data();
					if (v != m.user_id[i]) verify_failure("user id", m.user_id[i], v);
					d.increment_address();
					if (showprogress) print_progress(i, 3);
				}
				if (showprogress) std::clog << std::endl;
			} else {
			}
			if (m.configuration_set) {
				for (size_t i = 0; i < (m.user_id_set ? 3 : 7); ++i) d.increment_address();
				std::clog << "Writing and verifying configuration bits..." << std::endl;
				for (size_t i = 0; i < 2; ++i) {
					d.load_data(m.configuration[i]);
					d.begin_programming();
					usleep(5000);
					uint16_t v = d.read_data();
					if (v != m.configuration[i]) verify_failure("configuration bits", m.user_id[i], v);
					d.increment_address();
					if (showprogress) print_progress(i, 1);
				}
				if (showprogress) std::clog << std::endl;
			}
		}
		std::clog << "Done." << std::endl;

	} else if (argc > 2) {
		std::clog << "Unknown command '" << argv[2] << "'." << std::endl;
		std::clog << "Run '" << argv[0] << "' (without arguments) for help." << std::endl;
		return 1;
	}

	d.end();

} catch (std::exception & e) {
	std::clog << e.what() << std::endl;
	return 1;
}

