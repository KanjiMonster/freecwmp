/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_DEVICE_H__

struct device
{
	char *manufacturer;
	char *oui;
	char *product_class;
	char *serial_number;
	char *hardware_version;
	char *software_version;
	char *provisioning_code;
};

void device_clean(void);
void device_clean(void);
char * device_get_manufacturer(void);
void device_set_manufacturer(char *c);
char * device_get_oui(void);
void device_set_oui(char *c);
char * device_get_product_class(void);
void device_set_product_class(char *c);
char * device_get_serial_number(void);
void device_set_serial_number(char *c);
char * device_get_hardware_version(void);
void device_set_hardware_version(char *c);
char * device_get_software_version(void);
void device_set_softdware_version(char *c);
char * device_get_provisioning_code(void);
void device_set_provisioning_code(char *c);

#endif

