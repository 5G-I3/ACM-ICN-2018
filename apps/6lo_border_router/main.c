/*
 * Copyright (C) 2015-18 Freie Universität Berlin
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
 * @author      Martine Lenders <m.lenders@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"
#include "pktcnt.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#ifdef MODULE_PKTCNT_FAST
static int pktcnt_fast(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    pktcnt_fast_print();
    return 0;
}
#else
#include "net/gnrc/netapi.h"
#include "net/gnrc/netif.h"
#endif

static int pktcnt_start(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    /* init pktcnt */
#ifndef MODULE_PKTCNT_FAST
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);
    netopt_enable_t set = NETOPT_ENABLE;
    gnrc_netapi_set(netif->pid, NETOPT_TX_END_IRQ, 0, &set, sizeof(set));
    if (pktcnt_init() != PKTCNT_OK) {
        puts("error: unable to initialize pktcnt");
        return 1;
    }
#endif
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
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT border router example application");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
