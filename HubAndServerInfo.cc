#include "HubAndServerInfo.h"


using namespace ns3;

ServerInfo serverInfo_;
std::vector<HubInfo> hubInfoList_;

HubInfo::HubInfo()
{
}

bool HubInfo::canHubSendToServer()
{ 
	return !cacheMissLeafRequests.empty();
}

bool HubInfo::canLeafSendToHub()
{ 
	return true;
}

bool HubInfo::canServerSendToHub()
{ 
	return !serverInfo_.hubRequests[starId].empty(); 
}

bool HubInfo::canHubSendToLeaf(uint32_t leafId) 
{ 
	return !leafResponses[leafId].empty();
}

uint64_t HubInfo::getRandomFrameId(uint32_t leafId)  
{ 
  uint64_t tag = randVar_[leafId]->GetInteger();
  return tag;
}

uint64_t HubInfo::getNextHubTxTag()
{
  uint64_t tag = 0;
  if (canHubSendToServer())
  {
    tag = cacheMissLeafRequests.front();
    cacheMissLeafRequests.pop();
  }
  return tag;
}

uint64_t HubInfo::getHubToLeafResponseFileId(uint32_t leafId)
{
  uint64_t tag = 0;
  if (canHubSendToLeaf(leafId))
  {
    tag = leafResponses[leafId].front();
    leafResponses[leafId].pop();
  }
  return tag;
}

uint64_t HubInfo::getServerToHubResponseFileId()
{
  uint64_t tag = 0;
  if (canServerSendToHub())
  {
    tag = serverInfo_.hubRequests[starId].front();
    serverInfo_.hubRequests[starId].pop();
  }
  return tag;
}

void HubInfo::init(uint32_t starId, uint32_t numOfSpokes)
{
  this->starId = starId;
  this->numOfSpokes = numOfSpokes;
  randVar_.resize(numOfSpokes);
  for (uint32_t i = 0; i < numOfSpokes; ++i)
  {
  	randVar_[i] = CreateObject<UniformRandomVariable>();
  	//randVar_[i] = CreateObject<SequentialRandomVariable>();
  	randVar_[i]->SetAttribute ("Min", DoubleValue (minLeafFileId));
    randVar_[i]->SetAttribute ("Max", DoubleValue (maxLeafFileId));
  }
}