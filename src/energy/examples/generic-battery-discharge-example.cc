/*
 * Copyright (c) 2023 Tokushima University
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
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include <ns3/core-module.h>
#include <ns3/energy-module.h>
#include <ns3/gnuplot.h>

#include <fstream>
#include <sstream>
#include <string>

using namespace ns3;
using namespace ns3::energy;

/**
 * This example shows the use of batteries in ns-3.
 * 5 batteries of different chemistries are discharged
 * using a constant current. Batteries can be configured
 * manually using the necessary parameters or using
 * presets.
 *
 * In this example, only the first battery uses parameters
 * to form a NiMh battery. The rest of the batteries in this
 * example use defined presets with already tested parameters.
 *
 * Users can make their own battery presets by setting
 * the necessary parameters as in the example in the first
 * battery.
 *
 * Plot files are produced as a result of this example.
 * Graphs can be obtained from the plot using:
 * \code{.sh}
   $> gnuplot <plotname>.plt
   \endcode
 *
 */

Gnuplot battDischPlot1 = Gnuplot("BattDisch1.eps");
Gnuplot2dDataset battDischDataset1;
std::ofstream battDischFile1("BattDischCurve1.plt");

Gnuplot battDischPlot2 = Gnuplot("BattDisch2.eps");
Gnuplot2dDataset battDischDataset2;
std::ofstream battDischFile2("BattDischCurve2.plt");

Gnuplot battDischPlot3 = Gnuplot("BattDisch3.eps");
Gnuplot2dDataset battDischDataset3;
std::ofstream battDischFile3("BattDischCurve3.plt");

Gnuplot battDischPlot4 = Gnuplot("BattDisch4.eps");
Gnuplot2dDataset battDischDataset4;
std::ofstream battDischFile4("BattDischCurve4.plt");

Gnuplot battDischPlot5 = Gnuplot("BattDisch5.eps");
Gnuplot2dDataset battDischDataset5;
std::ofstream battDischFile5("BattDischCurve5.plt");

void
GraphBattery1(Ptr<GenericBatteryModel> es)
{
    // NiMh battery  Panasonic HHR650D NiMH
    double cellVoltage = es->GetSupplyVoltage();
    Time currentTime = Simulator::Now();
    battDischDataset1.Add(currentTime.GetMinutes(), cellVoltage);
    // battDischDataset1.Add(currentTime.GetHours(), cellVoltage);

    if (!Simulator::IsFinished())
    {
        Simulator::Schedule(Seconds(20), &GraphBattery1, es);
    }
}

void
GraphBattery2(Ptr<GenericBatteryModel> es)
{
    // CSB GP1272 Lead Acid
    double cellVoltage = es->GetSupplyVoltage();
    Time currentTime = Simulator::Now();
    battDischDataset2.Add(currentTime.GetMinutes(), cellVoltage);
    // battDischDataset2.Add(currentTime.GetHours(), cellVoltage);

    if (!Simulator::IsFinished())
    {
        Simulator::Schedule(Seconds(20), &GraphBattery2, es);
    }
}

void
GraphBattery3(Ptr<GenericBatteryModel> es)
{
    // Panasonic CGR18650DA Li-on
    double cellVoltage = es->GetSupplyVoltage();
    double dischargeCapacityAh = es->GetDrainedCapacity();
    battDischDataset3.Add(dischargeCapacityAh * 1000, cellVoltage);

    if (!Simulator::IsFinished())
    {
        Simulator::Schedule(Seconds(20), &GraphBattery3, es);
    }
}

void
GraphBattery4(Ptr<GenericBatteryModel> es)
{
    // Rs Pro LGP12100 Lead Acid
    double cellVoltage = es->GetSupplyVoltage();
    Time currentTime = Simulator::Now();
    battDischDataset4.Add(currentTime.GetMinutes(), cellVoltage);
    // battDischDataset4.Add(currentTime.GetHours(), cellVoltage);

    if (!Simulator::IsFinished())
    {
        Simulator::Schedule(Seconds(20), &GraphBattery4, es);
    }
}

void
GraphBattery5(Ptr<GenericBatteryModel> es)
{
    // Panasonic N-700AAC NiCd
    double cellVoltage = es->GetSupplyVoltage();
    Time currentTime = Simulator::Now();
    // battDischDataset5.Add(currentTime.GetMinutes(), cellVoltage);
    battDischDataset5.Add(currentTime.GetHours(), cellVoltage);

    if (!Simulator::IsFinished())
    {
        Simulator::Schedule(Seconds(20), &GraphBattery5, es);
    }
}

