#ifndef NETLINK_METHODS_H
#define NETLINK_METHODS_H
#include <linux/netlink.h>
#include <asm/types.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <addrlist.h>


int get_addr(struct nlmsghdr *nh, struct Address **addr);
void log_conflict_msg(int ifa_index1, int ifa_index2);
int handle_newaddr(struct nlmsghdr *nh, struct Address **list);
int handle_deladdr(struct nlmsghdr *nh, struct Address **list);
void sig_handler(int sigint);
void request_addrs(int fd, int sequence_number);
void receive(struct sockaddr_nl *sa, int sa_size, int fd, struct Address **list);

#endif