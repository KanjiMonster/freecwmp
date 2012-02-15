/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_LOCAL_H__

#include <stdint.h>

struct local
{
	char *ip;
	uint8_t wait_source;
	uint16_t port;
	uint8_t event;
};

void local_init();
void local_clean();
char * local_get_ip(void);
void local_set_source(char *c);
uint8_t local_get_wait_source(void);
void local_set_wait_source(char *c);
uint16_t local_get_port(void);
void local_set_port(char *c);
uint8_t local_get_event(void);
void local_set_event(char *c);

#endif

