#include "apps/ndn-app.hpp" 
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"
#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"

namespace ns3 {
namespace ndn {

class Vehicle: public App {
public:
  static TypeId GetTypeId(); 
  void SendEventToRsu(uint32_t rsuNodeId,
                      const std::string &eventType,
                      const std::string &timestamp,
                      double x, double y);
  Vehicle();
  virtual ~Vehicle();
  static uint64_t g_totalRxDrops; 
  static uint64_t g_totalTxDrops;
  static void PrintGlobalStats();

protected:
  virtual void StartApplication() override; 
  virtual void StopApplication() override;


  virtual void OnInterest(std::shared_ptr<const Interest> interest) override;

  // schedule and send periodic beacon
  void ScheduleNextBeacon();
  void SendBeacon();


private:
  Time m_beaconInterval;
  EventId m_beaconEvent;
  uint32_t m_seq;               // beacon sequence number
  std::string m_vehicleId;      // configured vehicle id
  Ptr<UniformRandomVariable> m_rand;
  void PhyRxDropTrace(Ptr<const Packet> packet, WifiPhyRxfailureReason reason);
  void MonitorTx(const Ptr< const Packet > packet, uint16_t channelFreqMhz, WifiTxVector txVector, MpduInfo aMpdu, uint16_t sta_id);
  void PromiscRx (Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu, SignalNoiseDbm sn, uint16_t sta_id);
  void PhyTxDropTrace(Ptr<const Packet> packet);
};

} // namespace ndn
} // namespace ns3
