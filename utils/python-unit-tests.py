#! /usr/bin/env python3

# Copyright (C) 2008-2011 INESC Porto

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# Author: Gustavo J. A. M. Carneiro <gjc@inescporto.pt>

import unittest
from ns.core import Simulator, Seconds, Config, int64x64_t
import ns.core
import ns.network
import ns.internet
import ns.mobility
import ns.csma
import ns.applications

UINT32_MAX = 0xFFFFFFFF


## TestSimulator class
class TestSimulator(unittest.TestCase):
    ## @var _received_packet
    #  received packet
    ## @var _args_received
    #  args
    ## @var _cb_time
    #  current time
    ## @var _context_received
    #  context

    def testScheduleNow(self):
        """! Test schedule now
        @param self this object
        @return none
        """
        def callback(args):
            """! Callback function
            @param args arguments
            return none
            """
            self._args_received = args
            self._cb_time = Simulator.Now()
        Simulator.Destroy()
        self._args_received = None
        self._cb_time = None
        Simulator.ScheduleNow(callback, "args")
        Simulator.Run()
        self.assertEqual(self._args_received, "args")
        self.assertEqual(self._cb_time.GetSeconds(), 0.0)

    def testSchedule(self):
        """! Test schedule
        @param self this object
        @return none
        """
        def callback(args):
            """! Callback function
            @param args arguments
            @return none
            """
            self._args_received = args
            self._cb_time = Simulator.Now()
        Simulator.Destroy()
        self._args_received = None
        self._cb_time = None
        Simulator.Schedule(Seconds(123), callback, "args")
        Simulator.Run()
        self.assertEqual(self._args_received, "args")
        self.assertEqual(self._cb_time.GetSeconds(), 123.0)

    def testScheduleDestroy(self):
        """! Test schedule destroy
        @param self this object
        @return none
        """
        def callback(args):
            """! Callback function
            @param args
            @return none
            """
            self._args_received = args
            self._cb_time = Simulator.Now()
        Simulator.Destroy()
        self._args_received = None
        self._cb_time = None
        def null(): pass
        Simulator.Schedule(Seconds(123), null)
        Simulator.ScheduleDestroy(callback, "args")
        Simulator.Run()
        Simulator.Destroy()
        self.assertEqual(self._args_received, "args")
        self.assertEqual(self._cb_time.GetSeconds(), 123.0)

    def testScheduleWithContext(self):
        """! Test schedule with context
        @param self this object
        @return none
        """
        def callback(context, args):
            """! Callback
            @param context the cntet
            @param args the arguments
            @return none
            """
            self._context_received = context
            self._args_received = args
            self._cb_time = Simulator.Now()
        Simulator.Destroy()
        self._args_received = None
        self._cb_time = None
        self._context_received = None
        Simulator.ScheduleWithContext(54321, Seconds(123), callback, "args")
        Simulator.Run()
        self.assertEqual(self._context_received, 54321)
        self.assertEqual(self._args_received, "args")
        self.assertEqual(self._cb_time.GetSeconds(), 123.0)

    def testTimeComparison(self):
        """! Test time comparison
        @param self this object
        @return none
        """
        self.assertTrue(Seconds(123) == Seconds(123))
        self.assertTrue(Seconds(123) >= Seconds(123))
        self.assertTrue(Seconds(123) <= Seconds(123))
        self.assertTrue(Seconds(124) > Seconds(123))
        self.assertTrue(Seconds(123) < Seconds(124))

    def testTimeNumericOperations(self):
        """! Test numeric operations
        @param self this object
        @return none
        """
        self.assertEqual(Seconds(10) + Seconds(5), Seconds(15))
        self.assertEqual(Seconds(10) - Seconds(5), Seconds(5))
        
        v1 = int64x64_t(5.0)*int64x64_t(10)
        self.assertEqual(v1, int64x64_t(50))

    def testConfig(self):
        """! Test configuration
        @param self this object
        @return none
        """
        Config.SetDefault("ns3::OnOffApplication::PacketSize", ns.core.UintegerValue(123))
        # hm.. no Config.Get?

    def testSocket(self):
        """! Test socket
        @param self
        @return none
        """
        node = ns.network.Node()
        internet = ns.internet.InternetStackHelper()
        internet.Install(node)
        self._received_packet = None

        def rx_callback(socket):
            """! Receive Callback
            @param socket the socket to receive
            @return none
            """
            assert self._received_packet is None
            self._received_packet = socket.Recv(maxSize=UINT32_MAX, flags=0)

        sink = ns.network.Socket.CreateSocket(node, ns.core.TypeId.LookupByName("ns3::UdpSocketFactory"))
        sink.Bind(ns.network.InetSocketAddress(ns.network.Ipv4Address.GetAny(), 80))
        sink.SetRecvCallback(rx_callback)

        source = ns.network.Socket.CreateSocket(node, ns.core.TypeId.LookupByName("ns3::UdpSocketFactory"))
        source.SendTo(ns.network.Packet(19), 0, ns.network.InetSocketAddress(ns.network.Ipv4Address("127.0.0.1"), 80))

        Simulator.Run()
        self.assertTrue(self._received_packet is not None)
        self.assertEqual(self._received_packet.GetSize(), 19)


    def testAttributes(self):
        """! Test attributes function
        @param self this object
        @return none
        """
        # Templated class DropTailQueue<Packet> in C++
        queue = ns.network.DropTailQueue__Ns3Packet()
        queueSizeValue = ns.network.QueueSizeValue (ns.network.QueueSize ("500p"))
        queue.SetAttribute("MaxSize", queueSizeValue)

        limit = ns.network.QueueSizeValue()
        queue.GetAttribute("MaxSize", limit)
        self.assertEqual(limit.Get(), ns.network.QueueSize ("500p"))

        ## -- object pointer values
        mobility = ns.mobility.RandomWaypointMobilityModel()
        ptr = ns.core.PointerValue()
        mobility.GetAttribute("PositionAllocator", ptr)
        self.assertEqual(ptr.GetObject(), None)
        
        pos = ns.mobility.ListPositionAllocator()
        mobility.SetAttribute("PositionAllocator", ns.core.PointerValue(pos))

        ptr = ns.core.PointerValue()
        mobility.GetAttribute("PositionAllocator", ptr)
        self.assertTrue(ptr.GetObject() is not None)

    def testIdentity(self):
        """! Test identify
        @param self this object
        @return none
        """
        csma = ns.csma.CsmaNetDevice()
        channel = ns.csma.CsmaChannel()
        csma.Attach(channel)
        
        c1 = csma.GetChannel()
        c2 = csma.GetChannel()

        self.assertTrue(c1 is c2)

    def testTypeId(self):
        """! Test type ID
        @param self this object
        @return none
        """
        typeId1 = ns.core.TypeId.LookupByNameFailSafe("ns3::UdpSocketFactory")
        self.assertEqual(typeId1.GetName (), "ns3::UdpSocketFactory")
        
        self.assertRaises(KeyError, ns.core.TypeId.LookupByNameFailSafe, "__InvalidTypeName__")

    def testCommandLine(self):
        """! Test command line
        @param self this object
        @return none
        """
        cmd = ns.core.CommandLine()
        cmd.AddValue("Test1", "this is a test option")
        cmd.AddValue("Test2", "this is a test option")
        cmd.AddValue("Test3", "this is a test option", variable="test_xxx")
        cmd.Test1 = None
        cmd.Test2 = None
        cmd.test_xxx = None
        class Foo:
            pass
        foo = Foo()
        foo.test_foo = None
        cmd.AddValue("Test4", "this is a test option", variable="test_foo", namespace=foo)
        
        cmd.Parse(["python", "--Test1=value1", "--Test2=value2", "--Test3=123", "--Test4=xpto"])

        self.assertEqual(cmd.Test1, "value1")
        self.assertEqual(cmd.Test2, "value2")
        self.assertEqual(cmd.test_xxx, "123")
        self.assertEqual(foo.test_foo, "xpto")

    def testSubclass(self):
        """! Test subclass
        @param self this object
        @return none
        """
        ## MyNode class
        class MyNode(ns.network.Node):
            def __init__(self):
                """! Initializer
                @param self this object
                @return none
                """
                super(MyNode, self).__init__()

        node = MyNode()


if __name__ == '__main__':
    unittest.main()
