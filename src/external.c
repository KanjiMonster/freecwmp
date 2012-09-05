/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <libubox/uloop.h>

#include "external.h"

#include "freecwmp.h"

static struct uloop_process uproc;

int external_get_action(char *action, char *name, char **value)
{
	int pfds[2];
	if (pipe(pfds) < 0)
		return -1;

	if ((uproc.pid = fork()) == -1)
		return -1;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--newline";
		argv[i++] = "--value";
		argv[i++] = "get";
		argv[i++] = action;
		argv[i++] = name;
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */
	close(pfds[1]);

	int status;
	while (wait(&status) != uproc.pid) {
		DD("waiting for child to exit");
	}

	char buffer[2];
	ssize_t rxed;

	*value = (char *) calloc(1, sizeof(char));
	while ((rxed = read(pfds[0], buffer, sizeof(buffer))) > 0) {
		*value = (char *) realloc(*value, (strlen(*value) + rxed + 1) * sizeof(char));
		if (!(*value)) return -1;
		bzero(*value + strlen(*value), rxed + 1);
		memcpy(*value + strlen(*value), buffer, rxed);
	}

	if (!strlen(*value)) {
		FREE(*value);
		return 0;
	}

	if (rxed < 0)
		return -1;
	
	return 0;
}

int external_set_action_write(char *action, char *name, char *value)
{
	FILE *fp;

	if (access(fc_script_set_actions, R_OK | W_OK | X_OK) != -1) {
		fp = fopen(fc_script_set_actions, "a");
		if (!fp) return -1;
	} else {
		fp = fopen(fc_script_set_actions, "w");
		if (!fp) return -1;

		fprintf(fp, "#!/bin/sh\n");

		if (chmod(fc_script_set_actions,
			strtol("0700", 0, 8)) < 0) {
			return -1;
		}
	}

#ifdef DUMMY_MODE
	fprintf(fp, "/bin/sh `pwd`/%s set %s %s %s\n", fc_script, action, name, value);
#else
	fprintf(fp, "/bin/sh %s set %s %s %s\n", fc_script, action, name, value);
#endif

	fclose(fp);

	return 0;
}

int external_set_action_execute()
{
	if ((uproc.pid = fork()) == -1) {
		return -1;
	}

	if (uproc.pid == 0) {
		/* child */

		const char *argv[3];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script_set_actions;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */

	int status;
	while (wait(&status) != uproc.pid) {
		DD("waiting for child to exit");
	}

	// TODO: add some kind of checks

	if (remove(fc_script_set_actions) != 0)
		return -1;

	return 0;
}

int external_simple(char *arg)
{
	if ((uproc.pid = fork()) == -1)
		return -1;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[4];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = arg;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */

	int status;
	while (wait(&status) != uproc.pid) {
		DD("waiting for child to exit");
	}

	// TODO: add some kind of checks

	return 0;
}

int external_download(char *url, char *size)
{
	if ((uproc.pid = fork()) == -1)
		return -1;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "download";
		argv[i++] = "--url";
		argv[i++] = url;
		argv[i++] = "--size";
		argv[i++] = size;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */
	int status;
	while (wait(&status) != uproc.pid) {
		DD("waiting for child to exit");
	}

	if (WIFEXITED(status) && !WEXITSTATUS(status))
		return 0;
	else
		return 1;

	return 0;
}

