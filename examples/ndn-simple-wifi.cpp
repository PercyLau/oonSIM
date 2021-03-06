#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM-module.h"

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.WiFiExample");

void
FileDownloadedTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName, double downloadSpeed, long milliSeconds)
{
  std::cout << "Trace: File finished downloading: " << Simulator::Now().GetMilliSeconds () << " "<< *interestName <<
     " Download Speed: " << downloadSpeed/1000.0 << " Kilobit/s in " << milliSeconds << " ms" << std::endl;
}

void
FileDownloadedManifestTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName, long fileSize)
{
  std::cout << "Trace: Manifest received: " << Simulator::Now().GetMilliSeconds () <<" "<< *interestName << " File Size: " << fileSize << std::endl;
}

void
FileDownloadStartedTrace(Ptr<ns3::ndn::App> app, shared_ptr<const ndn::Name> interestName)
{
  std::cout << "Trace: File started downloading: " << Simulator::Now().GetMilliSeconds () <<" "<< *interestName << std::endl;
}

int
main(int argc, char* argv[])
{
  // disable fragmentation
  Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
  Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
                     StringValue("OfdmRate24Mbps"));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  //////////////////////
  //////////////////////
  //////////////////////
  WifiHelper wifi = WifiHelper::Default();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
  //wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate24Mbps"));

  YansWifiChannelHelper wifiChannel; // = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel");
  wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel");

  // YansWifiPhy wifiPhy = YansWifiPhy::Default();
  YansWifiPhyHelper wifiPhyHelper = YansWifiPhyHelper::Default();
  wifiPhyHelper.SetChannel(wifiChannel.Create());
  wifiPhyHelper.Set("TxPowerStart", DoubleValue(1)); //power start and end must be the same
  wifiPhyHelper.Set("TxPowerEnd", DoubleValue(1));

  NqosWifiMacHelper wifiMacHelper = NqosWifiMacHelper::Default();
  wifiMacHelper.SetType("ns3::AdhocWifiMac");

  Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable>();
  randomizer->SetAttribute("Min", DoubleValue(10)); // 
  randomizer->SetAttribute("Max", DoubleValue(100));

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizer),
                                "Y", PointerValue(randomizer), "Z", PointerValue(randomizer));
  //mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  //mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
  //                               "MinX", DoubleValue (0.0),
  //                               "MinY", DoubleValue (50.0),
  //                               "DeltaX", DoubleValue (10.0),
  //                               "DeltaY", DoubleValue (10.0),
  //                               "GridWidth", UintegerValue (10),
  //                              "LayoutType", StringValue ("RowFirst"));


  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue (Rectangle(0,100,0,100)),"Distance",DoubleValue(4),"Speed",PointerValue(randomizer));

  NodeContainer nodes;
  nodes.Create(9);

  ////////////////
  // 1. Install Wifi
  NetDeviceContainer wifiNetDevices = wifi.Install(wifiPhyHelper, wifiMacHelper, nodes);

  // 2. Install Mobility model
  mobility.Install(nodes);

  // 3. Install NDN stack
  NS_LOG_INFO("Installing NDN stack");
  ndn::StackHelper ndnHelper;
  // ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback
  // (MyNetDeviceFaceCallback));
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "100");
  ndnHelper.setCsSize(100);
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.Install(nodes);

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::Install(nodes, "/", "ndn:/localhost/nfd/strategy/client-control");

  // 4. Set up applications
  NS_LOG_INFO("Installing Applications");

  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  ndn::AppHelper consumerHelper("ns3::ndn::FileConsumerCbr");

  //DASH parameters
  consumerHelper.SetAttribute("FileToRequest", StringValue("/myprefix/file1.img"));


  consumerHelper.Install(nodes.Get(0));
  //consumerHelper.SetAttribute("Frequency", DoubleValue(24.0));
  //consumerHelper.SetPrefix("/node1/prefix");
  consumerHelper.Install(nodes.Get(1));
  //consumerHelper.SetAttribute("Frequency", DoubleValue(10.0));
  //consumerHelper.SetPrefix("/node2/prefix");
  consumerHelper.Install(nodes.Get(2));
  //consumerHelper.SetAttribute("Frequency", DoubleValue(60.0));
  //consumerHelper.SetPrefix("/node3/prefix");
  consumerHelper.Install(nodes.Get(3));
  //consumerHelper.SetAttribute("Frequency", DoubleValue(40.0));
  //consumerHelper.SetPrefix("/node4/prefix");
  //consumerHelper.SetAttribute("Frequency", DoubleValue(50.0));
  consumerHelper.Install(nodes.Get(4));

  //ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Connect Tracers
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/FileDownloadFinished",
                               MakeCallback(&FileDownloadedTrace));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/ManifestReceived",
                               MakeCallback(&FileDownloadedManifestTrace));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/FileDownloadStarted",
                               MakeCallback(&FileDownloadStartedTrace));

  //

  //DASH parameters
  ndn::AppHelper producerHelper("ns3::ndn::FileServer");
  producerHelper.SetPrefix("/myprefix");
  producerHelper.SetAttribute("ContentDirectory", StringValue("/home/someuser/somedata"));
  producerHelper.Install(nodes.Get(8));
  //producerHelper.Install(nodes.Get(2));
  //producerHelper.Install(nodes.Get(3));
  //producerHelper.Install(nodes.Get(4));
  //producerHelper.Install(nodes.Get(5));
  //producerHelper.Install(nodes.Get(6));
  producerHelper.Install(nodes.Get(7));
  //producerHelper.Install(nodes.Get(8));
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  //ndnGlobalRoutingHelper.AddOrigins("/myprefix",nodes.Get(1));
  ndnGlobalRoutingHelper.AddOrigins("/myprefix",nodes.Get(8));
  //ndnGlobalRoutingHelper.AddOrigins("/myprefix",nodes.Get(3));
  //ndnGlobalRoutingHelper.AddOrigins("/myprefix",nodes.Get(4));
  //ndnGlobalRoutingHelper.AddOrigins("/myprefix",nodes.Get(5));
  //ndnGlobalRoutingHelper.AddOrigins("/myprefix",nodes.Get(6));
  //ndnGlobalRoutingHelper.AddOrigins("/myprefix",nodes.Get(7));
  //ndnGlobalRoutingHelper.AddOrigins("/myprefix",nodes.Get(8));
  ndn::GlobalRoutingHelper::CalculateRoutes();

  //producerHelper.Install(nodes.Get(6));
  //producerHelper.SetPrefix("/node1/prefix");
  //producerHelper.SetAttribute("PayloadSize", StringValue("1500"));
  //producerHelper.Install(nodes.Get(7));
  //producerHelper.SetPrefix("/");
  //producerHelper.SetAttribute("PayloadSize", StringValue("1500"));
  //producerHelper.Install(nodes.Get(8));
  ////////////////

  Simulator::Stop(Seconds(80.0));

  ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
  ndn::FileConsumerLogTracer::InstallAll("file-consumer-log-trace.txt");

  Simulator::Run();
  Simulator::Destroy();

  NS_LOG_UNCOND("Simulation Finished.");
  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}