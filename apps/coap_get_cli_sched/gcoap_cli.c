/*
 * Copyright (c) 2015-2017 Ken Bannister. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       gcoap CLI support
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 *
 * @}
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "net/gcoap.h"
#include "od.h"
#include "fmt.h"
#include "random.h"
#include "xtimer.h"
#include "net/netopt.h"
#include "pktcnt.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/ipv6/nib/ft.h"
#include "ps.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define I3_PORT     "5683"
#define I3_PATH     "/i3/gasval"
#ifndef I3_MIN_WAIT
#define I3_MIN_WAIT (1000)
#endif
#ifndef I3_MAX_WAIT
#define I3_MAX_WAIT (1000)
#endif
#ifndef I3_MAX_REQ
#define I3_MAX_REQ      (3600U)
#endif
#ifndef I3_MAX_SERVER
#define I3_MAX_SERVER   (1U)
#endif
#ifndef I3_SEND_MSG_TYPE
#define I3_SEND_MSG_TYPE    (0x3475)
#endif

#define REQ_GEN_STACK_SIZE (THREAD_STACKSIZE_MAIN)
#define REQ_GEN_PRIO       (THREAD_PRIORITY_MAIN)

static char req_gen_stack[REQ_GEN_STACK_SIZE];

#ifdef MODULE_PKTCNT_FAST
extern char pktcnt_addr_str[17];
#endif

/*
 * Response callback.
 */
static void _resp_handler(unsigned req_state, coap_pkt_t* pdu,
                          sock_udp_ep_t *remote)
{
    (void)remote;       /* not interested in the source currently */

    if (req_state == GCOAP_MEMO_TIMEOUT) {
        /* printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu)); */
        return;
    }
    else if (req_state == GCOAP_MEMO_ERR) {
        /* printf("gcoap: error in response\n"); */
        return;
    }

    /* char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS) */
    /*                         ? "Success" : "Error"; */
    /* printf("gcoap: response %s, code %1u.%02u", class_str, */
    /*                                             coap_get_code_class(pdu), */
    /*                                             coap_get_code_detail(pdu)); */
#ifdef MODULE_PKTCNT_FAST
    printf("%1u.%02u;%u-%s\n",
           coap_get_code_class(pdu),
           coap_get_code_detail(pdu),
           coap_get_id(pdu),
           pktcnt_addr_str);
#endif
    if (pdu->payload_len) {
        if (pdu->content_type == COAP_FORMAT_TEXT
                || pdu->content_type == COAP_FORMAT_LINK
                || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE
                || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE) {
            /* Expecting diagnostic payload in failure cases */
            /* printf(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len, */
            /*                                               (char *)pdu->payload); */
        }
        else {
            /* printf(", %u bytes\n", pdu->payload_len); */
            /* od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT); */
        }
    }
    else {
        /* printf(", empty payload\n"); */
    }
}

static size_t _send(uint8_t *buf, size_t len, ipv6_addr_t *addr, char *port_str)
{
    size_t bytes_sent;
    sock_udp_ep_t remote;

    remote.family = AF_INET6;

    memcpy(&remote.addr.ipv6[0], &addr->u8[0], sizeof(addr->u8));

    /* parse port */
    remote.port = atoi(port_str);
    if (remote.port == 0) {
        /* puts("gcoap_cli: unable to parse destination port"); */
        return 0;
    }

    bytes_sent = gcoap_req_send2(buf, len, &remote, _resp_handler);
    return bytes_sent;
}

