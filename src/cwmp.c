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

pthread_mutex_t event_lock;
pthread_mutex_t notification_lock;

static void cwmp_periodic_inform(struct uloop_timeout *timeout)
{
	if (cwmp->periodic_inform_enabled && cwmp->periodic_inform_interval) {
		uloop_timeout_set(&periodic_inform_timer, cwmp->periodic_inform_interval * 1000);
		cwmp_add_event(PERIODIC, NULL);
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

	config_get_cwmp("InternetGatewayDevice.ManagementServer.PeriodicInformInterval", &c);
	if (c) {
		cwmp->periodic_inform_interval = atoi(c);
		uloop_timeout_set(&periodic_inform_timer, cwmp->periodic_inform_interval * 1000);
		free(c);
		c = NULL;
	}

	config_get_cwmp("InternetGatewayDevice.ManagementServer.PeriodicInformEnable", &c);
	if (c) {
		cwmp->periodic_inform_enabled = atoi(c);
		free(c);
		c = NULL;
	}

	pthread_mutex_init(&event_lock, NULL);
	pthread_mutex_init(&notification_lock, NULL);

	http_server_init();
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
	cwmp_clear_events();
	cwmp_add_event(code, NULL);
	uloop_timeout_set(&inform_timer, 500);
}

void cwmp_add_event(int code, char *key)
{
	struct event *e = NULL;
	struct list_head *l, *p;
	bool uniq = true;

	pthread_mutex_lock(&event_lock);

	list_for_each(p, &cwmp->events) {
		e = list_entry(p, struct event, list);
		if (e->code == code) {
			uniq = false;
			break;
		}
	}

	pthread_mutex_unlock(&event_lock);
				
	if (uniq) {
		int len = key ? strlen(key) : 0;
		e = calloc(1, sizeof(*e) + sizeof(int) + sizeof(char *) + len);
		if (!e) return;

		pthread_mutex_lock(&event_lock);

		list_add_tail(&e->list, &cwmp->events);
		e->code = code;
		e->key = key ? strdup(key) : NULL;

		pthread_mutex_unlock(&event_lock);
	}
}

void cwmp_clear_events(void)
{
	struct event *n, *p;

	pthread_mutex_lock(&event_lock);

	list_for_each_entry_safe(n, p, &cwmp->events, list) {
		FREE(n->key);
		list_del(&n->list);
		FREE(n);
	}

	pthread_mutex_unlock(&event_lock);
}

void cwmp_add_notification(char *parameter, char *value)
{
	char *c = NULL;
	external_get_action("notification", parameter, &c);
	if (!c) return;

	struct notification *n = NULL;
	struct list_head *l, *p;
	bool uniq = true;

	pthread_mutex_lock(&notification_lock);

	list_for_each(p, &cwmp->notifications) {
		n = list_entry(p, struct notification, list);
		if (!strcmp(n->parameter, parameter)) {
			free(n->value);
			n->value = strdup(value);
			uniq = false;
			break;
		}
	}

	pthread_mutex_unlock(&notification_lock);
				
	if (uniq) {
		n = calloc(1, sizeof(*n) + sizeof(char *) + strlen(parameter) + strlen(value));
		if (!n) return;

		pthread_mutex_lock(&notification_lock);

		list_add_tail(&n->list, &cwmp->notifications);
		n->parameter = strdup(parameter);
		n->value = strdup(value);

		pthread_mutex_unlock(&notification_lock);
	}


	cwmp_add_event(VALUE_CHANGE, NULL);
	if (!strncmp(c, "2", 1)) {
		cwmp_inform();
	}
}

void cwmp_clear_notifications(void)
{
	struct notification *n, *p;

	pthread_mutex_lock(&notification_lock);

	list_for_each_entry_safe(n, p, &cwmp->notifications, list) {
		FREE(n->parameter);
		FREE(n->value);
		list_del(&n->list);
		FREE(n);
	}

	pthread_mutex_unlock(&notification_lock);
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

