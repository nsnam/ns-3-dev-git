# -*-  Mode: Python; -*-
#
# Copyright 2010 University of Washington
#
# SPDX-License-Identifier: GPL-2.0-only
#

import sys

try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )


def main(argv):
    ns.CommandLine().Parse(argv)

    #
    # We are interacting with the outside, real, world.  This means we have to
    # interact in real-time and therefore we have to use the real-time simulator
    # and take the time to calculate checksums.
    #
    ns.GlobalValue.Bind("SimulatorImplementationType", ns.StringValue("ns3::RealtimeSimulatorImpl"))
    ns.GlobalValue.Bind("ChecksumEnabled", ns.BooleanValue(True))

    #
    # Create two ghost nodes.  The first will represent the virtual machine host
    # on the left side of the network; and the second will represent the VM on
    # the right side.
    #
    nodes = ns.NodeContainer()
    nodes.Create(2)

    #
    # Use a CsmaHelper to get a CSMA channel created, and the needed net
    # devices installed on both of the nodes.  The data rate and delay for the
    # channel can be set through the command-line parser.
    #
    csma = ns.CsmaHelper()
    devices = csma.Install(nodes)

    #
    # Use the TapBridgeHelper to connect to the pre-configured tap devices for
    # the left side.  We go with "UseLocal" mode since the wifi devices do not
    # support promiscuous mode (because of their natures0.  This is a special
    # case mode that allows us to extend a linux bridge into ns-3 IFF we will
    # only see traffic from one other device on that bridge.  That is the case
    # for this configuration.
    #
    tapBridge = ns.TapBridgeHelper()
    tapBridge.SetAttribute("Mode", ns.StringValue("UseLocal"))
    tapBridge.SetAttribute("DeviceName", ns.StringValue("tap-left"))
    tapBridge.Install(nodes.Get(0), devices.Get(0))

    #
    # Connect the right side tap to the right side wifi device on the right-side
    # ghost node.
    #
    tapBridge.SetAttribute("DeviceName", ns.StringValue("tap-right"))
    tapBridge.Install(nodes.Get(1), devices.Get(1))

    #
    # Run the simulation for ten minutes to give the user time to play around
    #
    ns.Simulator.Stop(ns.Seconds(600))
    ns.Simulator.Run()  # signal_check_frequency = -1
    ns.Simulator.Destroy()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
