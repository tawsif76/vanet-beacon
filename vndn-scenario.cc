#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/netanim-module.h"

#include "ns3/ndnSIM-module.h"
#include "ns3/wifi-setup-helper.h"

#include "vehicle.hpp"
#include "ns2-node-utility.hpp"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("vndn-scenario");


int main(int argc, char* argv[]) {
    std::string traceFile = "scratch/vanet-beacon/sumo/ns2mobility.tcl";
    CommandLine cmd;
    cmd.AddValue("traceFile", "Path to NS2 mobility trace file", traceFile);
    cmd.Parse(argc, argv);

    // LogComponentEnable("vndn-scenario", LOG_LEVEL_INFO);
    // LogComponentEnable("Rsu", LOG_LEVEL_INFO);
    LogComponentEnable("Vehicle", LOG_LEVEL_INFO);

    Ns2NodeUtility ns2_utility;
    ns2_utility.Parse(traceFile);

    NodeContainer vehicleNodes;
    vehicleNodes.Create(ns2_utility.GetTotalNodeCount());

    Ns2MobilityHelper ns2(traceFile);
    ns2.Install(vehicleNodes.Begin(), vehicleNodes.End());


    ns3::ndn::WifiSetupHelper wifiHelper;
    wifiHelper.ConfigureDevices(vehicleNodes, false);


    ns3::ndn::StackHelper ndnHelper;
    ndnHelper.setCsSize(2000);
    ndnHelper.SetDefaultRoutes(true);
    ndnHelper.Install(vehicleNodes);


    // ----------------------------------------------------------------------
    // Set VNDN Strategies
    // ----------------------------------------------------------------------
    ns3::ndn::StrategyChoiceHelper::InstallAll("/beacon", "/localhost/nfd/strategy/multicast");

    // ----------------------------------------------------------------------
    // Install Vehicle Applications
    // ----------------------------------------------------------------------
    for (uint32_t i = 0; i < vehicleNodes.GetN(); ++i) {
        Ptr<Node> veh = vehicleNodes.Get(i);
        Ptr<ns3::ndn::Vehicle> app = CreateObject<ns3::ndn::Vehicle>();
        app->SetAttribute("VehicleId", StringValue("veh-" + std::to_string(i)));
        app->SetAttribute("BeaconInterval", StringValue("1s"));
        app->SetStartTime (Seconds(ns2_utility.GetEntryTimeForNode(i)));
        app->SetStopTime (Seconds (ns2_utility.GetExitTimeForNode(i)));
        // std::cerr << "start and time for vehicle " << i << " " <<  ns2_utility.GetEntryTimeForNode(i) << " " << ns2_utility.GetExitTimeForNode(i) << '\n';
        // app->SetStartTime(Seconds (0));
        // app->SetStopTime (Seconds (400.0));
        veh->AddApplication(app);
    }


    Simulator::Stop(Seconds(400));
    Simulator::Run();
    ns3::ndn::Vehicle::PrintGlobalStats();
    Simulator::Destroy();

    return 0;
}
