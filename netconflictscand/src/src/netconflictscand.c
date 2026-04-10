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


int main(void)
{
	setlogmask(LOG_UPTO(LOG_DEBUG));
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGABRT, SIG_IGN);
	signal(SIGILL, SIG_IGN);
	signal(SIGFPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGIO, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);

	openlog("netconflictscand", LOG_CONS | LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);
	syslog(LOG_INFO, "Starting netconflictscand process");

	int ret = EXIT_SUCCESS;
	struct sockaddr_nl sa = { 0 };
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_IPV4_IFADDR;

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
	struct Address *list = NULL;

	int sequence_number = 0;
	request_addrs(fd, ++sequence_number);
	receive(&sa, sizeof(sa), fd, &list);

	receive(&sa, sizeof(sa), fd, &list);

	free_addr_list(&list);
err_no_bind:
	close(fd);
err_no_socket:

	syslog(LOG_INFO, "Netconflictscand process finished");
	return ret;
}
