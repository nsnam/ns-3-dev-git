from ns import *

# Some useful tricks for visualizer
# we need to check if the node has a mobility model, but we can't pass Ptr<MobilityModel> to python
ns.cppyy.cppdef("""using namespace ns3; bool hasMobilityModel(Ptr<Node> node){ return !(node->GetObject<MobilityModel>() == 0); };""")
ns.cppyy.cppdef("""using namespace ns3; Vector3D getNodePosition(Ptr<Node> node){ return node->GetObject<MobilityModel>()->GetPosition(); };""")
ns.cppyy.cppdef("""using namespace ns3; Ptr<Ipv4> getNodeIpv4(Ptr<Node> node){ return node->GetObject<Ipv4>(); };""")
ns.cppyy.cppdef("""using namespace ns3; Ptr<Ipv6> getNodeIpv6(Ptr<Node> node){ return node->GetObject<Ipv6>(); };""")
ns.cppyy.cppdef("""using namespace ns3; std::string getMobilityModelName(Ptr<Node> node){ return node->GetObject<MobilityModel>()->GetInstanceTypeId().GetName(); };""")
ns.cppyy.cppdef("""using namespace ns3; bool hasOlsr(Ptr<Node> node){ return !(node->GetObject<olsr::RoutingProtocol>() == 0); };""")
ns.cppyy.cppdef("""using namespace ns3; Ptr<olsr::RoutingProtocol> getNodeOlsr(Ptr<Node> node){ return node->GetObject<olsr::RoutingProtocol>(); };""")


from .core import start, register_plugin, set_bounds, add_initialization_hook

