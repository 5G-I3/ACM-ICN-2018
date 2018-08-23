# CoAP GET client for scheduled data emission

## Usage

This application works in tandem with the [coap_get_srv_sched app].

### Preparation
[Start an experiment] with 1 [A8 node] and at least 1 [M3 node]. The M3 nodes
should be flashed with the [coap_get_srv_sched app].

Compile the client application (this application) according to the desired
configuration. You can configure the client with the following environment
variables

* `CONFIRMABLE`: leave this unset for the client to send Non-Confirmable CoAP
  GET message. Set it to any other value for the client to send Confirmable CoAP
  messages.
* `MEDIAN_WAIT`: the median delay between GET requests in microseconds (default:
  1000).
* `MAX_REQ`: the maximum number of GET requests send to each server (default:
  100).
* `MAX_SERVER`: the maximum number of servers to send GET requests to. This
  should be lesser or equal to the number of M3 nodes you selected for your
  experiment (default: 50).

You can use the `pktcnt_fast` module by setting the environment variable
`USEMODULE = pktcnt_fast` if you are not interested in the packet flow at every
node and just want to observe the data flow.

Flash the application to the A8-M3 node and access it as a border router
according to the IoT-LAB's [border router tutorial]. Now either wait for RPL to
construct a full DODAG (the root should have routes to at least `MAX_SERVER`
nodes) or configure the routes to `MAX_SERVER` M3 nodes. You can check or
configure that with the `nib route`, the addresses you can get for each node
with the `ifconfig` command).

If you want to disable RPL during your experiment you can do that with the
`rpl kill` command.

To start the experiment after this setup is completed run the `pktcnt` command
on all nodes (A8-M3 and M3 nodes, but ideally the M3 nodes first!).

[coap_get_srv_sched app]: ../coap_get_srv_sched
[Start an experiment]: https://www.iot-lab.info/tutorials/iotlab-experiment-client/
[A8 node]: https://www.iot-lab.info/hardware/a8/
[M3 node]: https://www.iot-lab.info/hardware/m3/
[border router tutorial]: https://www.iot-lab.info/tutorials/riot-public-ipv66lowpan-network-with-a8-m3-nodes/
