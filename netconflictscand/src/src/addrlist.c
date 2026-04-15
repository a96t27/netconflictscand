#include <addrlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <net/if.h>

uint32_t get_mask(uint32_t prefix)
{
        return ntohl(0xFFFFFFFF << (32 - prefix));
}

struct Address *create_address(uint32_t ip, uint32_t prefix, uint32_t ifa_index)
{
        if (prefix > 32) {
                return NULL;
        }
        struct Address *addr = NULL;
        addr = (struct Address *)malloc(sizeof(struct Address));
        if (addr == NULL) {
                return NULL;
        }
        addr->ifa_index = ifa_index;
        addr->prefix = prefix;
        addr->ip = ip;
        addr->next = NULL;
        return addr;
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

int are_addrs_equal(struct Address *a1, struct Address *a2)
{
        if (a1 == NULL || a2 == NULL) {
                return a1 == a2;
        }
        return a1->ip == a2->ip && a1->ifa_index == a2->ifa_index && a1->prefix == a2->prefix;
}

int del_addr(struct Address **list, struct Address *addr)
{
        if (list == NULL || addr == NULL || *list == NULL) {
                return EXIT_FAILURE;
        }

        struct Address *to_del = *list;
        if (are_addrs_equal(to_del, addr)) {
                *list = to_del->next;
                free(to_del);
                return EXIT_SUCCESS;
        }

        for (struct Address *temp = *list; temp->next != NULL; temp = temp->next) {
                to_del = temp->next;
                if (are_addrs_equal(to_del, addr)) {
                        temp->next = to_del->next;
                        free(to_del);
                        return EXIT_SUCCESS;
                }
        }
        return EXIT_FAILURE;
}

int are_overlapping_subnets(struct Address *a1, struct Address *a2)
{
        if (a1 == NULL || a2 == NULL) {
                return 0;
        }
        if (a1->ifa_index == a2->ifa_index) {
                return 0;
        }
        uint32_t mask1 = a1->prefix;
        uint32_t ip1 = a1->ip;
        uint32_t net1_start = ip1 & mask1;
        uint32_t net1_end = net1_start | (~mask1);

        uint32_t mask2 = a2->prefix;
        uint32_t ip2 = a2->ip;
        uint32_t net2_start = ip2 & mask2;
        uint32_t net2_end = net2_start | (~mask2);

        return (net1_start >= net2_start && net1_start <= net2_end)
                || (net1_end >= net2_start && net1_end <= net2_end);
}

struct Address *find_conflicts(struct Address **list, struct Address *addr)
{
        if (list == NULL || addr == NULL) {
                return NULL;
        }
        struct Address *conflict_list = NULL;

        for (struct Address *temp = *list; temp != NULL; temp = temp->next) {
                if (temp->ifa_index == addr->ifa_index) {
                        continue;
                }
                if (are_overlapping_subnets(temp, addr)) {
                        add_addr(&conflict_list, create_address(temp->ip, temp->prefix, temp->ifa_index));
                }
        }
        if (conflict_list != NULL) {
                add_addr(&conflict_list, create_address(addr->ip, addr->prefix, addr->ifa_index));
        }
        return conflict_list;
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