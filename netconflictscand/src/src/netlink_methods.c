#include <netlink_methods.h>
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


int get_addr(struct nlmsghdr *nh, struct Address **addr)
{
        if (nh == NULL || addr == NULL) {
                return EXIT_FAILURE;
        }

        struct ifaddrmsg *ifa = NLMSG_DATA(nh);

        if (ifa->ifa_family == AF_INET6 || ifa->ifa_scope == RT_SCOPE_HOST) {
                return EXIT_UNSUPPORTED_NETWORK_TYPE;
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


int handle_newaddr(struct nlmsghdr *nh, struct Address **list, struct Address **conflicts)
{
        if (nh == NULL || list == NULL || conflicts == NULL) {
                return EXIT_FAILURE;
        }
        struct Address *addr = NULL;
        if (get_addr(nh, &addr) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
        }
        *conflicts = find_conflicts(list, addr);

        if (add_addr(list, addr) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
        }

        if (conflicts != NULL) {
                return EXIT_SUBNET_CONFLICT;
        }
        return EXIT_SUCCESS;
}

int handle_deladdr(struct nlmsghdr *nh, struct Address **list)
{
        if (nh == NULL || list == NULL) {
                return EXIT_FAILURE;
        }
        struct Address *addr = NULL;
        if (get_addr(nh, &addr) != EXIT_SUCCESS || addr == NULL) {
                return EXIT_FAILURE;
        }
        int ret = del_addr(list, addr);
        free(addr);
        return ret;
}


void request_addrs(int fd, int sequence_number)
{
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

int receive_netlink_events(struct sockaddr_nl *sa, int sa_size, int fd, struct Address **list, struct Address **conflicts)
{
        if (sa == NULL || sa_size < 1 || fd < 0 || list == NULL || conflicts == NULL) {
                return EXIT_FAILURE;
        }
        *conflicts = NULL;
        char buf[8192];
        struct nlmsghdr *nh;
        struct iovec iov = { buf, sizeof(buf) };
        struct msghdr msg = { sa, sa_size, &iov, 1, NULL, 0, 0 };
        int size = recvmsg(fd, &msg, 0);
        int ret = EXIT_SUCCESS;
        if (size < 0) {
                syslog(LOG_ERR, "Failed to receive message");
                return EXIT_FAILURE;
        }

        if (msg.msg_flags & MSG_TRUNC) {
                syslog(LOG_DEBUG, "Truncated message");
                return EXIT_FAILURE;
        }

        for (nh = (struct nlmsghdr *)buf; NLMSG_OK(nh, size);
                nh = NLMSG_NEXT(nh, size)) {

                switch (nh->nlmsg_type) {

                case NLMSG_DONE:
                        return EXIT_SUCCESS;

                case NLMSG_ERROR: {
                        struct nlmsgerr *err = NLMSG_DATA(nh);
                        if (err->error != 0) {
                                syslog(LOG_ERR, "Netlink error: %d\n", err->error);
                        }
                        return EXIT_FAILURE;
                }

                case RTM_DELADDR:
                        ret = handle_deladdr(nh, list);
                        if (ret == EXIT_SUCCESS) {
                                syslog(LOG_DEBUG, "Address removed");
                        } else {
                                syslog(LOG_ERR, "Failed to handle RTM_DELADDR message");
                        }
                        break;

                case RTM_NEWADDR:
                        ret = handle_newaddr(nh, list, conflicts);
                        switch (ret) {
                        case EXIT_SUCCESS:
                                syslog(LOG_DEBUG, "New address added");
                                break;
                        case EXIT_SUBNET_CONFLICT:
                                syslog(LOG_ERR, "Found conflicting subnet");
                                break;
                        default:
                                syslog(LOG_ERR, "Failed to add new addres");
                                break;
                        }
                        break;
                }
        }
        return ret;
}
