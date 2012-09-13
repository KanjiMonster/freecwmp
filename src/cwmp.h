/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_CWMP_H__
#define _FREECWMP_CWMP_H__

#include <libubox/uloop.h>

struct event {
	struct list_head list;

	int code;
	char *key;
};

struct notification {
	struct list_head list;

	char *parameter;
	char *value;
};


struct cwmp_internal {
	int periodic_inform_enabled;
	uint64_t periodic_inform_interval;
	int retry_count;
	struct list_head events;
	struct list_head notifications;
};

extern struct cwmp_internal *cwmp;
extern pthread_mutex_t event_lock;
extern pthread_mutex_t notification_lock;

static void cwmp_periodic_inform(struct uloop_timeout *timeout);
static void cwmp_do_inform(struct uloop_timeout *timeout);

void cwmp_init(void);
void cwmp_exit(void);

int cwmp_inform(void);
int cwmp_handle_messages(void);
void cwmp_connection_request(int code);

void cwmp_add_event(int code, char *key);
void cwmp_clear_events(void);

void cwmp_add_notification(char *parameter, char *value);
void cwmp_clear_notifications(void);

int cwmp_set_parameter_write_handler(char *name, char *value);

#endif

