# MQTT-SN PUBLISH client

## Usage

Use `mosquitto.rsmb`'s `broker_mqtts` pre-installed on the IoT-LAB's A8 nodes,
as described in the [MQTT-SN Tutorial] with the [6lo_border_router app] as the
border router with [broker.cfg] as configuration.

### Preparation
[Start an experiment] with 1 [A8 node] and at least 1 [M3 node]. The M3 nodes
should be flashed with the client application (this application). The A8 node's
A8-M3 node should be flashed with the [6lo_border_router app]. Please refer to
its README for how to do that.

You can compile this application according to the desired configuration with the
following environment variables:

* `CONFIRMABLE`: leave this unset for the client to send QoS 0 MQTT-SN messages.
  Set it to any other value for the client to send QoS 1 MQTT-SN messages.
* `MEDIAN_WAIT`: the median delay between PUT requests in microseconds (default:
  1000).
* `MAX_REQ`: the maximum number of PUT requests send to each server (default:
  100).
* `SERVER`: the address of the A8 node the PUT server is running on (default:
  `2001:db8::1`).

You can use the `pktcnt_fast` module by setting the environment variable
`USEMODULE = pktcnt_fast` if you are not interested in the packet flow at every
node and just want to observe the data flow.

Access the border router application's terminal (see tutorial in its README).
Now either wait for RPL to construct a full DODAG or configure the routes to the
M3 nodes. You can check or configure that with the `nib route`, the addresses
you can get for each node with the `ifconfig` command.

If you want to disable RPL during your experiment you can do that with the
`rpl kill` command.

To start the experiment after this setup is completed run the `pktcnt` command
on all nodes (A8-M3 and M3 nodes).

[broker.cfg]: broker.cfg
[MQTT-SN Tutorial]: https://www.iot-lab.info/tutorials/mqtt-sn-using-riot-with-a8-m3-nodes/
[6lo_border_router app]: ../6lo_border_router
[Start an experiment]: https://www.iot-lab.info/tutorials/iotlab-experiment-client/
[A8 node]: https://www.iot-lab.info/hardware/a8/
[M3 node]: https://www.iot-lab.info/hardware/m3/