int gcoap_cli_cmd(int argc, char **argv)
{
    /* Ordered like the RFC method code numbers, but off by 1. GET is code 0. */
    char *method_codes[] = {"get", "post", "put"};
    uint8_t buf[GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;
    size_t len;

    if (argc == 1) {
        /* show help for main commands */
        goto end;
    }

    if (strcmp(argv[1], "info") == 0) {
        /* uint8_t open_reqs = gcoap_op_state(); */

        /* printf("CoAP server is listening on port %u\n", GCOAP_PORT); */
        /* printf("CoAP open requests: %u\n", open_reqs); */
        return 0;
    }

    /* if not 'info', must be a method code */
    int code_pos = -1;
    for (size_t i = 0; i < sizeof(method_codes) / sizeof(char*); i++) {
        if (strcmp(argv[1], method_codes[i]) == 0) {
            code_pos = i;
        }
    }
    if (code_pos == -1) {
        goto end;
    }

    /* parse options */
    int apos          = 2;               /* position of address argument */
    unsigned msg_type = COAP_TYPE_NON;
    if (argc > apos && strcmp(argv[apos], "-c") == 0) {
        msg_type = COAP_TYPE_CON;
        /* puts("CONFIRMABLE"); */
        apos++;
    }

    if (argc == apos + 3 || argc == apos + 4) {
        gcoap_req_init(&pdu, &buf[0], GCOAP_PDU_BUF_SIZE, code_pos+1, argv[apos+2]);
        if (argc == apos + 4) {
            memcpy(pdu.payload, argv[apos+3], strlen(argv[apos+3]));
        }
        coap_hdr_set_type(pdu.hdr, msg_type);

        if (argc == apos + 4) {
            len = gcoap_finish(&pdu, strlen(argv[apos+3]), COAP_FORMAT_TEXT);
        }
        else {
            len = gcoap_finish(&pdu, 0, COAP_FORMAT_NONE);
        }

        /* printf("gcoap_cli: sending msg ID %u, %u bytes\n", coap_get_id(&pdu), */
        /*        (unsigned) len); */
#ifdef MODULE_PKTCNT_FAST
        printf("%1u.%02u;%u-%s\n",
               coap_get_code_class(&pdu),
               coap_get_code_detail(&pdu),
               coap_get_id(&pdu),
               pktcnt_addr_str);
#endif
        if (!_send(&buf[0], len, (ipv6_addr_t *)argv[apos], argv[apos+1])) {
            /* puts("gcoap_cli: msg send failed"); */
        }
        return 0;
    }
    else {
        /* printf("usage: %s <get|post|put> [-c] <addr>[%%iface] <port> <path> [data]\n", */
        /*        argv[0]); */
        /* printf("Options\n"); */
        /* printf("    -c  Send confirmably (defaults to non-confirmable)\n"); */
        return 1;
    }

    end:
    /* printf("usage: %s <get|post|put|info>\n", argv[0]); */
    return 1;
}

static evtimer_t req_timer;
typedef struct {
    ipv6_addr_t server;
    evtimer_msg_event_t event;
    unsigned req_count;
} _server_event_t;
static _server_event_t server_event[I3_MAX_SERVER];
static char *i3_args[] = {
    "coap",
    "get",
#ifdef I3_CONFIRMABLE
    "-c",
#endif
    NULL,
    I3_PORT,
    I3_PATH
};

static void _send_req(ipv6_addr_t *server) {
#ifdef I3_CONFIRMABLE
    i3_args[3] = (char *)server;
#else
    i3_args[2] = (char *)server;
#endif
    gcoap_cli_cmd(sizeof(i3_args) / sizeof(i3_args[0]), i3_args);
}

static inline uint32_t _next_msg(void)
{
#if I3_MIN_WAIT < I3_MAX_WAIT
    return random_uint32_range(I3_MIN_WAIT, I3_MAX_WAIT);
#else
    return I3_MIN_WAIT;
#endif
}

static void *req_gen(void *arg)
{
    (void)arg;
    msg_t msg_queue[8];
    gnrc_netif_t *netif = NULL;
    gnrc_ipv6_nib_ft_t fib;
    void *state = NULL;
    unsigned num_server = 0;

    msg_init_queue(msg_queue, 8);
    while ((netif = gnrc_netif_iter(netif))) {
        if (gnrc_netapi_get(netif->pid, NETOPT_IS_WIRED, 0, NULL, 0) != 1) {
            break;
        }
    }
#ifdef MODULE_PKTCNT_FAST
    netopt_enable_t set = NETOPT_ENABLE;
    gnrc_netapi_set(netif->pid, NETOPT_TX_END_IRQ, 0, &set, sizeof(set));
    if (netif != NULL) {
        int res;
        ipv6_addr_t addrs[GNRC_NETIF_IPV6_ADDRS_NUMOF];

        if ((res = gnrc_netif_ipv6_addrs_get(netif, addrs, sizeof(addrs))) > 0) {
            for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
                if (ipv6_addr_is_link_local(&addrs[i]) &&
                    (pktcnt_addr_str[0] == '\0')) {
                    fmt_bytes_hex(pktcnt_addr_str, &addrs[i].u8[8], 8);
                    pktcnt_addr_str[16] = '\0';
                }
            }
        }
    }
#endif
    evtimer_init_msg(&req_timer);

    /* Trigger CoAP gets for all downstream nodes in FIB */
    while (gnrc_ipv6_nib_ft_iter(NULL, netif->pid, &state, &fib) &&
           (num_server < I3_MAX_SERVER)) {
        if (fib.dst_len == 128U) {
            _server_event_t *event = &server_event[num_server++];

            memcpy(&event->server, &fib.dst, sizeof(fib.dst));
            event->event.msg.type = I3_SEND_MSG_TYPE;
            event->event.msg.content.ptr = event;
            event->event.event.offset = _next_msg();
            evtimer_add_msg(&req_timer, &event->event, sched_active_pid);
            event->req_count = 0;
        }
    }
    while (1) {
        msg_t msg;
        msg_receive(&msg);
        switch (msg.type) {
            case I3_SEND_MSG_TYPE: {
                _server_event_t *event = msg.content.ptr;
                if (event->req_count++ < I3_MAX_REQ) {
                    event->event.event.offset = _next_msg();
                    evtimer_add_msg(&req_timer, &event->event, sched_active_pid);
                }
                _send_req(&event->server);
                break;
            }
            default:
                break;
        }
    }
    return NULL;
}

void gcoap_cli_init(void)
{
    thread_create(req_gen_stack, REQ_GEN_STACK_SIZE, REQ_GEN_PRIO,
                  THREAD_CREATE_STACKTEST, req_gen, NULL, "i3-req-gen");
}
