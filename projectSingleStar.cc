#include <iostream>
#include <sstream>
#include <chrono>
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-star.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

int main(int argc, char** argv)
{

  std::string hubToCentralLinkBW = "100Mbps";
  std::string hubToCentralLinkDelay = "100ms";

  PointToPointHelper p2pHelper;
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue(hubToCentralLinkBW));
  p2pHelper.SetChannelAttribute ("Delay", StringValue(hubToCentralLinkDelay));

  PointToPointStarHelper p2pSingleStar(1024,p2pHelper);

  InternetStackHelper stack;
  p2pSingleStar.InstallStack(stack);

  Ipv4AddressHelper address; 
  address.SetBase("10.1.1.0", "255.255.255.0");
  p2pSingleStar.AssignIpv4Addresses(address);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  std::cout << "Hello from Single Star" << std::endl;
 
  //Simulator::Run();

  //Simulator::Destroy();

  return 0;

}
