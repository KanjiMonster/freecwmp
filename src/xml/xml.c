/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <microxml.h>

#include "../freecwmp.h"
#include "../config.h"
#include "../cwmp/cwmp.h"
#include "../cwmp/messages.h"
#include "../mix/time.h"

#include "xml.h"

const static char *soap_env_url = "http://schemas.xmlsoap.org/soap/envelope/";
const static char *soap_enc_url = "http://schemas.xmlsoap.org/soap/encoding/";
const static char *xsd_url = "http://www.w3.org/2001/XMLSchema";
const static char *xsi_url = "http://www.w3.org/2001/XMLSchema-instance";
const static char *cwmp_urls[] = {
		"urn:dslforum-org:cwmp-1-0", 
		"urn:dslforum-org:cwmp-1-1", 
		"urn:dslforum-org:cwmp-1-2", 
		NULL };

static void
xml_recreate_namespace(char *msg)
{
	FC_DEVEL_DEBUG("enter");

	mxml_node_t *tree;
	char *c;
	uint8_t i;
	const char *cwmp_urn;

	if (ns.soap_env)
		free(ns.soap_env);
	if (ns.soap_enc)
		free(ns.soap_enc);
	if (ns.xsd)
		free(ns.xsd);
	if (ns.xsi)
		free(ns.xsi);
	if (ns.cwmp)
		free(ns.cwmp);

	tree = mxmlLoadString(NULL, msg, MXML_NO_CALLBACK);
	if (!tree)
		goto done;

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
	FC_DEVEL_DEBUG("exit");
}

int8_t
xml_init(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return FC_SUCCESS;
}

int8_t
xml_exit(void)
{
	FC_DEVEL_DEBUG("enter");
	
	if (ns.soap_env) {
		free(ns.soap_env);
		ns.soap_env = NULL;
	}
	if (ns.soap_enc) {
		free(ns.soap_enc);
		ns.soap_enc = NULL;
	}
	if (ns.xsd) {
		free(ns.xsd);
		ns.xsd = NULL;
	}
	if (ns.xsi) {
		free(ns.xsi);
		ns.xsi = NULL;
	}
	if (ns.cwmp) {
		free(ns.cwmp);
		ns.cwmp = NULL;
	}

	FC_DEVEL_DEBUG("exit");
	return FC_SUCCESS;
}

