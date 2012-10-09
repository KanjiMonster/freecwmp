/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_XML_H__
#define _FREECWMP_XML_H__

#include <microxml.h>

void xml_exit(void);

int xml_prepare_inform_message(char **msg_out);
int xml_parse_inform_response_message(char *msg_in);
int xml_handle_message(char *msg_in, char **msg_out);

static int xml_handle_set_parameter_values(mxml_node_t *body_in,
					   mxml_node_t *tree_in,
					   mxml_node_t *tree_out);

static int xml_handle_get_parameter_values(mxml_node_t *body_in,
					   mxml_node_t *tree_in,
					   mxml_node_t *tree_out);

static int xml_handle_set_parameter_attributes(mxml_node_t *body_in,
					       mxml_node_t *tree_in,
					       mxml_node_t *tree_out);

static int xml_handle_download(mxml_node_t *body_in,
			       mxml_node_t *tree_in,
			       mxml_node_t *tree_out);

static int xml_handle_factory_reset(mxml_node_t *body_in,
				    mxml_node_t *tree_in,
				    mxml_node_t *tree_out);

static int xml_handle_reboot(mxml_node_t *body_in,
			     mxml_node_t *tree_in,
			     mxml_node_t *tree_out);

static int xml_create_generic_fault_message(mxml_node_t *body,
					    bool client,
					    char *code,
					    char *string);

#endif

