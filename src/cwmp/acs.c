/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>

#include "acs.h"
#include "../config.h"

struct acs acs;

void
acs_init()
{
	FC_DEVEL_DEBUG("enter");

	acs.scheme = NULL;
	acs.username = NULL;
	acs.password = NULL;
	acs.password = NULL;
	acs.hostname = NULL;
	acs.port = 0;
	acs.path = NULL;

	FC_DEVEL_DEBUG("exit");
}

void
acs_clean()
{
	FC_DEVEL_DEBUG("enter");

	if (acs.scheme) free(acs.scheme);
	acs.scheme = NULL;
	if (acs.username) free(acs.username);
	acs.username = NULL;
	if (acs.password) free(acs.password);
	acs.password = NULL;
	if (acs.password) free(acs.password);
	acs.password = NULL;
	if (acs.hostname) free(acs.hostname);
	acs.hostname = NULL;
	acs.port = 0;
	if (acs.path) free(acs.path);
	acs.path = NULL;

	FC_DEVEL_DEBUG("exit");
}

char *
acs_get_scheme(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return acs.scheme;
}

void
acs_set_scheme(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (acs.scheme)
		free(acs.scheme);
	acs.scheme = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
acs_get_username(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return acs.username;
}

void
acs_set_username(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (acs.username)
		free(acs.username);
	acs.username = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
acs_get_password(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return acs.password;
}

void
acs_set_password(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (acs.password)
		free(acs.password);
	acs.password = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
acs_get_hostname(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return acs.hostname;
}

void
acs_set_hostname(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (acs.hostname)
		free(acs.hostname);
	acs.hostname = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

uint16_t
acs_get_port(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return acs.port;
}

void
acs_set_port(char *c)
{
	FC_DEVEL_DEBUG("enter");

	acs.port = atoi(c);

	FC_DEVEL_DEBUG("exit");
}

char *
acs_get_path(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return acs.path;
}

void
acs_set_path(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (acs.path)
		free(acs.path);
	acs.path = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

