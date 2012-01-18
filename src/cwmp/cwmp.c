/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <libubox/uloop.h>

#include "cwmp.h"
#include "external.h"
#include "../freecwmp.h"
#include "../http/http.h"
#include "../xml/xml.h"

int8_t
cwmp_init(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;
	char *c = NULL;

	cwmp.retry_count = 0;
	cwmp.acs_reload_required = 0;
	cwmp.acs_connection_required = 0;
	cwmp.periodic_inform_enabled = 0;
	cwmp.periodic_inform_interval = 0;
	cwmp.event_code = local_get_event();

	status = cwmp_get_parameter_handler("InternetGatewayDevice.ManagementServer.PeriodicInformInterval", &c);
	if (status == FC_SUCCESS && c) {
		cwmp.periodic_inform_interval = atoi(c);
		cwmp.periodic_inform_t.cb = cwmp_periodic_inform;
		uloop_timeout_set(&cwmp.periodic_inform_t, cwmp.periodic_inform_interval * 1000);
	}
	if (c) {
		free(c); c = NULL;
	}

	status = cwmp_get_parameter_handler("InternetGatewayDevice.ManagementServer.PeriodicInformEnable", &c);
	if (status == FC_SUCCESS && c) {
		cwmp.periodic_inform_enabled = atoi(c);
	}
	if (c) {
		free(c); c = NULL;
	}

	status = http_server_init();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "http_server_init failed\n");
#endif
		goto error;
	}

	status = FC_SUCCESS;
	goto done;
	
error:
	status = FC_ERROR;
	goto done;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_exit()
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	status = http_client_exit();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "http_client_exit failed\n");
#endif
		goto error;
	}

	status = http_server_exit();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "http_server_exit failed\n");
#endif
		goto error;
	}

	status = xml_exit();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "xml_exit failed\n");
#endif
		goto error;
	}

	status = FC_SUCCESS;
	goto done;
	
error:
	status = FC_ERROR;
	goto done;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_reload_http_client(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	status = http_client_exit();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "http_client_exit failed\n");
#endif
		goto error;
	}

	status = http_client_init();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "http_client_init failed\n");
#endif
		goto error;
	}

	status = FC_SUCCESS;
	goto done;
	
error:
	status = FC_ERROR;
	goto done;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_reload_xml(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	status = xml_exit();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "xml_exit failed\n");
#endif
		goto error;
	}

	status = xml_init();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "xml_init failed\n");
#endif
		goto error;
	}

	status = FC_SUCCESS;
	goto done;
	
error:
	status = FC_ERROR;
	goto done;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}


int8_t
cwmp_periodic_inform(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	if (cwmp.periodic_inform_enabled && cwmp.periodic_inform_interval) {
		cwmp.periodic_inform_t.cb = cwmp_periodic_inform;
		uloop_timeout_set(&cwmp.periodic_inform_t, cwmp.periodic_inform_interval * 1000);
		cwmp.event_code = PERIODIC;
	}

	if (cwmp.periodic_inform_enabled)
		return cwmp_inform();
	else
		return FC_SUCCESS;
}

int8_t
cwmp_inform(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;
	char *msg_in, *msg_out;
	msg_in = msg_out = NULL;
	cwmp.acs_connection_required = 0;

	status = http_client_init();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "http_client_init failed\n");
#endif
		goto error;
	}

	status = xml_init();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "xml_init failed\n");
#endif
		goto error;
	}

	status = xml_prepare_inform_message(&msg_out);
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "xml_prepare_inform_message failed\n");
#endif
		goto error_xml;
	}

	status = http_send_message(msg_out, &msg_in);
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "http_send_message failed\n");
#endif
		goto error_http;
	}
	
	if (msg_out) {
		free(msg_out);
		msg_out = NULL;
	}

	status = xml_parse_inform_response_message(msg_in, &msg_out);
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "xml_parse_inform_response_message failed\n");
#endif
		goto error_xml;
	}

	if (msg_in) {
		free(msg_in);
		msg_in = NULL;
	}
	if (msg_out) {
		free(msg_out);
		msg_out = NULL;
	}
	cwmp.retry_count = 0;

	status = cwmp_handle_messages();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "cwmp_handle_messages failed\n");
#endif
		goto error;
	}

	status = http_client_exit();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "http_client_exit failed\n");
#endif
		goto error;
	}

	status = xml_exit();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "xml_exit failed\n");
#endif
		goto error;
	}

	if (cwmp.acs_connection_required) {
		cwmp.connection_request_t.cb = cwmp_inform;
		uloop_timeout_set(&cwmp.connection_request_t, 2000);
	}

	status = FC_SUCCESS;
	goto done;

error_xml:
	status = cwmp_reload_xml();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "cwmp_reload_xml failed\n");
#endif
		goto error;
	}
	
error_http:
	status = cwmp_reload_http_client();
	if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
		fprintf(stderr, "cwmp_reload_http_client failed\n");
#endif
		goto error;
	}
