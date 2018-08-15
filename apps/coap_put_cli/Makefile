# name of your application
APPLICATION = coap_put_cli

# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-m3

BOARD_WHITELIST := iotlab-m3

RIOTBASE ?= $(CURDIR)/../../../RIOT


DEVELHELP ?= 1

DEFAULT_CHANNEL ?= 11

ifneq (,$(I3_CONFIRMABLE))
  CFLAGS += -DI3_CONFIRMABLE
  CFLAGS += -DGCOAP_RESEND_BUFS_MAX=100
endif
CFLAGS += -DGNRC_IPV6_NIB_NUMOF=64
CFLAGS += -DGNRC_IPV6_NIB_OFFL_NUMOF=64
CFLAGS += -DGCOAP_REQ_WAITING_MAX=100
CFLAGS += -DGCOAP_STACK_SIZE=1536
CFLAGS += -DGCOAP_NON_TIMEOUT=2000000
CFLAGS += -DGCOAP_MSG_QUEUE_SIZE=32
CFLAGS += -D_NETIF_NETAPI_MSG_QUEUE_SIZE=32
CFLAGS += -DIEEE802154_DEFAULT_CHANNEL=$(DEFAULT_CHANNEL)

USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gcoap
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl

USEMODULE += od
USEMODULE += fmt
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += xtimer
USEMODULE += pktcnt

QUIET ?= 1

include $(RIOTBASE)/Makefile.include
