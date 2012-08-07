/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>
#include <string.h>
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
	local.interface = NULL;
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
	if (local.interface) free(local.interface);
	local.interface = NULL;
	local.port = 0;
	local.event = 0;

	FC_DEVEL_DEBUG("exit");
}

char *
local_get_interface(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return local.interface;
}

void
local_set_interface(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (local.interface) free(local.interface);
	local.interface = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
local_get_ip(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return local.ip;
}

void
local_set_ip(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (local.ip) free(local.ip);
	local.ip = strdup(c);

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

char *
local_get_ubus_socket(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return local.ubus_socket;
}

void
local_set_ubus_socket(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (local.ubus_socket) free(local.ubus_socket);
	local.ubus_socket = strdup(c);

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
