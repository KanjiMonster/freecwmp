/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 *	Copyright (C) 2012 Jonas Gorski <jonas.gorski@gmail.com>
 */

#include <stdbool.h>
#include <libfreecwmp.h>
#include <microxml.h>

#include "xml.h"

#include "config.h"
#include "cwmp.h"
#include "external.h"
#include "freecwmp.h"
#include "messages.h"
#include "time.h"

struct rpc_method {
	const char *name;
	int (*handler)(mxml_node_t *body_in, mxml_node_t *tree_in,
			  mxml_node_t *tree_out);
};

const static char *soap_env_url = "http://schemas.xmlsoap.org/soap/envelope/";
const static char *soap_enc_url = "http://schemas.xmlsoap.org/soap/encoding/";
const static char *xsd_url = "http://www.w3.org/2001/XMLSchema";
const static char *xsi_url = "http://www.w3.org/2001/XMLSchema-instance";
const static char *cwmp_urls[] = {
		"urn:dslforum-org:cwmp-1-0", 
		"urn:dslforum-org:cwmp-1-1", 
		"urn:dslforum-org:cwmp-1-2", 
		NULL };

static struct cwmp_namespaces
{
	char *soap_env;
	char *soap_enc;
	char *xsd;
	char *xsi;
	char *cwmp;
} ns;

const struct rpc_method rpc_methods[] = {
	{ "SetParameterValues", xml_handle_set_parameter_values },
	{ "GetParameterValues", xml_handle_get_parameter_values },
	{ "SetParameterAttributes", xml_handle_set_parameter_attributes },
	{ "Download", xml_handle_download },
	{ "FactoryReset", xml_handle_factory_reset },
	{ "Reboot", xml_handle_reboot },
};

static void xml_recreate_namespace(char *msg)
{
	mxml_node_t *tree;
	const char *cwmp_urn;
	char *c;
	int i;

	FREE(ns.soap_env);
	FREE(ns.soap_enc);
	FREE(ns.xsd);
	FREE(ns.xsi);
	FREE(ns.cwmp);

	tree = mxmlLoadString(NULL, msg, MXML_NO_CALLBACK);
	if (!tree) goto done;

	c = (char *) mxmlElementGetAttrName(tree, soap_env_url);
	if (*(c + 5) == ':') {
		ns.soap_env = strdup((c + 6));
	}

	c = (char *) mxmlElementGetAttrName(tree, soap_enc_url);
	if (*(c + 5) == ':') {
		ns.soap_enc = strdup((c + 6));
	}

	c = (char *) mxmlElementGetAttrName(tree, xsd_url);
	if (*(c + 5) == ':') {
		ns.xsd = strdup((c + 6));
	}

	c = (char *) mxmlElementGetAttrName(tree, xsi_url);
	if (*(c + 5) == ':') {
		ns.xsi = strdup((c + 6));
	}

	for (i = 0; cwmp_urls[i] != NULL; i++) {
		cwmp_urn = cwmp_urls[i];
		c = (char *) mxmlElementGetAttrName(tree, cwmp_urn);
		if (c && *(c + 5) == ':') {
			ns.cwmp = strdup((c + 6));
			break;
		}
	}

done:
	mxmlDelete(tree);
}

void xml_exit(void)
{
	FREE(ns.soap_env);
	FREE(ns.soap_enc);
	FREE(ns.xsd);
	FREE(ns.xsi);
	FREE(ns.cwmp);
}

static int xml_prepare_events_inform(mxml_node_t *tree)
{
	mxml_node_t *node, *b1, *b2;
	char *c;
	int n = 0;
	struct list_head *p;
	struct event *event;

	b1 = mxmlFindElement(tree, tree, "Event", NULL, NULL, MXML_DESCEND);
	if (!b1) return -1;

	pthread_mutex_lock(&event_lock);

	list_for_each(p, &cwmp->events) {
		event = list_entry (p, struct event, list);
		node = mxmlNewElement (b1, "EventStruct");
		if (!node) goto error;

		b2 = mxmlNewElement (node, "EventCode");
		if (!b2) goto error;

		b2 = mxmlNewText(b2, 0, freecwmp_str_event_code(event->code));
		if (!b2) goto error;

		b2 = mxmlNewElement (node, "CommandKey");
		if (!b2) goto error;

		if (event->key) {
			b2 = mxmlNewText(b2, 0, event->key);
			if (!b2) goto error;
		}

		mxmlAdd(b1, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, node);
		n++;
	}

	pthread_mutex_unlock(&event_lock);

	if (n) {
		asprintf(&c, "cwmp:EventStruct[%u]", n);
		if (!c) return -1;

		mxmlElementSetAttr(b1, "soap_enc:arrayType", c);
		FREE(c);
	}

	return 0;

error:
	pthread_mutex_unlock(&event_lock);
	return -1;
}

