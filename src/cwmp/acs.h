/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_ACS_H__

#include <stdint.h>

struct acs
{
	char *scheme;
	char *username;
	char *password;
	char *hostname;
	uint16_t port;
	char *path;
};

void acs_init();
void acs_clean();
char * acs_get_scheme(void);
void acs_set_scheme(char *c);
char * acs_get_username(void);
void acs_set_username(char *c);
char * acs_get_password(void);
void acs_set_password(char *c);
char * acs_get_hostname(void);
uint16_t acs_get_port(void);
void acs_set_port(char *c);
char * acs_get_path(void);
void acs_set_path(char *c);

#endif