int8_t
xml_prepare_inform_message(char **msg_out)
{
	FC_DEVEL_DEBUG("enter");

	mxml_node_t *tree, *busy_node;
	int8_t status;
	char *c, *tmp;

#ifdef DUMMY_MODE
	FILE *fp;
	fp = fopen("./ext/soap_msg_templates/cwmp_inform_message.xml", "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);
#else
	tree = mxmlLoadString(NULL, CWMP_INFORM_MESSAGE, MXML_NO_CALLBACK);
#endif
	if (!tree)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "RetryCount", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewInteger(busy_node, cwmp_get_retry_count());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "Manufacturer", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, config->device->manufacturer);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "OUI", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, config->device->oui);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "ProductClass", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, config->device->product_class);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "SerialNumber", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, config->device->serial_number);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "EventCode", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, cwmp_get_event_code());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "CurrentTime", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, mix_get_time());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.Manufacturer", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, config->device->manufacturer);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ManufacturerOUI", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, config->device->oui);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ProductClass", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = config->device->product_class; c = c ? c : "";
	busy_node = mxmlNewText(busy_node, 0, c);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.SerialNumber", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = config->device->serial_number; c = c ? c : "";
	busy_node = mxmlNewText(busy_node, 0, c);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.HardwareVersion", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = config->device->hardware_version; c = c ? c : "";
	busy_node = mxmlNewText(busy_node, 0, c);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.SoftwareVersion", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = config->device->software_version; c = c ? c : "";
	busy_node = mxmlNewText(busy_node, 0, c);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ProvisioningCode", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = config->device->provisioning_code;
	if (c) {
		busy_node = mxmlNewText(busy_node, 0, c);
		if (!busy_node)
			goto error;
	}

	tmp = "InternetGatewayDevice.WANDevice.1.WANConnectionDevice.1.WANIPConnection.1.ExternalIPAddress";
	busy_node = mxmlFindElementText(tree, tree, tmp, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = NULL;
	status = cwmp_get_parameter_handler(tmp, &c);
	if (status != FC_SUCCESS)
		goto error;
	if (c) {
		busy_node = mxmlNewText(busy_node, 0, c);
		free(c);
		if (!busy_node)
			goto error;
	}

	tmp = "InternetGatewayDevice.ManagementServer.ConnectionRequestURL";
	busy_node = mxmlFindElementText(tree, tree, tmp, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = NULL;
	status = cwmp_get_parameter_handler(tmp, &c);
	if (status != FC_SUCCESS)
		goto error;
	if (c) {
		busy_node = mxmlNewText(busy_node, 0, c);
		free(c);
		if (!busy_node)
			goto error;
	}

	/* notifications */
	struct notification *n;
	struct list_head *l, *p;
	l = cwmp_get_notifications();
	list_for_each(p, l) {
		n = list_entry(p, struct notification, list);

		tmp = "InternetGatewayDevice.ManagementServer.ConnectionRequestURL";
		busy_node = mxmlFindElementText(tree, tree, tmp, MXML_DESCEND);
		if (!busy_node)
			goto error;
		
		busy_node = busy_node->parent->parent->parent;
		busy_node = mxmlNewElement(busy_node, "ParameterValueStruct");
		busy_node = mxmlNewElement(busy_node, "Name");
		busy_node = mxmlNewText(busy_node, 0, n->parameter);
		busy_node = busy_node->parent->parent;
		busy_node = mxmlNewElement(busy_node, "Value");
		busy_node = mxmlNewText(busy_node, 0, n->value);
	}

	*msg_out = mxmlSaveAllocString(tree, MXML_NO_CALLBACK);

#ifdef DEBUG_SKIP
	printf("+++ INFORM MESSAGE +++\n");
	printf("%s", *msg_out);
	printf("--- INFORM MESSAGE ---\n");
#endif

	status = FC_SUCCESS;
	goto done;

error:
#ifdef DEBUG
	if (errno == ENOMEM) {
		perror(__func__);
	}
#endif
	status = FC_ERROR;

done:
	mxmlDelete(tree);

	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
xml_parse_inform_response_message(char *msg_in, char **msg_out)
{
	FC_DEVEL_DEBUG("enter");

	mxml_node_t *tree, *busy_node;
	int8_t status;
	char *c, *soap_env_fault;
	uint8_t len;

	if (!msg_in)
		goto error;

	tree = mxmlLoadString(NULL, msg_in, MXML_NO_CALLBACK);
	if (!tree)
		goto error;

	len = snprintf(NULL, 0, "%s:%s", ns.soap_env, "Fault");
	soap_env_fault = (char *) calloc((len + 1), sizeof(char));
	if (!soap_env_fault)
		goto error;
	snprintf(soap_env_fault, (len + 1), "%s:%s\0", ns.soap_env, "Fault");

	busy_node = mxmlFindElement(tree, tree, soap_env_fault, NULL, NULL, MXML_DESCEND);
	free(soap_env_fault);
	// TODO: ACS responded with error message, right now we are not handeling this
	if (busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "MaxEnvelopes", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlWalkNext(busy_node, tree, MXML_DESCEND_FIRST);
	if (!busy_node || !busy_node->value.text.string)
		goto error;

#ifdef DEBUG_SKIP 
	printf("+++ ACS MaxEnvelopes +++\n");
	printf("%s\n", busy_node->value.text.string);
	printf("--- ACS MaxEnvelopes ---\n");
#endif

	status = FC_SUCCESS;
	goto done;

error:
#ifdef DEBUG
	if (errno == ENOMEM) {
		perror(__func__);
	}
#endif
	status = FC_ERROR;

done:
	mxmlDelete(tree);

	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
xml_handle_message(char *msg_in, char **msg_out)
{
	FC_DEVEL_DEBUG("enter");

	mxml_node_t *tree_in, *tree_out, *node;
	mxml_node_t *busy_node, *tmp_node;
	char *c, *parameter_name, *parameter_value, *parameter_notification,
		*download_url, *download_size;
	uint8_t status, len;
	uint16_t att_cnt;

#ifdef DUMMY_MODE
	FILE *fp;
	fp = fopen("./ext/soap_msg_templates/cwmp_response_message.xml", "r");
	tree_out = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);
#else
	tree_out = mxmlLoadString(NULL, CWMP_RESPONSE_MESSAGE, MXML_NO_CALLBACK);
#endif
	if (!tree_out)
		goto error;
		
	xml_recreate_namespace(msg_in);

	tree_in = mxmlLoadString(NULL, msg_in, MXML_NO_CALLBACK);
	if (!tree_in)
		goto error;

	/* handle cwmp:ID */
	len = snprintf(NULL, 0, "%s:%s", ns.cwmp, "ID");
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto error;
	snprintf(c, (len + 1), "%s:%s\0", ns.cwmp, "ID");

	busy_node = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	free(c); c = NULL;
	if (!busy_node)
		/* ACS did not send ID parameter, we are continuing without it */
		goto set_parameter;

	busy_node = mxmlWalkNext(busy_node, tree_in, MXML_DESCEND_FIRST);
	if (!busy_node || !busy_node->value.text.string)
		goto error;
	tmp_node = busy_node;
	busy_node = mxmlFindElement(tree_out, tree_out, "cwmp:ID", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, tmp_node->value.text.string);
	if (!busy_node)
		goto error;

set_parameter:
	/* handle cwmp:SetParameterValues */
	len = snprintf(NULL, 0, "%s:%s", ns.cwmp, "SetParameterValues");
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto error;
	snprintf(c, (len + 1), "%s:%s\0", ns.cwmp, "SetParameterValues");
	node = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	free(c); c = NULL;
	if (!node)
		goto get_parameter;
	busy_node = node;
	parameter_name = NULL;
	parameter_value = NULL;
	while (busy_node != NULL) {
		if (busy_node &&
		    busy_node->type == MXML_TEXT &&
		    busy_node->value.text.string &&
		    busy_node->parent->type == MXML_ELEMENT &&
		    !strcmp(busy_node->parent->value.element.name, "Name")) {
			parameter_name = busy_node->value.text.string;
		}
		if (busy_node &&
		    busy_node->type == MXML_TEXT &&
		    busy_node->value.text.string &&
		    busy_node->parent->type == MXML_ELEMENT &&
		    !strcmp(busy_node->parent->value.element.name, "Value")) {
			parameter_value = busy_node->value.text.string;
		}
		if (parameter_name && parameter_value) {
			status = cwmp_set_parameter_write_handler(parameter_name, parameter_value);
			if (status != FC_SUCCESS)
				goto error_set_parameter;
			parameter_name = NULL;
			parameter_value = NULL;
		}
		busy_node = mxmlWalkNext(busy_node, node, MXML_DESCEND);
	}

	status = cwmp_set_action_execute_handler();
	if (status != FC_SUCCESS)
		goto error_set_parameter;

	if (node) {
		status = cwmp_reload_changes();
		goto done_set_parameter;
	}

get_parameter:
	/* handle cwmp:GetParameterValues */
	len = snprintf(NULL, 0, "%s:%s", ns.cwmp, "GetParameterValues");
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto error;
	snprintf(c, (len + 1), "%s:%s\0", ns.cwmp, "GetParameterValues");
	node = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	free(c); c = NULL;
	if (!node)
		goto set_parameter_attributes;
	busy_node = node;
	tmp_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!tmp_node)
		goto error;
	tmp_node = mxmlNewElement(tmp_node, "cwmp:GetParameterValuesResponse");
	if (!tmp_node)
		goto error;
	tmp_node = mxmlNewElement(tmp_node, "ParameterList");
	if (!tmp_node)
		goto error;
#ifdef ACS_MULTI
	mxmlElementSetAttr(tmp_node, "xsi:type", "soap_enc:Array");
#endif
	att_cnt = 0;
	parameter_name = NULL;
	parameter_value = NULL;
	while (busy_node != NULL) {
		if (busy_node &&
		    busy_node->type == MXML_TEXT &&
		    busy_node->value.text.string &&
		    busy_node->parent->type == MXML_ELEMENT &&
		    !strcmp(busy_node->parent->value.element.name, "string")) {
			parameter_name = busy_node->value.text.string;
		}
		if (parameter_name) {
			status = cwmp_get_parameter_handler(parameter_name, &parameter_value);
			if (status != FC_SUCCESS)
				goto error_get_parameter;
			att_cnt++;
			tmp_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
			if (!tmp_node)
				goto error;
			// TODO: check with other ACS if is needed to add atributes
			tmp_node = mxmlNewElement(tmp_node, "ParameterValueStruct");
			if (!tmp_node)
				goto error;
			tmp_node = mxmlNewElement(tmp_node, "Name");
			if (!tmp_node)
				goto error;
			tmp_node = mxmlNewText(tmp_node, 0, parameter_name);
			if (!tmp_node)
				goto error;
			tmp_node = tmp_node->parent->parent;
			tmp_node = mxmlNewElement(tmp_node, "Value");
			if (!tmp_node)
				goto error;
#ifdef ACS_MULTI
			mxmlElementSetAttr(tmp_node, "xsi:type", "xsd:string");
#endif
			tmp_node = mxmlNewText(tmp_node, 0, parameter_value);
			/*
			 * three day's work to finally find memory leak if we
			 * free parameter_name;
			 * it points to: busy_node->value.text.string
			 * 
			 * also, parameter_value can be NULL so we don't do checks
			 */

			parameter_name = NULL;
		}
		if (parameter_value) {
			free(parameter_value);
			parameter_value = NULL;
		}
		busy_node = mxmlWalkNext(busy_node, node, MXML_DESCEND);
	}
#ifdef ACS_MULTI
	busy_node = mxmlFindElement(tree_out, tree_out, "ParameterList", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	len = snprintf(NULL, 0, "cwmp:ParameterValueStruct[%d]\0", att_cnt);
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto error;
	snprintf(c, (len + 1), "cwmp:ParameterValueStruct[%d]\0", att_cnt);
	mxmlElementSetAttr(busy_node, "soap_enc:arrayType", c);
	free(c); c = NULL;
#endif
	if (node) {
		goto create_msg;
	}

set_parameter_attributes:
	/* handle cwmp:SetParameterAttributes */
	len = snprintf(NULL, 0, "%s:%s", ns.cwmp, "SetParameterAttributes");
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto error;
	snprintf(c, (len + 1), "%s:%s\0", ns.cwmp, "SetParameterAttributes");
	node = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	free(c); c = NULL;
	if (!node)
		goto download;
	busy_node = node;
	uint8_t attr_notification_update;
	while (busy_node != NULL) {
		if (busy_node &&
		    busy_node->type == MXML_ELEMENT &&
		    !strcmp(busy_node->value.element.name, "SetParameterAttributesStruct")) {
			attr_notification_update = 0;
			parameter_name = NULL;
			parameter_notification = NULL;
		}
		if (busy_node &&
		    busy_node->type == MXML_TEXT &&
		    busy_node->value.text.string &&
		    busy_node->parent->type == MXML_ELEMENT &&
		    !strcmp(busy_node->parent->value.element.name, "Name")) {
			parameter_name = busy_node->value.text.string;
		}
		if (busy_node &&
		    busy_node->type == MXML_TEXT &&
		    busy_node->value.text.string &&
		    busy_node->parent->type == MXML_ELEMENT &&
		    !strcmp(busy_node->parent->value.element.name, "NotificationChange")) {
			attr_notification_update = (uint8_t) atoi(busy_node->value.text.string);
		}
		if (busy_node &&
		    busy_node->type == MXML_TEXT &&
		    busy_node->value.text.string &&
		    busy_node->parent->type == MXML_ELEMENT &&
		    !strcmp(busy_node->parent->value.element.name, "Notification")) {
			parameter_notification = busy_node->value.text.string;
		}
		if (attr_notification_update && parameter_name && parameter_notification) {
			status = cwmp_set_notification_write_handler(parameter_name, parameter_notification);
			if (status != FC_SUCCESS)
				goto error_set_notification;
			attr_notification_update = 0;
			parameter_name = NULL;
			parameter_notification = NULL;
		}
		busy_node = mxmlWalkNext(busy_node, node, MXML_DESCEND);
	}

	status = cwmp_set_action_execute_handler();
	if (status != FC_SUCCESS)
		goto error_set_notification;

	if (node) {
		goto done_set_notification;
	}

download:
	/* handle cwmp:Download */
	len = snprintf(NULL, 0, "%s:%s", ns.cwmp, "Download");
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto error;
	snprintf(c, (len + 1), "%s:%s\0", ns.cwmp, "Download");
	node = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	free(c); c = NULL;
	if (!node)
		goto factory_reset;
	busy_node = node;
	download_url = NULL;
	download_size = NULL;
	while (busy_node != NULL) {
		if (busy_node &&
		    busy_node->type == MXML_TEXT &&
		    busy_node->value.text.string &&
		    busy_node->parent->type == MXML_ELEMENT &&
		    !strcmp(busy_node->parent->value.element.name, "URL")) {
			download_url = busy_node->value.text.string;
		}
		if (busy_node &&
		    busy_node->type == MXML_TEXT &&
		    busy_node->value.text.string &&
		    busy_node->parent->type == MXML_ELEMENT &&
		    !strcmp(busy_node->parent->value.element.name, "FileSize")) {
			download_size = busy_node->value.text.string;
		}
		busy_node = mxmlWalkNext(busy_node, node, MXML_DESCEND);
	}
	if (!download_url || !download_size)
		goto error;

	tmp_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!tmp_node)
		goto error;

	tmp_node = mxmlNewElement(tmp_node, "cwmp:DownloadResponse");
	if (!tmp_node)
		goto error;

	busy_node = mxmlNewElement(tmp_node, "Status");
	if (!busy_node)
		goto error;

	busy_node = mxmlNewElement(tmp_node, "StartTime");
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, mix_get_time());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tmp_node, tree_out, "Status", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	status = cwmp_download_handler(download_url, download_size);
	if (status != FC_SUCCESS)
		busy_node = mxmlNewText(busy_node, 0, "9000");
	else
		busy_node = mxmlNewText(busy_node, 0, "0");

	busy_node = mxmlNewElement(tmp_node, "CompleteTime");
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, mix_get_time());
	if (!busy_node)
		goto error;

	if (node) {
		goto create_msg;
	}

factory_reset:
	/* handle cwmp:FactoryReset */
	len = snprintf(NULL, 0, "%s:%s", ns.cwmp, "FactoryReset");
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto error;
	snprintf(c, (len + 1), "%s:%s\0", ns.cwmp, "FactoryReset");
	node = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	free(c); c = NULL;
	if (!node)
		goto reboot;
	tmp_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!tmp_node)
		goto error;
	tmp_node = mxmlNewElement(tmp_node, "cwmp:FactoryResetResponse");
	if (!tmp_node)
		goto error;

	status = cwmp_factory_reset_handler();
	if (status != FC_SUCCESS)
		goto error_factory_reset;

	if (node) {
		goto create_msg;
	}

