#include "vehicle.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/string.h"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/random-variable-stream.h" 
#include "ns3/core-module.h" 
#include "ns3/node-list.h"  
#include "ns3/wifi-net-device.h"
#include "ndn-cxx/detail/common.hpp"
#include <ndn-cxx/encoding/block-helpers.hpp> 
#include <ndn-cxx/encoding/tlv.hpp>
#include "ndn-cxx/encoding/buffer.hpp"
#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#define RED_CODE "\033[91m"
#define END_CODE "\033[0m"
#define GREEN_CODE "\033[32m"

NS_LOG_COMPONENT_DEFINE("Vehicle");
namespace ns3 { namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Vehicle);
uint64_t Vehicle::g_totalRxDrops = 0;
uint64_t Vehicle::g_totalTxDrops = 0;

TypeId Vehicle::GetTypeId() {
  static TypeId tid = TypeId("ns3::ndn::Vehicle")
    .SetParent<App>()
    .AddConstructor<Vehicle>()
    .AddAttribute("BeaconInterval", "Beacon interval", TimeValue(Seconds(0.1)),
                  MakeTimeAccessor(&Vehicle::m_beaconInterval),
                  MakeTimeChecker())
    .AddAttribute("VehicleId", "Vehicle identifier", StringValue("veh-0"),
                  MakeStringAccessor(&Vehicle::m_vehicleId),
                  MakeStringChecker())
    ;
  return tid;
}

Vehicle::Vehicle(): m_seq(0){
  m_rand = CreateObject<UniformRandomVariable>();
}

Vehicle::~Vehicle() {}

void Vehicle::PhyRxDropTrace(Ptr<const Packet> packet, WifiPhyRxfailureReason reason)
{
  g_totalRxDrops++;
  std::cout << Simulator::Now().GetSeconds() << "s " << RED_CODE 
            << "[PHY DROP] Node " << GetNode()->GetId() 
            << " Reason Code: " << reason << END_CODE << std::endl;
}

void Vehicle::MonitorTx (const Ptr< const Packet > packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, uint16_t sta_id)
{
  std::cout << Now().GetSeconds() << GREEN_CODE << " >>>> Node " << GetNode()->GetId() << " transmitting : " << channelFreqMhz << END_CODE << "\n";
}

void Vehicle::PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu, SignalNoiseDbm sn, uint16_t sta_id) {

    std::cout << Now () << " PromiscRx() : Node " << GetNode()->GetId() << " : ChannelFreq: " << channelFreq << " Mode: " << tx.GetMode()
                 << " Signal: " << sn.signal << " Noise: " << sn.noise << " Size: " << packet->GetSize()
                 << " Mode " << tx.GetMode ();    
}

void Vehicle::PhyTxDropTrace(Ptr<const Packet> packet) {
  g_totalTxDrops++;
  std:: cout << Now().GetSeconds() << RED_CODE << " Node " << GetNode()->GetId() << " TxDrop. " << END_CODE << "\n";
}

void Vehicle::StartApplication()
{
  App::StartApplication();


  Time first = Seconds(m_rand->GetValue(0.0, m_beaconInterval.GetSeconds()));
  m_beaconEvent = Simulator::Schedule(first, &Vehicle::SendBeacon, this);

  // 2. NEW: Hook into the underlying WiFi Device for monitoring
  Ptr<Node> n = GetNode();
  for (uint32_t i = 0; i < n->GetNDevices(); i++)
  {
      Ptr<NetDevice> dev = n->GetDevice(i);
      
      // Check if this is a WifiNetDevice
      Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(dev);
      if (wifiDev) {
      
          Ptr<WifiPhy> phy = wifiDev->GetPhy();

          phy->TraceConnectWithoutContext ("MonitorSnifferRx", MakeCallback(&Vehicle::PromiscRx, this));
          phy->TraceConnectWithoutContext ("MonitorSnifferTx", MakeCallback(&Vehicle::MonitorTx, this));
          phy->TraceConnectWithoutContext ("PhyTxDrop", MakeCallback(&Vehicle::PhyTxDropTrace, this));
          phy->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&Vehicle::PhyRxDropTrace, this));
          break; 
      }
  }

}

void
Vehicle::StopApplication()
{
  App::StopApplication();
  Simulator::Cancel(m_beaconEvent);
}

void Vehicle::ScheduleNextBeacon() {
  m_beaconEvent = Simulator::Schedule(m_beaconInterval, &Vehicle::SendBeacon, this);
}

void Vehicle::SendBeacon() {
  // build name: /localhop/beacon/<veh-id>/<x>/<y>/<speed>/<seq>
  Ptr<Node> node = this->GetNode();
  Ptr<MobilityModel> mm = node->GetObject<MobilityModel>();
  Vector pos = mm->GetPosition();

  // speed: best-effort using mobility model (set 0 if not available)
  double speed = 0.0;
  if (mm->GetObject<ConstantVelocityMobilityModel>() != nullptr) {
    //check
    speed = mm->GetObject<ConstantVelocityMobilityModel>()->GetVelocity().x; // or magnitude
  }

  std::ostringstream oss;
  oss << "/beacon/"
      << m_vehicleId << "/"
      << std::fixed << pos.x << "/"
      << pos.y << "/"
      << speed << "/"
      << m_seq++;

  Name beaconName(oss.str());

  // create Interest; we do not expect a Data reply
  auto interest = std::make_shared<Interest>(beaconName);
  interest->setCanBePrefix(false);
  interest->setInterestLifetime(ndn::time::seconds(100));
 

  m_transmittedInterests(interest, this, m_face); //tracing
  m_appLink->onReceiveInterest(*interest);


  // NS_LOG_INFO(Simulator::Now().GetSeconds() << "s Sent beacon " << beaconName.toUri()
  //             << " from node " << node->GetId());

  ScheduleNextBeacon();
}

void Vehicle::OnInterest(std::shared_ptr<const Interest> interest) {
  App::OnInterest(interest);
  // std::cerr << "OnInterst called in Vehicle" << std::endl;
  Name name = interest->getName();
  std::string s = name.toUri();

  NS_LOG_INFO(Simulator::Now().GetSeconds()
                << " Received beacon: " << s
                << " at node " << this->GetNode()->GetId());
}

void Vehicle::PrintGlobalStats() {
    std::cout << "\n\n" << "==========================================" << std::endl;
    std::cout << "           GLOBAL SIMULATION STATS           " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Total PHY Rx Drops: " << g_totalRxDrops << std::endl;
    std::cout << "Total PHY Tx Drops: " << g_totalTxDrops << std::endl;
    std::cout << "Total Drops: " << g_totalTxDrops + g_totalRxDrops << std::endl;
    std::cout << "==========================================" << std::endl;
}

}} // namespace ns3::ndn
