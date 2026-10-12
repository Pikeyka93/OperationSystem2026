#include "common.h"

static const char PROG[] = "child1";

static size_t to_lower(const unsigned char *in, size_t size, unsigned char *out, int *carry) {
	size_t o = 0;
	size_t i = 0;

	while (i < size) {
		unsigned char c = in[i];

		if (*carry != -1) {
			unsigned char lead = (unsigned char)*carry;
			*carry = -1;
			if (lead == 0xD0 && c >= 0x90 && c <= 0x9F) {
				out[o++] = 0xD0;
				out[o++] = c + 0x20;
			} else if (lead == 0xD0 && c >= 0xA0 && c <= 0xAF) {
				out[o++] = 0xD1;
				out[o++] = c - 0x20;
			} else if (lead == 0xD0 && c == 0x81) {
				out[o++] = 0xD1;
				out[o++] = 0x91;
			} else {
				out[o++] = lead;
				out[o++] = c;
			}
			++i;
			continue;
		}

		if (c >= 'A' && c <= 'Z') {
			out[o++] = c + ('a' - 'A');
		} else if (c == 0xD0) {
			*carry = c;
		} else {
			out[o++] = c;
		}
		++i;
	}

	return o;
}

int main(void) {
	unsigned char in[BUF_SIZE];
	unsigned char out[BUF_SIZE + 1];
	int carry = -1;
	ssize_t bytes;

	while ((bytes = read(STDIN_FILENO, in, sizeof(in))) != 0) {
		if (bytes == -1) {
			if (errno == EINTR)
				continue;
			die(PROG, "failed to read from stdin");
		}

		size_t n = to_lower(in, (size_t)bytes, out, &carry);
		if (write_all(STDOUT_FILENO, (const char *)out, n) == -1)
			die(PROG, "failed to write to stdout");
	}

	if (carry != -1) {
		unsigned char last = (unsigned char)carry;
		if (write_all(STDOUT_FILENO, (const char *)&last, 1) == -1)
			die(PROG, "failed to write to stdout");
	}

	return EXIT_SUCCESS;
}
