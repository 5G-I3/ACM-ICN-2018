/*
 * Copyright (C) 2015 Freie Universität Berlin
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
 * @brief       Example application for demonstrating the RIOT network stack
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fmt.h"
#include "mutex.h"
#include "shell.h"
#include "thread.h"
#include "xtimer.h"
#include "net/asymcute.h"
#include "net/gnrc/netif.h"
#include "net/ipv6/addr.h"
#include "pktcnt.h"
#include "random.h"

#define LISTENER_PRIO       (THREAD_PRIORITY_MAIN - 1)

#define I3_TOPIC            "/i3/gasval"
#ifndef I3_BROKER
#define I3_BROKER           "affe::1"
#endif
#ifndef I3_MIN_WAIT
#define I3_MIN_WAIT (1000)
#endif
#ifndef I3_MAX_WAIT
#define I3_MAX_WAIT (1000)
#endif
#ifndef I3_MAX_REQ
#define I3_MAX_REQ      (3600U)
#endif
#define I3_PORT            (MQTTSN_DEFAULT_PORT)

#define PUB_GEN_STACK_SIZE (THREAD_STACKSIZE_MAIN)
#define PUB_GEN_PRIO       (THREAD_PRIORITY_MAIN - 1)

#ifndef REQ_CTX_NUMOF
#define REQ_CTX_NUMOF       (128U)
#endif

#define MSG_TYPE_CONNECT    (0x5b4d)
#define MSG_TYPE_REGISTER   (0x5b4e)
#define MSG_TYPE_PUBLISH    (0x5b4f)

typedef enum {
    _UNINITIALIZED = 0,
    _CONNECTING,
    _REGISTERING,
    _PUBLISHING,
} i3_state_t;

static char pub_gen_stack[PUB_GEN_STACK_SIZE];

static char mqtt_stack[THREAD_STACKSIZE_DEFAULT];
#ifdef MODULE_PKTCNT_FAST
extern char pktcnt_addr_str[17];
#endif
static const char *payload = "{\"id\":\"0x12a77af232\",\"val\":3000}";
static char client_id[(2 * GNRC_NETIF_L2ADDR_MAXLEN) + 1];
static sock_udp_ep_t gw = { .family = AF_INET6, .port = I3_PORT };
#ifdef I3_CONFIRMABLE
static const unsigned flags = MQTTSN_QOS_1;
#else
static const unsigned flags = MQTTSN_QOS_0;
#endif
static xtimer_t _pub_timer;
static msg_t _pub_timer_msg;
static asymcute_con_t _connection;
static asymcute_req_t _reqs[REQ_CTX_NUMOF];
static asymcute_topic_t _topic;
static i3_state_t _state;
static kernel_pid_t _pub_gen_pid;

static asymcute_req_t *_get_req_ctx(void)
{
    for (unsigned i = 0; i < REQ_CTX_NUMOF; i++) {
        if (!asymcute_req_in_use(&_reqs[i])) {
            return &_reqs[i];
        }
    }
    puts("error: no request context available\n");
    return NULL;
}

static inline uint32_t _next_msg(void)
{
#if I3_MIN_WAIT < I3_MAX_WAIT
    return random_uint32_range(I3_MIN_WAIT,
                               I3_MAX_WAIT) * US_PER_MS;
#else
    return I3_MIN_WAIT * US_PER_MS;
#endif
}

static void _connect(void)
{
    asymcute_req_t *req = _get_req_ctx();
    assert(!asymcute_is_connected(&_connection));
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, I3_BROKER) == NULL) {
        puts("Unable to parse broker address");
    }
    asymcute_connect(&_connection, req, &gw, client_id, true, NULL);
}

static void _register(void)
{
    asymcute_req_t *req = _get_req_ctx();
    asymcute_topic_init(&_topic, I3_TOPIC, 0);
    if (asymcute_register(&_connection, req, &_topic) == ASYMCUTE_GWERR) {
        _connect();
    };
}

static void _publish(void)
{
    asymcute_req_t *req = _get_req_ctx();
    if (req == NULL) {
        return;
    }
    /* publish sensor data */
    asymcute_publish(&_connection, req, &_topic, payload, strlen(payload),
                     flags);
    _pub_timer_msg.type = MSG_TYPE_PUBLISH;
    xtimer_set_msg(&_pub_timer, _next_msg(), &_pub_timer_msg,
                   _pub_gen_pid);
}

