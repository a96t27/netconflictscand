#ifndef NETLINK_METHODS_H
#define NETLINK_METHODS_H
#include <linux/netlink.h>
#include <asm/types.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <addrlist.h>

enum {
        EXIT_SUBNET_CONFLICT = EXIT_FAILURE + 1,
        EXIT_UNSUPPORTED_NETWORK_TYPE,
};

int get_addr(struct nlmsghdr *nh, struct Address **addr);
int handle_newaddr(struct nlmsghdr *nh, struct Address **list, struct Address **conflicts);
int handle_deladdr(struct nlmsghdr *nh, struct Address **list);
void request_addrs(int fd, int sequence_number);
int receive_netlink_events(struct sockaddr_nl *sa, int sa_size, int fd, struct Address **list, struct Address **conflicts);

#endif