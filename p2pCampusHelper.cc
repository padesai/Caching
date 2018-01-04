
#include "p2pCampusHelper.h"

#include "HubAndServerInfo.h"
#include "MyUdpClient-ServerHelper.h"
#include "MyUdpClient.h"
#include "MyTag.h"


uint16_t portLeaf = 4000;

ApplicationContainer serverEchoApps;
ApplicationContainer serverTransmitEchoApps;

ApplicationContainer leafSendToHubApps;
ApplicationContainer leafReceiveFromHubApps;

ApplicationContainer hubReceiveFromLeafApps;
ApplicationContainer hubSendToServerApps;
ApplicationContainer hubReceiveFromServerApps;
ApplicationContainer hubSendToLeafApps;

std::map<LeafId_t, fileStats> leafStats;

#define PRINT_CALLBACKS

// --------------------------- CALLBACKS - Begin --------------------------------------------------------------------
void HubReceiveFromLeafCallBack(int leafId, int starId, PointToPointCampusHelper *p2p, Ptr<const Packet> item) 
{

  MyTag tag;
  item->PeekPacketTag(tag);
  uint32_t fileId = static_cast<uint32_t>(tag.GetTag());
  if (fileId > maxFileId)
  {
    return;
  }
  #ifdef PRINT_CALLBACKS
  std::cout << __FUNCTION__ <<  ": star_id = " << starId << "  leaf_id = " << leafId << " file_id = " << fileId <<  std::endl << std::endl;
  #endif
  StarInfo& starInfo = p2p->starInfoList_[starId];
  HubInfo& hubInfo = hubInfoList_[starId];

  if (starInfo.isFileCacheMiss(fileId))
  {
    hubInfo.cacheMissLeafRequests.push(fileId);  // if MISS, then push it in the miss queue (to be sent to server)
    hubInfo.cacheMissLeafRequestsMap[fileId] = leafId; 
  }
  else
  {
    hubInfo.leafResponses[leafId].push(fileId); // if HIT, then push it in the reply queue (to be sent to leaf)  
  }
  starInfo.accessFileCache(fileId);
}

void HubReceiveFromServerCallBack(int starId, PointToPointCampusHelper *p2p, Ptr<const Packet> item)
{ 
  MyTag tag;
  item->PeekPacketTag(tag);
  uint32_t fileId = static_cast<uint32_t>(tag.GetTag());
  if (fileId > maxFileId)
  {
    return;
  }  

  #ifdef PRINT_CALLBACKS
  std::cout << __FUNCTION__ <<  ": star_id = " << starId << " file_id = " << fileId <<  std::endl << std::endl;
  #endif
  HubInfo& hubInfo = hubInfoList_[starId];

  LeafId_t leafId = 0;
  std::map<FileId_t, LeafId_t>::iterator it = hubInfo.cacheMissLeafRequestsMap.find(fileId);
  bool wasAFileCacheMiss = hubInfo.cacheMissLeafRequestsMap.end() != it;
  if (wasAFileCacheMiss)
  {
    leafId = it->second;
    hubInfo.cacheMissLeafRequestsMap.erase(fileId);
  }
  hubInfo.leafResponses[leafId].push(fileId);              // put in the send queue to be sent to a particular leaf
   
  if (wasAFileCacheMiss)
  {
    StarInfo& starInfo = p2p->starInfoList_[starId];
    starInfo.accessFileCache(fileId);                   // update the cache statistics if it was a miss (in case of hits already done) 
  }
}

void LeafReceiveFromHubCallBack(int leafId, int starId, PointToPointCampusHelper *p2p, Ptr<const Packet> item)
{
  MyTag tag;
  item->PeekPacketTag(tag);
  uint32_t fileId = static_cast<uint32_t>(tag.GetTag());
  if (fileId > maxFileId)
  {
    return;
  } 
  #ifdef PRINT_CALLBACKS
  std::cout << __FUNCTION__ <<  ": star_id = " << starId << "  leaf_id = " << leafId << " file_id = " << fileId <<  std::endl << std::endl;
  #endif

 //StarInfo& starInfo = p2p->starInfoList_[starId];
 // starInfo.numOfPacketsTransfered[leafId] += 1;
 leafStats[leafId][fileId].endTime = Simulator::Now().GetSeconds();
 leafStats[leafId][fileId].elapsedTime += (leafStats[leafId][fileId].endTime - leafStats[leafId][fileId].startTime);
 //std::cout << "Leaf Rx, file id = " << fileId << " , time = " << leafStats[leafId][fileId].endTime << std::endl;
}

