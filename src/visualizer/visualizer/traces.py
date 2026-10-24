#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Trace sources the visualizer listens to, connected only for built modules.

The C++ PyViz helper only provides generic handlers ("Tx", "Rx", "Drop" for
trace sources with the signature void (Ptr<const Packet>), and RecordTx(),
RecordRx() and RecordDrop() to call directly), so libvisualizer does not link
against any device or protocol module. This file decides which trace sources
of which types feed them. Trace sources with other signatures get small C++
adapters, compiled at run time by Cppyy in this process, where the modules
that define those types are already loaded.
"""

import logging

from ns import ns

## Logger of the trace source discovery.
_log = logging.getLogger(__name__)

## Config path prefix matching every device of every node.
_DEVICE = "/NodeList/*/DeviceList/*/"

## Trace sources as (required TypeId, generic handler, Config path) tuples.
## Device Tx/Rx sources are not listed: they are discovered per device by
## _discover_device_traces().
TRACE_SOURCES = [
    ("ns3::NrNetDevice", "Rx", _DEVICE + "$ns3::NrNetDevice/Rx"),
    ("ns3::NrNetDevice", "Drop", _DEVICE + "$ns3::NrNetDevice/Drop"),
    (None, "QueueDrop", _DEVICE + "TxQueue/Drop"),
    (
        "ns3::PointToPointNetDevice",
        "ChannelDrop",
        _DEVICE + "$ns3::PointToPointNetDevice/PhyRxDrop",
    ),
    ("ns3::CsmaNetDevice", "ChannelDrop", _DEVICE + "$ns3::CsmaNetDevice/PhyRxDrop"),
]

## Conventional NetDevice trace source names, mapped to the generic handlers.
## Devices of csma, point-to-point, wifi, lr-wpan, spectrum (aloha),
## virtual-net-device, fd-net-device and others follow it, on the device
## itself or on an object it points to (e.g. its "Mac" attribute).
_DEVICE_TRACE_NAMES = {"MacTx": "Tx", "MacRx": "Rx"}

## Callback signature the generic handlers accept.
_PACKET_CALLBACK = "ns3::Packet::TracedCallback"

# C++ adapters for trace sources whose signature is not void (Ptr<const Packet>).
# Each is compiled only when its module is part of the build.
## C++ adapter sources, keyed by name, as (required TypeId, C++ code) tuples.
_ADAPTERS = {
    # QueueDisc Drop: most full-queue losses of IP nodes happen here, before
    # the device's own queue.
    "queue-disc-drop": (
        "ns3::QueueDisc",
        r"""
        #include "ns3/queue-disc.h"
        namespace pyviz_adapters
        {
        void QueueDiscDrop(ns3::visualizer::PyViz* viz, uint32_t node, std::string,
                           ns3::Ptr<const ns3::QueueDiscItem> item)
        {
            viz->RecordDrop(node, item->GetPacket(), "queue");
        }
        bool ConnectQueueDiscDrop(ns3::visualizer::PyViz* viz, uint32_t node, const std::string& path)
        {
            return ns3::Config::ConnectFailSafe(path, ns3::MakeBoundCallback(&QueueDiscDrop, viz, node));
        }
        }
        """,
    ),
    # Ipv4L3Protocol Drop: (header, packet, reason, ipv4, interface). The
    # header was already removed from the packet, so it is added back.
    "ipv4-drop": (
        "ns3::Ipv4L3Protocol",
        r"""
        #include "ns3/ipv4-l3-protocol.h"
        namespace pyviz_adapters
        {
        void Ipv4Drop(ns3::visualizer::PyViz* viz, uint32_t node, std::string,
                      const ns3::Ipv4Header& header, ns3::Ptr<const ns3::Packet> packet,
                      ns3::Ipv4L3Protocol::DropReason reason, ns3::Ptr<ns3::Ipv4>, uint32_t)
        {
            ns3::Ptr<ns3::Packet> copy = packet->Copy();
            copy->AddHeader(header);
            viz->RecordDrop(node, copy,
                            reason == ns3::Ipv4L3Protocol::DROP_TTL_EXPIRED ? "ttl" : "other");
        }
        bool ConnectIpv4Drop(ns3::visualizer::PyViz* viz, uint32_t node, const std::string& path)
        {
            return ns3::Config::ConnectFailSafe(path, ns3::MakeBoundCallback(&Ipv4Drop, viz, node));
        }
        }
        """,
    ),
    # LteNetDevice has no packet trace sources, so the IP packets an LTE
    # device sends and receives are taken from the Ipv4L3Protocol and
    # Ipv6L3Protocol Tx and Rx trace sources of its interface.
    "lte-ip": (
        "ns3::LteNetDevice",
        r"""
        #include "ns3/ipv4-l3-protocol.h"
        #include "ns3/ipv6-l3-protocol.h"
        namespace pyviz_adapters
        {
        ns3::Ptr<ns3::NetDevice> LteDevice(ns3::Ptr<ns3::NetDevice> device)
        {
            static const ns3::TypeId lte = ns3::TypeId::LookupByName("ns3::LteNetDevice");
            return device && device->GetInstanceTypeId().IsChildOf(lte) ? device : nullptr;
        }
        template <class IP, bool tx>
        void LteIp(ns3::visualizer::PyViz* viz, uint32_t node, std::string,
                   ns3::Ptr<const ns3::Packet> packet, ns3::Ptr<IP> ip, uint32_t interface)
        {
            if (auto device = LteDevice(ip->GetNetDevice(interface)))
            {
                if (tx)
                {
                    viz->RecordTx(node, device->GetIfIndex(), packet);
                }
                else
                {
                    viz->RecordRx(node, device->GetIfIndex(), packet);
                }
            }
        }
        bool ConnectLteIp(ns3::visualizer::PyViz* viz, uint32_t node, const std::string& path,
                          bool ipv6, bool tx)
        {
            if (ipv6)
            {
                return ns3::Config::ConnectFailSafe(
                    path,
                    ns3::MakeBoundCallback(tx ? &LteIp<ns3::Ipv6, true> : &LteIp<ns3::Ipv6, false>,
                                           viz,
                                           node));
            }
            return ns3::Config::ConnectFailSafe(
                path,
                ns3::MakeBoundCallback(tx ? &LteIp<ns3::Ipv4, true> : &LteIp<ns3::Ipv4, false>,
                                       viz,
                                       node));
        }
        }
        """,
    ),
    # Per-flow statistics of the flow monitor panel, with the definitions of
    # the flow-monitor module. They are collected here rather than with a
    # FlowMonitor, whose packet tags a FlowMonitor of the scenario would
    # mistake for its own, and the other way round.
    "flow-stats": (
        "ns3::Ipv4L3Protocol",
        r"""
        #include "ns3/ipv4-l3-protocol.h"
        #include "ns3/ipv6-l3-protocol.h"
        #include "ns3/config.h"
        #include "ns3/node-list.h"
        #include "ns3/queue-disc.h"
        #include "ns3/simulator.h"
        #include <cmath>
        #include <cstdlib>
        #include <map>
        #include <sstream>
        #include <string>
        #include <tuple>
        #include <vector>
        namespace pyviz_adapters::flows
        {
        class FlowTag : public ns3::Tag
        {
          public:
            static ns3::TypeId GetTypeId()
            {
                static ns3::TypeId tid = ns3::TypeId("ns3::visualizer::FlowTag")
                                             .SetParent<ns3::Tag>()
                                             .SetGroupName("Visualizer")
                                             .AddConstructor<FlowTag>();
                return tid;
            }
            ns3::TypeId GetInstanceTypeId() const override { return GetTypeId(); }
            uint32_t GetSerializedSize() const override { return 4 + 4 + 8; }
            void Serialize(ns3::TagBuffer buf) const override
            {
                buf.WriteU32(flow);
                buf.WriteU32(packet);
                buf.WriteU64(txTime);
            }
            void Deserialize(ns3::TagBuffer buf) override
            {
                flow = buf.ReadU32();
                packet = buf.ReadU32();
                txTime = buf.ReadU64();
            }
            void Print(std::ostream& os) const override { os << "flow=" << flow; }
            uint32_t flow{0};
            uint32_t packet{0};
            int64_t txTime{0};
        };
        struct Flow
        {
            uint32_t id;
            std::string source;      // address text
            std::string destination; // address text
            ns3::Address sourceAddress;
            ns3::Address destinationAddress;
            uint16_t sourcePort;
            uint16_t destinationPort;
            uint8_t protocol;
            uint64_t txBytes{0};
            uint64_t rxBytes{0};
            uint32_t txPackets{0};
            uint32_t rxPackets{0};
            uint32_t lostPackets{0};
            int64_t delaySum{0};   // time steps
            int64_t jitterSum{0};  // time steps
            int64_t lastDelay{-1}; // time steps
            // Histograms as bin index -> count; bin = floor(value / width).
            std::map<uint32_t, uint32_t> delayHistogram;
            std::map<uint32_t, uint32_t> jitterHistogram;
            std::map<uint32_t, uint32_t> packetSizeHistogram;
            uint32_t nextPacket{0};
            // Packet id -> time step it was last sent or forwarded.
            std::map<uint32_t, int64_t> inFlight;
        };
        // Counters of one flow at a sample time.
        struct Sample
        {
            uint32_t flow;
            double time; // seconds
            uint64_t txBytes;
            uint64_t rxBytes;
            uint32_t txPackets;
            uint32_t rxPackets;
            uint32_t lostPackets;
            double delaySum;
            double jitterSum;
        };
        using Key = std::tuple<ns3::Address, ns3::Address, uint16_t, uint16_t, uint8_t>;

        // The IPv4 and IPv6 types the probes below are instantiated with.
        struct V4
        {
            using L3 = ns3::Ipv4L3Protocol;
            using Header = ns3::Ipv4Header;
            using Ip = ns3::Ipv4;

            // Like the Ipv4FlowClassifier, only unicast is classified.
            static bool Classified(ns3::Ptr<L3> ip, const Header& header)
            {
                return ip->IsUnicast(header.GetDestination());
            }

            static uint8_t Protocol(const Header& header)
            {
                return header.GetProtocol();
            }
        };

        struct V6
        {
            using L3 = ns3::Ipv6L3Protocol;
            using Header = ns3::Ipv6Header;
            using Ip = ns3::Ipv6;

            // Like the Ipv6FlowClassifier, everything but multicast is classified.
            static bool Classified(ns3::Ptr<L3>, const Header& header)
            {
                return !header.GetDestination().IsMulticast();
            }

            static uint8_t Protocol(const Header& header)
            {
                return header.GetNextHeader();
            }
        };

        template <class A>
        std::string Text(const A& address)
        {
            std::ostringstream os;
            os << address;
            return os.str();
        }
        std::vector<Flow> flows;
        std::map<Key, uint32_t> flowIds;
        // Samples taken since Python last collected them.
        std::vector<Sample> samples;
        int64_t interval = 0;   // time steps between samples
        int64_t nextSample = 0; // time step of the next sample
        // Packets not received this long after last being sent or forwarded
        // are lost, as in FlowMonitor.
        const ns3::Time maxDelay = ns3::Seconds(10);
        // Histogram bin widths, the FlowMonitor defaults.
        const double delayBinWidth = 0.001;   // seconds
        const double jitterBinWidth = 0.001;  // seconds
        const double packetSizeBinWidth = 20; // bytes

        void AddToHistogram(std::map<uint32_t, uint32_t>& histogram, double value, double width)
        {
            histogram[static_cast<uint32_t>(std::floor(value / width))]++;
        }

        void CheckForLostPackets();

        void Record(const Flow& flow, int64_t at)
        {
            samples.push_back({flow.id,
                               ns3::TimeStep(at).GetSeconds(),
                               flow.txBytes,
                               flow.rxBytes,
                               flow.txPackets,
                               flow.rxPackets,
                               flow.lostPackets,
                               ns3::TimeStep(flow.delaySum).GetSeconds(),
                               ns3::TimeStep(flow.jitterSum).GetSeconds()});
        }

        // Sample every flow at the sample times passed, before the counters
        // change. No events are scheduled for this, so a simulation without
        // a stop time still ends. Counters do not change between packets, so
        // a gap of several sample times only needs its first and last sample.
        void Advance()
        {
            int64_t now = ns3::Simulator::Now().GetTimeStep();
            if (interval == 0 || now < nextSample)
            {
                return;
            }
            int64_t last = nextSample + (now - nextSample) / interval * interval;
            CheckForLostPackets();
            for (int64_t at : {nextSample, last})
            {
                for (const auto& flow : flows)
                {
                    Record(flow, at);
                }
                if (last == nextSample)
                {
                    break;
                }
            }
            nextSample = last + interval;
        }

        template <class V>
        void SendOutgoing(ns3::Ptr<typename V::L3> ip, const typename V::Header& header,
                          ns3::Ptr<const ns3::Packet> payload, uint32_t)
        {
            Advance();
            uint8_t protocol = V::Protocol(header);
            // Like the FlowMonitor classifiers, only TCP and UDP is classified.
            if (!V::Classified(ip, header) || (protocol != 6 && protocol != 17) ||
                payload->GetSize() < 4)
            {
                return;
            }
            // A packet carrying a tag is tunneled (e.g. GTP-U); like FlowMonitor,
            // only the flow inside the tunnel is counted.
            FlowTag existing;
            if (payload->FindFirstMatchingByteTag(existing))
            {
                return;
            }
            uint8_t data[4];
            payload->CopyData(data, 4);
            uint16_t ports[2] = {uint16_t((data[0] << 8) | data[1]),
                                 uint16_t((data[2] << 8) | data[3])};
            Key key{header.GetSource().ConvertTo(),
                    header.GetDestination().ConvertTo(),
                    ports[0],
                    ports[1],
                    protocol};
            auto [it, added] = flowIds.emplace(key, flows.size() + 1);
            if (added)
            {
                Flow flow;
                flow.id = it->second;
                flow.source = Text(header.GetSource());
                flow.destination = Text(header.GetDestination());
                flow.sourceAddress = std::get<0>(key);
                flow.destinationAddress = std::get<1>(key);
                flow.sourcePort = ports[0];
                flow.destinationPort = ports[1];
                flow.protocol = protocol;
                flows.push_back(flow);
                // Zero counters at the previous sample time, so the first
                // interval of the flow is plotted.
                Record(flow, nextSample - interval);
            }
            Flow& flow = flows[it->second - 1];
            FlowTag tag;
            tag.flow = flow.id;
            tag.packet = flow.nextPacket++;
            tag.txTime = ns3::Simulator::Now().GetTimeStep();
            payload->AddByteTag(tag);
            flow.txPackets++;
            flow.txBytes += payload->GetSize() + header.GetSerializedSize();
            flow.inFlight[tag.packet] = tag.txTime;
        }

        // The flow of a tagged packet still in flight, or nullptr. Like the
        // FlowMonitor probes, the tag must match the addresses of the IP
        // header, so the end of a tunnel is not taken for the packet inside.
        template <class Header>
        Flow* InFlight(const Header& header, ns3::Ptr<const ns3::Packet> payload, FlowTag& tag)
        {
            auto it = payload->GetByteTagIterator();
            while (it.HasNext())
            {
                auto item = it.Next();
                if (item.GetTypeId() != FlowTag::GetTypeId())
                {
                    continue;
                }
                item.GetTag(tag);
                if (tag.flow == 0 || tag.flow > flows.size())
                {
                    continue;
                }
                Flow* flow = &flows[tag.flow - 1];
                if (flow->sourceAddress == header.GetSource().ConvertTo() &&
                    flow->destinationAddress == header.GetDestination().ConvertTo() &&
                    flow->inFlight.count(tag.packet))
                {
                    return flow;
                }
            }
            return nullptr;
        }

        template <class V>
        void LocalDeliver(const typename V::Header& header, ns3::Ptr<const ns3::Packet> payload,
                          uint32_t)
        {
            Advance();
            FlowTag tag;
            Flow* flow = InFlight(header, payload, tag);
            if (!flow)
            {
                return;
            }
            flow->inFlight.erase(tag.packet);
            int64_t delay = ns3::Simulator::Now().GetTimeStep() - tag.txTime;
            uint32_t size = payload->GetSize() + header.GetSerializedSize();
            flow->delaySum += delay;
            AddToHistogram(flow->delayHistogram, ns3::TimeStep(delay).GetSeconds(), delayBinWidth);
            if (flow->rxPackets > 0)
            {
                int64_t jitter = std::abs(flow->lastDelay - delay);
                flow->jitterSum += jitter;
                AddToHistogram(flow->jitterHistogram,
                               ns3::TimeStep(jitter).GetSeconds(),
                               jitterBinWidth);
            }
            flow->lastDelay = delay;
            flow->rxBytes += size;
            AddToHistogram(flow->packetSizeHistogram, size, packetSizeBinWidth);
            flow->rxPackets++;
        }

        template <class V>
        void Drop(const typename V::Header& header, ns3::Ptr<const ns3::Packet> payload,
                  typename V::L3::DropReason, ns3::Ptr<typename V::Ip>, uint32_t)
        {
            Advance();
            FlowTag tag;
            if (Flow* flow = InFlight(header, payload, tag))
            {
                flow->inFlight.erase(tag.packet);
                flow->lostPackets++;
            }
        }

        template <class V>
        void Forward(const typename V::Header& header, ns3::Ptr<const ns3::Packet> payload,
                     uint32_t)
        {
            Advance();
            FlowTag tag;
            if (Flow* flow = InFlight(header, payload, tag))
            {
                flow->inFlight[tag.packet] = ns3::Simulator::Now().GetTimeStep();
            }
        }

        // Queues hold whole IP packets but report no header, so like the
        // FlowMonitor probes, the first tag of a packet in flight is taken.
        void QueueDrop(ns3::Ptr<const ns3::Packet> packet)
        {
            Advance();
            auto it = packet->GetByteTagIterator();
            while (it.HasNext())
            {
                auto item = it.Next();
                if (item.GetTypeId() != FlowTag::GetTypeId())
                {
                    continue;
                }
                FlowTag tag;
                item.GetTag(tag);
                if (tag.flow == 0 || tag.flow > flows.size())
                {
                    continue;
                }
                Flow& flow = flows[tag.flow - 1];
                if (flow.inFlight.erase(tag.packet))
                {
                    flow.lostPackets++;
                    return;
                }
            }
        }

        void QueueDiscDrop(ns3::Ptr<const ns3::QueueDiscItem> item)
        {
            QueueDrop(item->GetPacket());
        }

        void CheckForLostPackets()
        {
            int64_t oldest = (ns3::Simulator::Now() - maxDelay).GetTimeStep();
            for (auto& flow : flows)
            {
                for (auto it = flow.inFlight.begin(); it != flow.inFlight.end();)
                {
                    if (it->second <= oldest)
                    {
                        it = flow.inFlight.erase(it);
                        flow.lostPackets++;
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }

        template <class V>
        void Connect(ns3::Ptr<ns3::Node> node)
        {
            auto ip = node->GetObject<typename V::L3>();
            if (!ip)
            {
                return;
            }
            ip->TraceConnectWithoutContext("SendOutgoing",
                                           ns3::MakeBoundCallback(&SendOutgoing<V>, ip));
            ip->TraceConnectWithoutContext("UnicastForward", ns3::MakeCallback(&Forward<V>));
            ip->TraceConnectWithoutContext("LocalDeliver", ns3::MakeCallback(&LocalDeliver<V>));
            ip->TraceConnectWithoutContext("Drop", ns3::MakeCallback(&Drop<V>));
        }

        void ConnectQueues(uint32_t node)
        {
            std::string prefix = "/NodeList/" + std::to_string(node);
            ns3::Config::ConnectWithoutContextFailSafe(prefix + "/DeviceList/*/TxQueue/Drop",
                                                       ns3::MakeCallback(&QueueDrop));
            ns3::Config::ConnectWithoutContextFailSafe(
                prefix + "/$ns3::TrafficControlLayer/RootQueueDiscList/*/Drop",
                ns3::MakeCallback(&QueueDiscDrop));
        }

        void Install(double sampleInterval)
        {
            flows.clear();
            flowIds.clear();
            samples.clear();
            interval = ns3::Seconds(sampleInterval).GetTimeStep();
            nextSample = ns3::Simulator::Now().GetTimeStep();
            for (uint32_t i = 0; i < ns3::NodeList::GetNNodes(); i++)
            {
                Connect<V4>(ns3::NodeList::GetNode(i));
                Connect<V6>(ns3::NodeList::GetNode(i));
                ConnectQueues(i);
            }
        }
        }
        """,
    ),
    # With EPC, eNB user data bypasses the eNB device's Send()/Receive(); the
    # EpcEnbApplication sees it instead: RxFromEnb (arrived over the radio)
    # and RxFromS1u (about to be sent over the radio). They are recorded on
    # the node's eNB device, which Python looks up.
    "lte-enb": (
        "ns3::EpcEnbApplication",
        r"""
        namespace pyviz_adapters
        {
        void EnbFromRadio(ns3::visualizer::PyViz* viz, uint32_t node, uint32_t device,
                          std::string, ns3::Ptr<ns3::Packet> packet)
        {
            viz->RecordRx(node, device, packet);
        }
        void EnbToRadio(ns3::visualizer::PyViz* viz, uint32_t node, uint32_t device,
                        std::string, ns3::Ptr<ns3::Packet> packet)
        {
            viz->RecordTx(node, device, packet);
        }
        bool ConnectEnb(ns3::visualizer::PyViz* viz, uint32_t node, uint32_t device,
                        const std::string& path, bool fromRadio)
        {
            return ns3::Config::ConnectFailSafe(
                path,
                ns3::MakeBoundCallback(fromRadio ? &EnbFromRadio : &EnbToRadio, viz, node, device));
        }
        }
        """,
    ),
    # NrNetDevice Tx also passes the destination Address.
    "nr-tx": (
        "ns3::NrNetDevice",
        r"""
        namespace pyviz_adapters
        {
        void NrTx(ns3::visualizer::PyViz* viz, uint32_t node, uint32_t device, std::string,
                  ns3::Ptr<const ns3::Packet> packet, const ns3::Address&)
        {
            viz->RecordTx(node, device, packet);
        }
        bool ConnectNrTx(ns3::visualizer::PyViz* viz, uint32_t node, uint32_t device,
                         const std::string& path)
        {
            return ns3::Config::ConnectFailSafe(path,
                                                ns3::MakeBoundCallback(&NrTx, viz, node, device));
        }
        }
        """,
    ),
    # NR counterpart of "lte-enb": RxFromGnb and RxFromS1u of NrEpcGnbApplication.
    "nr-gnb": (
        "ns3::NrEpcGnbApplication",
        r"""
        namespace pyviz_adapters
        {
        void GnbFromRadio(ns3::visualizer::PyViz* viz, uint32_t node, uint32_t device,
                          std::string, ns3::Ptr<ns3::Packet> packet)
        {
            viz->RecordRx(node, device, packet);
        }
        void GnbToRadio(ns3::visualizer::PyViz* viz, uint32_t node, uint32_t device,
                        std::string, ns3::Ptr<ns3::Packet> packet)
        {
            viz->RecordTx(node, device, packet);
        }
        bool ConnectGnb(ns3::visualizer::PyViz* viz, uint32_t node, uint32_t device,
                        const std::string& path, bool fromRadio)
        {
            return ns3::Config::ConnectFailSafe(
                path,
                ns3::MakeBoundCallback(fromRadio ? &GnbFromRadio : &GnbToRadio, viz, node, device));
        }
        }
        """,
    ),
}
## Names of the adapters already compiled.
_compiled = set()


def type_is_registered(type_name):
    """Return whether a TypeId exists, i.e. whether its module was built.

    @param type_name The TypeId name to look up.
    @return True if the TypeId is registered.
    """
    return bool(ns.TypeId.LookupByNameFailSafe(type_name, ns.TypeId()))


def _adapter(name):
    """Compile an adapter once; return the pyviz_adapters namespace, or None.

    @param name The adapter key in _ADAPTERS.
    @return The cppyy pyviz_adapters namespace, or None if the type is not built.
    """
    type_name, code = _ADAPTERS[name]
    if not type_is_registered(type_name):
        return None
    if name not in _compiled:
        ns.cppyy.cppdef(code)
        _compiled.add(name)
    return ns.cppyy.gbl.pyviz_adapters


def _type_name(obj):
    """Get the TypeId name of an object instance.

    @param obj The ns-3 object.
    @return The instance TypeId name as a string.
    """
    return str(obj.GetInstanceTypeId().GetName())


def connect_traces(sim_helper, sources=TRACE_SOURCES):
    """Connect each trace source whose TypeId is registered; return the paths.

    Config path resolution aborts on unknown type names, so the TypeId is
    checked before connecting.

    @param sim_helper The C++ PyViz helper providing ConnectTrace().
    @param sources Trace source tuples to connect.
    @return List of connected Config paths.
    """
    connected = []
    for type_name, handler, path in sources:
        if type_name is not None and not type_is_registered(type_name):
            continue
        if sim_helper.ConnectTrace(handler, path):
            connected.append(path)
    if sources is TRACE_SOURCES:
        connected.extend(_connect_adapted(sim_helper))
    return connected


def _connect_adapted(sim_helper):
    """Connect the trace sources that need compiled C++ adapters.

    @param sim_helper The C++ PyViz helper the adapters record into.
    @return List of connected Config paths.
    """
    connected = _discover_device_traces(sim_helper)
    ipv4 = _adapter("ipv4-drop")
    queue_disc = _adapter("queue-disc-drop")
    enb = _adapter("lte-enb")
    lte_ip = _adapter("lte-ip")
    nr_tx = _adapter("nr-tx")
    gnb = _adapter("nr-gnb")
    for index in range(int(ns.NodeList.GetNNodes())):
        node = ns.NodeList.GetNode(index)
        if ipv4 is not None and node.GetObject[ns.Ipv4L3Protocol]():
            path = "/NodeList/%d/$ns3::Ipv4L3Protocol/Drop" % index
            if ipv4.ConnectIpv4Drop(sim_helper, index, path):
                connected.append(path)
        if queue_disc is not None and node.GetObject[ns.TrafficControlLayer]():
            path = "/NodeList/%d/$ns3::TrafficControlLayer/RootQueueDiscList/*/Drop" % index
            if queue_disc.ConnectQueueDiscDrop(sim_helper, index, path):
                connected.append(path)
        types = [_type_name(node.GetDevice(d)) for d in range(int(node.GetNDevices()))]
        if lte_ip is not None and any(
            t in ("ns3::LteUeNetDevice", "ns3::LteEnbNetDevice") for t in types
        ):
            for protocol, ipv6 in (("Ipv4L3Protocol", False), ("Ipv6L3Protocol", True)):
                for source, tx in (("Tx", True), ("Rx", False)):
                    path = "/NodeList/%d/$ns3::%s/%s" % (index, protocol, source)
                    if lte_ip.ConnectLteIp(sim_helper, index, path, ipv6, tx):
                        connected.append(path)
        if nr_tx is not None:
            for d, type_name in enumerate(types):
                if type_name in ("ns3::NrUeNetDevice", "ns3::NrGnbNetDevice"):
                    path = "/NodeList/%d/DeviceList/%d/$ns3::NrNetDevice/Tx" % (index, d)
                    if nr_tx.ConnectNrTx(sim_helper, index, d, path):
                        connected.append(path)
        for adapter, device_type, app_type, from_radio_source, prefix in (
            (enb, "ns3::LteEnbNetDevice", "ns3::EpcEnbApplication", "RxFromEnb", "Enb"),
            (gnb, "ns3::NrGnbNetDevice", "ns3::NrEpcGnbApplication", "RxFromGnb", "Gnb"),
        ):
            if adapter is None or device_type not in types:
                continue
            connect = getattr(adapter, "Connect" + prefix)
            for source, from_radio in ((from_radio_source, True), ("RxFromS1u", False)):
                path = "/NodeList/%d/ApplicationList/*/$%s/%s" % (index, app_type, source)
                if connect(sim_helper, index, types.index(device_type), path, from_radio):
                    connected.append(path)
    return connected


def _packet_trace_sources(tid):
    """Get the conventional packet trace sources of a TypeId and its parents.

    Sources with a conventional name but another signature are skipped, and
    logged at debug level so a device that is not shown can be diagnosed.

    @param tid The TypeId to inspect.
    @return Dict of trace source name -> generic handler name.
    """
    found = {}
    type_name = str(tid.GetName())
    while True:
        for i in range(int(tid.GetTraceSourceN())):
            info = tid.GetTraceSource(i)
            name = str(info.name)
            if name not in _DEVICE_TRACE_NAMES:
                continue
            if str(info.callback) == _PACKET_CALLBACK:
                found.setdefault(name, _DEVICE_TRACE_NAMES[name])
            else:
                _log.debug(
                    "%s: trace source %s skipped, its signature is %s, not %s",
                    type_name,
                    name,
                    info.callback,
                    _PACKET_CALLBACK,
                )
        parent = tid.GetParent()
        if parent == tid:
            return found
        tid = parent


def _pointed_objects(obj):
    """Get the objects referenced by the Pointer attributes of an object.

    @param obj The ns-3 object.
    @return List of (attribute name, object) pairs, objects not None.
    """
    objects = []
    tid = obj.GetInstanceTypeId()
    seen = set()
    while True:
        for i in range(int(tid.GetAttributeN())):
            info = tid.GetAttribute(i)
            name = str(info.name)
            if name in seen or str(info.checker.GetValueTypeName()) != "ns3::PointerValue":
                continue
            seen.add(name)
            value = ns.PointerValue()
            if obj.GetAttributeFailSafe(name, value):
                pointed = value.GetObject()
                if pointed:
                    objects.append((name, pointed))
        parent = tid.GetParent()
        if parent == tid:
            return objects
        tid = parent


def _device_trace_owner(device):
    """Find the object of a device that holds its conventional packet trace sources.

    Candidates are the device itself, then the objects its Pointer attributes
    refer to, the one named "Mac" first. The first candidate with all of them
    is taken, so MacTx and MacRx always come from the same object; failing
    that, the first candidate with any of them.

    @param device The NetDevice.
    @return Pair of (Config path suffix to the object, e.g. "" or "Mac/",
            dict of trace source name -> generic handler name).
    """
    # Runs once per device when the visualizer starts, so walking every
    # Pointer attribute costs nothing while the simulation runs.
    pointed = sorted(_pointed_objects(device), key=lambda item: item[0] != "Mac")
    candidates = [("", device)] + [(name + "/", obj) for name, obj in pointed]
    partial = None
    for prefix, obj in candidates:
        sources = _packet_trace_sources(obj.GetInstanceTypeId())
        if len(sources) == len(_DEVICE_TRACE_NAMES):
            return prefix, sources
        if sources and partial is None:
            partial = (prefix, sources)
    return partial or ("", {})


def _discover_device_traces(sim_helper):
    """Connect the conventional packet trace sources of every device.

    A new device type needs no change here if it follows the MacTx/MacRx
    convention, on the device or on an object it points to.

    @param sim_helper The C++ PyViz helper providing ConnectTrace().
    @return List of connected Config paths.
    """
    connected = []
    for index in range(int(ns.NodeList.GetNNodes())):
        node = ns.NodeList.GetNode(index)
        for d in range(int(node.GetNDevices())):
            prefix, sources = _device_trace_owner(node.GetDevice(d))
            for name, handler in sources.items():
                path = "/NodeList/%d/DeviceList/%d/%s%s" % (index, d, prefix, name)
                if sim_helper.ConnectTrace(handler, path):
                    connected.append(path)
    return connected


def flow_stats():
    """Get the per-flow statistics collector of the flow monitor panel.

    @return The cppyy pyviz_adapters::flows namespace, or None if the internet
            module is not built.
    """
    adapters = _adapter("flow-stats")
    return None if adapters is None else adapters.flows
