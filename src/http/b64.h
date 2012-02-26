/*
 *	zstream - Micro URL I/O stream library
 *	Copyright (C) 2010 Steven Barth <steven@midlink.org>
 *	Copyright (C) 2010 John Crispin <blogic@openwrt.org>
 *
 *	This library is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU Lesser General Public License as published
 *	by the Free Software Foundation; either version 2.1 of the License,
 *	or (at your option) any later version.
 *
 *	This library is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *	See the GNU Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public License
 *	along with this library; if not, write to the Free Software Foundation,
 *	Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
 *
 */

#ifndef _FREECWMP_B64_H__
#define _FREECWMP_B64_H__

#ifndef HTTP_ZSTREAM
char* zstream_b64encode(const void *in, size_t *len);
void* zstream_b64decode(const char *in, size_t *len);
#endif /* HTTP_ZSTREAM */

#endif

