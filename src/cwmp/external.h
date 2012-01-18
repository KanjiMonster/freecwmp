/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_EXTERNAL_H__
#define _FREECWMP_EXTERNAL_H__

#ifdef DUMMY_MODE
static char *fc_script = "./ext/openwrt/scripts/freecwmp.sh";
#else
static char *fc_script = "/usr/sbin/freecwmp";
#endif

int8_t external_get_parameter(char *name, char **value);
int8_t external_set_parameter(char *name, char *value);
int8_t external_simple(char *arg);
int8_t external_download(char *url, char *size);

#endif

