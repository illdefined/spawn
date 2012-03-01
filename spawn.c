#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <spawn.h>

#include <defy/bool>
#include <defy/expect>
#include <defy/nil>

#include <ev.h>

static char const *self;

static char **childArgv;
static char **childEnvp;

static bool respawn = true;
static double interval = 0.2;
static unsigned long number = 4;
static double penalty = 1.0;

static double argDouble(int argc, char *argv[const restrict], int *restrict argi) {
	if (unlikely(*argi + 1 >= argc)) {
		fprintf(stderr, "%s: option ‘%s’ requires a floating-point argument\n",
			self, argv[*argi]);
		exit(EXIT_FAILURE);
	}

	errno = 0;
	char *endp;
	double val = strtod(argv[++*argi], &endp);
	if (unlikely(errno)) {
		fprintf(stderr, "%s: invalid floating-point argument ‘%s’ to option ‘%s’: %s\n",
			self, argv[*argi], argv[*argi - 1], strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if (unlikely(*endp != '\0')) {
		fprintf(stderr, "%s: invalid trailing characters ‘%s’ to floating-point argument of option ‘%s’\n",
			self, endp, argv[*argi - 1]);
		exit(EXIT_FAILURE);
	}
	else if (unlikely(val < 0.0)) {
		fprintf(stderr, "%s: floating-point argument ‘%s’ to option ‘%s’ must be positive\n",
			self, argv[*argi], argv[*argi - 1]);
		exit(EXIT_FAILURE);
	}

	return val;
}

static unsigned long argInteger(int argc, char *argv[const restrict], int *restrict argi) {
	if (unlikely(*argi + 1 >= argc)) {
		fprintf(stderr, "%s: option ‘%s’ requires an integer argument\n", self, argv[*argi]);
		exit(EXIT_FAILURE);
	}

	errno = 0;
	char *endp;
	unsigned long val = strtoul(argv[++*argi], &endp, 0);
	if (unlikely(errno)) {
		fprintf(stderr, "%s: invalid integer argument ‘%s’ to option ‘%s’: %s\n",
			self, argv[*argi], argv[*argi - 1], strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if (unlikely(*endp != '\0')) {
		fprintf(stderr, "%s: invalid trailing characters ‘%s’ to integer argument of option ‘%s’\n",
			self, endp, argv[*argi - 1]);
		exit(EXIT_FAILURE);
	}

	return val;
}

static void timerEvent(struct ev_loop *restrict loop, struct ev_timer *watcher, int events) {
	ev_timer_stop(loop, watcher);

	struct ev_child *childWatcher = (struct ev_child *) watcher->data;

	pid_t pid;
	int err = posix_spawnp(&pid, childArgv[0], nil, nil, childArgv, childEnvp);
	if (unlikely(err)) {
		fprintf(stderr, "%s: failed to respawn child: %s\n", self, strerror(err));
		return;
	}

	ev_child_set(childWatcher, pid, 0);
	ev_child_start(loop, childWatcher);
}

static void childEvent(struct ev_loop *restrict loop, struct ev_child *watcher, int events) {
	ev_child_stop(loop, watcher);

	struct ev_timer *timerWatcher = (struct ev_timer *) watcher->data;
	double ival = interval;

	if ((WIFEXITED(watcher->rstatus) &&
		WEXITSTATUS(watcher->rstatus)) ||
		WIFSIGNALED(watcher->rstatus)) {
		if (!respawn)
			return;

		ival += penalty;
	}

	ev_timer_set(timerWatcher, ival, 0.0);
	ev_timer_start(loop, timerWatcher);
}

int main(int argc, char *argv[], char *envp[]) {
	self = argv[0];

	int argi;

	for (argi = 1; argi < argc; ++argi) {
		if (argv[argi][0] != '-' ||
			argv[argi][2] != '\0')
			break;

		switch (argv[argi][1]) {
		case '-':
			break;

		case 'e':
			respawn = false;
			break;

		case 'i':
			interval = argDouble(argc, argv, &argi);
			break;

		case 'n':
			number = argInteger(argc, argv, &argi);
			break;

		case 'p':
			penalty = argDouble(argc, argv, &argi);
			break;

		default:
			fprintf(stderr, "%s: invalid option ‘%s’\n", self, argv[argi]);
			return EXIT_FAILURE;
		}
	}

	if (unlikely(argi >= argc)) {
		fprintf(stderr, "usage: %s [-e] [-i interval] [-n number] [-p penalty] [command]\n", self);
		return EXIT_FAILURE;
	}

	childArgv = argv + argi;
	childEnvp = envp;

	struct ev_child childWatcher[number];
	struct ev_timer timerWatcher[number];

	struct ev_loop *loop = ev_default_loop(EVFLAG_AUTO);
	if (unlikely(!loop)) {
		fprintf(stderr, "%s: failed to initialise default event loop\n", self);
		return EXIT_FAILURE;
	}

	for (unsigned long iter = 0; iter < number; ++iter) {
		pid_t pid;
		int err = posix_spawnp(&pid, childArgv[0], nil, nil, childArgv, childEnvp);
		if (unlikely(err)) {
			fprintf(stderr, "%s: failed to spawn child: %s\n", self, strerror(err));
			return EXIT_FAILURE;
		}

		ev_child_init(childWatcher + iter, childEvent, pid, 0);
		childWatcher[iter].data = timerWatcher + iter;

		ev_timer_init(timerWatcher + iter, timerEvent, 0.0, 0.0);
		timerWatcher[iter].data = childWatcher + iter;

		ev_child_start(loop, childWatcher + iter);
	}

	ev_run(loop, 0);

	return EXIT_SUCCESS;
}
