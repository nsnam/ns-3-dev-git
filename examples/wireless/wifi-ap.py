# -*-  Mode: Python; -*-
# /*
#  * Copyright (c) 2005,2006,2007 INRIA
#  * Copyright (c) 2009 INESC Porto
#  *
#  * This program is free software; you can redistribute it and/or modify
#  * it under the terms of the GNU General Public License version 2 as
#  * published by the Free Software Foundation;
#  *
#  * This program is distributed in the hope that it will be useful,
#  * but WITHOUT ANY WARRANTY; without even the implied warranty of
#  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  * GNU General Public License for more details.
#  *
#  * You should have received a copy of the GNU General Public License
#  * along with this program; if not, write to the Free Software
#  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#  *
#  * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
#  *          Gustavo Carneiro <gjc@inescporto.pt>
#  */

import sys

try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )

# void
# DevTxTrace (std::string context, Ptr<const Packet> p, Mac48Address address)
# {
#   std::cout << " TX to=" << address << " p: " << *p << std::endl;
# }
# void
# DevRxTrace(std::string context, Ptr<const Packet> p, Mac48Address address)
# {
#   std::cout << " RX from=" << address << " p: " << *p << std::endl;
# }
# void
# PhyRxOkTrace(std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, enum WifiPreamble preamble)
# {
#   std::cout << "PHYRXOK mode=" << mode << " snr=" << snr << " " << *packet << std::endl;
# }
# void
# PhyRxErrorTrace(std::string context, Ptr<const Packet> packet, double snr)
# {
#   std::cout << "PHYRXERROR snr=" << snr << " " << *packet << std::endl;
# }
# void
# PhyTxTrace(std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
# {
#   std::cout << "PHYTX mode=" << mode << " " << *packet << std::endl;
# }
# void
# PhyStateTrace(std::string context, Time start, Time duration, enum WifiPhy::State state)
# {
#   std::cout << " state=";
#   switch(state) {
#   case WifiPhy::TX:
#     std::cout << "tx      ";
#     break;
#   case WifiPhy::SYNC:
#     std::cout << "sync    ";
#     break;
#   case WifiPhy::CCA_BUSY:
#     std::cout << "cca-busy";
#     break;
#   case WifiPhy::IDLE:
#     std::cout << "idle    ";
#     break;
#   }
#   std::cout << " start="<<start<<" duration="<<duration<<std::endl;
# }

ns.cppyy.cppdef(
    """
    using namespace ns3;
    void AdvancePosition(Ptr<Node> node){
        Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
        Vector pos = mob->GetPosition();
        pos.x += 5.0;
        if (pos.x >= 210.0)
            return;
        mob->SetPosition(pos);
        Simulator::Schedule(Seconds(1.0), AdvancePosition, node);
    }"""
)


def main(argv):
    ns.CommandLine().Parse(argv)

    ns.Packet.EnablePrinting()

    wifi = ns.WifiHelper()
    mobility = ns.MobilityHelper()
    stas = ns.NodeContainer()
    ap = ns.NodeContainer()
    # NetDeviceContainer staDevs;
    packetSocket = ns.PacketSocketHelper()

    stas.Create(2)
    ap.Create(1)

    # give packet socket powers to nodes.
    packetSocket.Install(stas)
    packetSocket.Install(ap)

    wifiPhy = ns.YansWifiPhyHelper()
    wifiChannel = ns.YansWifiChannelHelper.Default()
    wifiPhy.SetChannel(wifiChannel.Create())

    ssid = ns.Ssid("wifi-default")
    wifiMac = ns.WifiMacHelper()

    # setup stas.
    wifiMac.SetType(
        "ns3::StaWifiMac",
        "ActiveProbing",
        ns.BooleanValue(True),
        "Ssid",
        ns.SsidValue(ssid),
    )
    staDevs = wifi.Install(wifiPhy, wifiMac, stas)
    # setup ap.
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", ns.SsidValue(ssid))
    wifi.Install(wifiPhy, wifiMac, ap)

    # mobility.
    mobility.Install(stas)
    mobility.Install(ap)

    ns.Simulator.Schedule(ns.Seconds(1.0), ns.cppyy.gbl.AdvancePosition, ap.Get(0))

    socket = ns.PacketSocketAddress()
    socket.SetSingleDevice(staDevs.Get(0).GetIfIndex())
    socket.SetPhysicalAddress(staDevs.Get(1).GetAddress())
    socket.SetProtocol(1)

    onoff = ns.OnOffHelper("ns3::PacketSocketFactory", socket.ConvertTo())
    onoff.SetConstantRate(ns.DataRate("500kb/s"))

    apps = onoff.Install(ns.NodeContainer(stas.Get(0)))
    apps.Start(ns.Seconds(0.5))
    apps.Stop(ns.Seconds(43.0))

    ns.Simulator.Stop(ns.Seconds(44.0))

    #   Config::Connect("/NodeList/*/DeviceList/*/Tx", MakeCallback(&DevTxTrace));
    #   Config::Connect("/NodeList/*/DeviceList/*/Rx", MakeCallback(&DevRxTrace));
    #   Config::Connect("/NodeList/*/DeviceList/*/Phy/RxOk", MakeCallback(&PhyRxOkTrace));
    #   Config::Connect("/NodeList/*/DeviceList/*/Phy/RxError", MakeCallback(&PhyRxErrorTrace));
    #   Config::Connect("/NodeList/*/DeviceList/*/Phy/Tx", MakeCallback(&PhyTxTrace));
    #   Config::Connect("/NodeList/*/DeviceList/*/Phy/State", MakeCallback(&PhyStateTrace));

    ns.Simulator.Run()
    ns.Simulator.Destroy()

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
