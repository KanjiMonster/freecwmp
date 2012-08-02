/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 *	Copyright (C) 2012 Jonas Gorski <jonas.gorski@gmail.com> for T-Labs
 *			   (Deutsche Telekom Innovation Laboratories)
 */

#include <microxml.h>

#include "../freecwmp.h"
#include "../cwmp/cwmp.h"
#include "../cwmp/messages.h"
#include "../cwmp/device.h"
#include "../mix/time.h"

#include "xml.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

const static char *soap_env_url = "http://schemas.xmlsoap.org/soap/envelope/";
const static char *soap_enc_url = "http://schemas.xmlsoap.org/soap/encoding/";
const static char *xsd_url = "http://www.w3.org/2001/XMLSchema";
const static char *xsi_url = "http://www.w3.org/2001/XMLSchema-instance";
const static char *cwmp_urls[] = {
		"urn:dslforum-org:cwmp-1-0", 
		"urn:dslforum-org:cwmp-1-1", 
		"urn:dslforum-org:cwmp-1-2", 
		NULL };

struct rpc_method {
	const char *name;
	int8_t (*handler)(mxml_node_t *body_in, mxml_node_t *tree_in,
			  mxml_node_t *tree_out);
};

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
	busy_node = mxmlNewText(busy_node, 0, device_get_manufacturer());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "OUI", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, device_get_oui());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "ProductClass", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, device_get_product_class());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElement(tree, tree, "SerialNumber", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, device_get_serial_number());
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
	busy_node = mxmlNewText(busy_node, 0, device_get_manufacturer());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ManufacturerOUI", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	busy_node = mxmlNewText(busy_node, 0, device_get_oui());
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ProductClass", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = device_get_product_class(); c = c ? c : "";
	busy_node = mxmlNewText(busy_node, 0, c);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.SerialNumber", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = device_get_serial_number(); c = c ? c : "";
	busy_node = mxmlNewText(busy_node, 0, c);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.HardwareVersion", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = device_get_hardware_version(); c = c ? c : "";
	busy_node = mxmlNewText(busy_node, 0, c);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.SoftwareVersion", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = device_get_software_version(); c = c ? c : "";
	busy_node = mxmlNewText(busy_node, 0, c);
	if (!busy_node)
		goto error;

	busy_node = mxmlFindElementText(tree, tree, "InternetGatewayDevice.DeviceInfo.ProvisioningCode", MXML_DESCEND);
	if (!busy_node)
		goto error;
	busy_node = busy_node->parent->next->next;
	if (mxmlGetType(busy_node) != MXML_ELEMENT)
		goto error;
	c = device_get_provisioning_code();
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
xml_handle_set_parameter_values(mxml_node_t *body_in, mxml_node_t *tree_in,
				mxml_node_t *tree_out)
{
	mxml_node_t *busy_node = body_in;
	char *parameter_name = NULL;
	char *parameter_value = NULL;
	uint8_t status;
	int8_t ret = FC_ERROR;

	FC_DEVEL_DEBUG("enter");

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
				return -1;
			parameter_name = NULL;
			parameter_value = NULL;
		}
		busy_node = mxmlWalkNext(busy_node, body_in, MXML_DESCEND);
	}

	status = cwmp_set_action_execute_handler();
	if (status != FC_SUCCESS)
		goto out;

	status = cwmp_reload_changes();

	busy_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL,
				    NULL, MXML_DESCEND);
	if (!busy_node)
		goto out;
	busy_node = mxmlNewElement(busy_node, "cwmp:SetParameterValuesResponse");
	if (!busy_node)
		goto out;
	busy_node = mxmlNewElement(busy_node, "Status");
	if (!busy_node)
		goto out;
	busy_node = mxmlNewText(busy_node, 0, "1");
	if (!busy_node)
		goto out;

	ret = FC_SUCCESS;
out:
	FC_DEVEL_DEBUG("exit");
	return ret;
}

int8_t
xml_handle_get_parameter_values(mxml_node_t *body_in, mxml_node_t *tree_in,
				mxml_node_t *tree_out)
{
	mxml_node_t *busy_node = body_in;
	mxml_node_t *tmp_node;
	char *c;
	char *parameter_name = NULL;
	char *parameter_value = NULL;
	uint8_t status, len;
	int8_t ret = FC_ERROR;
	uint16_t att_cnt;

	FC_DEVEL_DEBUG("enter");

	tmp_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!tmp_node)
		goto out;
	tmp_node = mxmlNewElement(tmp_node, "cwmp:GetParameterValuesResponse");
	if (!tmp_node)
		goto out;
	tmp_node = mxmlNewElement(tmp_node, "ParameterList");
	if (!tmp_node)
		goto out;
