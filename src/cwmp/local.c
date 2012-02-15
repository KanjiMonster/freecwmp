/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "local.h"
#include "cwmp.h"
#include "../config.h"

static struct local local;

void
local_init()
{
	FC_DEVEL_DEBUG("enter");

	local.ip = NULL;
	local.wait_source = 0;
	local.port = 0;
	local.event = 0;

	FC_DEVEL_DEBUG("exit");
}

void
local_clean()
{
	FC_DEVEL_DEBUG("enter");

	if (local.ip) free(local.ip);
	local.ip = NULL;
	local.wait_source = 0;
	local.port = 0;
	local.event = 0;

	FC_DEVEL_DEBUG("exit");
}

void
local_set_source(char *c)
{
	FC_DEVEL_DEBUG("enter");

	struct sockaddr_in sa;

	if (local.ip) {
		free(local.ip);
		local.ip = NULL;
	}

	if (inet_pton(AF_INET, c, &(sa.sin_addr))) {
		local.ip = strdup(c);
		goto done;
	}

	/* find ip out of interface name */
	struct ifreq ifr;
	int fd, status;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, c, IFNAMSIZ - 1);
	status = ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

	if (!status)
		local.ip = strdup(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

done:
	FC_DEVEL_DEBUG("exit");
}

char *
local_get_ip(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return local.ip;
}

uint8_t
local_get_wait_source(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return local.wait_source;
}

void
local_set_wait_source(char *c)
{
	FC_DEVEL_DEBUG("enter");

	local.wait_source = atoi(c);

	FC_DEVEL_DEBUG("exit");
}

uint16_t
local_get_port(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return local.port;
}

void
local_set_port(char *c)
{
	FC_DEVEL_DEBUG("enter");

	local.port = atoi(c);

	FC_DEVEL_DEBUG("exit");
}

uint8_t
local_get_event(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return local.event;
}

void
local_set_event(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (strcasecmp("bootstrap", c) == 0)
		local.event = BOOTSTRAP;

	if (strcasecmp("boot", c) == 0)
		local.event = BOOT;

	FC_DEVEL_DEBUG("exit");
}
