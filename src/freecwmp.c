/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <libubox/uloop.h>

#include "freecwmp.h"
#include "config.h"
#include "cwmp/cwmp.h"
#include "cwmp/local.h"

static void freecwmp_kickoff(struct uloop_timeout *);

static void netlink_new_msg(struct uloop_fd *ufd, unsigned events);
static struct uloop_fd netlink_event = { .cb = netlink_new_msg };
static struct uloop_timeout freecwmp_kickoff_t = { .cb = freecwmp_kickoff};

static void
print_help(void) {
	printf("Usage: %s [OPTIONS]\n", FREECWMP_NAME);
	printf(" -f, --foreground    Run in the foreground\n");
	printf(" -h, --help          Display this help text\n");
}

static void
freecwmp_kickoff(struct uloop_timeout *timeout)
{
	FC_DEVEL_DEBUG("enter");
	int8_t status;

	status = cwmp_exit();
	if (status != FC_SUCCESS)
		fprintf(stderr, "freecwmp cleaning failed\n");

	status = config_reload();
	if (status != FC_SUCCESS)
		fprintf(stderr, "configuration reload failed\n");

	status = cwmp_init();
	if (status != FC_SUCCESS)
		fprintf(stderr, "freecwmp initialization failed\n");

	cwmp_inform();

	FC_DEVEL_DEBUG("exit");
}

static void
freecwmp_netlink_interface(struct nlmsghdr *nlh)
{
	FC_DEVEL_DEBUG("enter");
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
		if (strncmp(local_get_interface(), if_name, IFNAMSIZ)) {
			rth = RTA_NEXT(rth, rtl);
			continue;
		}

		inet_ntop(AF_INET, &(addr), if_addr, INET_ADDRSTRLEN);

		local_set_ip(if_addr);
		break;
	}

	if (strlen(if_addr) == 0) goto done;

#ifdef DEVEL_DEBUG
	char buffer[BUFSIZ];
	memset(&buffer, 0, sizeof(buffer));
	snprintf(buffer, BUFSIZ - 1, "interface %s has ip %s", if_name, if_addr);
	FC_DEVEL_DEBUG(buffer);
#endif

	uloop_timeout_set(&freecwmp_kickoff_t, 3 * 1000);
done:
	FC_DEVEL_DEBUG("exit");
}

static void
netlink_new_msg(struct uloop_fd *ufd, unsigned events)
{
	FC_DEVEL_DEBUG("enter");
	struct nlmsghdr *nlh;
	char buffer[BUFSIZ];
	int msg_size;

	memset(&buffer, 0, sizeof(buffer));

	nlh = (struct nlmsghdr *)buffer;
	if ((msg_size = recv(ufd->fd, nlh, BUFSIZ, 0)) == -1) {
		FC_DEVEL_DEBUG("error receiving netlink message");
		goto netlink_new_msg_done;
	}

	while (msg_size > sizeof(*nlh)) {
		int len = nlh->nlmsg_len;
		int req_len = len - sizeof(*nlh);

		if (req_len < 0 || len > msg_size) {
			FC_DEVEL_DEBUG("error reading netlink message");
			goto netlink_new_msg_done;
		}

		if (!NLMSG_OK(nlh, msg_size)) {
			FC_DEVEL_DEBUG("netlink message is not NLMSG_OK");
			goto netlink_new_msg_done;
		}

		if (nlh->nlmsg_type == RTM_NEWADDR)
			freecwmp_netlink_interface(nlh);

		msg_size -= NLMSG_ALIGN(len);
		nlh = (struct nlmsghdr*)((char*)nlh + NLMSG_ALIGN(len));
	}

netlink_new_msg_done:
	;
	FC_DEVEL_DEBUG("exit");
}

static int8_t
netlink_init(void)
{
	FC_DEVEL_DEBUG("enter");
	struct {
		struct nlmsghdr hdr;
		struct ifaddrmsg msg;
	} req;
	struct sockaddr_nl addr;
	int sock[2];

	memset(&addr, 0, sizeof(addr));
	memset(&req, 0, sizeof(req));

	if ((sock[0] = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {
		FC_DEVEL_DEBUG("couldn't open NETLINK_ROUTE socket");
		return FC_ERROR;
	}

	addr.nl_family = AF_NETLINK;
	addr.nl_groups = RTMGRP_IPV4_IFADDR;
	if ((bind(sock[0], (struct sockaddr *)&addr, sizeof(addr))) == -1) {
		FC_DEVEL_DEBUG("couldn't bind netlink socket");
		return FC_ERROR;
	}

	netlink_event.fd = sock[0];
	uloop_fd_add(&netlink_event, ULOOP_READ | ULOOP_EDGE_TRIGGER);

	if ((sock[1] = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) == -1) {
		FC_DEVEL_DEBUG("couldn't open NETLINK_ROUTE socket");
		return FC_ERROR;
	}

	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	req.hdr.nlmsg_type = RTM_GETADDR;
	req.msg.ifa_family = AF_INET;

	if ((send(sock[1], &req, req.hdr.nlmsg_len, 0)) == -1) {
		FC_DEVEL_DEBUG("couldn't send netlink socket");
		return FC_ERROR;
	}

	struct uloop_fd dummy_event = { .fd = sock[1] };
	netlink_new_msg(&dummy_event, 0);

	FC_DEVEL_DEBUG("exit");
	return FC_SUCCESS;
}

int main (int argc, char **argv)
{
	FC_DEVEL_DEBUG("enter");

	setlocale(LC_CTYPE, "");
	umask(0037);

#ifndef DUMMY_MODE
	if (getuid() != 0) {
		fprintf(stderr, "run %s as root\n", FREECWMP_NAME);
		exit(EXIT_FAILURE);
	}
#endif

	struct option long_opts[] = {
		{"foreground", no_argument, NULL, 'f'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};


	uint8_t foreground = 0;

	int c;
	while (1) {
		c = getopt_long(argc, argv, "fh", long_opts, NULL);
		if (c == EOF)
			break;
		switch (c) {
			case 'f':
				foreground = 1;
				break;
			case 'h':
				print_help();
				exit(EXIT_FAILURE);
			default:
				fprintf(stderr, "error while parsing options\n");
				exit(EXIT_FAILURE);
		}
	}

	int8_t status;
	uloop_init();

	status = config_init_all();
	if (status != FC_SUCCESS) {
		fprintf(stderr, "configuration initialization failed\n");
		return EXIT_FAILURE;
	}

	status = netlink_init();
	if (status != FC_SUCCESS) {
		fprintf(stderr, "netlink initialization failed\n");
		return EXIT_FAILURE;
	}

	pid_t pid, sid;
	if (!foreground) {
		pid = fork();
		if (pid < 0)
			return EXIT_FAILURE;
		if (pid > 0)
			return EXIT_SUCCESS;

		sid = setsid();
		if (sid < 0) {
			fprintf(stderr, "setsid() function returned error\n");
			return EXIT_FAILURE;
		}

#ifdef DUMMY_MODE
		char *directory = ".";
#else
		char *directory = "/";
#endif
		if ((chdir(directory)) < 0) {
			fprintf(stderr, "chdir() function returned error\n");
			return EXIT_FAILURE;
		}

		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

	uloop_run();
}

