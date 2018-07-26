/*
 * Copyright (C) 2018 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */


#include <stdio.h>

#ifdef MODULE_TLSF
#include "tlsf-malloc.h"
#endif
#include "msg.h"
#include "shell.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/pktdump.h"
#include "pktcnt.h"
#include "xtimer.h"

#include "ccn-lite-riot.h"
#include "ccnl-pkt-builder.h"
#include "net/hopp/hopp.h"

/* main thread's message queue */
#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#ifdef MODULE_TLSF
/* 10kB buffer for the heap should be enough for everyone */
#define TLSF_BUFFER     ((40 * 1024) / sizeof(uint32_t))
static uint32_t _tlsf_heap[TLSF_BUFFER];
#endif

#ifndef PREFIX
#define PREFIX                   "i3"
#endif

#define I3_DATA     "{\"id\":\"0x12a77af232\",\"val\":3000}"
const char *i3_data = "{\"id\":\"0x12a77af232\",\"val\":3000}";

#ifndef NUM_REQUESTS_NODE
#define NUM_REQUESTS_NODE            (3600u)
#endif

#ifndef DELAY_REQUEST
#define DELAY_REQUEST           (30 * 1000000) // us = 30sec
#endif

#ifndef DELAY_JITTER
#define DELAY_JITTER            (15 * 1000000) // us = 15sec
#endif

#define DELAY_MAX               (DELAY_REQUEST + DELAY_JITTER)
#define DELAY_MIN               (DELAY_REQUEST - DELAY_JITTER)

#ifndef REQ_DELAY
#define REQ_DELAY               (random_uint32_range(DELAY_MIN, DELAY_MAX))
#endif

#ifndef CONSUMER_THREAD_PRIORITY
#define CONSUMER_THREAD_PRIORITY (THREAD_PRIORITY_MAIN - 1)
#endif

#ifndef CONSUMER_STACKSIZE
#define CONSUMER_STACKSIZE (1024)
#endif

#ifndef HOPP_PRIO
#define HOPP_PRIO (HOPP_PRIO - 3)
#endif

uint8_t my_hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
char my_hwaddr_str[GNRC_NETIF_L2ADDR_MAXLEN * 3];
bool i_am_root = false;
gnrc_netif_t *netif;

static char _consumer_stack[CONSUMER_STACKSIZE];

/* state for running pktcnt module */
uint8_t pktcnt_running = 0;

void *_consumer_event_loop(void *arg)
{
    (void)arg;
    /* periodically request content items */
    static char name[80];
    for (unsigned i=0; i<NUM_REQUESTS_NODE; i++) {
        xtimer_usleep(REQ_DELAY);
        unsigned name_len = snprintf(name, 80, "/i3/%s/gasval/%04d", my_hwaddr_str, i);
#ifdef MODULE_PKTCNT_FAST
        uint64_t now = xtimer_now_usec64();
        printf("PUB;%s;%lu%06lu\n", name,
            (unsigned long)div_u64_by_1000000(now),
            (unsigned long)now % US_PER_SEC);
#endif
        hopp_publish_content(name, name_len, (unsigned char*)i3_data, 32);
    }
    return 0;
}

static int _req_start(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (i_am_root) {
        return 0;
    }

    thread_create(_consumer_stack, sizeof(_consumer_stack),
                  CONSUMER_THREAD_PRIORITY,
                  THREAD_CREATE_STACKTEST, _consumer_event_loop,
                  NULL, "consumer");
    return 0;
}

static int _root(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    static const char *name = "/i3";

    i_am_root = true;

    hopp_root_start(name, strlen(name));
    return 0;
}

#ifdef MODULE_PKTCNT_FAST
static int _pktcnt_p(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    pktcnt_fast_print();
    return 0;
}
#else
static int _pktcnt_start(int argc, char **argv) {
    (void)argc;
    (void)argv;
#ifdef MODULE_HOPP
    printf("RANK: %u\n", dodag.rank);
#endif
#ifdef MODULE_PKTCNT
    /* init pktcnt */
    if (pktcnt_init() != PKTCNT_OK) {
        return 1;
    }
    pktcnt_running=1;
#endif
    return 0;
}
#endif

static const shell_command_t shell_commands[] = {
    { "hr", "start HoPP root", _root },
    { "req_start", "start periodic content requests", _req_start },
#ifdef MODULE_PKTCNT_FAST
    { "pktcnt_p", "print variables of pktcnt_fast module", _pktcnt_p },
#else
    { "pktcnt_start", "start pktcnt module", _pktcnt_start },
#endif
    { NULL, NULL, NULL }
};

int main(void)
{
    uint16_t src_len = 8;
#ifdef MODULE_TLSF
    tlsf_create_with_pool(_tlsf_heap, sizeof(_tlsf_heap));
#endif
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("ndn_hopp");

    ccnl_core_init();

    ccnl_start();

    /* get the default interface */
    netif = gnrc_netif_iter(NULL);

    gnrc_netapi_set(netif->pid, NETOPT_SRC_LEN, 0, &src_len, sizeof(src_len));

    /* set the relay's PID, configure the interface to use CCN nettype */
    if (ccnl_open_netif(netif->pid, GNRC_NETTYPE_CCN) < 0) {
        return -1;
    }

#ifdef MODULE_GNRC_PKTDUMP
    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                          gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_CCN_CHUNK, &dump);
#endif

    /* save hw address globally */
#ifdef BOARD_NATIVE
    gnrc_netapi_get(netif->pid, NETOPT_ADDRESS, 0, my_hwaddr, sizeof(my_hwaddr));
#else
    gnrc_netapi_get(netif->pid, NETOPT_ADDRESS_LONG, 0, my_hwaddr, sizeof(my_hwaddr));
#endif
    gnrc_netif_addr_to_str(my_hwaddr, sizeof(my_hwaddr), my_hwaddr_str);

    printf("hwaddr: %s\n", my_hwaddr_str);

#ifdef MODULE_HOPP
    hopp_netif = netif;
    hopp_pid = thread_create(hopp_stack, sizeof(hopp_stack), HOPP_PRIO,
                             THREAD_CREATE_STACKTEST, hopp, &ccnl_relay,
                             "hopp");

    if (hopp_pid <= KERNEL_PID_UNDEF) {
        return 1;
    }

#endif

#ifdef MODULE_PKTCNT_FAST
    bool set = true;
    gnrc_netapi_set(netif->pid, NETOPT_TX_END_IRQ, 0, &set, sizeof(set));
#endif

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
