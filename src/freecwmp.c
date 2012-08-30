/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <libfreecwmp.h>
#include <libubox/uloop.h>

#include "freecwmp.h"
#include "config.h"
#include "cwmp/cwmp.h"
#include "ubus/ubus.h"

static void freecwmp_kickoff(struct uloop_timeout *);
static void freecwmp_do_reload(struct uloop_timeout *timeout);
static void netlink_new_msg(struct uloop_fd *ufd, unsigned events);

static struct uloop_fd netlink_event = { .cb = netlink_new_msg };
static struct uloop_timeout kickoff_timer = { .cb = freecwmp_kickoff };
static struct uloop_timeout reload_timer = { .cb = freecwmp_do_reload };

static void
print_help(void)
{
	printf("Usage: %s [OPTIONS]\n", NAME);
	printf(" -f, --foreground    Run in the foreground\n");
	printf(" -h, --help          Display this help text\n");
}

static void
freecwmp_kickoff(struct uloop_timeout *timeout)
{
	cwmp_exit();
	cwmp_init();
	if (ubus_init()) D("ubus initialization failed\n");
	cwmp_inform();
}

static void freecwmp_do_reload(struct uloop_timeout *timeout)
{
	config_load();
}

void freecwmp_reload(void)
{
	uloop_timeout_set(&reload_timer, 100);
}


static void
freecwmp_netlink_interface(struct nlmsghdr *nlh)
{
	struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
	struct rtattr *rth = IFA_RTA(ifa);
	int rtl = IFA_PAYLOAD(nlh);
	char if_name[IFNAMSIZ], if_addr[INET_ADDRSTRLEN];

	memset(&if_name, 0, sizeof(if_name));
	memset(&if_addr, 0, sizeof(if_addr));

	while (rtl && RTA_OK(rth, rtl)) {
		if (rth->rta_type != IFA_LOCAL) {
			rth = RTA_NEXT(rth, rtl);
			continue;
		}

		uint32_t addr = htonl(* (uint32_t *)RTA_DATA(rth));
		if (htonl(13) == 13) {
			// running on big endian system
		} else {
			// running on little endian system
			addr = __builtin_bswap32(addr);
		}

		if_indextoname(ifa->ifa_index, if_name);
		if (strncmp(config->local->interface, if_name, IFNAMSIZ)) {
			rth = RTA_NEXT(rth, rtl);
			continue;
		}

		inet_ntop(AF_INET, &(addr), if_addr, INET_ADDRSTRLEN);

		FREE(config->local->ip);
		config->local->ip = strdup(if_addr);
		break;
	}

	if (strlen(if_addr) == 0) return;

	freecwmp_log_message(NAME, L_NOTICE, "interface %s has ip %s\n", \
			     if_name, if_addr);
	uloop_timeout_set(&kickoff_timer, 2500);
}

static void
netlink_new_msg(struct uloop_fd *ufd, unsigned events)
{
	struct nlmsghdr *nlh;
	char buffer[BUFSIZ];
	int msg_size;

	memset(&buffer, 0, sizeof(buffer));

	nlh = (struct nlmsghdr *)buffer;
	if ((msg_size = recv(ufd->fd, nlh, BUFSIZ, 0)) == -1) {
		DD("error receiving netlink message");
		return;
	}

	while (msg_size > sizeof(*nlh)) {
		int len = nlh->nlmsg_len;
		int req_len = len - sizeof(*nlh);

		if (req_len < 0 || len > msg_size) {
			DD("error reading netlink message");
			return;
		}

		if (!NLMSG_OK(nlh, msg_size)) {
			DD("netlink message is not NLMSG_OK");
			return;
		}

		if (nlh->nlmsg_type == RTM_NEWADDR)
			freecwmp_netlink_interface(nlh);

		msg_size -= NLMSG_ALIGN(len);
		nlh = (struct nlmsghdr*)((char*)nlh + NLMSG_ALIGN(len));
	}
}

static int
netlink_init(void)
{
	struct {
		struct nlmsghdr hdr;
		struct ifaddrmsg msg;
	} req;
	struct sockaddr_nl addr;
	int sock[2];

	memset(&addr, 0, sizeof(addr));
	memset(&req, 0, sizeof(req));

	if ((sock[0] = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {
		D("couldn't open NETLINK_ROUTE socket");
		return -1;
	}

	addr.nl_family = AF_NETLINK;
	addr.nl_groups = RTMGRP_IPV4_IFADDR;
	if ((bind(sock[0], (struct sockaddr *)&addr, sizeof(addr))) == -1) {
		D("couldn't bind netlink socket");
		return -1;
	}

	netlink_event.fd = sock[0];
	uloop_fd_add(&netlink_event, ULOOP_READ | ULOOP_EDGE_TRIGGER);

	if ((sock[1] = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) == -1) {
		D("couldn't open NETLINK_ROUTE socket");
		return -1;
	}

	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	req.hdr.nlmsg_type = RTM_GETADDR;
	req.msg.ifa_family = AF_INET;

	if ((send(sock[1], &req, req.hdr.nlmsg_len, 0)) == -1) {
		D("couldn't send netlink socket");
		return -1;
	}

	struct uloop_fd dummy_event = { .fd = sock[1] };
	netlink_new_msg(&dummy_event, 0);

	return 0;
}

int main (int argc, char **argv)
{
	freecwmp_log_message(NAME, L_NOTICE, "daemon started\n");

	bool foreground = false;

	setlocale(LC_CTYPE, "");
	umask(0037);

#ifndef DUMMY_MODE
	if (getuid() != 0) {
		D("run %s as root\n", NAME);
		exit(EXIT_FAILURE);
	}
#endif

	struct option long_opts[] = {
		{"foreground", no_argument, NULL, 'f'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	int c;
	while (1) {
		c = getopt_long(argc, argv, "fh", long_opts, NULL);
		if (c == EOF)
			break;
		switch (c) {
			case 'f':
				foreground = true;
				break;
			case 'h':
				print_help();
				exit(EXIT_FAILURE);
			default:
				fprintf(stderr, "error while parsing options\n");
				exit(EXIT_FAILURE);
		}
	}

	uloop_init();

	if (config_load()) {
		D("configuration initialization failed\n");
		exit(EXIT_FAILURE);
	}

	if (netlink_init()) {
		D("netlink initialization failed\n");
		exit(EXIT_FAILURE);
	}

	pid_t pid, sid;
	if (!foreground) {
		pid = fork();
		if (pid < 0)
			exit(EXIT_FAILURE);
		if (pid > 0)
			exit(EXIT_SUCCESS);

		sid = setsid();
		if (sid < 0) {
			D("setsid() returned error\n");
			exit(EXIT_FAILURE);
		}

#ifdef DUMMY_MODE
		char *directory = ".";
#else
		char *directory = "/";
#endif
		if ((chdir(directory)) < 0) {
			D("chdir() returned error\n");
			exit(EXIT_FAILURE);
		}

		close(STDIN_FILENO);
#ifndef DEBUG
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
#endif
	}

	freecwmp_log_message(NAME, L_NOTICE, "entering main loop\n");
	uloop_run();

	ubus_exit();
	uloop_done();
	
	closelog();

	freecwmp_log_message(NAME, L_NOTICE, "exiting\n");
	return 0;
}

