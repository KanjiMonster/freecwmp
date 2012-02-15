/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_HTTP_H__
#define _FREECWMP_HTTP_H__

#include <stdint.h>
#include <libubox/uloop.h>

#ifdef HTTP_CURL
#include <curl/curl.h>
#endif

#ifdef HTTP_ZSTREAM
#include <zstream.h>
#endif

#ifdef DUMMY_MODE
static char *fc_cookies = "./ext/tmp/freecwmp_cookies";
#else
static char *fc_cookies = "/tmp/freecwmp_cookies";
#endif

struct http_client
{
#ifdef HTTP_CURL
	struct curl_slist *header_list;
#endif
#ifdef HTTP_ZSTREAM
	zstream_t *stream;
#endif
	char *url;
};

struct http_server
{
	struct uloop_fd http_event;
};

#ifdef HTTP_CURL
static uint64_t http_get_response(void *buffer, size_t size, size_t rxed, char **msg_in);
#endif

int8_t http_client_init(void);
int8_t http_client_exit(void);
int8_t http_send_message(char *msg_out, char **msg_in);

int8_t http_server_init(void);
int8_t http_server_exit(void);
static void http_new_client(struct uloop_fd *ufd, unsigned events);
static void http_del_client(struct uloop_process *uproc, int ret);

#endif

