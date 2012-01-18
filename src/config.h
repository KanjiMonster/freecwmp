/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_CONFIG_H__
#define _FREECWMP_CONFIG_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <uci.h>

#include "freecwmp.h"

static struct uci_package *config_init_package(const char *config);

int8_t config_init_local(void);
int8_t config_init_acs(void);
int8_t config_refresh_acs(void);
int8_t config_init_device(void);
int8_t config_refresh_device(void);
int8_t config_reload(void);
int8_t config_init_all(void);

#endif

