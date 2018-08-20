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
# Author:   Martine Lenders <m.lenders@fu-berlin.de>

import logging
import asyncio
import aiocoap
import aiocoap.resource


logging.basicConfig(level=logging.INFO)


class SensorResource(aiocoap.resource.Resource):
    async def render_put(self, request):
        print("PUT /i3/gasval from [%s]:%u: %s" % (request.remote.sockaddr[0],
                                                   request.remote.sockaddr[1],
                                                   request.payload))
        return aiocoap.Message(code=aiocoap.CHANGED, payload="")


def main():
    root = aiocoap.resource.Site()
    root.add_resource(
            ('.well-known', 'core'),
            aiocoap.resource.WKCResource(root.get_resources_as_linkheader)
        )
    root.add_resource(('i3', 'gasval'), SensorResource())
    asyncio.Task(aiocoap.Context.create_server_context(root))
    asyncio.get_event_loop().run_forever()

if __name__ == "__main__":
    main()