static int xml_prepare_notifications_inform(mxml_node_t *tree)
{
	/* notifications */
	mxml_node_t *node, *b;
	char *c;
	int n = 0;
	struct list_head *p;
	struct notification *notification;

	b = mxmlFindElement(tree, tree, "Event", NULL, NULL, MXML_DESCEND);
	if (!b) return -1;

	pthread_mutex_lock(&notification_lock);

	list_for_each(p, &cwmp->notifications) {
		notification = list_entry(p, struct notification, list);

		c = "InternetGatewayDevice.ManagementServer.ConnectionRequestURL";
		b = mxmlFindElementText(tree, tree, c, MXML_DESCEND);
		if (!b) goto error;
		
		b = b->parent->parent->parent;
		b = mxmlNewElement(b, "ParameterValueStruct");
		if (!b) goto error;

		b = mxmlNewElement(b, "Name");
		if (!b) goto error;

		b = mxmlNewText(b, 0, notification->parameter);
		if (!b) goto error;

		b = b->parent->parent;
		b = mxmlNewElement(b, "Value");
		if (!b) goto error;

		b = mxmlNewText(b, 0, notification->value);
		if (!b) goto error;
	}

	pthread_mutex_unlock(&notification_lock);
	return 0;

error:
	pthread_mutex_unlock(&notification_lock);
	return -1;
}

int xml_prepare_inform_message(char **msg_out)
{
	mxml_node_t *tree, *b;
	char *c, *tmp;

#ifdef DUMMY_MODE
	FILE *fp;
	fp = fopen("./ext/soap_msg_templates/cwmp_inform_message.xml", "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);
#else
	tree = mxmlLoadString(NULL, CWMP_INFORM_MESSAGE, MXML_NO_CALLBACK);
#endif
	if (!tree) goto error;

	b = mxmlFindElement(tree, tree, "RetryCount", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewInteger(b, cwmp->retry_count);
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "Manufacturer", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, config->device->manufacturer);
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "OUI", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, config->device->oui);
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "ProductClass", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, config->device->product_class);
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "SerialNumber", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, config->device->serial_number);
	if (!b) goto error;

	if (xml_prepare_events_inform(tree))
		goto error;

	b = mxmlFindElement(tree, tree, "CurrentTime", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, mix_get_time());
	if (!b) goto error;

	b = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.Manufacturer", MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	b = mxmlNewText(b, 0, config->device->manufacturer);
	if (!b) goto error;

	b = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ManufacturerOUI", MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	b = mxmlNewText(b, 0, config->device->oui);
	if (!b) goto error;

	b = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ProductClass", MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	b = mxmlNewText(b, 0, config->device->product_class);
	if (!b) goto error;

	b = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.SerialNumber", MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	b = mxmlNewText(b, 0, config->device->serial_number);
	if (!b)
		goto error;

	b = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.HardwareVersion", MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	b = mxmlNewText(b, 0, config->device->hardware_version);
	if (!b) goto error;

	b = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.SoftwareVersion", MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	b = mxmlNewText(b, 0, config->device->software_version);
	if (!b) goto error;

	b = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ProvisioningCode", MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	tmp = "InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.WANIPConnection.1.ExternalIPAddress";
	b = mxmlFindElementText(tree, tree, tmp, MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	c = NULL;
	if (external_get_action("value", tmp, &c)) goto error;
	if (c) {
		b = mxmlNewText(b, 0, c);
		FREE(c);
		if (!b) goto error;
	}

	tmp = "InternetGatewayDevice.ManagementServer.ConnectionRequestURL";
	b = mxmlFindElementText(tree, tree, tmp, MXML_DESCEND);
	if (!b) goto error;

	b = b->parent->next->next;
	if (mxmlGetType(b) != MXML_ELEMENT)
		goto error;

	c = NULL;
	if (external_get_action("value", tmp, &c)) goto error;
	if (c) {
		b = mxmlNewText(b, 0, c);
		FREE(c);
		if (!b) goto error;
	}

	if (xml_prepare_notifications_inform(tree))
		goto error;

	*msg_out = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);

	mxmlDelete(tree);
	return 0;

