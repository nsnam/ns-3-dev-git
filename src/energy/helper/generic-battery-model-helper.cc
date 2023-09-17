/*
 * Copyright (c) 2023 Tokushima University, Japan.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "generic-battery-model-helper.h"

namespace ns3
{

GenericBatteryModelHelper::GenericBatteryModelHelper()
{
    m_batteryModel.SetTypeId("ns3::GenericBatteryModel");
}

GenericBatteryModelHelper::~GenericBatteryModelHelper()
{
}

void
GenericBatteryModelHelper::Set(std::string name, const AttributeValue& v)
{
    m_batteryModel.Set(name, v);
}

Ptr<EnergySource>
GenericBatteryModelHelper::DoInstall(Ptr<Node> node) const
{
    NS_ASSERT(node != nullptr);
    Ptr<EnergySource> energySource = m_batteryModel.Create<EnergySource>();
    NS_ASSERT(energySource != nullptr);
    energySource->SetNode(node);
    return energySource;
}

Ptr<EnergySourceContainer>
GenericBatteryModelHelper::Install(NodeContainer c) const
{
    Ptr<EnergySourceContainer> batteryContainer = CreateObject<EnergySourceContainer>();
    for (auto i = c.Begin(); i != c.End(); i++)
    {
        batteryContainer->Add(DoInstall(*i));
    }
    return batteryContainer;
}

Ptr<EnergySource>
GenericBatteryModelHelper::Install(Ptr<Node> node, BatteryModel bm) const
{
    NS_ASSERT(node != nullptr);
    Ptr<EnergySource> energySource = m_batteryModel.Create<EnergySource>();
    NS_ASSERT(energySource != nullptr);

    energySource->SetAttribute("FullVoltage", DoubleValue(g_batteryPreset[bm].vFull));
    energySource->SetAttribute("MaxCapacity", DoubleValue(g_batteryPreset[bm].qMax));

    energySource->SetAttribute("NominalVoltage", DoubleValue(g_batteryPreset[bm].vNom));
    energySource->SetAttribute("NominalCapacity", DoubleValue(g_batteryPreset[bm].qNom));

    energySource->SetAttribute("ExponentialVoltage", DoubleValue(g_batteryPreset[bm].vExp));
    energySource->SetAttribute("ExponentialCapacity", DoubleValue(g_batteryPreset[bm].qExp));

    energySource->SetAttribute("InternalResistance",
                               DoubleValue(g_batteryPreset[bm].internalResistance));
    energySource->SetAttribute("TypicalDischargeCurrent",
                               DoubleValue(g_batteryPreset[bm].typicalCurrent));
    energySource->SetAttribute("CutoffVoltage", DoubleValue(g_batteryPreset[bm].cuttoffVoltage));

    energySource->SetAttribute("BatteryType", EnumValue(g_batteryPreset[bm].batteryType));

    energySource->SetNode(node);
    return energySource;
}

EnergySourceContainer
GenericBatteryModelHelper::Install(NodeContainer c, BatteryModel bm) const
{
    EnergySourceContainer batteryContainer;
    for (auto i = c.Begin(); i != c.End(); i++)
    {
        Ptr<EnergySource> energySource = Install(*i, bm);
        batteryContainer.Add(energySource);
    }
    return batteryContainer;
}

void
GenericBatteryModelHelper::SetCellPack(Ptr<EnergySource> energySource,
                                       uint8_t series,
                                       uint8_t parallel) const
{
    NS_ASSERT_MSG(series > 0, "The value of cells in series must be > 0");
    NS_ASSERT_MSG(parallel > 0, "The value of cells in parallel must be > 0");
    NS_ASSERT(energySource != nullptr);

    DoubleValue vFull;
    DoubleValue q;
    DoubleValue vExp;
    DoubleValue qExp;
    DoubleValue vNom;
    DoubleValue qNom;
    DoubleValue r;

    // Get the present values of the battery cell
    energySource->GetAttribute("FullVoltage", vFull);
    energySource->GetAttribute("MaxCapacity", q);

    energySource->GetAttribute("NominalVoltage", vNom);
    energySource->GetAttribute("NominalCapacity", qNom);

    energySource->GetAttribute("ExponentialVoltage", vExp);
    energySource->GetAttribute("ExponentialCapacity", qExp);

    energySource->GetAttribute("InternalResistance", r);

    // Configuring the Cell packs
    energySource->SetAttribute("FullVoltage", DoubleValue(vFull.Get() * series));
    energySource->SetAttribute("MaxCapacity", DoubleValue(q.Get() * parallel));

    energySource->SetAttribute("NominalVoltage", DoubleValue(vNom.Get() * series));
    energySource->SetAttribute("NominalCapacity", DoubleValue(qNom.Get() * parallel));

    energySource->SetAttribute("ExponentialVoltage", DoubleValue(vExp.Get() * series));
    energySource->SetAttribute("ExponentialCapacity", DoubleValue(qExp.Get() * parallel));

    energySource->SetAttribute("InternalResistance", DoubleValue(r.Get() * (series / parallel)));
}

void
GenericBatteryModelHelper::SetCellPack(EnergySourceContainer energySourceContainer,
                                       uint8_t series,
                                       uint8_t parallel) const
{
    NS_ASSERT_MSG(energySourceContainer.GetN() > 0, "This energy container is empty");

    for (auto i = energySourceContainer.Begin(); i != energySourceContainer.End(); i++)
    {
        SetCellPack(*i, series, parallel);
    }
}

} // namespace ns3
