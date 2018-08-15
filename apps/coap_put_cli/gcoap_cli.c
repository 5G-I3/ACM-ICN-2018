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

#define ENABLE_DEBUG (0)
#include "debug.h"

#define DATA_GEN_STACK_SIZE (THREAD_STACKSIZE_DEFAULT)
#define DATA_GEN_PRIO       (THREAD_PRIORITY_MAIN - 1)
#ifndef I3_SERVER
#define I3_SERVER   "affe::1"
#endif
#define I3_PORT     "5683"
#define I3_PATH     "/i3/gasval"
#define I3_DATA     "{\"id\":\"0x12a77af232\",\"val\":3000}"
#ifndef I3_MIN_WAIT
#define I3_MIN_WAIT (1000)
#endif
#ifndef I3_MAX_WAIT
#define I3_MAX_WAIT (1000)
#endif
#ifndef I3_MAX_REQ
#define I3_MAX_REQ      (3600U)
#endif

static char data_gen_stack[DATA_GEN_STACK_SIZE];
#ifdef MODULE_PKTCNT_FAST
extern char pktcnt_addr_str[17];
#endif
static char *i3_args[] = {
        "coap",
        "put",
#ifdef I3_CONFIRMABLE
        "-c",
#endif
        I3_SERVER,
        I3_PORT,
        I3_PATH,
        I3_DATA
    };

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
    /* printf("%1u.%02u;%u-%s\n", */
    /*        coap_get_code_class(pdu), */
    /*        coap_get_code_detail(pdu), */
    /*        coap_get_id(pdu), */
    /*        pktcnt_addr_str); */
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

static size_t _send(uint8_t *buf, size_t len, char *addr_str, char *port_str)
{
    ipv6_addr_t addr;
    size_t bytes_sent;
    sock_udp_ep_t remote;

    remote.family = AF_INET6;

    /* parse for interface */
    int iface = ipv6_addr_split_iface(addr_str);
    if (iface == -1) {
        if (gnrc_netif_numof() == 1) {
            /* assign the single interface found in gnrc_netif_numof() */
            remote.netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
        }
        else {
            remote.netif = SOCK_ADDR_ANY_NETIF;
        }
    }
    else {
        if (gnrc_netif_get_by_pid(iface) == NULL) {
            /* puts("gcoap_cli: interface not valid"); */
            return 0;
        }
        remote.netif = iface;
    }

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        /* puts("gcoap_cli: unable to parse destination address"); */
        return 0;
    }
    if ((remote.netif == SOCK_ADDR_ANY_NETIF) && ipv6_addr_is_link_local(&addr)) {
        /* puts("gcoap_cli: must specify interface for link local target"); */
        return 0;
    }
    memcpy(&remote.addr.ipv6[0], &addr.u8[0], sizeof(addr.u8));

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
        if (!_send(&buf[0], len, argv[apos], argv[apos+1])) {
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

static inline uint32_t _next_msg(void)
{
#if I3_MIN_WAIT < I3_MAX_WAIT
    return random_uint32_range(I3_MIN_WAIT, I3_MAX_WAIT) * US_PER_MS;
#else
    return I3_MIN_WAIT * US_PER_MS;
#endif
}

static void *data_gen(void *arg)
{
    bool unbootstrapped = true;
    (void)arg;

    /* wait for network to be set-up */
    while (unbootstrapped) {
        ipv6_addr_t addrs[GNRC_NETIF_IPV6_ADDRS_NUMOF];
        gnrc_netif_t *netif = gnrc_netif_iter(NULL);
        int res;

#ifdef MODULE_PKTCNT_FAST
        netopt_enable_t set = NETOPT_ENABLE;
        gnrc_netapi_set(netif->pid, NETOPT_TX_END_IRQ, 0, &set, sizeof(set));
#endif
        xtimer_sleep(1);
        if ((res = gnrc_netif_ipv6_addrs_get(netif, addrs, sizeof(addrs))) > 0) {
            for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
#ifdef MODULE_PKTCNT_FAST
                if (ipv6_addr_is_link_local(&addrs[i]) &&
                    (pktcnt_addr_str[0] == '\0')) {
                    fmt_bytes_hex(pktcnt_addr_str, &addrs[i].u8[8], 8);
                    pktcnt_addr_str[16] = '\0';
                }
                else
#endif
                if (!ipv6_addr_is_link_local(&addrs[i])) {
                    /* char addr_str[IPV6_ADDR_MAX_STR_LEN]; */
                    /* printf("Global address %s configured\n", */
                    /*        ipv6_addr_to_str(addr_str, &addrs[i], */
                    /*                         sizeof(addr_str))); */
                    unbootstrapped = false;
#ifndef MODULE_PKTCNT_FAST
                    break;
#endif
                }
            }
        }
    }
    printf("Start sending every [%i, %i] s\n", (int)I3_MIN_WAIT,
           I3_MAX_WAIT);
    for (unsigned i = 0; i < I3_MAX_REQ; i++) {
        xtimer_usleep(_next_msg());
        printf("req: %u\n", i);
        gcoap_cli_cmd(sizeof(i3_args) / sizeof(i3_args[0]), i3_args);
    }
    return NULL;
}

void gcoap_cli_init(void)
{
    thread_create(data_gen_stack, DATA_GEN_STACK_SIZE, DATA_GEN_PRIO,
                  THREAD_CREATE_STACKTEST, data_gen, NULL, "i3-data-gen");
}
