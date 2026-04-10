#include <addrlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <net/if.h>

uint32_t get_mask(uint32_t prefix)
{
        return 0xFFFFFFFF << (32 - prefix);
}

struct Address *create_address(uint32_t ip, uint32_t prefix, uint32_t ifa_index)
{
        if (prefix > 32) {
                prefix = 0;
        }
        struct Address *addr = NULL;
        addr = (struct Address *)malloc(sizeof(struct Address));
        addr->ifa_index = ifa_index;
        addr->prefix = prefix;
        addr->ip = ip;
        addr->next = NULL;
        return addr;
}

void print_addrs(struct Address **list)
{
        if (list == NULL) {
                return;
        }
        printf("\n");
        uint32_t mask = 0;
        char ipbuf[INET_ADDRSTRLEN] = { 0 };
        char maskbuf[INET_ADDRSTRLEN] = { 0 };
        for (struct Address *cur = *list; cur != NULL; cur = cur->next) {
                mask = ntohl(get_mask(cur->prefix));
                inet_ntop(AF_INET, &cur->ip, ipbuf, sizeof(ipbuf));
                inet_ntop(AF_INET, &mask, maskbuf, sizeof(maskbuf));
                char label[IF_NAMESIZE] = { 0 };
                if_indextoname(cur->ifa_index, label);
                printf("if:%.*s(%x) ip:%.*s mask:%.*s\n", IF_NAMESIZE, label, cur->ifa_index, INET_ADDRSTRLEN, ipbuf, INET_ADDRSTRLEN, maskbuf);
        }
        printf("\n");
}


int add_addr(struct Address **list, struct Address *addr)
{
        if (list == NULL || addr == NULL) {
                return EXIT_FAILURE;
        }

        addr->next = *list;
        *list = addr;

        return EXIT_SUCCESS;
}


int del_addr(struct Address **list, struct Address *addr)
{
        if (list == NULL || addr == NULL) {
                return EXIT_FAILURE;
        }
        if (*list == NULL) {
                return EXIT_FAILURE;
        }

        struct Address *to_del = *list;
        if (to_del->ip == addr->ip && to_del->ifa_index == addr->ifa_index && to_del->prefix == addr->prefix) {
                *list = to_del->next;
                free(to_del);
                return EXIT_SUCCESS;
        }

        for (struct Address *temp = *list; temp->next != NULL; temp = temp->next) {
                to_del = temp->next;
                if (to_del->ip == addr->ip && to_del->ifa_index == addr->ifa_index && to_del->prefix == addr->prefix) {
                        temp->next = to_del->next;
                        free(to_del);
                        return EXIT_SUCCESS;
                }
        }
        return EXIT_FAILURE;
}


int find_conflict(struct Address **list, struct Address *addr)
{
        if (list == NULL || addr == NULL) {
                return -1;
        }
        for (struct Address *temp = *list; temp != NULL; temp = temp->next) {
                if (temp->ifa_index == addr->ifa_index) {
                        continue;
                }
                uint32_t mask1 = get_mask(addr->prefix);
                uint32_t ip1 = ntohl(addr->ip);
                uint32_t net1_start = ip1 & mask1;
                uint32_t net1_end = net1_start | (~mask1);

                uint32_t mask2 = get_mask(temp->prefix);
                uint32_t ip2 = ntohl(temp->ip);
                uint32_t net2_start = ip2 & mask2;
                uint32_t net2_end = net2_start | (~mask2);

                if ((net1_start >= net2_start && net1_start <= net2_end) || (net1_end >= net2_start && net1_end <= net2_end)) {
                        return temp->ifa_index;
                }
        }
        return -1;
}

void free_addr_list(struct Address **list)
{
        if (list == NULL || *list == NULL) {
                return;
        }
        struct Address *to_del;
        struct Address *temp = *list;
        while (temp != NULL) {
                to_del = temp;
                temp = temp->next;
                free(to_del);
        }
        *list = NULL;
}