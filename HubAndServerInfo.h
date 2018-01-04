#ifndef HUB_AND_SERVER_INFO_H_
#define HUB_AND_SERVER_INFO_H_
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-star.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv6-interface-container.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"


#include "MemCacheDefines.h"

#include <map>
#include <vector>
#include <queue>

namespace ns3
{
 class MyUdpClientHelper;
 class MyUdpHubClient;
}


struct ServerInfo
{
 std::vector<ns3::MyUdpClientHelper*> serverToHubSender;
 std::map<StarId_t, std::queue<FileId_t>> hubRequests;    // hub requests, each server sender picks up from its own queue
};

struct HubInfo
{

  HubInfo();
  
  void init(uint32_t starId, uint32_t numOfSpokes);
  bool canHubSendToServer();
  bool canLeafSendToHub();
  bool canServerSendToHub();
  bool canHubSendToLeaf(uint32_t leafId);

  uint64_t getRandomFrameId(uint32_t leafId);
  uint64_t getNextHubTxTag();
  uint64_t getHubToLeafResponseFileId(uint32_t leafId);
  uint64_t getServerToHubResponseFileId();
  
  std::vector<ns3::Ptr<ns3::UniformRandomVariable>> randVar_;

  StarId_t starId;
  uint32_t numOfSpokes;

  std::vector<ns3::MyUdpClientHelper*> hubToLeafSender;   
  ns3::MyUdpClientHelper* leafToHubSender;
  ns3::MyUdpClientHelper* hubToServerSender;

  std::map<FileId_t, LeafId_t> cacheMissLeafRequestsMap;    // map of <file_id, leaf_id> for misses
  std::queue<FileId_t> cacheMissLeafRequests;               // misses, to be sent to server
  std::map<LeafId_t, std::queue<FileId_t>> leafResponses;  // responses to be sent back to leaf (separate response queue for each leaf)                                                           // (reply from server for misses + reply for hits from hub)
};

extern ServerInfo serverInfo_;
extern std::vector<HubInfo> hubInfoList_;
#endif