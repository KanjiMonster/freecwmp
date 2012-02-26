/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freecwmp@lukaperkov.net>
 *
 *	This code is taken from zstream.
 *
 *	zstream - Micro URL I/O stream library
 *	Copyright (C) 2010 Steven Barth <steven@midlink.org>
 *	Copyright (C) 2010 John Crispin <blogic@openwrt.org>
 *
 */

#ifndef _FREECWMP_B64_H__
#define _FREECWMP_B64_H__

#ifndef HTTP_ZSTREAM
char* zstream_b64encode(const void *in, size_t *len);
void* zstream_b64decode(const char *in, size_t *len);
#endif /* HTTP_ZSTREAM */

#endif

