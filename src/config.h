/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_CONFIG_H__
#define _FREECWMP_CONFIG_H__

#include <uci.h>

#include "freecwmp.h"

int config_init_local(void);
int config_init_acs(void);
int config_refresh_acs(void);
int config_init_device(void);
int config_refresh_device(void);
int config_reload(void);
int config_init_all(void);

struct local {
	char *ip;
	char *interface;
	char *port;
	char *ubus_socket;
	int event;
};

struct core_config {
	struct local *local;
};

extern struct core_config *config;

#endif

