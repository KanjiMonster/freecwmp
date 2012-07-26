/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

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
#include "../cwmp/cwmp.h"
#include "../cwmp/local.h"
#include "../cwmp/acs.h"

static struct http_client http_c;
static struct http_server http_s;

int8_t
http_client_init(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;
	uint8_t len;
	char *scheme, *username, *password, *hostname, *path;
	uint16_t port;
	scheme = acs_get_scheme();
	username = acs_get_username();
	password = acs_get_password();
	hostname = acs_get_hostname();
	port = acs_get_port();
	path = acs_get_path();

	len = snprintf(NULL, 0, "%s://%s:%s@%s:%d%s",
			scheme,
			username,
			password,
			hostname,
			port,
			path);

	http_c.url = (char *) calloc((len + 1), sizeof(char));
	if (errno == ENOMEM)
		goto error;
	snprintf(http_c.url, (len + 1), "%s://%s:%s@%s:%d%s\0",
			scheme,
			username,
			password,
			hostname,
			port,
			path);

#ifdef DEBUG
	printf("+++ HTTP CLIENT CONFIGURATION +++\n");
	printf("URL: '%s'\n", http_c.url);
# ifdef HTTP_CURL
	if (acs_get_ssl_cert())
		printf("ssl_cert: '%s\n", acs_get_ssl_cert());
	if (acs_get_ssl_cacert())
		printf("ssl_cacert: '%s\n", acs_get_ssl_cacert());
	if (!acs_get_ssl_verify())
		printf("ssl_verify: SSL certificate validation disabled.\n");
# endif /* HTTP_CURL */
	printf("--- HTTP CLIENT CONFIGURATION ---\n");
#endif

#ifdef HTTP_CURL
	http_c.header_list = NULL;
	http_c.header_list = curl_slist_append(http_c.header_list, "User-Agent: freecwmp");
	if (!http_c.header_list)
		goto error;
	http_c.header_list = curl_slist_append(http_c.header_list, "Content-Type: text/xml");
	if (!http_c.header_list)
		goto error;
#endif /* HTTP_CURL */

#ifdef HTTP_ZSTREAM
	http_c.stream = zstream_open(http_c.url, ZSTREAM_POST);
	if (!http_c.stream)
		goto error;

# ifdef ACS_HDM
	status = zstream_http_configure(http_c.stream, ZSTREAM_HTTP_COOKIES, 1);
# elif ACS_MULTI
	status = zstream_http_configure(http_c.stream, ZSTREAM_HTTP_COOKIES, 3);
# endif
	if (status)
		goto error;

	status = zstream_http_addheader(http_c.stream, "User-Agent", "freecwmp");
	if (status)
		goto error;

	status = zstream_http_addheader(http_c.stream, "Content-Type", "text/xml");
	if (status)
		goto error;
#endif /* HTTP_ZSTREAM */

	status = FC_SUCCESS;
	goto done;

error:
#ifdef DEBUG
	if (errno == ENOMEM) {
		perror(__func__);
	}
#endif
	status = FC_ERROR;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
http_client_exit(void)
{
	FC_DEVEL_DEBUG("enter");

	if (http_c.url) {
		free(http_c.url);
		http_c.url = NULL;
	}

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

done:
	FC_DEVEL_DEBUG("exit");
	return FC_SUCCESS;
}

#ifdef HTTP_CURL
static uint64_t
http_get_response(void *buffer, size_t size, size_t rxed, char **msg_in)
{
	FC_DEVEL_DEBUG("enter");

	*msg_in = (char *) realloc(*msg_in, (strlen(*msg_in) + size * rxed + 1) * sizeof(char));
	bzero(*msg_in + strlen(*msg_in), rxed + 1);
	memcpy(*msg_in + strlen(*msg_in), buffer, rxed);
# ifdef DEVEL_DEBUG
	printf("+++ RECEIVED HTTP RESPONSE (PART) +++\n");
	printf("%.*s", rxed, buffer);
	printf("--- RECEIVED HTTP RESPONSE (PART) ---\n");
# endif

	FC_DEVEL_DEBUG("exit");
	return size * rxed;
}
#endif

int8_t
http_send_message(char *msg_out, char **msg_in)
{
	FC_DEVEL_DEBUG("enter");

	int status;

#ifdef HTTP_CURL
	CURLcode res;
	CURL *curl;

	curl = curl_easy_init();
	if (!curl)
		goto error;

	curl_easy_setopt(curl, CURLOPT_URL, http_c.url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_c.header_list);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg_out);
	if (msg_out)
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(msg_out));
	else
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_get_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, msg_in);

# ifdef DEVEL_DEBUG
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
# endif

	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, fc_cookies);
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, fc_cookies);

	/* TODO: test this with real ACS configuration */
	if (acs_get_ssl_cert())
		curl_easy_setopt(curl, CURLOPT_SSLCERT, acs_get_ssl_cert());
	if (acs_get_ssl_cacert())
		curl_easy_setopt(curl, CURLOPT_CAINFO, acs_get_ssl_cacert());
	if (!acs_get_ssl_verify())
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

	*msg_in = (char *) calloc (1, sizeof(char));

	res = curl_easy_perform(curl);

	if (!strlen(*msg_in)) {
		free(*msg_in);
		*msg_in = NULL;
	}

	curl_easy_cleanup(curl);
	
	if (res)
		goto error;

