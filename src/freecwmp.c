/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <unistd.h>
#include <libubox/uloop.h>

#include "freecwmp.h"
#include "config.h"
#include "cwmp/cwmp.h"
#include "cwmp/local.h"

static void print_help(void) {
	printf("Usage: %s [OPTIONS]\n", FREECWMP_NAME);
	printf(" -f, --foreground    Run in the foreground\n");
	printf(" -h, --help          Display this help text\n");
}

int main (int argc, char **argv)
{
	FC_DEVEL_DEBUG("enter");

	setlocale(LC_CTYPE, "");
	umask(0037);

#ifndef DUMMY_MODE
	if (getuid() != 0) {
		fprintf(stderr, "run %s as root\n", FREECWMP_NAME);
		exit(EXIT_FAILURE);
	}
#endif

	struct option long_opts[] = {
		{"foreground", no_argument, NULL, 'f'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};


	uint8_t foreground = 0;

	int c;
	while (1) {
		c = getopt_long(argc, argv, "fh", long_opts, NULL);
		if (c == EOF)
			break;
		switch (c) {
			case 'f':
				foreground = 1;
				break;
			case 'h':
				print_help();
				exit(EXIT_FAILURE);
			default:
				fprintf(stderr, "error while parsing options\n");
				exit(EXIT_FAILURE);
		}
	}

	int8_t status;
	uloop_init();

init_config:
	status = config_init_all();
	if (status != FC_SUCCESS) {
		fprintf(stderr, "configuration initialization failed\n");
		return EXIT_FAILURE;
	}

	/* wait for interface to get ip */
	if (local_get_wait_source() && !local_get_ip()) {
#ifdef DEVEL_DEBUG
		sleep(3);
#else
		sleep(15);
#endif
	} else {
		goto init_cwmp;
	}

	status = config_reload();
	if (status != FC_SUCCESS) {
		fprintf(stderr, "configuration reload failed\n");
		return EXIT_FAILURE;
	}

	goto init_config;

init_cwmp:
	status = cwmp_init();
	if (status != FC_SUCCESS) {
		fprintf(stderr, "freecwmp initialization failed\n");
		return EXIT_FAILURE;
	}


	pid_t pid, sid;
	if (!foreground) {
		pid = fork();
		if (pid < 0)
			return EXIT_FAILURE;
		if (pid > 0)
			return EXIT_SUCCESS;

		sid = setsid();
		if (sid < 0) {
			fprintf(stderr, "setsid() function returned error\n");
			return EXIT_FAILURE;
		}

#ifdef DUMMY_MODE
		char *directory = ".";
#else
		char *directory = "/";
#endif
		if ((chdir(directory)) < 0) {
			fprintf(stderr, "chdir() function returned error\n");
			return EXIT_FAILURE;
		}

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

	cwmp_inform();
	uloop_run();
}

