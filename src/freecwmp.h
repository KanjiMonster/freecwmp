/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_FREECWMP_H__
#define _FREECWMP_FREECWMP_H__

#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FREECWMP_NAME "freecwmpd"
#define FREECWMP_VERSION "0.1"

#define FC_SUCCESS 0
#define FC_ERROR 1

#ifdef DEVEL_DEBUG
#	define FC_DEVEL_DEBUG(msg) \
		printf("__[ %s | %s ]__ : %s\n", __FILE__, __func__, msg);
#else
#	define FC_DEVEL_DEBUG(msg)
#endif


#endif

