#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

struct Port {

private:
	int fd;

	Port(Port const &);
	Port & operator = (Port const &);

public:

	Port(char const * f) {
		fd = open(f, O_RDWR);
		if (fd < 0) throw std::runtime_error(std::string("Unable to open ") + f + ".");
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
			if (r < 0) throw std::runtime_error(std::string("Unable to read: ") + strerror(errno));
			if (r != 1) throw std::runtime_error("Unable to read a byte.");
			usleep(1);
			return b;
		}
	}

	~Port() {
		close(fd);
	}

};

char const * default_port = "/dev/picp0";

inline bool is_port(char const * p) {
	return p[0] == '/';
}
