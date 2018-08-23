# 6Lo border router

This is a modified version of RIOT's [border router example] to include `pktcnt`
and configurations for large numbered RPL-DODAGs. This is used with the MQTT-SN,
CoAP OBS, and CoAP PUT applications. For usage with `iotlab-a8-m3` board, see
this [tutorial].

You can use the `pktcnt_fast` module by setting the environment variable
`USEMODULE = pktcnt_fast` if you are not interested in the packet flow at every
node and just want to observe the data flow.

[border router example]: https://github.com/RIOT-OS/RIOT/tree/master/examples/gnrc_border_router
[tutorial]: https://www.iot-lab.info/tutorials/riot-public-ipv66lowpan-network-with-a8-m3-nodes/