error:
	mxmlDelete(tree);
	return -1;
}

int xml_parse_inform_response_message(char *msg_in)
{
	mxml_node_t *tree, *b;
	char *c;

	if (!msg_in) goto error;

	tree = mxmlLoadString(NULL, msg_in, MXML_NO_CALLBACK);
	if (!tree) goto error;

	asprintf(&c, "%s:%s", ns.soap_env, "Fault");
	if (!c) goto error;

	b = mxmlFindElement(tree, tree, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	// TODO: ACS responded with error message, right now we are not handeling this
	if (b) goto error;

	b = mxmlFindElement(tree, tree, "MaxEnvelopes", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlWalkNext(b, tree, MXML_DESCEND_FIRST);
	if (!b || !b->value.text.string)
		goto error;

	mxmlDelete(tree);
	return 0;

error:
	mxmlDelete(tree);
	return -1;
}

int xml_handle_message(char *msg_in, char **msg_out)
{
	mxml_node_t *tree_in, *tree_out, *b;
	const struct rpc_method *method;
	char *c;
	int i;

#ifdef DUMMY_MODE
	FILE *fp;
	fp = fopen("./ext/soap_msg_templates/cwmp_response_message.xml", "r");
	tree_out = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);
#else
	tree_out = mxmlLoadString(NULL, CWMP_RESPONSE_MESSAGE, MXML_NO_CALLBACK);
#endif
	if (!tree_out) goto error;
		
	xml_recreate_namespace(msg_in);

	tree_in = mxmlLoadString(NULL, msg_in, MXML_NO_CALLBACK);
	if (!tree_in) goto error;

	/* handle cwmp:ID */
	asprintf(&c, "%s:%s", ns.cwmp, "ID");
	if (!c) goto error;

	b = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	/* ACS did not send ID parameter, we are continuing without it */
	if (!b) goto find_method;

	b = mxmlWalkNext(b, tree_in, MXML_DESCEND_FIRST);
	if (!b || !b->value.text.string) goto error;
	c = strdup(b->value.text.string);

	b = mxmlFindElement(tree_out, tree_out, "cwmp:ID", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, c);
	FREE(c);

	if (!b) goto error;

find_method:
	asprintf(&c, "%s:%s", ns.soap_env, "Body");
	if (!c) goto error;

	b = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);
	if (!b) goto error;

	while (1) {
		b = mxmlWalkNext(b, tree_in, MXML_DESCEND_FIRST);
		if (!b) goto error;
		if (b->type == MXML_ELEMENT) break;
	}
	
	c = b->value.element.name;
	/* convert QName to localPart, check that ns is the expected one */
	if (strchr(c, ':')) {
		char *tmp = strchr(c, ':');
		size_t ns_len = tmp - c;

		if (strlen(ns.cwmp) != ns_len)
			goto error;

		if (strncmp(ns.cwmp, c, ns_len))
			goto error;

		c = tmp + 1;
	} else {
		goto error;
	}

	method = NULL;
	for (i = 0; i < ARRAY_SIZE(rpc_methods); i++) {
		if (!strcmp(c, rpc_methods[i].name)) {
			method = &rpc_methods[i];
			break;
		}
	}

	if (method) {
		if (method->handler(b, tree_in, tree_out)) goto error;
	} else {
		char *fault_message;

		b = mxmlFindElement(tree_out, tree_out, "soap_env:Body",
				    NULL, NULL, MXML_DESCEND);
		if (!b) goto error;

		asprintf(&fault_message, "%s not supported", c);
		if (!fault_message) goto error;

		if (!xml_create_generic_fault_message(b, true, "9000", fault_message)) {
			FREE(fault_message);
			goto error;
		}

		FREE(fault_message);
	}

	*msg_out = mxmlSaveAllocString(tree_out, MXML_NO_CALLBACK);

	if (tree_in) mxmlDelete(tree_in);
	if (tree_out) mxmlDelete(tree_out);
	return 0;

error:
	if (tree_in) mxmlDelete(tree_in);
	if (tree_out) mxmlDelete(tree_out);
	return -1;
}

