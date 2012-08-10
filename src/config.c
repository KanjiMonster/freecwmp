/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "cwmp/cwmp.h"
#include "cwmp/acs.h"
#include "cwmp/device.h"

static struct uci_context *uci_ctx;
static struct uci_package *uci_freecwmp;

struct core_config *config;

int
config_init_local(void)
{
	struct uci_section *s;
	struct uci_element *e;

	uci_foreach_element(&uci_freecwmp->sections, e) {
		s = uci_to_section(e);
		if (strcmp(s->type, "local") == 0)
			goto section_found;
	}
	D("uci section local not found...\n");
	return -1;

section_found:
	uci_foreach_element(&s->options, e) {
		if (!strcmp((uci_to_option(e))->e.name, "interface")) {
			config->local->interface = strdup(uci_to_option(e)->v.string);
			DD("freecwmp.@local[0].interface=%s\n", config->local->interface);
			goto next;
		}

		if (!strcmp((uci_to_option(e))->e.name, "port")) {
			if (!atoi((uci_to_option(e))->v.string)) {
				D("in section local port has invalid value...\n");
				return -1;
			}
			config->local->port = strdup(uci_to_option(e)->v.string);
			DD("freecwmp.@local[0].port=%s\n", config->local->port);
			goto next;
		}

		if (!strcmp((uci_to_option(e))->e.name, "ubus_socket")) {
			config->local->ubus_socket = strdup(uci_to_option(e)->v.string);
			DD("freecwmp.@local[0].ubus_socket=%s\n", config->local->ubus_socket);
			goto next;
		}

		if (!strcmp((uci_to_option(e))->e.name, "event")) {
			if (!strcasecmp("bootstrap", uci_to_option(e)->v.string))
				config->local->event = BOOTSTRAP;

			if (!strcasecmp("boot", uci_to_option(e)->v.string))
				config->local->event = BOOT;

			DD("freecwmp.@local[0].event=%s\n", uci_to_option(e)->v.string);
		}

next:
		;
	}

	if (!config->local->interface)
		printf("in local you must define interface\n");

	if (!config->local->interface)
		printf("in local you must define port\n");

	if (!config->local->ubus_socket)
		printf("in local you must define ubus_socket\n");

	return 0;
}

int
config_init_acs(void)
{
	int8_t status, n;
	struct uci_section *s;
	struct uci_element *e;
	n = 0;

	uci_foreach_element(&uci_freecwmp->sections, e) {
		s = uci_to_section(e);
		if (strcmp(s->type, "acs") == 0)
			goto section_found;
	}
	D("uci section acs not found...\n");
	return -1;

section_found:
	acs_init();

	status = FC_SUCCESS;
	uci_foreach_element(&s->options, e) {
		/* scheme */
		status = strcmp((uci_to_option(e))->e.name, "scheme");
		if (status == FC_SUCCESS) {
			/* ok, it's late and this does what i need */
			if (!(!(strcmp((uci_to_option(e))->v.string, "http") == 0) ||
			    !(strcmp((uci_to_option(e))->v.string, "https") == 0))) {
				D("in section acs scheme should be either http or https...\n");
				return -1;
			}
			acs_set_scheme((uci_to_option(e))->v.string);
			n++;
			goto next;
		}

		/* username */
		status = strcmp((uci_to_option(e))->e.name, "username");
		if (status == FC_SUCCESS) {
			acs_set_username((uci_to_option(e))->v.string);
			n++;
			goto next;
		}

		/* password */
		status = strcmp((uci_to_option(e))->e.name, "password");
		if (status == FC_SUCCESS) {
			acs_set_password((uci_to_option(e))->v.string);
			n++;
			goto next;
		}

		/* hostname */
		status = strcmp((uci_to_option(e))->e.name, "hostname");
		if (status == FC_SUCCESS) {
			acs_set_hostname((uci_to_option(e))->v.string);
			n++;
			goto next;
		}

		/* port */
		status = strcmp((uci_to_option(e))->e.name, "port");
		if (status == FC_SUCCESS) {
			if (!atoi((uci_to_option(e))->v.string)) {
				D("in section acs port has invalid value...\n");
				return -1;
			}
			acs_set_port((uci_to_option(e))->v.string);
			n++;
			goto next;
		}

		/* path */
		status = strcmp((uci_to_option(e))->e.name, "path");
		if (status == FC_SUCCESS) {
			acs_set_path((uci_to_option(e))->v.string);
			n++;
			goto next;
		}

#ifdef HTTP_CURL
		/* ssl_cert */
		status = strcmp((uci_to_option(e))->e.name, "ssl_cert");
		if (status == FC_SUCCESS) {
			acs_set_ssl_cert((uci_to_option(e))->v.string);
			goto next;
		}

		/* ssl_cacert */
		status = strcmp((uci_to_option(e))->e.name, "ssl_cacert");
		if (status == FC_SUCCESS) {
			acs_set_ssl_cacert((uci_to_option(e))->v.string);
			goto next;
		}

		/* ssl_verify */
		status = strcmp((uci_to_option(e))->e.name, "ssl_verify");
		if (status == FC_SUCCESS) {
			acs_set_ssl_verify((uci_to_option(e))->v.string);
			goto next;
		}
#endif /* HTTP_CURL */

next:
		;
	}

	/* TODO: not critical, but watch out */
	if (n == 6) {
		return 0;
	} else {
		D("in acs are not all options defined...\n");
		return -1;
	}
}

