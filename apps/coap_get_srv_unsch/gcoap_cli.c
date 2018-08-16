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
#ifdef MODULE_PKTCNT_FAST
#include "net/gnrc/netif.h"
#endif
#include "pktcnt.h"
#include "od.h"
#include "fmt.h"
#include "mutex.h"
#include "random.h"
#include "xtimer.h"

#define ENABLE_DEBUG (0)
#include "debug.h"


#define I3_PAYLOAD_FMT      "{\"ts\":\"%012lu\",\"cnt\":%04u}"

#define DATA_GEN_STACK_SIZE (THREAD_STACKSIZE_MAIN)
#define DATA_GEN_PRIO       (THREAD_PRIORITY_MAIN - 1)
#ifndef DATA_GEN_MIN_WAIT
#define DATA_GEN_MIN_WAIT   (1000000U)
#endif
#ifndef DATA_GEN_MAX_WAIT
#define DATA_GEN_MAX_WAIT   (3000000U)
#endif
#ifndef DATA_GEN_MAX_AMOUNT
#define DATA_GEN_MAX_AMOUNT (1000U)
#endif

static void _resp_handler(unsigned req_state, coap_pkt_t* pdu,
                          sock_udp_ep_t *remote);
static ssize_t _stats_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx);
static ssize_t _handle_i3_gasval(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx);
static ssize_t _riot_board_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx);

static mutex_t i3_payload_mutex = MUTEX_INIT;
static char i3_payload[33];
static char data_gen_stack[DATA_GEN_STACK_SIZE];

/* CoAP resources */
static const coap_resource_t _resources[] = {
    { "/cli/stats", COAP_GET | COAP_PUT, _stats_handler, NULL },
    { "/i3/gasval", COAP_GET, _handle_i3_gasval, NULL },
    { "/riot/board", COAP_GET, _riot_board_handler, NULL },
};


static gcoap_listener_t _listener = {
    (coap_resource_t *)&_resources[0],
    sizeof(_resources) / sizeof(_resources[0]),
    NULL
};

/* Counts requests sent by CLI. */
static uint16_t req_count = 0;

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

/*
 * Server callback for /cli/stats. Accepts either a GET or a PUT.
 *
 * GET: Returns the count of packets sent by the CLI.
 * PUT: Updates the count of packets. Rejects an obviously bad request, but
 *      allows any two byte value for example purposes. Semantically, the only
 *      valid action is to set the value to 0.
 */
static ssize_t _stats_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch(method_flag) {
        case COAP_GET:
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

            /* write the response buffer with the request count value */
            size_t payload_len = fmt_u16_dec((char *)pdu->payload, req_count);

            return gcoap_finish(pdu, payload_len, COAP_FORMAT_TEXT);

        case COAP_PUT:
            /* convert the payload to an integer and update the internal
               value */
            if (pdu->payload_len <= 5) {
                char payload[6] = { 0 };
                memcpy(payload, (char *)pdu->payload, pdu->payload_len);
                req_count = (uint16_t)strtoul(payload, NULL, 10);
                return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
            }
            else {
                return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
            }
    }

    return 0;
}

static ssize_t _handle_i3_gasval(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    size_t payload_len;
    (void)ctx;
    mutex_lock(&i3_payload_mutex);
    if (i3_payload[0] != '\0') {
        gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
        memcpy(pdu->payload, i3_payload, strlen(i3_payload));
        payload_len = strlen(i3_payload);
    }
    else {
        gcoap_resp_init(pdu, buf, len, COAP_CODE_SERVICE_UNAVAILABLE);
        payload_len = 0U;
    }
    mutex_unlock(&i3_payload_mutex);
    return gcoap_finish(pdu, payload_len, COAP_FORMAT_JSON);
}

static ssize_t _riot_board_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, void *ctx)
{
    (void)ctx;
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    /* write the RIOT board name in the response buffer */
    memcpy(pdu->payload, RIOT_BOARD, strlen(RIOT_BOARD));
    return gcoap_finish(pdu, strlen(RIOT_BOARD), COAP_FORMAT_TEXT);
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
    if (bytes_sent > 0) {
        req_count++;
    }
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
        /* printf(" CLI requests sent: %u\n", req_count); */
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
        if (!_send(&buf[0], len, argv[apos], argv[apos+1])) {
            /* puts("gcoap_cli: msg send failed"); */
        }
        else {
            /* send Observe notification for /cli/stats */
            switch (gcoap_obs_init(&pdu, &buf[0], GCOAP_PDU_BUF_SIZE,
                    &_resources[0])) {
            case GCOAP_OBS_INIT_OK:
                DEBUG("gcoap_cli: creating /cli/stats notification\n");
                size_t payload_len = fmt_u16_dec((char *)pdu.payload, req_count);
                len = gcoap_finish(&pdu, payload_len, COAP_FORMAT_TEXT);
                gcoap_obs_send(&buf[0], len, &_resources[0]);
                break;
            case GCOAP_OBS_INIT_UNUSED:
                DEBUG("gcoap_cli: no observer for /cli/stats\n");
                break;
            case GCOAP_OBS_INIT_ERR:
                DEBUG("gcoap_cli: error initializing /cli/stats notification\n");
                break;
            }
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

static void *data_gen(void *arg)
{
    (void)arg;

    for (unsigned cnt = 0; cnt < DATA_GEN_MAX_AMOUNT; cnt++) {
        xtimer_usleep(random_uint32_range(DATA_GEN_MIN_WAIT,
                                          DATA_GEN_MAX_WAIT));
        sprintf(i3_payload, I3_PAYLOAD_FMT, (long unsigned)xtimer_now_usec(),
                cnt);
        printf("CG;;%s\n", i3_payload);
    }
    return NULL;
}

void gcoap_cli_init(void)
{
#ifdef MODULE_PKTCNT_FAST
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    netopt_enable_t set = NETOPT_ENABLE;
    gnrc_netapi_set(netif->pid, NETOPT_TX_END_IRQ, 0, &set, sizeof(set));
#endif
    thread_create(data_gen_stack, DATA_GEN_STACK_SIZE, DATA_GEN_PRIO,
                  THREAD_CREATE_STACKTEST, data_gen, NULL, "i3-data-gen");
    gcoap_register_listener(&_listener);
}
