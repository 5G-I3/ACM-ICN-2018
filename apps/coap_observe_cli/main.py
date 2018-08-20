#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (C) 2017 Freie Universität Berlin
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Small tool that parses pre-defined events printed by RIOT nodes to STDIO and
# forwards them to a central data sink for logging
#
# Author:   Hauke Petersen <hauke.petersen@fu-berlin.de>
# Author:   Martine Lenders <m.lenders@fu-berlin.de>

import argparse
import logging
import asyncio
import random
import time
from aiocoap import Context, Message, GET, NON


OBSERVE_TIMEOUT_MIN = 5
OBSERVE_TIMEOUT_MAX = 10

logging.basicConfig(level=logging.INFO)
successful_observations = 0

def _get_timeout():
    return random.randint(OBSERVE_TIMEOUT_MIN * 1000,
                          OBSERVE_TIMEOUT_MAX * 1000) / 1000

async def main(addr, timeout):
    global successful_observations
    uri = "coap://{}/i3/gasval".format(addr)
    print("OBSERVE request to '{}'".format(uri))

    protocol = await Context.create_client_context()
    await asyncio.sleep(random.randint(0, 2000) / 1000)
    while True:
        while True:
            retry = _get_timeout()
            t = _get_timeout()
            try:
                request = Message(mtype=NON, code=GET, uri=uri, observe=0)
                pr = protocol.request(request)
                r = await asyncio.wait_for(pr.response,
                                           timeout=t)
            except asyncio.TimeoutError:
                print("Response from %s took longer than %.03fsec. Resending GET" %
                      (uri, t))
            except OSError:
                print("Unable to sent to %s. Resending in %.03fsec." % \
                      (uri, retry))
                await asyncio.sleep(retry)
            else:
                if r.code.is_successful():
                    break
                print("Invalid response %s from %s. Resending GET" %
                      (r.code, uri))
                await asyncio.sleep(retry)
        print("First response for %s: %s\n\t%r" %
              (uri, r.code, r.payload))
        successful_observations += 1
        try:
            async for r in pr.observation:
                print("%u: Next result for %s: %s\n\t%r" %
                      (successful_observations, uri, r.code, r.payload))
        except Exception as e:
            print("Error on observation of %s:\n%s\n. Resending GET" % (e, uri))
            if not pr.observation.cancelled:
                pr.observation.cancel()
            await asyncio.sleep(_get_timeout())
            successful_observations -= 1


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("addrs", default="[::1]:5683", nargs="+",
                   help="IPv6 address of target node")
    p.add_argument("interval", default="1", nargs="?",
                   help="Request interval")
    p.add_argument("-c", "--confirmable", action="store_true",
                   help="Send CoAP message as confirmable")
    p.add_argument("-t", "--timeout", default=1,
                   help="Timeout for follow-up response")
    args = p.parse_args()
    start_observe = None
    while start_observe != "start_observe":
        start_observe = input("Type \"start_observe\" to start OBSERVE")
    tasks = map(lambda addr: main(addr, args.timeout), args.addrs)
    asyncio.get_event_loop().run_until_complete(asyncio.gather(*tasks))
