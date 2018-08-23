# name of your application
APPLICATION = mqtt_pub_cli

# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-m3

BOARD_WHITELIST := iotlab-m3

RIOTBASE ?= $(CURDIR)/../../../RIOT

DEVELHELP ?= 1

DEFAULT_CHANNEL ?= 11

MEDIAN_WAIT   ?= 1000
MAX_REQ       ?= 100
SERVER        ?= "2001:db8::1"

ifneq (,$(CONFIRMABLE))
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
CFLAGS += -DI3_MIN_WAIT=$(shell echo "$$(( $(MEDIAN_WAIT) - ($(MEDIAN_WAIT) / 2) ))")
CFLAGS += -DI3_MAX_WAIT=$(shell echo "$$(( $(MEDIAN_WAIT) + ($(MEDIAN_WAIT) / 2) ))")
CFLAGS += -DI3_MAX_REQ=$(MAX_REQ)
CFLAGS += -DI3_BROKER="\"$(SERVER)\""

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
