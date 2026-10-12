#include "common.h"

static const char PROG[] = "child2";

static int is_space(char c) {
	return c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r';
}

int main(void) {
	char buf[BUF_SIZE];
	ssize_t bytes;

	while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) != 0) {
		if (bytes == -1) {
			if (errno == EINTR)
				continue;
			die(PROG, "failed to read from stdin");
		}

		for (ssize_t i = 0; i < bytes; ++i) {
			if (is_space(buf[i]))
				buf[i] = '_';
		}

		if (write_all(STDOUT_FILENO, buf, (size_t)bytes) == -1)
			die(PROG, "failed to write to stdout");
	}

	return EXIT_SUCCESS;
}
