#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

#include "p2pCampusHelper.h"
#include "ns3/mpi-interface.h"

const uint32_t numOfStars = 4;
const uint32_t numOfSpokesPerStar = 8;

uint32_t minFileId = 0;
uint32_t maxFileId = 1024;

uint32_t minLeafFileId = 0;
uint32_t maxLeafFileId = 32;

uint32_t MaxPacketSize = 1024;
uint32_t maxPacketCount = 64;
double interPacketTime = 0.0001;

uint32_t cacheSize = 16;

std::string hubToServerBW = "100Mbps";
std::string hubToServerDelay = "100ms";

std::string spokeToHubBW = "100Mbps";
std::string spokeToHubDelay = "1ms";

int main(int argc, char** argv)
{
  PointToPointCampusHelper p2pCampusHelper;

  std::cout << "1" << std::endl;

  PointToPointHelper p2pServerHub;
  p2pServerHub.SetDeviceAttribute ("DataRate", StringValue(hubToServerBW));
  p2pServerHub.SetChannelAttribute ("Delay", StringValue(hubToServerDelay));
  p2pCampusHelper.setServerInfo(p2pServerHub, numOfStars);
  

  PointToPointHelper p2pHubSpoke;
  p2pHubSpoke.SetDeviceAttribute ("DataRate", StringValue(spokeToHubBW));
  p2pHubSpoke.SetChannelAttribute ("Delay", StringValue(spokeToHubDelay));
  p2pCampusHelper.setStarInfo(p2pHubSpoke, numOfSpokesPerStar);
  
  p2pCampusHelper.createMemCacheTopology();

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  std::cout << "Hello" << std::endl;
  Simulator::Stop(Seconds(10.0));  

  Simulator::Run();
   
  uint32_t numOfPackets = 0;
  for (uint32_t i = 0; i < numOfStars; ++i)
  {
    StarInfo& starInfo = p2pCampusHelper.starInfoList_[i];
    for (uint32_t j =0; j < numOfSpokesPerStar; ++j)
    {
      numOfPackets += starInfo.numOfPacketsTransfered[j];
    }
  }

  uint32_t numOfFileAccess = 0;
  double totalElapsedTime = 0;
  for (auto& it: leafStats)
  {
    for (auto& statIt: it.second)
    {
      Stats& stat = statIt.second;
      numOfFileAccess += stat.numOfAccesses;
      totalElapsedTime += stat.elapsedTime;
    }
  }
  std::cout << "\n======================================== SUMMARY =======================================\n";
  std::cout << "\nNumOfStars = " << numOfStars << "\nNumOfSpokesPerStar = " << numOfSpokesPerStar << std::endl;
  std::cout << "\nhubToServerBandWidth = " << hubToServerBW << "\nHubToServerDelay = " << hubToServerDelay << std::endl;
  std::cout << "\nSpokeToHubBandWidth = " << spokeToHubBW << "\nSpokeToHubDelay = " << spokeToHubDelay << std::endl;
  std::cout << "\nMaxPacketCount = " << maxPacketCount << "\nInterPacketInterval = " << interPacketTime << std::endl;
  std::cout << "\nCacheSize = " << cacheSize << " file entries" << std::endl; 
  std::cout << "\nTotal Files Accessed = " << numOfFileAccess << "\tTotal Time for File Access = " 
  << totalElapsedTime << " s " << std::endl;
  std::cout << "\nAvg. File Access Time Of System = " << (totalElapsedTime / numOfFileAccess) << " s" << std::endl;
  std::cout << "\n=========================================================================================\n";
  Simulator::Destroy();

  return 0;

}