void ServerReceiveFromHubCallBack(int starId, PointToPointCampusHelper *p2p, Ptr<const Packet> item)
{
  MyTag tag;
  item->PeekPacketTag(tag);
  uint32_t fileId = static_cast<uint32_t>(tag.GetTag());
  if (fileId > maxFileId)
  {
    return;
  }   
  #ifdef PRINT_CALLBACKS
  std::cout << __FUNCTION__ <<  ": star_id = " << starId << " file_id = " << fileId <<  std::endl << std::endl;
  #endif

  serverInfo_.hubRequests[starId].push(fileId);                // server sender will pick from this queue
}

void LeafTransmitToHubCallBack(int leafId, int starId, PointToPointCampusHelper *p2p, Ptr<const Packet> item)
{
  MyTag tag;
  item->PeekPacketTag(tag);
  uint32_t fileId = static_cast<uint32_t>(tag.GetTag());
  if (fileId > maxFileId)
  {
    return;
  } 
  #ifdef PRINT_CALLBACKS
  std::cout << __FUNCTION__ <<  ": star_id = " << starId << "  leaf_id = " << leafId << " file_id = " << fileId <<  std::endl << std::endl;
  #endif  
 leafStats[leafId][fileId].startTime = Simulator::Now().GetSeconds();
 leafStats[leafId][fileId].numOfAccesses += 1;
 //std::cout << "Leaf Tx, file id = " << fileId << " , time = " << Simulator::Now().GetSeconds() << std::endl;
}
// ---------------------------- CALLBACKS - END ------------------------------------------------------

void PointToPointCampusHelper::createMemCacheTopology()
{
  std::cout << "Creating Mem Cache topology ..... \n";  
  
  starInfoList_.resize(numOfStars_);
  hubInfoList_.resize(numOfStars_);
  serverInfo_.serverToHubSender.resize(numOfStars_);

  serverNode_.Create(1);

  InternetStackHelper stack;
  stack.Install(serverNode_);

  for (uint32_t starNum = 0; starNum < numOfStars_; ++starNum)
  {
    createMemCacheTopologyForAStar(starNum);
  }

  std::cout << "Mem Cache topology created and initalized ..... \n";
}

