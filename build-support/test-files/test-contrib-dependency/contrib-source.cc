#include "contrib-header.h"

#include "ns3/simulator.h"

void
testPrint()
{
    std::cout << ns3::Simulator::Now() << std::endl;
}