int
main(int argc, char** argv)
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    LogComponentEnable("GenericBatteryModel", LOG_LEVEL_DEBUG);

    Ptr<Node> node;
    GenericBatteryModelHelper batteryHelper;
    Ptr<GenericBatteryModel> batteryModel;
    Ptr<SimpleDeviceEnergyModel> devicesEnergyModel;

    //////////////////////// PANASONIC HHR650D NiMH discharge 1C,2C,5C ////////////////////

    // Discharge 6.5A (1C)
    battDischDataset1 = Gnuplot2dDataset("Panasonic NiMH HHR650D 6.5 A (1C)");

    node = CreateObject<Node>();
    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    batteryModel = CreateObject<GenericBatteryModel>();

    batteryModel->SetAttribute("FullVoltage", DoubleValue(1.39)); // Vfull
    batteryModel->SetAttribute("MaxCapacity", DoubleValue(7.0));  // Q

    batteryModel->SetAttribute("NominalVoltage", DoubleValue(1.18));  // Vnom
    batteryModel->SetAttribute("NominalCapacity", DoubleValue(6.25)); // QNom

    batteryModel->SetAttribute("ExponentialVoltage", DoubleValue(1.28)); // Vexp
    batteryModel->SetAttribute("ExponentialCapacity", DoubleValue(1.3)); // Qexp

    batteryModel->SetAttribute("InternalResistance", DoubleValue(0.0046));   // R
    batteryModel->SetAttribute("TypicalDischargeCurrent", DoubleValue(1.3)); // i typical
    batteryModel->SetAttribute("CutoffVoltage", DoubleValue(1.0));           // End of charge.

    // Capacity Ah(qMax) * (Vfull) voltage * 3600 = (7 * 1.39 * 3.6) = 35028
    batteryModel->SetAttribute("BatteryType", EnumValue(NIMH_NICD)); // Battery type

    // The Generic battery model allow users to simulate different types of
    // batteries based on some parameters. However, presets of batteries are
    // included in ns-3, for example, the previous battery values can be
    // configured using a helper to set a NiMh battery preset:

    // batteryModel = DynamicCast<GenericBatteryModel>
    //                (batteryHelper.Install(node,PANASONIC_HHR650D_NIMH));

    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(6.5);

    GraphBattery1(batteryModel);

    battDischPlot1.AddDataset(battDischDataset1);
    // 18717 secs around 5.3hrs, 750secs for 32.5 current, or   (4200 70 mins)
    Simulator::Stop(Seconds(3600));
    Simulator::Run();
    Simulator::Destroy();

    // Discharge 13A (2C)
    battDischDataset1 = Gnuplot2dDataset("Panasonic NiMH HHR650D 13 A (2C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_HHR650D_NIMH));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(13);

    GraphBattery1(batteryModel);

    battDischPlot1.AddDataset(battDischDataset1);
    Simulator::Stop(Seconds(1853));
    Simulator::Run();
    Simulator::Destroy();

    // Discharge 32.5A (5C)
    battDischDataset1 = Gnuplot2dDataset("Panasonic NiMH HHR650D 32.5 A (5C)");
    node = CreateObject<Node>();
    batteryModel = CreateObject<GenericBatteryModel>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_HHR650D_NIMH));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(32.5);

    GraphBattery1(batteryModel);
    battDischPlot1.AddDataset(battDischDataset1);

    Simulator::Stop(Seconds(716));
    Simulator::Run();
    Simulator::Destroy();

    battDischPlot1.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    battDischPlot1.SetLegend(" Time (minutes)", "Voltage (V)");
    battDischPlot1.SetExtra("set xrange[0:70]\n\
                               set yrange [0.8:1.8]\n\
                               set xtics 10\n\
                               set ytics 0.1\n\
                               set grid\n\
                               set style line 1 linewidth 5\n\
                               set style line 2 linewidth 5\n\
                               set style line 3 linewidth 5\n\
                               set style line 4 linewidth 5\n\
                               set style line 5 linewidth 5\n\
                               set style line 6 linewidth 5\n\
                               set style line 7 linewidth 5\n\
                               set style line 8 linewidth 5\n\
                               set style increment user\n\
                               set key reverse Left");

    battDischPlot1.GenerateOutput(battDischFile1);
    battDischFile1.close();
    std::cout << "The end, plotting now\n";

    //////////////////////// CSB GP1272 Lead Acid  discharge 0.5C, 0.9C ////////////

    // Discharge 0.36A (0.05C)
    battDischDataset2 = Gnuplot2dDataset("CSB GP1272 0.36 A (0.05C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, CSB_GP1272_LEADACID));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(0.36);

    GraphBattery2(batteryModel);
    battDischPlot2.AddDataset(battDischDataset2);

    Simulator::Stop(Seconds(55000));
    Simulator::Run();
    Simulator::Destroy();

    // Discharge 0.648A (0.09C)
    battDischDataset2 = Gnuplot2dDataset("CSB GP1272 0.648 A (0.09C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, CSB_GP1272_LEADACID));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(0.648);

    GraphBattery2(batteryModel);
    battDischPlot2.AddDataset(battDischDataset2);

    Simulator::Stop(Seconds(30000));
    Simulator::Run();
    Simulator::Destroy();

    battDischPlot2.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    battDischPlot2.SetLegend(" Time (Minutes)", "Voltage (V)");
    battDischPlot2.SetExtra("set xrange[1:1800]\n\
                               set yrange [7:14]\n\
                               set logscale x \n\
                               set tics scale 3\n\
                               set xtics (1,2,3,5,10,20,30,60,120,180,300,600,1200,1800)\n\
                               set ytics (0,8,9,10,11,12,13,14)\n\
                               set grid\n\
                               set style line 1 linewidth 5\n\
                               set style line 2 linewidth 5\n\
                               set style line 3 linewidth 5\n\
                               set style line 4 linewidth 5\n\
                               set style line 5 linewidth 5\n\
                               set style line 6 linewidth 5\n\
                               set style line 7 linewidth 5\n\
                               set style line 8 linewidth 5\n\
                               set style increment user\n\
                               set key reverse Left");
    battDischPlot2.GenerateOutput(battDischFile2);
    battDischFile2.close();
    std::cout << "The end, plotting now\n";

    //////////////////////// Panasonic Li-on CGR18650DA,  discharge 0.2C,1C,2C  ///////////

    // Discharge 0.466A (0.2C)
    battDischDataset3 = Gnuplot2dDataset("Panasonic Li-on CGR18650DA 0.466 A (0.2C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_CGR18650DA_LION));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(0.466);

    GraphBattery3(batteryModel);
    battDischPlot3.AddDataset(battDischDataset3);

    Simulator::Stop(Seconds(17720));
    Simulator::Run();
    Simulator::Destroy();

    // Discharge 2.33A (1C)
    battDischDataset3 = Gnuplot2dDataset("Panasonic Li-on CGR18650DA 2.33 A (1C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_CGR18650DA_LION));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(2.33);

    GraphBattery3(batteryModel);
    battDischPlot3.AddDataset(battDischDataset3);

    Simulator::Stop(Seconds(3528));
    Simulator::Run();
    Simulator::Destroy();

    // Discharge 4.66A (2C)
    battDischDataset3 = Gnuplot2dDataset("Panasonic Li-on CGR18650DA 4.66 A (2C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_CGR18650DA_LION));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(4.66);

    GraphBattery3(batteryModel);
    battDischPlot3.AddDataset(battDischDataset3);

    Simulator::Stop(Seconds(1744));
    Simulator::Run();
    Simulator::Destroy();

    battDischPlot3.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    battDischPlot3.SetLegend(" Discharge Capacity (mAh)", "Voltage (V)");
    battDischPlot3.SetExtra("set xrange[0:2400]\n\
                               set yrange [2.6:4.4]\n\
                               set xtics 400\n\
                               set ytics 0.2\n\
                               set grid\n\
                               set style line 1 linewidth 5\n\
                               set style line 2 linewidth 5\n\
                               set style line 3 linewidth 5\n\
                               set style line 4 linewidth 5\n\
                               set style line 5 linewidth 5\n\
                               set style line 6 linewidth 5\n\
                               set style line 7 linewidth 5\n\
                               set style line 8 linewidth 5\n\
                               set style increment user\n\
                               set key reverse Left");
    battDischPlot3.GenerateOutput(battDischFile3);
    battDischFile3.close();
    std::cout << "The end, plotting now\n";

    //////////////////////// Rs PRO LGP12100 Lead Acid  discharge 0.05C, 1C ///////////////

    // Discharge 0.36A (0.05C)
    battDischDataset4 = Gnuplot2dDataset("Rs PRO LGP12100  5A (0.05C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, RSPRO_LGP12100_LEADACID));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(5);

    GraphBattery4(batteryModel);
    battDischPlot4.AddDataset(battDischDataset4);

    Simulator::Stop(Seconds(65000));
    Simulator::Run();
    Simulator::Destroy();

    // Discharge 100A (1C)
    battDischDataset4 = Gnuplot2dDataset("Rs PRO LGP12100  100A (1C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, RSPRO_LGP12100_LEADACID));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(100);

    GraphBattery4(batteryModel);
    battDischPlot4.AddDataset(battDischDataset4);

    Simulator::Stop(Seconds(2800));
    Simulator::Run();
    Simulator::Destroy();

    battDischPlot4.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    battDischPlot4.SetLegend(" Time (Minutes)", "Voltage (V)");
    battDischPlot4.SetExtra("set xrange[1:1800]\n\
                               set yrange [7:13]\n\
                               set logscale \n\
                               set tics scale 3\n\
                               set xtics (1,2,4,6,8,10,20,40,60,120,240,360,480,600,1200)\n\
                               set ytics (7,8,9,10,11,12,13)\n\
                               set grid\n\
                               set style line 1 linewidth 5\n\
                               set style line 2 linewidth 5\n\
                               set style line 3 linewidth 5\n\
                               set style line 4 linewidth 5\n\
                               set style line 5 linewidth 5\n\
                               set style line 6 linewidth 5\n\
                               set style line 7 linewidth 5\n\
                               set style line 8 linewidth 5\n\
                               set style increment user\n\
                               set key reverse Left");
    battDischPlot4.GenerateOutput(battDischFile4);
    battDischFile4.close();
    std::cout << "The end, plotting now\n";

    //////////////////////// Panasonic N-700AAC NiCd discharge  ///////////////////////////

    // Discharge 0.7A (0.1C)
    battDischDataset5 = Gnuplot2dDataset("Panasonic N-700AAC  0.7A (0.01C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_N700AAC_NICD));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(0.07);

    GraphBattery5(batteryModel);
    battDischPlot5.AddDataset(battDischDataset5);

    Simulator::Stop(Seconds(38500));
    Simulator::Run();
    Simulator::Destroy();

    // Discharge 0.14A (0.2C)
    battDischDataset5 = Gnuplot2dDataset("Panasonic N-700AAC 0.14A (0.2C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_N700AAC_NICD));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(0.14);

    GraphBattery5(batteryModel);
    battDischPlot5.AddDataset(battDischDataset5);

    Simulator::Stop(Seconds(19200));
    Simulator::Run();
    Simulator::Destroy();

    // Discharge 0.35A (0.5C)
    battDischDataset5 = Gnuplot2dDataset("Panasonic N-700AAC 0.35A (0.5C)");
    node = CreateObject<Node>();
    batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_N700AAC_NICD));

    devicesEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    devicesEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(devicesEnergyModel);
    devicesEnergyModel->SetNode(node);

    devicesEnergyModel->SetCurrentA(0.35);

    GraphBattery5(batteryModel);
    battDischPlot5.AddDataset(battDischDataset5);

    Simulator::Stop(Seconds(7700));
    Simulator::Run();
    Simulator::Destroy();

    battDischPlot5.SetTerminal("postscript eps color enh \"Times-BoldItalic\"");
    battDischPlot5.SetLegend(" Time (Hours)", "Voltage (V)");
    battDischPlot5.SetExtra("set xrange[0:16]\n\
                            set yrange [0.7:1.5]\n\
                            set tics scale 3\n\
                            set xtics 2\n\
                            set ytics 0.1\n\
                            set grid\n\
                            set style line 1 linewidth 5\n\
                            set style line 2 linewidth 5\n\
                            set style line 3 linewidth 5\n\
                            set style line 4 linewidth 5\n\
                            set style line 5 linewidth 5\n\
                            set style line 6 linewidth 5\n\
                            set style line 7 linewidth 5\n\
                            set style line 8 linewidth 5\n\
                            set style increment user\n\
                            set key reverse Left");
    battDischPlot5.GenerateOutput(battDischFile5);
    battDischFile5.close();
    std::cout << "The end, plotting now\n";
    return 0;
}
