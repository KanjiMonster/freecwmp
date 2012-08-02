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

#include "../cwmp/cwmp.h"

static struct ubus_context *ctx;
static struct ubus_watch_object test_event;
static struct blob_buf b;

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

	blobmsg_parse(notify_policy, ARRAY_SIZE(notify_policy), tb, blob_data(msg), blob_len(msg));

	if (!tb[NOTIFY_PARAM])
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (!tb[NOTIFY_VALUE])
		return UBUS_STATUS_INVALID_ARGUMENT;

	cwmp_add_notification(blobmsg_data(tb[NOTIFY_PARAM]), blobmsg_data(tb[NOTIFY_VALUE]));

	return 0;
}

static const struct ubus_method freecwmp_methods[] = {
	UBUS_METHOD("notify", freecwmpd_handle_notify, notify_policy),
};

static struct ubus_object_type main_object_type =
	UBUS_OBJECT_TYPE("freecwmpd", freecwmp_methods);

static struct ubus_object main_object = {
	.name = "tr069",
	.type = &main_object_type,
	.methods = freecwmp_methods,
	.n_methods = ARRAY_SIZE(freecwmp_methods),
};

bool
ubus_init(void)
{
	ctx = ubus_connect((char *) local_get_ubus_socket());
	if (!ctx) return false;

	ubus_add_uloop(ctx);

	if (ubus_add_object(ctx, &main_object)) return false;

	return true;
}

void
ubus_exit(void)
{
	ubus_free(ctx);
}
