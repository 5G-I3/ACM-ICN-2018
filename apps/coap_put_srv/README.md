# CoAP PUT server

## Requirements

* Python 3.4 or higher
* [aiocoap 0.3](https://pypi.org/project/aiocoap/0.3/)

## Usage
This script works in tandem with the [coap_put_cli app].

Run it on an A8 node in the IoT-LAB Testbed that is running the
[6lo_border_router app] on its M3 node:

```sh
./main.py
```

[coap_put_cli app]: ../coap_put_cli/
[6lo_border_router app]: ../6lo_border_router/
