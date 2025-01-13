/*
 * Copyright (c) 2024 Tokushima University, Tokushima, Japan.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/*
   This example shows a comparison between node mobility set with and without the
   use of helpers. Furthermore it shows a simple way to update the node positions.
   More advanced position allocations and mobility patterns can be achieve with
   the use of other helpers.

   In this example, node N1 moves to the right in 3 different instances
   during the simulation while node N0 remains static. A trace is used to print a
   message when movement is detected in node N1.

               10 m         {Sec 2}      10m         {Sec 4}      10 m      {Sec 6}
   N0,N1 --------------------->N1--------------------> N1 ------------------> N1
  (0,0,0)     Move Right    (10,0,0)   Move Right   (20,0,0)   Move Right   (30,0,0)
*/

#include "ns3/mobility-module.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

using namespace ns3;

void
CourseChangeCallback(Ptr<const MobilityModel> mobility)
{
    std::cout << Simulator::Now().As(Time::S) << " Movement detected (Node "
              << mobility->GetObject<Node>()->GetId() << ")!\n";
}

void
MoveNode(Ptr<Node> node, const Vector& pos)
{
    Ptr<MobilityModel> mobilityModel = node->GetObject<MobilityModel>();
    NS_ASSERT_MSG(mobilityModel, "Node doesn't have a mobility model");
    mobilityModel->SetPosition(pos);

    Vector newPos = mobilityModel->GetPosition();

    std::cout << Simulator::Now().As(Time::S) << " Node " << node->GetId() << " | MoveRight, Pos ("
              << newPos.x << "," << newPos.y << "," << newPos.z << ")\n";
}

int
main(int argc, char* argv[])
{
    // Method 1: Install the mobility model using the helper
    //           The helper creates and installs the mobility to a single node
    //           or multiple nodes in a node container. Different helpers might
    //           have additional options.

    NodeContainer nodeContainer(1);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAllocator = CreateObject<ListPositionAllocator>();
    positionAllocator->Add(Vector(0, 0, 0));

    mobility.SetPositionAllocator(positionAllocator);
    Ptr<Node> n0 = nodeContainer.Get(0);
    mobility.Install(n0);

    // Method 2: Install the mobility manually
    //           You need to manually create both, the node object and the mobility object.
    //           After that, you aggregate the mobility to a node.

    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<ConstantPositionMobilityModel> mob1 = CreateObject<ConstantPositionMobilityModel>();
    n1->AggregateObject(mob1);
    mob1->SetPosition(Vector(0, 0, 0));

    // Set the mobility trace hook to node n1 to keep track of its movements
    mob1->TraceConnectWithoutContext("CourseChange", MakeBoundCallback(&CourseChangeCallback));

    // Schedule movements to node n1
    Simulator::ScheduleWithContext(n1->GetId(), Seconds(2), &MoveNode, n1, Vector(10, 0, 0));

    Simulator::ScheduleWithContext(n1->GetId(), Seconds(4), &MoveNode, n1, Vector(20, 0, 0));

    Simulator::ScheduleWithContext(n1->GetId(), Seconds(6), &MoveNode, n1, Vector(30, 0, 0));

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
