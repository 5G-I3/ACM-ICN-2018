# CoAP OBSERVE client

## Requirements

* Python 3.4 or higher
* [aiocoap 0.3](https://pypi.org/project/aiocoap/0.3/)

## Usage
This script works in tandem with the [coap_observe_srv app].

Run this script on an A8 node in the IoT-LAB Testbed that is running the
[6lo_border_router app] on its M3 node:

```
usage: main.py [-h] addrs [addrs ...]

positional arguments:
  addrs       IPv6 addresses of target nodes

optional arguments:
  -h, --help  show this help message and exit
```

Note that for numeric IPv6 addresses (not hostnames) square brackets are
required around the addresses (so use `'[2001:db8::1]'` instead of `2001:db8::1`
in the arguments).

Type `start_observe` to start the experiment, when the setup is ready (see
README of [coap_observe_srv app] for that).

[coap_observe_srv app]: ../coap_observe_srv/
[6lo_border_router app]: ../6lo_border_router/
