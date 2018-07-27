APPLICATION = ndn_cinnamon
# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-m3

BOARD_WHITELIST := fox iotlab-m3 msba2 mulle native pba-d-01-kw2x samr21-xpro

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

CFLAGS += -DUSE_LINKLAYER
CFLAGS += -DCCNL_UAPI_H_
CFLAGS += -DUSE_SUITE_NDNTLV
CFLAGS += -DNEEDS_PREFIX_MATCHING
CFLAGS += -DNEEDS_PACKET_CRAFTING

CFLAGS += -DCONSUMER_THREAD_PRIORITY="THREAD_PRIORITY_MAIN-1"
CFLAGS += -DPKTCNT_PRIO="THREAD_PRIORITY_MAIN-2"
CFLAGS += -DHOPP_PRIO="THREAD_PRIORITY_MAIN-3"
CFLAGS += -DCCNL_THREAD_PRIORITY="THREAD_PRIORITY_MAIN-4"

CFLAGS += -DCCNL_CACHE_SIZE=20
CFLAGS += -DCCNL_FACE_TIMEOUT=15
CFLAGS += -DNDN_DEFAULT_INTEREST_LIFETIME=10000
CFLAGS += -DCCNL_MAX_INTEREST_RETRANSMIT=4
CFLAGS += -DCCNL_INTEREST_RETRANS_TIMEOUT=2000
CFLAGS += -DCOMPAS_NAM_CACHE_RETRIES=3
CFLAGS += -DCCNL_STACK_SIZE="THREAD_STACKSIZE_DEFAULT+512+1024+256"
CFLAGS += -DCCNL_QUEUE_SIZE=16
CFLAGS += -D_NETIF_NETAPI_MSG_QUEUE_SIZE=16

CFLAGS += -DHOPP_STACKSZ="THREAD_STACKSIZE_DEFAULT+1024+512+128"
CFLAGS += -DHOPP_QSZ=16
CFLAGS += -DPKTCNT_STACKSIZE="768+256"

CFLAGS += -DCOMPAS_PREFIX_LEN=3
CFLAGS += -DCOMPAS_NAM_CACHE_LEN=25
CFLAGS += -DCOMPAS_NAME_SUFFIX_LEN=15

ifneq (,$(filter pktcnt_fast,$(USEMODULE)))
  USEMODULE += netstats_l2
endif

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

USEMODULE += ps
USEMODULE += shell
USEMODULE += shell_commands
# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += timex
USEMODULE += xtimer
USEMODULE += random
USEMODULE += prng_xorshift
USEMODULE += evtimer
USEMODULE += pktcnt
USEMODULE += hopp

USEPKG += compas
USEPKG += tlsf
USEPKG += ccn-lite

# we need this to enable code parts in
# in ccn-lite-riot.c:ccnl_app_RX()
CFLAGS += -DNDN_CINNAMON
include $(RIOTBASE)/Makefile.include
