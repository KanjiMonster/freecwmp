/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libfreecwmp.h>
#include <libubox/uloop.h>
#include <libubox/usock.h>

#ifdef HTTP_CURL
#include <curl/curl.h>
#endif

#ifdef HTTP_ZSTREAM
#include <zstream.h>
#include <zstream/http.h>
#endif

#ifndef HTTP_ZSTREAM
#include "b64.h"
#endif

#include "http.h"

#include "../freecwmp.h"
#include "../config.h"
#include "../cwmp/cwmp.h"

static struct http_client http_c;
static struct http_server http_s;

int
http_client_init(void)
{
	int len;

	len = snprintf(NULL, 0, "%s://%s:%s@%s:%s%s",
		       config->acs->scheme,
		       config->acs->username,
		       config->acs->password,
		       config->acs->hostname,
		       config->acs->port,
		       config->acs->path);

	http_c.url = (char *) calloc((len + 1), sizeof(char));
	if (!http_c.url) return -1;
	snprintf(http_c.url, (len + 1), "%s://%s:%s@%s:%s%s\0",
		 config->acs->scheme,
		 config->acs->username,
		 config->acs->password,
		 config->acs->hostname,
		 config->acs->port,
		 config->acs->path);

	DDF("+++ HTTP CLIENT CONFIGURATION +++\n");
	DD("url: %s\n", http_c.url);
#ifdef HTTP_CURL
	if (config->acs->ssl_cert)
		DD("ssl_cert: %s\n", config->acs->ssl_cert);
	if (config->acs->ssl_cacert)
		DD("ssl_cacert: %s\n", config->acs->ssl_cacert);
	if (!config->acs->ssl_verify)
		DD("ssl_verify: SSL certificate validation disabled.\n");
#endif /* HTTP_CURL */
	DDF("--- HTTP CLIENT CONFIGURATION ---\n");

#ifdef HTTP_CURL
	http_c.header_list = NULL;
	http_c.header_list = curl_slist_append(http_c.header_list, "User-Agent: freecwmp");
	if (!http_c.header_list) return -1;
	http_c.header_list = curl_slist_append(http_c.header_list, "Content-Type: text/xml");
	if (!http_c.header_list) return -1;
#endif /* HTTP_CURL */

#ifdef HTTP_ZSTREAM
	http_c.stream = zstream_open(http_c.url, ZSTREAM_POST);
	if (!http_c.stream)
		return -1;

# ifdef ACS_HDM
	if (zstream_http_configure(http_c.stream, ZSTREAM_HTTP_COOKIES, 1))
# elif ACS_MULTI
	if (zstream_http_configure(http_c.stream, ZSTREAM_HTTP_COOKIES, 3))
# endif
		return -1;

	if (zstream_http_addheader(http_c.stream, "User-Agent", "freecwmp"))
		return -1;

	if (zstream_http_addheader(http_c.stream, "Content-Type", "text/xml"))
		return -1;
#endif /* HTTP_ZSTREAM */

	freecwmp_log_message(NAME, L_NOTICE, "configured acs url %s", http_c.url);
	return 0;
}

void
http_client_exit(void)
{
	FREE(http_c.url);

#ifdef HTTP_CURL
	if (!http_c.header_list)
		curl_slist_free_all(http_c.header_list);
	if (access(fc_cookies, W_OK) == 0)
		remove(fc_cookies);
#endif /* HTTP_CURL */

#ifdef HTTP_ZSTREAM
	if (http_c.stream) {
		zstream_close(http_c.stream);
		http_c.stream = NULL;
	}
#endif /* HTTP_ZSTREAM */
}

#ifdef HTTP_CURL
static size_t
http_get_response(void *buffer, size_t size, size_t rxed, char **msg_in)
{
	*msg_in = (char *) realloc(*msg_in, (strlen(*msg_in) + size * rxed + 1) * sizeof(char));
	bzero(*msg_in + strlen(*msg_in), rxed + 1);
	memcpy(*msg_in + strlen(*msg_in), buffer, rxed);

	DDF("+++ RECEIVED HTTP RESPONSE (PART) +++\n");
	DDF("%.*s", rxed, buffer);
	DDF("--- RECEIVED HTTP RESPONSE (PART) ---\n");

	return size * rxed;
}
#endif

