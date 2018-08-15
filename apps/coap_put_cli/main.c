/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
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
 * @brief       gcoap example
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 *
 * @}
 */

#include <stdio.h>
#include "msg.h"

#include "net/gnrc.h"
#include "net/gcoap.h"
#include "shell.h"
#include "pktcnt.h"

#define MAIN_QUEUE_SIZE (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

extern int gcoap_cli_cmd(int argc, char **argv);
extern void gcoap_cli_init(void);

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
    (void)argc;
    (void)argv;
    /* init pktcnt */
#ifndef MODULE_PKTCNT_FAST
    if (pktcnt_init() != PKTCNT_OK) {
        puts("error: unable to initialize pktcnt");
        return 1;
    }
#endif
    gcoap_cli_init();
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "coap", "CoAP example", gcoap_cli_cmd },
    { "pktcnt", "Start pktcnt", pktcnt_start },
#ifdef MODULE_PKTCNT_FAST
    { "pktcnt_fast", "Fast counters", pktcnt_fast },
#endif
    { NULL, NULL, NULL }
};

int main(void)
{
    /* for the thread running the shell */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("gcoap example app");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}
