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

static int running = 1;

int get_addr(struct nlmsghdr *nh, struct Address **addr)
{
	if (nh == NULL || addr == NULL) {
		return EXIT_FAILURE;
	}

	struct ifaddrmsg *ifa = NLMSG_DATA(nh);

	if (ifa->ifa_family == AF_INET6 || ifa->ifa_scope == RT_SCOPE_HOST) {
		return EXIT_FAILURE;
	}

	struct rtattr *rth = IFA_RTA(ifa);
	int len = IFA_PAYLOAD(nh);

	uint32_t ip = 0;

	for (; RTA_OK(rth, len); rth = RTA_NEXT(rth, len)) {
		switch (rth->rta_type) {
		case IFA_LOCAL:
			memcpy(&ip, RTA_DATA(rth), sizeof(ip));
			break;
		case IFA_ADDRESS:
			if (!ip) {
				memcpy(&ip, RTA_DATA(rth), sizeof(ip));
			}
			break;
		}
	}
	*addr = create_address(ip, ifa->ifa_prefixlen, ifa->ifa_index);
	if (addr == NULL) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void print_conflict_msg(int ifa_index1, int ifa_index2)
{
	char label1[IF_NAMESIZE] = { 0 };
	if_indextoname(ifa_index1, label1);
	char label2[IF_NAMESIZE] = { 0 };
	if_indextoname(ifa_index2, label2);
	printf("WARNING!!! found conflict between %.*s and %.*s interfaces\n", IF_NAMESIZE, label1, IF_NAMESIZE, label2);
}

int handle_newaddr(struct nlmsghdr *nh, struct Address **list)
{
	struct Address *addr = NULL;
	if (get_addr(nh, &addr) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}
	int ifa_index;
	if ((ifa_index = find_conflict(list, addr)) >= 0) {
		print_conflict_msg(addr->ifa_index, ifa_index);
	}
	return add_addr(list, addr);
}

int handle_deladdr(struct nlmsghdr *nh, struct Address **list)
{
	struct Address *addr = NULL;
	if (get_addr(nh, &addr) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}
	int ret = del_addr(list, addr);
	free(addr);
	return ret;
}


void sig_handler(int sigint)
{
	(void)sigint;
	running = 0;
}

void request_addrs(int fd, int sequence_number)
{
	if (!running) {
		return;
	}
	struct {
		struct nlmsghdr nh;
		struct ifaddrmsg ifa;
	} req_addr;

	memset(&req_addr, 0, sizeof(req_addr));
	req_addr.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req_addr.nh.nlmsg_type = RTM_GETADDR;
	req_addr.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req_addr.nh.nlmsg_seq = sequence_number;
	req_addr.ifa.ifa_family = AF_INET;

	send(fd, &req_addr, req_addr.nh.nlmsg_len, 0);
}

void receive(struct sockaddr_nl *sa, int sa_size, int fd, struct Address **list)
{

	char buf[8192];
	struct nlmsghdr *nh;
	while (running) {

		struct iovec iov = { buf, sizeof(buf) };
		struct msghdr msg = { sa, sa_size, &iov, 1, NULL, 0, 0 };

		int size = recvmsg(fd, &msg, 0);
		if (size < 0) {
			printf("Failed to receive message");
			continue;
		}

		if (msg.msg_flags & MSG_TRUNC) {
			printf("Truncated message\n");
			continue;
		}

		for (nh = (struct nlmsghdr *)buf; NLMSG_OK(nh, size);
			nh = NLMSG_NEXT(nh, size)) {

			switch (nh->nlmsg_type) {

			case NLMSG_DONE:
				// printf("done\n");
				return;

			case NLMSG_ERROR: {
				struct nlmsgerr *err = NLMSG_DATA(nh);
				if (err->error == 0) {
					printf("ack\n");
				} else {
					printf("netlink error: %d\n", err->error);
				}
				return;
			}

			case RTM_DELADDR:
				if (handle_deladdr(nh, list) == EXIT_SUCCESS) {
					printf("Address deleted. Current table:\n");
					print_addrs(list);
				}
				break;

			case RTM_NEWADDR:
				if (handle_newaddr(nh, list) == EXIT_SUCCESS) {
					printf("New address added. Current table:\n");
					print_addrs(list);
				}
				break;
			}
		}
	}
}

int main(void)
{
	struct sockaddr_nl sa;

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_IPV4_IFADDR;

	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("socket");
		return 1;
	}

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("bind");
		return 1;
	}
	struct Address *list = NULL;
	signal(SIGINT, sig_handler);
	int sequence_number = 0;
	request_addrs(fd, ++sequence_number);
	receive(&sa, sizeof(sa), fd, &list);
	receive(&sa, sizeof(sa), fd, &list);

	free_addr_list(&list);

	return 0;
}
