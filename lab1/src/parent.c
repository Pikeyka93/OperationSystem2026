#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>

#include "common.h"

static const char PROG[] = "parent";
static const char CHILD1_NAME[] = "child1";
static const char CHILD2_NAME[] = "child2";

static void get_program_dir(const char *argv0, char *dir, size_t size) {
	ssize_t len = readlink("/proc/self/exe", dir, size - 1);

	if (len == -1) {
		len = 0;
		while (argv0[len] != '\0' && (size_t)len < size - 1) {
			dir[len] = argv0[len];
			++len;
		}
	}
	dir[len] = '\0';

	while (len > 0 && dir[len] != '/')
		--len;

	if (dir[len] == '/') {
		dir[len] = '\0';
	} else {
		dir[0] = '.';
		dir[1] = '\0';
	}
}

static pid_t spawn(const char *dir, const char *name, int in_fd, int out_fd,
                   const int *fds, size_t fds_count) {
	pid_t pid = fork();

	if (pid == -1)
		die(PROG, "failed to fork");

	if (pid == 0) {
		if (dup2(in_fd, STDIN_FILENO) == -1)
			die(name, "failed to redirect stdin");
		if (dup2(out_fd, STDOUT_FILENO) == -1)
			die(name, "failed to redirect stdout");

		for (size_t i = 0; i < fds_count; ++i)
			close(fds[i]);

		char path[1024];
		size_t len = 0;
		str_append(path, sizeof(path), &len, dir);
		str_append(path, sizeof(path), &len, "/");
		str_append(path, sizeof(path), &len, name);

		char *const args[] = {(char *)name, NULL};
		execv(path, args);

		die(name, "failed to exec child program");
	}

	return pid;
}

static int wait_child(pid_t pid, const char *name) {
	int status;

	while (waitpid(pid, &status, 0) == -1) {
		if (errno != EINTR) {
			print_error(PROG, "failed to wait for child");
			return -1;
		}
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)
		return 0;

	char msg[128];
	size_t len = 0;
	str_append(msg, sizeof(msg), &len, PROG);
	str_append(msg, sizeof(msg), &len, ": ");
	str_append(msg, sizeof(msg), &len, name);
	if (WIFEXITED(status)) {
		str_append(msg, sizeof(msg), &len, " exited with code ");
		int_append(msg, sizeof(msg), &len, WEXITSTATUS(status));
	} else {
		str_append(msg, sizeof(msg), &len, " was killed by signal ");
		int_append(msg, sizeof(msg), &len, WTERMSIG(status));
	}
	str_append(msg, sizeof(msg), &len, "\n");
	write_all(STDERR_FILENO, msg, len);
	return -1;
}

int main(int argc, char **argv) {
	(void)argc;

	struct sigaction sa = {0};
	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) == -1)
		die(PROG, "failed to ignore SIGPIPE");

	char dir[1024];
	get_program_dir(argv[0], dir, sizeof(dir));

	int pipe1[2];
	int pipe_mid[2];
	int pipe2[2];
	if (pipe(pipe1) == -1 || pipe(pipe_mid) == -1 || pipe(pipe2) == -1)
		die(PROG, "failed to create pipe");

	const int all_fds[] = {pipe1[0], pipe1[1], pipe_mid[0], pipe_mid[1], pipe2[0], pipe2[1]};
	const size_t all_count = sizeof(all_fds) / sizeof(all_fds[0]);

	pid_t child1 = spawn(dir, CHILD1_NAME, pipe1[0], pipe_mid[1], all_fds, all_count);
	pid_t child2 = spawn(dir, CHILD2_NAME, pipe_mid[0], pipe2[1], all_fds, all_count);

	close(pipe1[0]);
	close(pipe_mid[0]);
	close(pipe_mid[1]);
	close(pipe2[1]);

	int to_child = pipe1[1];
	int from_child = pipe2[0];

	int flags = fcntl(to_child, F_GETFL);
	if (flags == -1 || fcntl(to_child, F_SETFL, flags | O_NONBLOCK) == -1)
		die(PROG, "failed to make pipe non-blocking");

	char in_buf[BUF_SIZE];
	size_t in_len = 0;
	size_t in_pos = 0;
	char out_buf[BUF_SIZE];
	int stdin_open = 1;
	int status = EXIT_SUCCESS;

	while (from_child != -1) {
		struct pollfd fds[2];
		nfds_t nfds = 0;

		fds[nfds].fd = from_child;
		fds[nfds].events = POLLIN;
		++nfds;

		if (in_pos < in_len) {
			fds[nfds].fd = to_child;
			fds[nfds].events = POLLOUT;
			++nfds;
		} else if (stdin_open) {
			fds[nfds].fd = STDIN_FILENO;
			fds[nfds].events = POLLIN;
			++nfds;
		}

		if (poll(fds, nfds, -1) == -1) {
			if (errno == EINTR)
				continue;
			die(PROG, "failed to poll");
		}

		if (fds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
			ssize_t n = read(from_child, out_buf, sizeof(out_buf));
			if (n == -1 && errno != EINTR) {
				die(PROG, "failed to read from pipe2");
			} else if (n == 0) {
				close(from_child);
				from_child = -1;
			} else if (n > 0 && write_all(STDOUT_FILENO, out_buf, (size_t)n) == -1) {
				die(PROG, "failed to write to stdout");
			}
		}

		if (nfds < 2 || fds[1].revents == 0)
			continue;

		if (fds[1].fd == STDIN_FILENO) {
			ssize_t n = read(STDIN_FILENO, in_buf, sizeof(in_buf));
			if (n == -1 && errno != EINTR)
				die(PROG, "failed to read from stdin");

			if (n == 0) {
				stdin_open = 0;
				close(to_child);
				to_child = -1;
			} else if (n > 0) {
				in_len = (size_t)n;
				in_pos = 0;
			}
		} else {
			ssize_t n = write(to_child, in_buf + in_pos, in_len - in_pos);
			if (n == -1) {
				if (errno == EAGAIN || errno == EINTR)
					continue;
				print_error(PROG, "failed to write to pipe1");
				status = EXIT_FAILURE;
				in_pos = in_len = 0;
				stdin_open = 0;
				close(to_child);
				to_child = -1;
			} else {
				in_pos += (size_t)n;
			}
		}
	}

	if (to_child != -1)
		close(to_child);

	if (wait_child(child1, CHILD1_NAME) == -1)
		status = EXIT_FAILURE;
	if (wait_child(child2, CHILD2_NAME) == -1)
		status = EXIT_FAILURE;

	return status;
}