void PointToPointCampusHelper::createMemCacheTopologyForAStar(StarId_t starId)
{
  uint32_t starNum = starId;

  HubInfo& hubInfo = hubInfoList_[starNum];
  hubInfo.init(starNum, numOfSpokesPerStar_);

  hubInfo.hubToLeafSender.resize(numOfSpokesPerStar_);

  InternetStackHelper stack;

  uint16_t leafHubPort = 5000 + starId;
  uint16_t hubServerPort = 6000 + starId;

  Time interPacketInterval = Seconds (interPacketTime);

  NetDeviceContainer p2pLeafHubNetDevices;
  NetDeviceContainer p2pCentralNetDevices;

  StarInfo& starInfo = starInfoList_[starNum];
  initStarInfo(starNum, p2pHubSpoke_, stack);


  //---------------------------------Create the star topology------------------------------
  Ipv4AddressHelper address;
  address.SetBase(("10." + std::to_string(starNum + 1) + ".1.0").c_str(), "255.255.255.0");     
  starInfo.starHelper.AssignIpv4Addresses(address);

  p2pCentralNetDevices = p2pServerHub_.Install(starInfo.starHelper.GetHub(),serverNode_.Get(0));
  devices_.Add(p2pCentralNetDevices);

  //Assign IP Addresses to the links between each star and the central serverNode_
  Ipv4AddressHelper address1;
  address1.SetBase(("10." + std::to_string(starNum + 200) + ".1.0").c_str(), "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address1.Assign(p2pCentralNetDevices);

  /*
  std::cout << "Server IP -- " << interfaces.GetAddress(1) << std::endl;
  std::cout << "Hub IP -- " << interfaces.GetAddress(0) << std::endl;
  std::cout << "Connections between Hub and Leaf serverNode_s " << std::endl;
  std::cout << "Hub IP -- " << starInfo.starHelper.GetHubIpv4Address(0) << std::endl;
  for (uint32_t j = 0; j < starInfo.starHelper.SpokeCount(); j++)
  {
    std::cout << "Spokes IP -- " << starInfo.starHelper.GetSpokeIpv4Address(j) << std::endl;
  }
  */
  // ------------------------------------- LEAF SEND TO HUB ------------------------------------------------------------------------------

  MyUdpClientHelper* leafToHubSender = new MyUdpClientHelper(&hubInfo, LEAF_TO_HUB_SENDER, starInfo.starHelper.GetHubIpv4Address(0), leafHubPort);
  leafToHubSender->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount)); 
  leafToHubSender->SetAttribute ("Interval", TimeValue (interPacketInterval));
  leafToHubSender->SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
  hubInfo.leafToHubSender = leafToHubSender;

  for (uint32_t leafId = 0; leafId < starInfo.starHelper.SpokeCount(); ++leafId)
  {
    leafSendToHubApps = leafToHubSender->Install(starInfo.starHelper.GetSpokeNode(leafId), leafId);
  }

  for (uint32_t leafId = 0; leafId < starInfo.starHelper.SpokeCount(); ++leafId)
  {
   starInfo.starHelper.GetHub()->GetDevice(leafId)->TraceConnectWithoutContext("PhyRxEnd", MakeBoundCallback(&HubReceiveFromLeafCallBack, leafId, starNum, this)); 
   starInfo.starHelper.GetHub()->GetDevice(leafId)->TraceConnectWithoutContext("PhyTxEnd", MakeBoundCallback(&LeafTransmitToHubCallBack, leafId, starNum, this)); 
  }

  UdpServerHelper hubReceiveFromLeafServer(leafHubPort);
  hubReceiveFromLeafApps = hubReceiveFromLeafServer.Install(starInfo.starHelper.GetHub());

  // ---------------------------------------------------------------------------------------------------------------------------------------


  // ------------------------------------- HUB SEND TO SERVER  ------------------------------------------------------------------------------

  MyUdpClientHelper* hubToServerSender = new MyUdpClientHelper(&hubInfo, HUB_TO_SERVER_SENDER, interfaces.GetAddress(1), hubServerPort);
  hubToServerSender->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount)); 
  hubToServerSender->SetAttribute ("Interval", TimeValue (interPacketInterval));
  hubToServerSender->SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
  hubSendToServerApps = hubToServerSender->Install(starInfo.starHelper.GetHub());
  hubInfo.hubToServerSender = hubToServerSender;
 
  UdpServerHelper serverReceiveFromHubServer(hubServerPort);
  serverEchoApps = serverReceiveFromHubServer.Install (serverNode_.Get (0));
  p2pCentralNetDevices.Get(1)->TraceConnectWithoutContext("PhyRxEnd", MakeBoundCallback(&ServerReceiveFromHubCallBack, starNum, this)); 
 
 // ---------------------------------------------------------------------------------------------------------------------------------------

 // ------------------------------------- SERVER SEND TO HUB ------------------------------------------------------------------------------ 
  MyUdpClientHelper* serverToHubSender = new MyUdpClientHelper(&hubInfo, SERVER_TO_HUB_SENDER, interfaces.GetAddress(0), hubServerPort);
  serverToHubSender->SetAttribute("MaxPackets", UintegerValue (maxPacketCount)); 
  serverToHubSender->SetAttribute("Interval", TimeValue (interPacketInterval));
  serverToHubSender->SetAttribute("PacketSize", UintegerValue (MaxPacketSize));
  serverTransmitEchoApps = serverToHubSender->Install (serverNode_.Get (0));
  serverInfo_.serverToHubSender[starNum] = serverToHubSender;  

  UdpServerHelper hubReceiveFromServerServer(hubServerPort);
  hubReceiveFromServerApps = hubReceiveFromServerServer.Install(starInfo.starHelper.GetHub());
  p2pCentralNetDevices.Get(0)->TraceConnectWithoutContext("PhyRxEnd", MakeBoundCallback(&HubReceiveFromServerCallBack, starNum, this)); 

 // ---------------------------------------------------------------------------------------------------------------------------------------
 

 // ------------------------------------- HUB SEND TO LEAF ------------------------------------------------------------------------------ 
 
   std::cout << "Spoke Count = " << starInfo.starHelper.SpokeCount() << std::endl;
   for (uint32_t j = 0; j < starInfo.starHelper.SpokeCount();j++)
   {
     MyUdpClientHelper* temp =  new MyUdpClientHelper(&hubInfo, HUB_TO_LEAF_SENDER, starInfo.starHelper.GetSpokeIpv4Address(j), portLeaf);
     temp->SetAttribute ("MaxPackets", UintegerValue (maxPacketCount)); 
     temp->SetAttribute ("Interval", TimeValue (interPacketInterval));
     temp->SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));   
     hubSendToLeafApps = temp->Install(starInfo.starHelper.GetHub(), j);
     hubInfo.hubToLeafSender[j] = temp;
  }

  for (uint32_t j = 0; j < starInfo.starHelper.SpokeCount();j++)
  {
    UdpServerHelper leafReceiveFromHubServer(leafHubPort);
    leafReceiveFromHubApps = leafReceiveFromHubServer.Install(starInfo.starHelper.GetSpokeNode(j));
  }

  for (uint32_t j = 0; j < starInfo.starHelper.SpokeCount();j++)
  {
    starInfo.starHelper.GetSpokeNode(j)->GetDevice(0)->TraceConnectWithoutContext("PhyRxEnd", MakeBoundCallback(&LeafReceiveFromHubCallBack,starNum, j, this)); 
  }

 // ---------------------------------------------------------------------------------------------------------------------------------------
 
  serverEchoApps.Start(Seconds (0.1));
  serverEchoApps.Stop (Seconds (5.0));
  
  leafReceiveFromHubApps.Start(Seconds (0.1));
  leafReceiveFromHubApps.Stop (Seconds (5.0));

  hubReceiveFromLeafApps.Start(Seconds (0.1));
  hubReceiveFromLeafApps.Stop (Seconds (5.0));

  hubReceiveFromServerApps.Start(Seconds (0.1));
  hubReceiveFromServerApps.Stop (Seconds (5.0)); 

  serverTransmitEchoApps.Start (Seconds (0.2));
  serverEchoApps.Stop (Seconds (5.0));

  leafSendToHubApps.Start (Seconds (0.2));
  leafSendToHubApps.Stop (Seconds (5.0));

  hubSendToServerApps.Start (Seconds (0.2));
  hubSendToServerApps.Stop (Seconds (5.0));

  hubSendToLeafApps.Start (Seconds (0.2));
  hubSendToLeafApps.Stop (Seconds (5.0));

