# name of your application
APPLICATION = mqtt_pub_cli

# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-m3

BOARD_WHITELIST := iotlab-m3

RIOTBASE ?= $(CURDIR)/../../../RIOT

DEVELHELP ?= 1

DEFAULT_CHANNEL ?= 11

ifneq (,$(I3_CONFIRMABLE))
  CFLAGS += -DI3_CONFIRMABLE
endif
CFLAGS += -DASYMCUTE_T_RETRY=2
CFLAGS += -DASYMCUTE_N_RETRY=4
CFLAGS += -DASYMCUTE_KEEPALIVE=5000
CFLAGS += -DASYMCUTE_KEEPALIVE_PING=4294
CFLAGS += -DGNRC_IPV6_NIB_NUMOF=64
CFLAGS += -DGNRC_IPV6_NIB_OFFL_NUMOF=64
CFLAGS += -D_NETIF_NETAPI_MSG_QUEUE_SIZE=32
CFLAGS += -DIEEE802154_DEFAULT_CHANNEL=$(DEFAULT_CHANNEL)

USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_ipv6_router_default
USEMODULE += asymcute
USEMODULE += pktcnt
# USEMODULE += pktcnt_fast
USEMODULE += fmt
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

QUIET ?= 1

include $(RIOTBASE)/Makefile.include
