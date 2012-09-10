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

#include <libfreecwmp.h>
#include <libubox/uloop.h>

#include "cwmp.h"

#include "config.h"
#include "external.h"
#include "freecwmp.h"
#include "http.h"
#include "xml.h"

struct cwmp_internal *cwmp;

static struct uloop_timeout inform_timer = { .cb = cwmp_do_inform };
static struct uloop_timeout periodic_inform_timer = { .cb = cwmp_periodic_inform };

static void cwmp_periodic_inform(struct uloop_timeout *timeout)
{
	if (cwmp->periodic_inform_enabled && cwmp->periodic_inform_interval) {
		uloop_timeout_set(&periodic_inform_timer, cwmp->periodic_inform_interval * 1000);
		cwmp->event_code = PERIODIC;
	}

	if (cwmp->periodic_inform_enabled)
		cwmp_inform();
}

static void cwmp_do_inform(struct uloop_timeout *timeout)
{
	cwmp_inform();
}

void cwmp_init(void)
{
	char *c = NULL;

	cwmp = calloc(1, sizeof(struct cwmp_internal));
	if (!cwmp) return;

	cwmp->event_code = config->local->event;

	external_get_action("value", "InternetGatewayDevice.ManagementServer.PeriodicInformInterval", &c);
	if (c) {
		cwmp->periodic_inform_interval = atoi(c);
		uloop_timeout_set(&periodic_inform_timer, cwmp->periodic_inform_interval * 1000);
	}
	FREE(c);

	external_get_action("value", "InternetGatewayDevice.ManagementServer.PeriodicInformEnable", &c);
	if (c) {
		cwmp->periodic_inform_enabled = atoi(c);
	}
	FREE(c);

	http_server_init();

	INIT_LIST_HEAD(&cwmp->notifications);
}

void cwmp_exit(void)
{
	FREE(cwmp);
	http_client_exit();
	xml_exit();
}

int cwmp_inform(void)
{
	char *msg_in, *msg_out;
	msg_in = msg_out = NULL;

	if (http_client_init()) {
		D("initializing http client failed\n");
		goto error;
	}

	if (xml_prepare_inform_message(&msg_out)) {
		D("xml message creating failed\n");
		goto error;
	}

	if (http_send_message(msg_out, &msg_in)) {
		D("sending http message failed\n");
		goto error;
	}
	
	if (msg_in && xml_parse_inform_response_message(msg_in)) {
		D("parse xml message from ACS failed\n");
		goto error;
	}

	FREE(msg_in);
	FREE(msg_out);

	cwmp->retry_count = 0;

	if (cwmp_handle_messages()) {
		D("handling xml message failed\n");
		goto error;
	}

	cwmp_clear_notifications();
	http_client_exit();
	xml_exit();

	return 0;

error:
	FREE(msg_in);
	FREE(msg_out);

	http_client_exit();
	xml_exit();

	cwmp->retry_count++;
	if (cwmp->retry_count < 100) {
		uloop_timeout_set(&inform_timer, 10000 * cwmp->retry_count);
	} else {
		/* try every 20 minutes */
		uloop_timeout_set(&inform_timer, 1200000);
	}

	return -1;
}

int cwmp_handle_messages(void)
{
	int8_t status;
	char *msg_in, *msg_out;
	msg_in = msg_out = NULL;

	while (1) {
		FREE(msg_in);

		if (http_send_message(msg_out, &msg_in)) {
			D("sending http message failed\n");
			goto error;
		}

		if (!msg_in)
			break;

		FREE(msg_out);

		if (xml_handle_message(msg_in, &msg_out)) {
			D("xml handling message failed\n");
			goto error;
		}

		if (!msg_out) {
			D("acs response message is empty\n");
			goto error;
		}
	}

	FREE(msg_in);
	FREE(msg_out);

	return 0;

error:
	FREE(msg_in);
	FREE(msg_out);

	return -1;
}

void cwmp_connection_request(int code)
{
	cwmp->event_code = code;
	uloop_timeout_set(&inform_timer, 500);
}

void cwmp_add_notification(char *parameter, char *value)
{
	char *c = NULL;
	external_get_action("notification", parameter, &c);
	if (!c) return;

	bool uniq = true;
	struct notification *n = NULL;
	struct list_head *l, *p;
	list_for_each(p, &cwmp->notifications) {
		n = list_entry(p, struct notification, list);
		if (!strcmp(n->parameter, parameter)) {
			free(n->value);
			n->value = strdup(value);
			uniq = false;
			break;
		}
	}
				
	if (uniq) {
		n = calloc(1, sizeof(*n) + sizeof(char *) + strlen(parameter) + strlen(value));
		if (!n) return;

		list_add_tail(&n->list, &cwmp->notifications);
		n->parameter = strdup(parameter);
		n->value = strdup(value);
	}

	cwmp->event_code = VALUE_CHANGE;
	if (!strncmp(c, "2", 1)) {
		cwmp_inform();
	}
}

struct list_head *
cwmp_get_notifications()
{
	return &cwmp->notifications;
}

void cwmp_clear_notifications(void)
{
	struct notification *n, *p;
	list_for_each_entry_safe(n, p, &cwmp->notifications, list) {
		free(n->parameter);
		free(n->value);
		list_del(&n->list);
		free(n);
	}
}

int cwmp_set_parameter_write_handler(char *name, char *value)
{
	if((strcmp(name, "InternetGatewayDevice.ManagementServer.PeriodicInformEnable")) == 0) {
		cwmp->periodic_inform_enabled = atoi(value);
	}

	if((strcmp(name, "InternetGatewayDevice.ManagementServer.PeriodicInformInterval")) == 0) {
		cwmp->periodic_inform_interval = atoi(value);
		uloop_timeout_set(&periodic_inform_timer, cwmp->periodic_inform_interval * 1000);
	}

	return external_set_action_write("value", name, value);
}