error:
	if (msg_in) {
		free(msg_in);
		msg_in = NULL;
	}
	if (msg_out) {
		free(msg_out);
		msg_out = NULL;
	}
	cwmp.acs_error_t.cb = cwmp_inform;
	if (cwmp.retry_count < 100) {
		cwmp.retry_count++;
		uloop_timeout_set(&cwmp.acs_error_t, 10000 * cwmp.retry_count);
	} else {
		/* try every 20 minutes */
		uloop_timeout_set(&cwmp.acs_error_t, 1200000);
	}
	goto done;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_handle_messages(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;
	char *msg_in, *msg_out;
	msg_in = msg_out = NULL;

	while (1) {

		if (msg_in) {
			free(msg_in);
			msg_in = NULL;
		}

		status = http_send_message(msg_out, &msg_in);
		if (status != FC_SUCCESS) {
#ifdef DEVEL_DEBUG
			fprintf(stderr, "cwmp_send_message failed\n");
#endif
			goto error;
		}

		if (!msg_in)
			break;

		if (msg_out) {
			free(msg_out);
			msg_out = NULL;
		}

		xml_handle_message(msg_in, &msg_out);
		if (!msg_out) {
#ifdef DEVEL_DEBUG
			fprintf(stderr, "xml_handle_message failed\n");
#endif
			goto error;
		}
	}

	/* checking should not be needed; but to be safe... */
	if (msg_in) {
		free(msg_in);
		msg_in = NULL;
	}
	if (msg_out) {
		free(msg_out);
		msg_out = NULL;
	}

	status = FC_SUCCESS;
	goto done;

error:
	if (msg_in)
		free(msg_in);
	if (msg_out)
		free(msg_out);
	status = FC_ERROR;
	goto done;


done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_connection_request(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	cwmp.event_code = CONNECTION_REQUEST;
	status = cwmp_inform();

	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_set_parameter_handler(char *name, char *value)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

#ifdef DEVEL_DEBUG
	printf("+++ CWMP HANDLE SET PARAMETER +++\n");
	printf("Name  : '%s'\n", name);
	printf("Value : '%s'\n", value);
	printf("--- CWMP HANDLE SET PARAMETER ---\n");
#endif

	if((strcmp(name, "InternetGatewayDevice.ManagementServer.Username")) == 0) {
		cwmp.acs_reload_required = 1;
	}

	if((strcmp(name, "InternetGatewayDevice.ManagementServer.Password")) == 0) {
		cwmp.acs_reload_required = 1;
	}

	if((strcmp(name, "InternetGatewayDevice.ManagementServer.URL")) == 0) {
		cwmp.event_code = VALUE_CHANGE;
		cwmp.acs_reload_required = 1;
		cwmp.acs_connection_required = 1; 
	}

	if((strcmp(name, "InternetGatewayDevice.ManagementServer.PeriodicInformEnable")) == 0) {
		cwmp.periodic_inform_enabled = atoi(value);
	}

	if((strcmp(name, "InternetGatewayDevice.ManagementServer.PeriodicInformInterval")) == 0) {
		cwmp.periodic_inform_interval = atoi(value);
		cwmp.periodic_inform_t.cb = cwmp_periodic_inform;
		uloop_timeout_set(&cwmp.periodic_inform_t, cwmp.periodic_inform_interval * 1000);
	}

	external_set_parameter(name, value);

	// TODO: check for errors

	goto done_ok;

done_ok:
	status = FC_SUCCESS;
	goto done;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_get_parameter_handler(char *name, char **value)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	status = external_get_parameter(name, value);

#ifdef DEVEL_DEBUG
	printf("+++ CWMP HANDLE GET PARAMETER +++\n");
	printf("Name  : '%s'\n", name);
	printf("Value : '%s'\n", *value);
	printf("--- CWMP HANDLE GET PARAMETER ---\n");
#endif

	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_download_handler(char *url, char *size)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	status = external_download(url, size);

#ifdef DEVEL_DEBUG
	printf("+++ CWMP HANDLE DOWNLOAD +++\n");
#endif

	FC_DEVEL_DEBUG("exit");
	return status;
}


int8_t
cwmp_reboot_handler(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	status = external_simple("reboot");

#ifdef DEVEL_DEBUG
	printf("+++ CWMP HANDLE REBOOT +++\n");
#endif

	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_factory_reset_handler(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	status = external_simple("factory_reset");

#ifdef DEVEL_DEBUG
	printf("+++ CWMP HANDLE FACTORY RESET +++\n");
#endif

	FC_DEVEL_DEBUG("exit");
	return status;
}

int8_t
cwmp_reload_changes(void)
{
	FC_DEVEL_DEBUG("enter");

	int8_t status;

	if (cwmp.acs_reload_required) {
		status = config_refresh_acs();
		if (status != FC_SUCCESS)
			goto done;
		cwmp.acs_reload_required = 0;
	}

	status = FC_SUCCESS;

done:
	FC_DEVEL_DEBUG("exit");
	return status;
}

char *
cwmp_get_event_code(void)
{
	FC_DEVEL_DEBUG("enter");

	char *cwmp_inform_event_code;
	switch (cwmp.event_code) {
		case BOOT:
			cwmp_inform_event_code = "1 BOOT";
			break;
		case PERIODIC:
			cwmp_inform_event_code = "2 PERIODIC";
			break;
		case SCHEDULED:
			cwmp_inform_event_code = "3 SCHEDULED";
			break;
		case VALUE_CHANGE:
			cwmp_inform_event_code = "4 VALUE CHANGE";
			break;
		case CONNECTION_REQUEST:
			cwmp_inform_event_code = "6 CONNECTION REQUEST";
			break;
		case BOOTSTRAP:
		default:
			cwmp_inform_event_code = "0 BOOTSTRAP";
			break;
	}

	FC_DEVEL_DEBUG("exit");
	return cwmp_inform_event_code;
}

int
cwmp_get_retry_count(void)
{
	FC_DEVEL_DEBUG("enter & exit");
	return cwmp.retry_count;
}

