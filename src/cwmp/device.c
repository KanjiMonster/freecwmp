/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>

#include "device.h"
#include "../config.h"

struct device dev;

void
device_init(void)
{
	FC_DEVEL_DEBUG("enter");

	dev.manufacturer = NULL;
	dev.oui = NULL;
	dev.product_class = NULL;
	dev.serial_number = NULL;
	dev.hardware_version = NULL;
	dev.software_version = NULL;
	dev.provisioning_code = NULL;

	FC_DEVEL_DEBUG("exit");
}

void
device_clean(void)
{
	FC_DEVEL_DEBUG("enter");

	if (dev.manufacturer) free(dev.manufacturer);
	dev.manufacturer = NULL;
	if (dev.oui) free(dev.oui);
	dev.oui = NULL;
	if (dev.product_class) free(dev.product_class);
	dev.product_class = NULL;
	if (dev.serial_number) free(dev.serial_number);
	dev.serial_number = NULL;
	if (dev.hardware_version) free(dev.hardware_version);
	dev.hardware_version = NULL;
	if (dev.software_version) free(dev.software_version);
	dev.software_version = NULL;
	if (dev.provisioning_code) free(dev.provisioning_code);
	dev.provisioning_code = NULL;

	FC_DEVEL_DEBUG("exit");
}

char *
device_get_manufacturer(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return dev.manufacturer;
}

void
device_set_manufacturer(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (dev.manufacturer)
		free(dev.manufacturer);
	dev.manufacturer = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
device_get_oui(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return dev.oui;
}

void
device_set_oui(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (dev.oui)
		free(dev.oui);
	dev.oui = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
device_get_product_class(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return dev.product_class;
}

void
device_set_product_class(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (dev.product_class)
		free(dev.product_class);
	dev.product_class = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
device_get_serial_number(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return dev.serial_number;
}

void
device_set_serial_number(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (dev.serial_number)
		free(dev.serial_number);
	dev.serial_number = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
device_get_hardware_version(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return dev.hardware_version;
}

void
device_set_hardware_version(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (dev.hardware_version)
		free(dev.hardware_version);
	dev.hardware_version = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
device_get_software_version(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return dev.software_version;
}

void
device_set_software_version(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (dev.software_version)
		free(dev.software_version);
	dev.software_version = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

char *
device_get_provisioning_code(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return dev.provisioning_code;
}

void
device_set_provisioning_code(char *c)
{
	FC_DEVEL_DEBUG("enter");

	if (dev.provisioning_code)
		free(dev.provisioning_code);
	dev.provisioning_code = strdup(c);

	FC_DEVEL_DEBUG("exit");
}

