#ifndef NETLIST
#define NETLIST
#include <stdint.h>


struct Address {
        uint32_t ip;
        uint32_t local;
        uint32_t ifa_index;
        uint32_t prefix;
        struct Address *next;
};

uint32_t get_mask(uint32_t prefix);
struct Address *create_address(uint32_t ip, uint32_t prefix, uint32_t ifa_index);
void print_addrs(struct Address **list);
int add_addr(struct Address **list, struct Address *addr);
int del_addr(struct Address **list, struct Address *addr);
int find_conflict(struct Address **list, struct Address *addr);
void free_addr_list(struct Address **list);

#endif