import os
import re

# 1. Limpeza do Header (gpsr.h)
with open('src/gpsr/model/gpsr.h', 'r') as f:
    h_data = f.read()

h_data = h_data.replace('UnicastForwardCallback ucb', 'const UnicastForwardCallback &ucb')
h_data = h_data.replace('MulticastForwardCallback mcb', 'const MulticastForwardCallback &mcb')
h_data = h_data.replace('LocalDeliverCallback lcb', 'const LocalDeliverCallback &lcb')
h_data = h_data.replace('ErrorCallback ecb', 'const ErrorCallback &ecb')
h_data = '\n'.join([line for line in h_data.split('\n') if 'LocationService' not in line])

with open('src/gpsr/model/gpsr.h', 'w') as f:
    f.write(h_data)

# 2. Atualizacao da Implementacao (gpsr.cc)
with open('src/gpsr/model/gpsr.cc', 'r') as f:
    cc_data = f.read()

# Correcoes de Ponteiros (0 -> nullptr)
cc_data = cc_data.replace('NS_ASSERT (m_ipv4 != 0);', 'NS_ASSERT (m_ipv4 != nullptr);')
cc_data = cc_data.replace('NS_ASSERT (p != 0);', 'NS_ASSERT (p != nullptr);')
cc_data = cc_data.replace('NS_ASSERT (p != 0 && p != Ptr<Packet> ());', 'NS_ASSERT (p != nullptr && p != Ptr<Packet> ());')
cc_data = cc_data.replace('if (wifi == 0)', 'if (wifi == nullptr)')
cc_data = cc_data.replace('if (mac == 0)', 'if (mac == nullptr)')
cc_data = cc_data.replace('if (wifi != 0)', 'if (wifi != nullptr)')
cc_data = cc_data.replace('if (mac != 0)', 'if (mac != nullptr)')
cc_data = cc_data.replace('if (oif != 0 &&', 'if (oif != nullptr &&')
cc_data = cc_data.replace('NS_ASSERT (route != 0);', 'NS_ASSERT (route != nullptr);')
cc_data = cc_data.replace('NS_ASSERT (m_lo != 0);', 'NS_ASSERT (m_lo != nullptr);')
cc_data = cc_data.replace('NS_ASSERT (socket != 0);', 'NS_ASSERT (socket != nullptr);')

# Correcoes da API de Roteamento
cc_data = cc_data.replace('UnicastForwardCallback ucb', 'const UnicastForwardCallback &ucb')
cc_data = cc_data.replace('MulticastForwardCallback mcb', 'const MulticastForwardCallback &mcb')
cc_data = cc_data.replace('LocalDeliverCallback lcb', 'const LocalDeliverCallback &lcb')
cc_data = cc_data.replace('ErrorCallback ecb', 'const ErrorCallback &ecb')

# Substituicao do LocationService por consultas diretas ao MobilityModel nativo
cc_data = cc_data.replace('m_locationService->IsInSearch (dst)', 'false')
cc_data = cc_data.replace('m_locationService->HasPosition (dst)', 'true')
cc_data = cc_data.replace('m_locationService->GetEntryUpdateTime (destination)', 'Simulator::Now()')
cc_data = cc_data.replace('m_locationService->GetEntryUpdateTime (dst)', 'Simulator::Now()')
cc_data = cc_data.replace('m_locationService->GetInvalidPosition ()', 'Vector(-1,-1,-1)')
cc_data = cc_data.replace('m_locationService->Clear ()', '// LS removido')

helper = '''
#include "ns3/node-list.h"
#include "ns3/ipv4.h"

namespace ns3 {
Vector GetGodPosition(Ipv4Address addr) {
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
        Ptr<Node> node = *i;
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        if (ipv4) {
            for (uint32_t j = 0; j < ipv4->GetNInterfaces(); j++) {
                if (ipv4->GetAddress(j, 0).GetLocal() == addr) {
                    return node->GetObject<MobilityModel>()->GetPosition();
                }
            }
        }
    }
    return Vector(-1,-1,-1);
}
}
'''
cc_data = cc_data.replace('#include <limits>', '#include <limits>\n' + helper)
cc_data = cc_data.replace('m_locationService->GetPosition (dst)', 'ns3::GetGodPosition(dst)')
cc_data = cc_data.replace('m_locationService->GetPosition (destination)', 'ns3::GetGodPosition(destination)')

cc_data = re.sub(r'Ptr<LocationService>\nRoutingProtocol::GetLS \(\)\n{\n  return m_locationService;\n}', '', cc_data, flags=re.DOTALL)
cc_data = re.sub(r'void\nRoutingProtocol::SetLS \(Ptr<LocationService> locationService\)\n{\n  m_locationService = locationService;\n}', '', cc_data, flags=re.DOTALL)

with open('src/gpsr/model/gpsr.cc', 'w') as f:
    f.write(cc_data)

# 3. Limpeza do Helper (gpsr-helper.cc)
if os.path.exists('src/gpsr/helper/gpsr-helper.cc'):
    with open('src/gpsr/helper/gpsr-helper.cc', 'r') as f:
        helper_data = f.read()
    helper_data = '\n'.join([line for line in helper_data.split('\n') if 'LocationService' not in line and 'SetLS' not in line])
    with open('src/gpsr/helper/gpsr-helper.cc', 'w') as f:
        f.write(helper_data)

print("✅ API do GPSR modernizada com sucesso!")