// ----------------------------------------------------------------------------------------------------------------   
}


// ---------------------------------------------------------------------------------------------------------------------------------------------

 PointToPointCampusHelper::PointToPointCampusHelper()
  : devices_()
  , starInfoList_()
  , globalFileStore_()
  , fileCacheSize_(cacheSize)
  , spokeToStarHubDelay_(0)
  , hubToCentralServerDelay_(0)
  , numOfSpokesPerStar_(0)
  , numOfStars_(0)
  , p2pServerHub_()
  , p2pHubSpoke_()
  , randVar_(CreateObject<UniformRandomVariable>())
  , missedTagList_()
{
  randVar_->SetAttribute ("Min", DoubleValue (minLeafFileId));
  randVar_->SetAttribute ("Max", DoubleValue (maxLeafFileId));
}

PointToPointCampusHelper::~PointToPointCampusHelper()
{
  for (uint32_t i = 0; i < hubInfoList_.size(); ++i)
  {
    HubInfo& hubInfo = hubInfoList_[i];
    for (uint32_t l = 0; l < hubInfo.hubToLeafSender.size(); ++l)
    {
      delete hubInfo.hubToLeafSender[l];
    }
    
    hubInfo.hubToLeafSender.clear();
    delete hubInfo.leafToHubSender;
    delete hubInfo.hubToServerSender;
  }

  for (uint32_t h = 0; h < serverInfo_.serverToHubSender.size(); ++h)
  {
    delete serverInfo_.serverToHubSender[h];
  }
  
  hubInfoList_.clear();
}

void PointToPointCampusHelper::setServerInfo(PointToPointHelper p2pServerHub, uint32_t numOfStars)
{
  p2pServerHub_ = p2pServerHub;
  numOfStars_ = numOfStars;
}

void PointToPointCampusHelper::setStarInfo(PointToPointHelper p2pHubSpoke, uint32_t numOfSpokesPerStar)
{
  p2pHubSpoke_ = p2pHubSpoke;
  numOfSpokesPerStar_ = numOfSpokesPerStar;
}

void PointToPointCampusHelper::initStarInfo(uint32_t starNumber, PointToPointHelper p2pStar, InternetStackHelper &stack)
{
  starInfoList_[starNumber].MAX_CACHE_SIZE = fileCacheSize_;
  starInfoList_[starNumber].missDelay = hubToCentralServerDelay_;
  starInfoList_[starNumber].hitDelay = spokeToStarHubDelay_;
  starInfoList_[starNumber].starNumber = starNumber;
  starInfoList_[starNumber].starHelper = PointToPointStarHelper(numOfSpokesPerStar_, p2pStar);
  starInfoList_[starNumber].starHelper.InstallStack(stack);

  starInfoList_[starNumber].numOfPacketsTransfered.resize(numOfSpokesPerStar_);
  for (uint32_t i = 0; i < numOfSpokesPerStar_; ++i)
  {
    starInfoList_[starNumber].numOfPacketsTransfered[i]  = 0;
  }
}

void PointToPointCampusHelper::pushMissedTag(uint64_t missedFileTag)
{
  missedTagList_.push(missedFileTag);
}

 uint64_t PointToPointCampusHelper::getTotalDelay()
{
  uint64_t totalAccessDelay = 0;
  for (uint32_t i = 0; i < starInfoList_.size(); ++i)
  {
    totalAccessDelay += starInfoList_[i].totalAccessDelay;
  }
  return totalAccessDelay;
}
