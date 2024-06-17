# Copyright (c) 2023 Tokushima University
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
#

#  Panasonic HHR650D NiMh battery (single cell)
#  Demonstrates the discharge behavior of a NIMH battery discharged with a
#  constant current of 6.5 A (1C)

try:
    from ns import ns
except ModuleNotFoundError:
    raise SystemExit(
        "Error: ns3 Python module not found;"
        " Python bindings may not be enabled"
        " or your PYTHONPATH might not be properly configured"
    )


def main(argv):
    """The main function in this Battery discharge example

    Parameters:
    argv: System parameters to use if necessary
    """

    ns.LogComponentEnable("GenericBatteryModel", ns.LOG_LEVEL_DEBUG)

    node = ns.Node()
    batteryHelper = ns.GenericBatteryModelHelper()
    batteryModel = ns.CreateObject[ns.energy.GenericBatteryModel]()
    devicesEnergyModel = ns.energy.SimpleDeviceEnergyModel()

    batteryModel.SetAttribute("FullVoltage", ns.DoubleValue(1.39))  # Vfull
    batteryModel.SetAttribute("MaxCapacity", ns.DoubleValue(7.0))  # Q

    batteryModel.SetAttribute("NominalVoltage", ns.DoubleValue(1.18))  # Vnom
    batteryModel.SetAttribute("NominalCapacity", ns.DoubleValue(6.25))  # QNom

    batteryModel.SetAttribute("ExponentialVoltage", ns.DoubleValue(1.28))  # Vexp
    batteryModel.SetAttribute("ExponentialCapacity", ns.DoubleValue(1.3))  # Qexp

    batteryModel.SetAttribute("InternalResistance", ns.DoubleValue(0.0046))  # R
    batteryModel.SetAttribute("TypicalDischargeCurrent", ns.DoubleValue(1.3))  # i typical
    batteryModel.SetAttribute("CutoffVoltage", ns.DoubleValue(1.0))  # End of charge.

    batteryModel.SetAttribute(
        "BatteryType", ns.EnumValue[ns.energy.GenericBatteryType](ns.energy.NIMH_NICD)
    )  # Battery type

    devicesEnergyModel.SetEnergySource(batteryModel)
    batteryModel.AppendDeviceEnergyModel(devicesEnergyModel)
    devicesEnergyModel.SetNode(node)

    devicesEnergyModel.SetCurrentA(6.5)

    ns.Simulator.Stop(ns.Seconds(3600))
    ns.Simulator.Run()
    ns.Simulator.Destroy()


if __name__ == "__main__":
    import sys

    main(sys.argv)