static int xml_create_generic_fault_message(mxml_node_t *body,
					    bool client,
					    char *code,
					    char *string)
{
	mxml_node_t *b, *t, *u;

	b = mxmlNewElement(body, "soap_env:Fault");
	if (!b) return -1;

	t = mxmlNewElement(b, "faultcode");
	if (!t) return -1;

	u = mxmlNewText(t, 0, client ? "Client" : "Server");
	if (!u) return -1;
	
	t = mxmlNewElement(b, "faultstring");
	if (!t) return -1;

	u = mxmlNewText(t, 0, "CWMP fault");
	if (!u) return -1;

	b = mxmlNewElement(b, "detail");
	if (!b) return -1;

	b = mxmlNewElement(b, "cwmp:Fault");
	if (!b) return -1;

	t = mxmlNewElement(b, "FaultCode");
	if (!t) return -1;

	u = mxmlNewText(t, 0, code);
	if (!u) return -1;

	t = mxmlNewElement(b, "FaultString");
	if (!b) return -1;

	u = mxmlNewText(t, 0, string);
	if (!u) return -1;

	return 0;
}

int xml_handle_set_parameter_values(mxml_node_t *body_in,
				    mxml_node_t *tree_in,
				    mxml_node_t *tree_out)
{
	mxml_node_t *b = body_in;
	char *parameter_name = NULL;
	char *parameter_value = NULL;

	while (b) {
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "Name")) {
			parameter_name = b->value.text.string;
		}
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "Value")) {
			parameter_value = b->value.text.string;
		}
		if (parameter_name && parameter_value) {
			if (external_set_action_write("value",
					parameter_name, parameter_value))
				return -1;
			parameter_name = NULL;
			parameter_value = NULL;
		}
		b = mxmlWalkNext(b, body_in, MXML_DESCEND);
	}

	if (external_set_action_execute())
		return -1;

	config_load();

	b = mxmlFindElement(tree_out, tree_out, "soap_env:Body",
			    NULL, NULL, MXML_DESCEND);
	if (!b) return -1;

	b = mxmlNewElement(b, "cwmp:SetParameterValuesResponse");
	if (!b) return -1;

	b = mxmlNewElement(b, "Status");
	if (!b) return -1;

	b = mxmlNewText(b, 0, "1");
	if (!b) return -1;
	
	return 0;
}

int xml_handle_get_parameter_values(mxml_node_t *body_in,
				    mxml_node_t *tree_in,
				    mxml_node_t *tree_out)
{
	mxml_node_t *n, *b = body_in;
	char *parameter_name = NULL;
	char *parameter_value = NULL;
	char *c;
	int counter = 0;

	n = mxmlFindElement(tree_out, tree_out, "soap_env:Body",
			    NULL, NULL, MXML_DESCEND);
	if (!n) return -1;

	n = mxmlNewElement(n, "cwmp:GetParameterValuesResponse");
	if (!n) return -1;

	n = mxmlNewElement(n, "ParameterList");
	if (!n) return -1;

#ifdef ACS_MULTI
	mxmlElementSetAttr(n, "xsi:type", "soap_enc:Array");
#endif

	while (b) {
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "string")) {
			parameter_name = b->value.text.string;
		}

		if (parameter_name) {
			if (!config_get_cwmp(parameter_name, &parameter_value)) {
				// got the parameter value using libuci
			} else if (!external_get_action("value",
					parameter_name, &parameter_value)) {
				// got the parameter value via external script
			} else {
				// error occurred when getting parameter value
				goto out;
			}
			counter++;

			n = mxmlFindElement(tree_out, tree_out, "ParameterList", NULL, NULL, MXML_DESCEND);
			if (!n) goto out;

			n = mxmlNewElement(n, "ParameterValueStruct");
			if (!n) goto out;

			n = mxmlNewElement(n, "Name");
			if (!n) goto out;

			n = mxmlNewText(n, 0, parameter_name);
			if (!n) goto out;

			n = n->parent->parent;
			n = mxmlNewElement(n, "Value");
			if (!n) goto out;

#ifdef ACS_MULTI
			mxmlElementSetAttr(n, "xsi:type", "xsd:string");
#endif
			n = mxmlNewText(n, 0, parameter_value);
			if (!n) goto out;

			/*
			 * three day's work to finally find memory leak if we
			 * free parameter_name;
			 * it points to: b->value.text.string
			 *
			 * also, parameter_value can be NULL so we don't do checks
			 */
			parameter_name = NULL;
		}
		
		FREE(parameter_value);
		b = mxmlWalkNext(b, body_in, MXML_DESCEND);
	}

