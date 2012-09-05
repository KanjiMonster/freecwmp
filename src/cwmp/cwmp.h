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

struct notification {
	struct list_head list;

	char *parameter;
	char *value;
};

static void cwmp_periodic_inform(struct uloop_timeout *timeout);

void cwmp_init(void);
void cwmp_exit(void);
int cwmp_inform(void);
int cwmp_handle_messages(void);
void cwmp_connection_request(int code);
void cwmp_reload(void);
void cwmp_add_notification(char *parameter, char *value);
struct list_head * cwmp_get_notifications();
int cwmp_set_notification_write_handler(char *name, char *value);
int cwmp_set_parameter_write_handler(char *name, char *value);
int cwmp_set_action_execute_handler();
int cwmp_get_parameter_handler(char *name, char **value);
int cwmp_get_notification_handler(char *name, char **value);
void cwmp_clear_notifications(void);
int cwmp_download_handler(char *url, char *size);
int8_t cwmp_reboot_handler(void);
int8_t cwmp_factory_reset_handler(void);
char * cwmp_get_event_code(void);
int cwmp_get_retry_count(void);

#endif

