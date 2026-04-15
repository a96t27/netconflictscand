#include <linux/netlink.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <signal.h>
#include <addrlist.h>
#include <syslog.h>
#include <unistd.h>
#include <netlink_methods.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <ubus_methods.h>
#include <netlink_methods.h>
#include <fcntl.h>


int main(void)
{
	setlogmask(LOG_UPTO(LOG_DEBUG));

	openlog("netconflictscand", LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);
	syslog(LOG_INFO, "Starting netconflictscand process");

	int ret = EXIT_SUCCESS;
	struct sockaddr_nl sa = {
		.nl_family = AF_NETLINK,
		.nl_groups = RTMGRP_IPV4_IFADDR,
	};

	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		syslog(LOG_CRIT, "Failed to open a netlink socket");
		ret = EXIT_FAILURE;
		goto err_no_socket;
	}

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		syslog(LOG_CRIT, "Failed to bind the netlink socket");
		ret = EXIT_FAILURE;
		goto err_no_bind;
	}

	fcntl(fd, F_SETFL, O_NONBLOCK);


	struct Address *list = NULL;
	struct NetlinkContext nl_ctx = {
		.list = &list,
		.sa = &sa,
		.sa_size = sizeof(sa),
		.fd = fd,
	};

	if (uloop_init() != EXIT_SUCCESS) {
		syslog(LOG_CRIT, "Failed to initiate ubus context");
		goto err_no_uloop_init;
	}
	struct ubus_context *ubus_ctx = ubus_connect(NULL);

	if (ubus_ctx == NULL) {
		syslog(LOG_CRIT, "Failed to connect to ubusd");
		goto err_no_ubus_ctx;
	}

	ubus_add_uloop(ubus_ctx);

	main_loop(ubus_ctx, &nl_ctx);

	ubus_free(ubus_ctx);
err_no_ubus_ctx:
	uloop_done();
err_no_uloop_init:
	free_addr_list(&list);
err_no_bind:
	close(fd);
err_no_socket:

	syslog(LOG_INFO, "Netconflictscand process finished");
	return ret;
}
