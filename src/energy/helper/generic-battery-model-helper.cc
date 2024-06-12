/*
 * Copyright (c) 2023 Tokushima University, Japan.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "generic-battery-model-helper.h"

namespace ns3
{

GenericBatteryModelHelper::GenericBatteryModelHelper()
{
    m_batteryModel.SetTypeId("ns3::energy::GenericBatteryModel");
}

GenericBatteryModelHelper::~GenericBatteryModelHelper()
{
}

void
GenericBatteryModelHelper::Set(std::string name, const AttributeValue& v)
{
    m_batteryModel.Set(name, v);
}

Ptr<energy::EnergySource>
GenericBatteryModelHelper::DoInstall(Ptr<Node> node) const
{
    NS_ASSERT(node != nullptr);
    Ptr<energy::EnergySource> energySource = m_batteryModel.Create<energy::EnergySource>();
    NS_ASSERT(energySource != nullptr);
    energySource->SetNode(node);
    return energySource;
}

Ptr<energy::EnergySourceContainer>
GenericBatteryModelHelper::Install(NodeContainer c) const
{
    Ptr<energy::EnergySourceContainer> batteryContainer =
        CreateObject<energy::EnergySourceContainer>();
    for (auto i = c.Begin(); i != c.End(); i++)
    {
        batteryContainer->Add(DoInstall(*i));
    }
    return batteryContainer;
}

Ptr<energy::EnergySource>
GenericBatteryModelHelper::Install(Ptr<Node> node, energy::BatteryModel bm) const
{
    NS_ASSERT(node != nullptr);
    Ptr<energy::EnergySource> energySource = m_batteryModel.Create<energy::EnergySource>();
    NS_ASSERT(energySource != nullptr);

    energySource->SetAttribute("FullVoltage", DoubleValue(energy::g_batteryPreset[bm].vFull));
    energySource->SetAttribute("MaxCapacity", DoubleValue(energy::g_batteryPreset[bm].qMax));

    energySource->SetAttribute("NominalVoltage", DoubleValue(energy::g_batteryPreset[bm].vNom));
    energySource->SetAttribute("NominalCapacity", DoubleValue(energy::g_batteryPreset[bm].qNom));

    energySource->SetAttribute("ExponentialVoltage", DoubleValue(energy::g_batteryPreset[bm].vExp));
    energySource->SetAttribute("ExponentialCapacity",
                               DoubleValue(energy::g_batteryPreset[bm].qExp));

    energySource->SetAttribute("InternalResistance",
                               DoubleValue(energy::g_batteryPreset[bm].internalResistance));
    energySource->SetAttribute("TypicalDischargeCurrent",
                               DoubleValue(energy::g_batteryPreset[bm].typicalCurrent));
    energySource->SetAttribute("CutoffVoltage",
                               DoubleValue(energy::g_batteryPreset[bm].cuttoffVoltage));

    energySource->SetAttribute("BatteryType", EnumValue(energy::g_batteryPreset[bm].batteryType));

    energySource->SetNode(node);
    return energySource;
}

energy::EnergySourceContainer
GenericBatteryModelHelper::Install(NodeContainer c, energy::BatteryModel bm) const
{
    energy::EnergySourceContainer batteryContainer;
    for (auto i = c.Begin(); i != c.End(); i++)
    {
        Ptr<energy::EnergySource> energySource = Install(*i, bm);
        batteryContainer.Add(energySource);
    }
    return batteryContainer;
}

void
GenericBatteryModelHelper::SetCellPack(Ptr<energy::EnergySource> energySource,
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
GenericBatteryModelHelper::SetCellPack(energy::EnergySourceContainer energySourceContainer,
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
