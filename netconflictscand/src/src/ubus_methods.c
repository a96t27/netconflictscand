#include <ubus_methods.h>
#include <libubus.h>
#include <netlink_methods.h>
#include <net/if.h>
#include <syslog.h>

struct NetlinkContext *nl_context = NULL;
struct ubus_context *u_context = NULL;

static int add_addr_list_to_buf(struct blob_buf *buf, struct Address *addrs)
{
        if (buf == NULL) {
                return EXIT_FAILURE;
        }
        void *array_cookie = blobmsg_open_array(buf, "subnets");
        void *device_cookie = NULL;
        char ipbuf[INET_ADDRSTRLEN] = { 0 };
        char label[IF_NAMESIZE] = { 0 };
        int mask = 0;
        int net = 0;
        for (struct Address *temp = addrs; temp != NULL; temp = temp->next) {
                device_cookie = blobmsg_open_table(buf, "");
                mask = get_mask(temp->prefix);
                net = temp->ip & mask;
                inet_ntop(AF_INET, &net, ipbuf, sizeof(ipbuf));
                blobmsg_add_string(buf, "net", ipbuf);
                inet_ntop(AF_INET, &mask, ipbuf, sizeof(ipbuf));
                blobmsg_add_string(buf, "mask", ipbuf);
                if_indextoname(temp->ifa_index, label);
                blobmsg_add_string(buf, "interface", label);
                blobmsg_close_table(buf, device_cookie);
        }
        blobmsg_close_array(buf, array_cookie);
        return EXIT_SUCCESS;
}

static int state_cb(struct ubus_context *ctx, struct ubus_object *obj, struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
        (void)obj;
        (void)method;
        (void)msg;
        struct blob_buf buf = { 0 };
        blobmsg_buf_init(&buf);
        blobmsg_add_u8(&buf, "success", 1);
        blobmsg_add_string(&buf, "msg", "got current subnets");
        void *data_cookie = blobmsg_open_table(&buf, "data");
        add_addr_list_to_buf(&buf, *(nl_context->list));
        blobmsg_close_table(&buf, data_cookie);
        ubus_send_reply(ctx, req, buf.head);
        blob_buf_free(&buf);
        return UBUS_STATUS_OK;
}

static const struct ubus_method netconflictscand_methods[] = {
        UBUS_METHOD_NOARG("state", state_cb),
};

static struct ubus_object_type netconflictscand_object_type = UBUS_OBJECT_TYPE("netconflictscand", netconflictscand_methods);

static struct ubus_object netconflictscand_object = {
        .name = "netconflictscand",
        .type = &netconflictscand_object_type,
        .methods = netconflictscand_methods,
        .n_methods = ARRAY_SIZE(netconflictscand_methods),
};



static int send_conflict_event(struct Address *conflict)
{
        if (conflict == NULL) {
                return EXIT_FAILURE;
        }
        struct blob_buf buf = { 0 };
        blobmsg_buf_init(&buf);
        blobmsg_add_u8(&buf, "success", 0);
        blobmsg_add_string(&buf, "msg", "found conflicting subnets");
        void *data_cookie = blobmsg_open_table(&buf, "data");
        add_addr_list_to_buf(&buf, conflict);
        blobmsg_close_table(&buf, data_cookie);
        ubus_notify(u_context, &netconflictscand_object, "subnet_conflict", buf.head, 3000);
        blob_buf_free(&buf);
        return EXIT_SUCCESS;
}

static void nl_socket_read_cb(struct uloop_fd *u, unsigned int events)
{
        (void)events;
        if (nl_context == NULL) {
                return;
        }
        struct Address *conflicts = NULL;
        if (receive_netlink_events(nl_context->sa, nl_context->sa_size, u->fd, nl_context->list, &conflicts) == EXIT_SUBNET_CONFLICT) {
                send_conflict_event(conflicts);
                free_addr_list(&conflicts);
        }
}

void main_loop(struct ubus_context *ubus_ctx, struct NetlinkContext *nl_ctx)
{
        ubus_add_object(ubus_ctx, &netconflictscand_object);
        u_context = ubus_ctx;
        nl_context = nl_ctx;
        struct uloop_fd ufd = {
                .fd = nl_ctx->fd,
                .cb = nl_socket_read_cb,
        };
        uloop_fd_add(&ufd, ULOOP_READ);

        request_addrs(nl_ctx->fd, 1);
        uloop_run();
}