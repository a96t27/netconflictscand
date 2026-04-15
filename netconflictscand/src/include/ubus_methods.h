#ifndef UBUS_NOTIFY
#define UBUS_NOTIFY
#include <libubus.h>

struct NetlinkContext {
        int fd;
        struct Address **list;
        struct sockaddr_nl *sa;
        int sa_size;
};

void main_loop(struct ubus_context *ubus_ctx, struct NetlinkContext *nl_ctx);

#endif