int8_t
http_send_message(char *msg_out, char **msg_in)
{
#ifdef HTTP_CURL
	CURLcode res;
	CURL *curl;

	curl = curl_easy_init();
	if (!curl) return -1;

	curl_easy_setopt(curl, CURLOPT_URL, http_c.url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_c.header_list);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg_out);
	if (msg_out)
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(msg_out));
	else
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_get_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, msg_in);

# ifdef DEVEL
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
# endif

	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, fc_cookies);
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, fc_cookies);

	/* TODO: test this with real ACS configuration */
	if (config->acs->ssl_cert)
		curl_easy_setopt(curl, CURLOPT_SSLCERT, config->acs->ssl_cert);
	if (config->acs->ssl_cacert)
		curl_easy_setopt(curl, CURLOPT_CAINFO, config->acs->ssl_cacert);
	if (!config->acs->ssl_verify)
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

	*msg_in = (char *) calloc (1, sizeof(char));

	res = curl_easy_perform(curl);

	if (!strlen(*msg_in)) {
		free(*msg_in);
		*msg_in = NULL;
	}

	curl_easy_cleanup(curl);
	
	if (res) return -1;

#endif /* HTTP_CURL */

#ifdef HTTP_ZSTREAM
	char buffer[BUFSIZ];
	ssize_t rxed;

	if (zstream_reopen(http_c.stream, http_c.url, ZSTREAM_POST)) {
		/* something not good, let's try recreate */
		http_client_exit();
		if (http_client_init()) return -1;
	}

	if(msg_out) {
		DDF("+++ SENDING POST MESSAGE +++\n");
		DDF("%s", msg_out);
		DDF("--- SENDING POST MESSAGE ---\n");
	} else {
		DDF("+++ SENDING EMPTY POST MESSAGE +++\n");
	}

	if (msg_out) {
		zstream_write(http_c.stream, msg_out, strlen(msg_out));
	} else {
		zstream_write(http_c.stream, NULL , 0);
	}

	*msg_in = (char *) calloc (1, sizeof(char));
	while ((rxed = zstream_read(http_c.stream, buffer, sizeof(buffer))) > 0) {
		*msg_in = (char *) realloc(*msg_in, (strlen(*msg_in) + rxed + 1) * sizeof(char));
		if (!(*msg_in)) return -1;
		bzero(*msg_in + strlen(*msg_in), rxed + 1);
		memcpy(*msg_in + strlen(*msg_in), buffer, rxed);
	}

	/* we got no response, that is ok and defined in documentation */
	if (!strlen(*msg_in)) {
		free(*msg_in);
		*msg_in = NULL;
	}

	if (rxed < 0)
		return -1;

#endif /* HTTP_ZSTREAM */

	if (*msg_in) {
		DDF("+++ RECEIVED HTTP RESPONSE +++\n");
		DDF("%s", *msg_in);
		DDF("--- RECEIVED HTTP RESPONSE ---\n");
 	} else {
		DDF("+++ RECEIVED EMPTY HTTP RESPONSE +++\n");
	}

	return 0;
}

void
http_server_init(void)
{
	http_s.http_event.cb = http_new_client;

	http_s.http_event.fd = usock(USOCK_TCP | USOCK_SERVER, config->local->ip, config->local->port);
	uloop_fd_add(&http_s.http_event, ULOOP_READ | ULOOP_EDGE_TRIGGER);

	DDF("+++ HTTP SERVER CONFIGURATION +++\n");
	if (config->local->ip)
		DDF("ip: '%s'\n", config->local->ip);
	else
		DDF("NOT BOUND TO IP\n");
	DDF("port: '%s'\n", config->local->port);
	DDF("--- HTTP SERVER CONFIGURATION ---\n");

	freecwmp_log_message(NAME, L_NOTICE, "http server initialized");
}

