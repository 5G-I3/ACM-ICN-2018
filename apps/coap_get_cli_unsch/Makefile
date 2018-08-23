# name of your application
APPLICATION = coap_get_cli_unsch

# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-a8-m3

BOARD_WHITELIST := iotlab-a8-m3 iotlab-m3

RIOTBASE ?= $(CURDIR)/../../../RIOT

DEVELHELP ?= 1

DEFAULT_CHANNEL ?= 11

GNRC_NETIF_NUMOF := 2
TAP              ?= tap0
IPV6_PREFIX      ?= 2001:db8::/64
ETHOS_BAUDRATE   ?= 500000

ifneq (,$(I3_CONFIRMABLE))
  CFLAGS += -DI3_CONFIRMABLE
  CFLAGS += -DGCOAP_RESEND_BUFS_MAX=120
endif
CFLAGS += -DLOG_LEVEL=LOG_NONE
CFLAGS += -DGNRC_IPV6_NIB_NUMOF=64
CFLAGS += -DGNRC_IPV6_NIB_OFFL_NUMOF=64
CFLAGS += -DGNRC_RPL_LIFETIME_UNIT=0
CFLAGS += -DGCOAP_NON_TIMEOUT=2000000
CFLAGS += -DGCOAP_REQ_WAITING_MAX=125
CFLAGS += -DGCOAP_STACK_SIZE=1536
CFLAGS += -DGCOAP_MSG_QUEUE_SIZE=32
CFLAGS += -D_NETIF_NETAPI_MSG_QUEUE_SIZE=32
CFLAGS += -DETHOS_BAUDRATE=$(ETHOS_BAUDRATE) -DUSE_ETHOS_FOR_STDIO
CFLAGS += -DIEEE802154_DEFAULT_CHANNEL=$(DEFAULT_CHANNEL)

# We override the `make term` command to use ethos
TERMDEPS  += host-tools
TERMPROG  ?= sudo sh $(RIOTBASE)/dist/tools/ethos/start_network.sh
TERMFLAGS ?= $(PORT) $(TAP) $(IPV6_PREFIX)

USEMODULE += ethos
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_sixlowpan_border_router_default
USEMODULE += fib
USEMODULE += gnrc_icmpv6_echo
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += pktcnt
USEMODULE += gcoap
USEMODULE += od
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl
USEMODULE += gnrc_uhcpc

QUIET ?= 1

include $(RIOTBASE)/Makefile.include

.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS make -C $(RIOTBASE)/dist/tools
