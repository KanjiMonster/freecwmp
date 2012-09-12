/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_FREECWMP_H__
#define _FREECWMP_FREECWMP_H__

#define NAME	"freecwmpd"

#define FREE(x) if (!x) { free(x) ; x = NULL; }

#ifdef DEBUG
#define D(format, ...) fprintf(stderr, "%s(%d): " format, __func__, __LINE__, ## __VA_ARGS__)
#else
#define D(format, ...) no_debug(0, format, ## __VA_ARGS__)
#endif

#ifdef DEVEL
#define DD(format, ...) fprintf(stderr, "%s(%d):: " format, __func__, __LINE__, ## __VA_ARGS__)
#define DDF(format, ...) fprintf(stderr, format, ## __VA_ARGS__)
#else
#define DD(format, ...) no_debug(0, format, ## __VA_ARGS__)
#define DDF(format, ...) no_debug(0, format, ## __VA_ARGS__)
#endif

static inline void no_debug(int level, const char *fmt, ...)
{
}

void freecwmp_reload(void);

#endif