reboot:
	/* handle cwmp:Reboot */
	len = snprintf(NULL, 0, "%s:%s", ns.cwmp, "Reboot");
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto error;
	snprintf(c, (len + 1), "%s:%s\0", ns.cwmp, "Reboot");
	node = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	free(c); c = NULL;
	if (!node)
		goto error;
	tmp_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!tmp_node)
		goto error;
	tmp_node = mxmlNewElement(tmp_node, "cwmp:RebootResponse");
	if (!tmp_node)
		goto error;

	status = cwmp_reboot_handler();
	if (status != FC_SUCCESS)
		goto error_reboot;

	if (node) {
		goto create_msg;
	}


error_set_notification:
	// TODO: just create a message
error_set_parameter:
	// TODO: just create a message
error_get_parameter:
error_download:
error_factory_reset:
error_reboot:
	goto create_msg;

done_set_notification:
	busy_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewElement(busy_node, "cwmp:SetParameterAttributesResponse");
	if (!busy_node)
		goto error;
	goto create_msg;

done_set_parameter:
	busy_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewElement(busy_node, "cwmp:SetParameterValuesResponse");
	if (!busy_node)
		goto error;
	busy_node = mxmlNewElement(busy_node, "Status");
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, "1");
	if (!busy_node)
		goto error;
	goto create_msg;


create_msg:
	*msg_out = mxmlSaveAllocString(tree_out, MXML_NO_CALLBACK);

#ifdef DEBUG_SKIP
	printf("+++ XML MESSAGE +++\n");
	printf("%s", *msg_out);
	printf("--- XML MESSAGE ---\n");
#endif

	status = FC_SUCCESS;
	goto done;

error:
#ifdef DEBUG
	if (errno == ENOMEM) {
		perror(__func__);
	}
#endif
	status = FC_ERROR;

done:
	if (tree_in) mxmlDelete(tree_in);
	if (tree_out) mxmlDelete(tree_out);

	FC_DEVEL_DEBUG("exit");
	return status;
}