static inline void _send_connect(void) {
    _state = _CONNECTING;
    _pub_timer_msg.type = MSG_TYPE_CONNECT;
    xtimer_set_msg(&_pub_timer, random_uint32_range(0, 2 * US_PER_MS),
                   &_pub_timer_msg, _pub_gen_pid);
}

static inline void _send_register(void) {
    _state = _REGISTERING;
    _pub_timer_msg.type = MSG_TYPE_REGISTER;
    xtimer_set_msg(&_pub_timer, random_uint32_range(0, 2 * US_PER_MS),
                   &_pub_timer_msg, _pub_gen_pid);
}

static void _on_con_evt(asymcute_req_t *req, unsigned evt_type)
{
    (void)req;
    switch (_state) {
        case _CONNECTING:
            if (evt_type == ASYMCUTE_CONNECTED) {
                msg_t msg = { .type = MSG_TYPE_REGISTER };

                _state = _REGISTERING;
                msg_send(&msg, _pub_gen_pid);
            }
            else {
                _send_connect();
            }
            break;
        case _REGISTERING:
            if (evt_type == ASYMCUTE_REGISTERED) {
                _state = _PUBLISHING;
                _pub_timer_msg.type = MSG_TYPE_PUBLISH;
                xtimer_set_msg(&_pub_timer, _next_msg(), &_pub_timer_msg,
                               _pub_gen_pid);
            }
            else {
                _send_register();
            }
            break;
        default:
            if (evt_type == ASYMCUTE_DISCONNECTED) {
                _send_connect();
            }
            break;
    }
}

static void *pub_gen(void *arg)
{
    (void)arg;
    unsigned pubs = 0;

    /* printf("pktcnt: MQTT-SN QoS%d push setup\n\n", (flags >> 5)); */
    xtimer_usleep(random_uint32_range(0, 2 * US_PER_MS));
    _state = _CONNECTING;
    _connect();
    while (pubs < I3_MAX_REQ) {
        msg_t msg;

        msg_receive(&msg);
        switch (msg.type) {
            case MSG_TYPE_CONNECT:
                xtimer_usleep(random_uint32_range(0, 2 * US_PER_MS));
                _state = _CONNECTING;
                _connect();
                break;
            case MSG_TYPE_REGISTER:
                _register();
                break;
            case MSG_TYPE_PUBLISH:
                _publish();
                pubs++;
                break;
            default:
                break;
        }

    }
    return NULL;
}

#ifdef MODULE_PKTCNT_FAST
static int pktcnt_fast(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    pktcnt_fast_print();
    return 0;
}
#endif

static int pktcnt_start(int argc, char **argv)
{
    bool unbootstrapped = true;
    (void)argc;
    (void)argv;
    /* init pktcnt */
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
        if (client_id[0] == '\0') {
            size_t res = fmt_bytes_hex(client_id, netif->l2addr, netif->l2addr_len);
            client_id[res] = '\0';
        }
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
#ifndef MODULE_PKTCNT_FAST
    if (pktcnt_init() != PKTCNT_OK) {
        /* puts("error: unable to initialize pktcnt"); */
        return 1;
    }
#endif
    /* start the publishing thread */
    _pub_gen_pid = thread_create(pub_gen_stack, sizeof(pub_gen_stack),
                                 PUB_GEN_PRIO, 0, pub_gen, NULL, "i3-pub-gen");
    assert(_pub_gen_pid > 0);
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "pktcnt", "Start pktcnt", pktcnt_start },
#ifdef MODULE_PKTCNT_FAST
    { "pktcnt_fast", "Fast counters", pktcnt_fast },
#endif
    { NULL, NULL, NULL }
};

int main(void)
{
    /* start the emcute thread */
    asymcute_listener_run(&_connection, mqtt_stack, sizeof(mqtt_stack),
                          LISTENER_PRIO, _on_con_evt);

    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