#ifdef ACS_MULTI
	b = mxmlFindElement(tree_out, tree_out, "ParameterList", 
			    NULL, NULL, MXML_DESCEND);
	if (!b) goto out;

	asprintf(&c, "cwmp:ParameterValueStruct[%d]", counter);
	if (!c) goto out;

	mxmlElementSetAttr(b, "soap_enc:arrayType", c);
	FREE(c);
#endif

	FREE(parameter_value);
	return 0;

out:
	FREE(parameter_value);
	return -1;
}

static int xml_handle_set_parameter_attributes(mxml_node_t *body_in,
					       mxml_node_t *tree_in,
					       mxml_node_t *tree_out) {

	mxml_node_t *n, *b = body_in;
	char *c, *parameter_name, *parameter_notification;
	uint8_t attr_notification_update;

	/* handle cwmp:SetParameterAttributes */
	asprintf(&c, "%s:%s", ns.cwmp, "SetParameterAttributes");
	if (!c) return -1;

	n = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if (!n) return -1;
	b = n;

	while (b != NULL) {
		if (b && b->type == MXML_ELEMENT &&
		    !strcmp(b->value.element.name, "SetParameterAttributesStruct")) {
			attr_notification_update = 0;
			parameter_name = NULL;
			parameter_notification = NULL;
		}
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "Name")) {
			parameter_name = b->value.text.string;
		}
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "NotificationChange")) {
			attr_notification_update = (uint8_t) atoi(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "Notification")) {
			parameter_notification = b->value.text.string;
		}
		if (attr_notification_update && parameter_name && parameter_notification) {
			if (external_set_action_write("notification",
					parameter_name, parameter_notification))
				return -1;
			attr_notification_update = 0;
			parameter_name = NULL;
			parameter_notification = NULL;
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}

	if (external_set_action_execute())
		return -1;

	b = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!b) return -1;

	b = mxmlNewElement(b, "cwmp:SetParameterAttributesResponse");
	if (!b) return -1;

	config_load();

	return 0;
}

static int xml_handle_download(mxml_node_t *body_in,
			       mxml_node_t *tree_in,
			       mxml_node_t *tree_out)
{
	mxml_node_t *n, *t, *b = body_in;
	char *c, *download_url, *download_size;

	asprintf(&c, "%s:%s", ns.cwmp, "Download");
	if (!c) return -1;

	n = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if (!n) return -1;
	b = n;

	download_url = NULL;
	download_size = NULL;
	while (b != NULL) {
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "URL")) {
			download_url = b->value.text.string;
		}
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "FileSize")) {
			download_size = b->value.text.string;
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}
	if (!download_url || !download_size)
		return -1;

	t = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!t) return -1;

	t = mxmlNewElement(t, "cwmp:DownloadResponse");
	if (!t) return -1;

	b = mxmlNewElement(t, "Status");
	if (!b) return -1;

	b = mxmlNewElement(t, "StartTime");
	if (!b) return -1;

	b = mxmlNewText(b, 0, mix_get_time());
	if (!b) return -1;

	b = mxmlFindElement(t, tree_out, "Status", NULL, NULL, MXML_DESCEND);
	if (!b) return -1;

	if (external_download(download_url, download_size))
		b = mxmlNewText(b, 0, "9000");
	else
		b = mxmlNewText(b, 0, "0");

	b = mxmlNewElement(t, "CompleteTime");
	if (!b) return -1;

	b = mxmlNewText(b, 0, mix_get_time());
	if (!b) return -1;

	return 0;
}

static int xml_handle_factory_reset(mxml_node_t *node,
				    mxml_node_t *tree_in,
				    mxml_node_t *tree_out)
{
	mxml_node_t *b;

	b = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!b) return -1;

	b = mxmlNewElement(b, "cwmp:FactoryResetResponse");
	if (!b) return -1;

	if (external_simple("factory_reset"))
		return -1;

	return 0;
}

static int xml_handle_reboot(mxml_node_t *node,
			     mxml_node_t *tree_in,
			     mxml_node_t *tree_out)
{
	mxml_node_t *b;

	b = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!b) return -1;

	b = mxmlNewElement(b, "cwmp:RebootResponse");
	if (!b) return -1;

	if (external_simple("reboot"))
		return -1;

	return 0;
}

