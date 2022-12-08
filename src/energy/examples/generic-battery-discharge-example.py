# Copyright (c) 2023 Tokushima University
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
#

#  Panasonic HHR650D NiMh battery (single cell)
#  Demonstrates the discharge behavior of a NIMH battery discharged with a
#  constant current of 6.5 A (1C)

from ns import ns

def main(argv):
    """The main function in this Battery discharge example

    Parameters:
    argv: System parameters to use if necessary
    """

    ns.core.LogComponentEnable("GenericBatteryModel", ns.core.LOG_LEVEL_DEBUG)

    node = ns.network.Node()
    batteryHelper =  ns.energy.GenericBatteryModelHelper()
    batteryModel = ns.CreateObject("GenericBatteryModel")
    devicesEnergyModel = ns.energy.SimpleDeviceEnergyModel()

    batteryModel.SetAttribute("FullVoltage", ns.core.DoubleValue(1.39))  # Vfull
    batteryModel.SetAttribute("MaxCapacity", ns.core.DoubleValue(7.0))  # Q

    batteryModel.SetAttribute("NominalVoltage", ns.core.DoubleValue(1.18))  # Vnom
    batteryModel.SetAttribute("NominalCapacity", ns.core.DoubleValue(6.25)) # QNom

    batteryModel.SetAttribute("ExponentialVoltage", ns.core.DoubleValue(1.28)) # Vexp
    batteryModel.SetAttribute("ExponentialCapacity", ns.core.DoubleValue(1.3)) # Qexp

    batteryModel.SetAttribute("InternalResistance", ns.core.DoubleValue(0.0046))   # R
    batteryModel.SetAttribute("TypicalDischargeCurrent", ns.core.DoubleValue(1.3)) # i typical
    batteryModel.SetAttribute("CutoffVoltage", ns.core.DoubleValue(1.0))   # End of charge.

    batteryModel.SetAttribute("BatteryType", ns.core.EnumValue(ns.NIMH_NICD))  # Battery type

    devicesEnergyModel.SetEnergySource(batteryModel)
    batteryModel.AppendDeviceEnergyModel(devicesEnergyModel)
    devicesEnergyModel.SetNode(node)

    devicesEnergyModel.SetCurrentA(6.5)


    ns.core.Simulator.Stop(ns.core.Seconds(3600))
    ns.core.Simulator.Run()
    ns.core.Simulator.Destroy()



if __name__ == '__main__':
    import sys
    main(sys.argv)