int
config_refresh_acs(void)
{
	acs_clean();
	if (config_reload()) return -1;
	if (config_init_acs()) return -1;

	return 0;
}

int
config_init_device(void)
{
	int8_t status;
	struct uci_section *s;
	struct uci_element *e;

	uci_foreach_element(&uci_freecwmp->sections, e) {
		s = uci_to_section(e);
		if (strcmp(s->type, "device") == 0)
			goto section_found;
	}
	D("uci section device not found...\n");
	return -1;

section_found:
	device_init();

	uci_foreach_element(&s->options, e) {
		if (strcmp((uci_to_option(e))->e.name, "manufacturer") == 0)
			device_set_manufacturer((uci_to_option(e))->v.string);
		else if (strcmp((uci_to_option(e))->e.name, "oui") == 0)
			device_set_oui((uci_to_option(e))->v.string);
		else if (strcmp((uci_to_option(e))->e.name, "product_class") == 0)
			device_set_product_class((uci_to_option(e))->v.string);
		else if (strcmp((uci_to_option(e))->e.name, "serial_number") == 0)
			device_set_serial_number((uci_to_option(e))->v.string);
		else if (strcmp((uci_to_option(e))->e.name, "hardware_version") == 0)
			device_set_hardware_version((uci_to_option(e))->v.string);
		else if (strcmp((uci_to_option(e))->e.name, "software_version") == 0)
			device_set_software_version((uci_to_option(e))->v.string);
		else if (strcmp((uci_to_option(e))->e.name, "provisioning_code") == 0)
			device_set_provisioning_code((uci_to_option(e))->v.string);
	}

	return 0;
}

int
config_refresh_device(void)
{
	device_clean();
	if (config_reload()) return -1;
	if (config_init_device()) return -1;

	return 0;
}

static struct uci_package *
config_init_package(const char *c)
{
	struct uci_context *ctx = uci_ctx;
	struct uci_package *p = NULL;

	config = malloc(sizeof(struct core_config));
	if (!config) return NULL;

	config->local = malloc(sizeof(struct local));
	if (!config->local) return NULL;

	if (!ctx) {
		ctx = uci_alloc_context();
		if (!ctx) return NULL;
		uci_ctx = ctx;

#ifdef DUMMY_MODE
		uci_set_confdir(ctx, "./ext/openwrt/config");
		uci_set_savedir(ctx, "./ext/tmp");
#endif

	} else {
		p = uci_lookup_package(ctx, c);
		if (p)
			uci_unload(ctx, p);
	}

	if (uci_load(ctx, c, &p)) {
		uci_free_context(ctx);
		return NULL;
	}

	return p;
}

int
config_reload(void)
{
	if (!uci_ctx) {
		uci_free_context(uci_ctx);
		uci_ctx = NULL;
	}

	FREE(config->local->ip)
	FREE(config->local->interface)
	FREE(config->local->port)
	FREE(config->local->ubus_socket)
	FREE(config->local)
	FREE(config)

	uci_freecwmp = config_init_package("freecwmp");
	if (!uci_freecwmp) return -1;

	return 0;
}

int
config_init_all(void)
{
	int8_t status;

	uci_ctx = NULL;
	uci_freecwmp = config_init_package("freecwmp");
	if (!uci_freecwmp) return -1;

	if (config_init_local()) return -1;
	if (config_init_acs()) return -1;
	if (config_init_device()) return -1;

	return 0;
}

