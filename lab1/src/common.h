#ifndef LAB1_COMMON_H
#define LAB1_COMMON_H

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 4096

static inline void str_append(char *dst, size_t size, size_t *pos, const char *src) {
	while (*src != '\0' && *pos + 1 < size)
		dst[(*pos)++] = *src++;
	dst[*pos] = '\0';
}

static inline void int_append(char *dst, size_t size, size_t *pos, int value) {
	char digits[16];
	int count = 0;
	unsigned int v = value < 0 ? 0u - (unsigned int)value : (unsigned int)value;

	do {
		digits[count++] = (char)('0' + v % 10);
		v /= 10;
	} while (v != 0);

	if (value < 0)
		digits[count++] = '-';

	char out[17];
	for (int i = 0; i < count; ++i)
		out[i] = digits[count - 1 - i];
	out[count] = '\0';

	str_append(dst, size, pos, out);
}

static inline int write_all(int fd, const char *buf, size_t size) {
	size_t done = 0;
	while (done < size) {
		ssize_t n = write(fd, buf + done, size - done);
		if (n == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		done += (size_t)n;
	}
	return 0;
}

static inline void print_error(const char *prog, const char *msg) {
	int saved_errno = errno;
	char out[512];
	size_t len = 0;

	str_append(out, sizeof(out), &len, prog);
	str_append(out, sizeof(out), &len, ": error: ");
	str_append(out, sizeof(out), &len, msg);
	str_append(out, sizeof(out), &len, " (errno=");
	int_append(out, sizeof(out), &len, saved_errno);
	str_append(out, sizeof(out), &len, ")\n");

	write_all(STDERR_FILENO, out, len);
}

static inline void die(const char *prog, const char *msg) {
	print_error(prog, msg);
	exit(EXIT_FAILURE);
}

#endif
