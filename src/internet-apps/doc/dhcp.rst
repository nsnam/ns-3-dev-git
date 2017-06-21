DHCP implementation in ns-3
---------------------------

Model Description
*****************

This section documents the implementation details of Dynamic Host Configuration Protocol (DHCP)

The source code for DHCP is located in ``src/internet-apps/model`` and consists of the 
following 6 files:
 - dhcp-server.h,
 - dhcp-server.cc,
 - dhcp-client.h,
 - dhcp-client.cc,
 - dhcp-header.h and
 - dhcp-header.cc

Helpers
*******

The following two files have been added to ``src/internet-apps/helper`` for DHCP: 
- dhcp-helper.h and 
- dhcp-helper.cc

Tests
*****
The tests for DHCP can be found at ``src/internet-apps/test/dhcp-test.cc`

Examples
********
The examples for DHCP can be found at ``src/internet-apps/examples/dhcp-example.cc`


Scope and Limitations
*********************

The server should be provided with a network address, mask and a range of address
for the pool. One client application can be installed on only one netdevice in a
node, and can configure address for only that netdevice.

The following five basic DHCP messages are supported: 

- DHCP DISCOVER,
- DHCP OFFER,
- DHCP REQUEST,
- DHCP ACK and
- DHCP NACK

Also, the following eight options of BootP are supported:
- 1 (Mask),
- 50 (Requested Address),
- 51 (Address Lease Time),
- 53 (DHCP message type),
- 54 (DHCP server identifier), 
- 58 (Address renew time),
- 59 (Address rebind time) and
- 255 (end)

The client identifier option (61) can be implemented in near future.

In the current implementation, a DHCP client can obtain IPv4 address dynamically 
from the DHCP server, and can renew it within a lease time period.

Multiple DHCP servers can be configured, but the implementation does not support
the use of a DHCP Relay yet.