static void
http_new_client(struct uloop_fd *ufd, unsigned events)
{
	int status;
	struct timeval t;

	t.tv_sec = 60;
	t.tv_usec = 0;

	for (;;) {
		int client = accept(ufd->fd, NULL, NULL);

		/* set one minute timeout */
		if (setsockopt(ufd->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof t)) {
			DD("setsockopt() failed\n");
		}

		if (client == -1)
			break;

		struct uloop_process *uproc = calloc(1, sizeof(*uproc));
		if (!uproc || (uproc->pid = fork()) == -1) {
			free(uproc);
			close(client);
		}

		if (uproc->pid != 0) {
			/* parent */
			/* register an event handler for when the child terminates */
			uproc->cb = http_del_client;
			uloop_process_add(uproc);
			close(client);
		} else {
			/* child */
			FILE *fp;
			char buffer[BUFSIZ];
			int8_t auth_status = 0;
			
			fp = fdopen(client, "r+");

			DDF("+++ RECEIVED HTTP REQUEST +++\n");
			while (fgets(buffer, sizeof(buffer), fp)) {
#ifdef DEVEL
				fwrite(buffer, 1, strlen(buffer), stderr);
#endif

				if (!strncasecmp(buffer, "Authorization: Basic ", strlen("Authorization: Basic "))) {
					const char *c1, *c2, *min, *val;
					char *username, *password;
					char *acs_auth_basic = NULL;
					char *auth_basic_check = NULL;
					int len;

					c1 = strrchr(buffer, '\r');
					c2 = strrchr(buffer, '\n');

					if (!c1)
						c1 = c2;
					if (!c2)
						c2 = c1;
					if (!c1 || !c2)
						continue;

					min = (c1 < c2) ? c1 : c2;
					
					val = strrchr(buffer, ' ');
					if (!val)
						continue;

					val += sizeof(char);
					ssize_t size = min - val;

					acs_auth_basic = (char *) zstream_b64decode(val, &size);
					if (!acs_auth_basic)
						continue;

					username = NULL;
					password = NULL;
					cwmp_get_parameter_handler("InternetGatewayDevice.ManagementServer.ConnectionRequestUsername", &username);
					cwmp_get_parameter_handler("InternetGatewayDevice.ManagementServer.ConnectionRequestPassword", &password);

					len = snprintf(NULL, 0, "%s:%s", username, password);
					auth_basic_check = (char *) calloc((len + 1), sizeof(char));
					if (errno == ENOMEM) {
						FREE(username);
						FREE(password);
						free(acs_auth_basic);
						goto error_child;
					}
					snprintf(auth_basic_check, (len + 1), "%s:%s\0", username, password);

					if (size == strlen(auth_basic_check)) {
						len = size;
					} else {
						auth_status = 0;
						goto free_resources;
					}

					if (!memcmp(acs_auth_basic, auth_basic_check, len * sizeof(char)))
						auth_status = 1;
					else
						auth_status = 0;

free_resources:
					FREE(username);
					FREE(password);
					free(acs_auth_basic);
					free(auth_basic_check);
				}

				if (buffer[0] == '\r' || buffer[0] == '\n') {
					/* end of http request (empty line) */
					goto http_end_child;
				}

			}
error_child:
			/* here we are because of an error, e.g. timeout */
			status = errno;
			goto done_child;

http_end_child:
			fflush(fp);
			if (auth_status) {
				status = 0;
				fputs("HTTP/1.1 204 No Content\r\n\r\n", fp);
			} else {
				status = EACCES;
				fputs("HTTP/1.1 401 Unauthorized\r\n", fp);
				fputs("Connection: close\r\n", fp);
				fputs("WWW-Authenticate: Basic realm=\"default\"\r\n", fp);
			}
			fputs("\r\n", fp);
			goto done_child;

done_child:
			fclose(fp);
			DDF("--- RECEIVED HTTP REQUEST ---\n");
			exit(status);
		}
	}
}

static void
http_del_client(struct uloop_process *uproc, int ret)
{
	free(uproc);

	/* child terminated ; check return code */
	if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
		DDF("+++ HTTP SERVER CONNECTION SUCCESS +++\n");
		freecwmp_log_message(NAME, L_NOTICE, "acs initiated connection");
		cwmp_connection_request();
	} else {
		DDF("+++ HTTP SERVER CONNECTION FAILED +++\n");
	}

	FC_DEVEL_DEBUG("exit");
}

