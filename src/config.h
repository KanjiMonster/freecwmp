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

void config_load(void);

struct acs {
	char *scheme;
	char *username;
	char *password;
	char *hostname;
	char *port;
	char *path;
#ifdef HTTP_CURL
	char *ssl_cert;
	char *ssl_cacert;
	bool ssl_verify;
#endif /* HTTP_CURL */
};

struct device {
	char *manufacturer;
	char *oui;
	char *product_class;
	char *serial_number;
	char *hardware_version;
	char *software_version;
	char *provisioning_code;
};

struct local {
	char *ip;
	char *interface;
	char *port;
	char *ubus_socket;
	int event;
};

struct core_config {
	struct acs *acs;
	struct device *device;
	struct local *local;
};

extern struct core_config *config;

#endif