#endif /* HTTP_CURL */

#ifdef HTTP_ZSTREAM
	char buffer[BUFSIZ];
	ssize_t rxed;

	status = zstream_reopen(http_c.stream, http_c.url, ZSTREAM_POST);
	if (status) {
		/* something not good, let's try recreate */
		if ((http_client_exit()) != FC_SUCCESS)
			goto error;
		if ((http_client_init()) != FC_SUCCESS)
			goto error;
	}

# ifdef DEVEL_DEBUG
	if(msg_out) {
		printf("+++ SENDING POST MESSAGE +++\n");
		printf("%s", msg_out);
		printf("--- SENDING POST MESSAGE ---\n");
	} else {
		printf("+++ SENDING EMPTY POST MESSAGE +++\n");
	}
# endif

	if (msg_out) {
		zstream_write(http_c.stream, msg_out, strlen(msg_out));
	} else {
		zstream_write(http_c.stream, NULL , 0);
	}

	*msg_in = (char *) calloc (1, sizeof(char));
	while ((rxed = zstream_read(http_c.stream, buffer, sizeof(buffer))) > 0) {
		*msg_in = (char *) realloc(*msg_in, (strlen(*msg_in) + rxed + 1) * sizeof(char));
		if (!(*msg_in))
			goto error;
		bzero(*msg_in + strlen(*msg_in), rxed + 1);
		memcpy(*msg_in + strlen(*msg_in), buffer, rxed);
	}

	/* we got no response, that is ok and defined in documentation */
	if (!strlen(*msg_in)) {
		free(*msg_in);
		*msg_in = NULL;
	}

	if (rxed < 0)
		goto error;

#endif /* HTTP_ZSTREAM */

#ifdef DEVEL_DEBUG
	if (*msg_in) {
		printf("+++ RECEIVED HTTP RESPONSE +++\n");
		printf("%s", *msg_in);
		printf("--- RECEIVED HTTP RESPONSE ---\n");
 	} else {
		printf("+++ RECEIVED EMPTY HTTP RESPONSE +++\n");
	}
#endif
	status = FC_SUCCESS;
	goto done;

error:
	status = FC_ERROR;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
http_server_init(void)
{
	FC_DEVEL_DEBUG("enter");

	char *ip, port[6];
	int status;

	ip = local_get_ip();
	snprintf(port, 6, "%d\0", local_get_port());

	http_s.http_event.cb = http_new_client;

	http_s.http_event.fd = usock(USOCK_TCP | USOCK_SERVER, ip, port);
	uloop_fd_add(&http_s.http_event, ULOOP_READ | ULOOP_EDGE_TRIGGER);

#ifdef DEVEL_DEBUG
	printf("+++ HTTP SERVER CONFIGURATION +++\n");
	if (ip)
		printf("IP: '%s'\n", ip);
	else
		printf("NOT BOUND TO IP\n");
	printf("PORT: '%s'\n", port);
	printf("--- HTTP SERVER CONFIGURATION ---\n");
#endif

	status = FC_SUCCESS;

	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
http_server_exit(void)
{
	FC_DEVEL_DEBUG("enter");

	int status;

	status = FC_SUCCESS;

	FC_DEVEL_DEBUG("exit");
	return status;
}

static void
http_new_client(struct uloop_fd *ufd, unsigned events)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;
	struct timeval t;

	t.tv_sec = 60;
	t.tv_usec = 0;

	for (;;) {
		int client = accept(ufd->fd, NULL, NULL);

		/* set one minute timeout */
		if (setsockopt (ufd->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof t)) {
#ifdef DEBUG
			perror(__func__);
#endif
		}

		if (client == -1)
			break;

		struct uloop_process *uproc = calloc(1, sizeof(*uproc));
		if (!uproc || (uproc->pid = fork()) == -1) {
			free(uproc);
			close(client);
		}

		if (uproc->pid != 0) {
			/* parrent */
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

#ifdef DEBUG
			printf("+++ RECEIVED HTTP REQUEST +++\n");
#endif
			while (fgets(buffer, sizeof(buffer), fp)) {
#ifdef DEBUG
				fwrite(buffer, 1, strlen(buffer), stdout);
#endif

				if (!strncasecmp(buffer, "Authorization: Basic ", strlen("Authorization: Basic "))) {
					const char *c1, *c2, *min, *val;
					char *username, *password;
					char *acs_auth_basic = NULL;
					char *auth_basic_check = NULL;
					uint16_t len;

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
						if (username) free(username);
						if (password) free(password);
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
					if (username) free(username);
					if (password) free(password);
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
				status = FC_SUCCESS;
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
#ifdef DEBUG
			printf("--- RECEIVED HTTP REQUEST ---\n");
#endif
			FC_DEVEL_DEBUG("exit");
			exit(status);
		}
	}
}

static void
http_del_client(struct uloop_process *uproc, int ret)
{
	FC_DEVEL_DEBUG("enter");

	free(uproc);

	/* child terminated ; check return code */
	if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
#ifdef DEBUG
		printf("+++ HTTP SERVER CONNECTION SUCCESS +++\n");
#endif
		cwmp_connection_request();
	} else {
#ifdef DEBUG
		printf("+++ HTTP SERVER CONNECTION FAILED +++\n");
#endif
#ifdef DEVEL_DEBUG
		perror(__func__);
#endif
	}

	FC_DEVEL_DEBUG("exit");
}

