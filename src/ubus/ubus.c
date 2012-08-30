/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <unistd.h>
#include <libubus.h>
#include <libfreecwmp.h>

#include "../freecwmp.h"
#include "../config.h"
#include "../cwmp/cwmp.h"

static struct ubus_context *ctx = NULL;

static enum notify {
	NOTIFY_PARAM,
	NOTIFY_VALUE,
	__NOTIFY_MAX
};

static const struct blobmsg_policy notify_policy[] = {
	[NOTIFY_PARAM] = { .name = "parameter", .type = BLOBMSG_TYPE_STRING },
	[NOTIFY_VALUE] = { .name = "value", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_notify(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__NOTIFY_MAX];

	blobmsg_parse(notify_policy, ARRAY_SIZE(notify_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[NOTIFY_PARAM])
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (!tb[NOTIFY_VALUE])
		return UBUS_STATUS_INVALID_ARGUMENT;

	freecwmp_log_message(NAME, L_NOTICE,
			     "triggered ubus notification parameter %s\n",
			     blobmsg_data(tb[NOTIFY_PARAM]));
	cwmp_add_notification(blobmsg_data(tb[NOTIFY_PARAM]),
			      blobmsg_data(tb[NOTIFY_VALUE]));

	return 0;
}

static enum inform {
	INFORM_EVENT,
	__INFORM_MAX
};

static const struct blobmsg_policy inform_policy[] = {
	[INFORM_EVENT] = { .name = "event", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_inform(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	int tmp;
	struct blob_attr *tb[__INFORM_MAX];

	blobmsg_parse(inform_policy, ARRAY_SIZE(inform_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[INFORM_EVENT])
		return UBUS_STATUS_INVALID_ARGUMENT;

	freecwmp_log_message(NAME, L_NOTICE,
			     "triggered ubus inform %s\n",
			     blobmsg_data(tb[INFORM_EVENT]));
	tmp = freecwmp_int_event_code(blobmsg_data(tb[INFORM_EVENT]));
	cwmp_connection_request(tmp);

	return 0;
}

static int
freecwmpd_handle_reload(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	freecwmp_log_message(NAME, L_NOTICE, "triggered ubus reload\n");
	freecwmp_reload();

	return 0;
}

static const struct ubus_method freecwmp_methods[] = {
	UBUS_METHOD("notify", freecwmpd_handle_notify, notify_policy),
	UBUS_METHOD("inform", freecwmpd_handle_inform, inform_policy),
	{ .name = "reload", .handler = freecwmpd_handle_reload },
};

static struct ubus_object_type main_object_type =
	UBUS_OBJECT_TYPE("freecwmpd", freecwmp_methods);

static struct ubus_object main_object = {
	.name = "tr069",
	.type = &main_object_type,
	.methods = freecwmp_methods,
	.n_methods = ARRAY_SIZE(freecwmp_methods),
};

int
ubus_init(void)
{
	ctx = ubus_connect(config->local->ubus_socket);
	if (!ctx) return -1;

	ubus_add_uloop(ctx);

	if (ubus_add_object(ctx, &main_object)) return -1;

	return 0;
}

void
ubus_exit(void)
{
	if (ctx) ubus_free(ctx);
}