#ifdef ACS_MULTI
	mxmlElementSetAttr(tmp_node, "xsi:type", "soap_enc:Array");
#endif
	att_cnt = 0;
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
				goto out;
			att_cnt++;
			tmp_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
			if (!tmp_node)
				goto out;
			// TODO: check with other ACS if is needed to add atributes
			tmp_node = mxmlNewElement(tmp_node, "ParameterValueStruct");
			if (!tmp_node)
				goto out;
			tmp_node = mxmlNewElement(tmp_node, "Name");
			if (!tmp_node)
				goto out;
			tmp_node = mxmlNewText(tmp_node, 0, parameter_name);
			if (!tmp_node)
				goto out;
			tmp_node = tmp_node->parent->parent;
			tmp_node = mxmlNewElement(tmp_node, "Value");
			if (!tmp_node)
				goto out;
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
		busy_node = mxmlWalkNext(busy_node, body_in, MXML_DESCEND);
	}
#ifdef ACS_MULTI
	busy_node = mxmlFindElement(tree_out, tree_out, "ParameterList", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto out;
	len = snprintf(NULL, 0, "cwmp:ParameterValueStruct[%d]\0", att_cnt);
	c = (char *) calloc((len + 1), sizeof(char));
	if (!c)
		goto out;
	snprintf(c, (len + 1), "cwmp:ParameterValueStruct[%d]\0", att_cnt);
	mxmlElementSetAttr(busy_node, "soap_enc:arrayType", c);
	free(c); c = NULL;
#endif

	ret = FC_SUCCESS;
out:
	if (parameter_value) {
		free(parameter_value);
		parameter_value = NULL;
	}

	FC_DEVEL_DEBUG("exit");
	return ret;
}

int8_t
xml_handle_set_parameter_attributes(mxml_node_t *body_in, mxml_node_t *tree_in,
				    mxml_node_t *tree_out)
{
	char *parameter_name, *parameter_value, *parameter_notification;
	mxml_node_t *busy_node = body_in;
	uint8_t attr_notification_update;
	uint8_t status;
	int8_t ret = FC_ERROR;

	FC_DEVEL_DEBUG("enter");

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
				goto out;
			attr_notification_update = 0;
			parameter_name = NULL;
			parameter_notification = NULL;
		}
		busy_node = mxmlWalkNext(busy_node, body_in, MXML_DESCEND);
	}

	status = cwmp_set_action_execute_handler();
	if (status != FC_SUCCESS)
		goto out;

	busy_node = mxmlFindElement(tree_out, tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!busy_node)
		goto out;
	busy_node = mxmlNewElement(busy_node, "cwmp:SetParameterAttributesResponse");
	if (!busy_node)
		goto out;

	ret = FC_SUCCESS;
out:
	FC_DEVEL_DEBUG("exit");
	return ret;
}

const struct rpc_method rpc_methods[] = {
	{ "SetParameterValues", xml_handle_set_parameter_values },
	{ "GetParameterValues", xml_handle_get_parameter_values },
	{ "SetParameterAttributes", xml_handle_set_parameter_attributes },
};

int8_t
xml_handle_message(char *msg_in, char **msg_out)
{
	FC_DEVEL_DEBUG("enter");

	mxml_node_t *tree_in, *tree_out, *node;
	mxml_node_t *busy_node, *tmp_node;
	char *c, *parameter_name, *parameter_value, *parameter_notification,
		*download_url, *download_size;
	uint8_t status, len;
	int i;
	const struct rpc_method *method;

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
		goto find_method;

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

find_method:
	len = snprintf(NULL, 0, "%s:%s", ns.soap_env, "Body");
	c = (char *) calloc(len + 1, sizeof(char));
	if (!c)
		goto error;
	snprintf(c, len + 1, "%s:%s", ns.soap_env, "Body");

	node = mxmlFindElement(tree_in, tree_in, c, NULL, NULL, MXML_DESCEND);
	free(c);
	if (!node)
		goto error;

	node = mxmlWalkNext(node, tree_in, MXML_DESCEND_FIRST);
	if (!node)
		goto error;

	c = node->value.element.name;
	if (!c)
		goto error;

	/* convert QName to locaPart, check that ns is the expected one */
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
		if (method->handler(node, tree_in, tree_out) != FC_SUCCESS)
			goto error;

		goto create_msg;
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


error_download:
error_factory_reset:
error_reboot:
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

