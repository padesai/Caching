#pragma once

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

using namespace ns3;

struct Stats
{
  Stats()
  {
    startTime = 0;
    endTime = 0; 
    elapsedTime = 0;
    numOfAccesses = 0;
  }

  double startTime;
  double endTime;
  double elapsedTime;
  uint32_t numOfAccesses;
};

typedef std::map<FileId_t, Stats> fileStats;

extern std::map<LeafId_t, fileStats> leafStats;

typedef std::map<FileId_t, std::pair<NumOfAccesses_t, TimeStamp_t>> FileCacheTable_t;

namespace ns3
{
 class MyUdpClientHelper;
 class MyUdpHubClient;
}


struct StarInfo
{
  StarInfo()
    : starHelper(0, PointToPointHelper())
    , fileCache()
    , starNumber(0)
    , totalAccessDelay(0)
    , currentTimeStamp(0)
    , MAX_CACHE_SIZE(cacheSize)
    , totalHits(0)
    , totalMisses(0)
    , mruAccess(0)
    , missDelay(0)
    , hitDelay(0)
  {
    flushFileCache();
  }

  void flushFileCache()
  {
    fileCache.clear();
  }

  void accessFileCache(FileId_t fileId)
  {
    currentTimeStamp++;
    if (MAX_CACHE_SIZE == 0)
    {
      //std::cout << "Max cache size 0 \n";
      return;
    }

    if (isFileCacheMiss(fileId))  // item not cached
    {
      std::cout << "File Cache Miss : " << " File Id - " << fileId << " , star - " << starNumber << std::endl;
      if (isCacheFull())
      {
        std::cout << "File Cache Full : " << " File Id - " << fileId << " , star - " << starNumber << std::endl;
        evictCacheItem();
      }
      fileCache[fileId] = std::make_pair<NumOfAccesses_t, TimeStamp_t>(0, 0);
      totalMisses += 1;
    }
    else
    {
      std::cout << "File Cache Hit : " << " File Id - " << fileId << " , star - " << starNumber << std::endl;
      totalHits += 1;
    }
    fileCache[fileId].first++;
    fileCache[fileId].second = currentTimeStamp;
    mruAccess = fileId;
  }

  bool isFileCacheMiss(FileId_t fileId)
  {
    return (fileCache.end() == fileCache.find(fileId));
  }

  uint64_t getTotalDelay()
  {
    totalAccessDelay = (hitDelay * (totalHits + totalMisses)) + (totalMisses * missDelay);
    return totalAccessDelay;
  }

  bool isCacheFull()
  {
    return (fileCache.size() == MAX_CACHE_SIZE) && (!fileCache.empty());
  }

  void evictCacheItem()
  {
    std::cout << "File Cache Eviction : " << " File Id - " << mruAccess << " , star - " << starNumber << std::endl; 
    fileCache.erase(mruAccess);
  }

  PointToPointStarHelper starHelper;
  FileCacheTable_t fileCache;

  uint32_t starNumber;
  uint32_t totalAccessDelay;
  uint64_t currentTimeStamp;
  size_t MAX_CACHE_SIZE;
  uint64_t totalHits;
  uint64_t totalMisses;
  FileId_t mruAccess;
  
  std::vector<uint64_t> numOfPacketsTransfered;
  uint32_t missDelay;
  uint32_t hitDelay;
};


class PointToPointCampusHelper
{
public:
  PointToPointCampusHelper();

  ~PointToPointCampusHelper();

  void pushMissedTag(uint64_t missedFileTag);

  uint64_t getTotalDelay();

  void setServerInfo(PointToPointHelper p2pServerHub, uint32_t numOfStars);
  void setStarInfo(PointToPointHelper p2pHubSpoke, uint32_t numOfSpokesPerStar_);

  void createMemCacheTopology();
  void createMemCacheTopologyForAStar(StarId_t starId);

  friend void receiveCallBack(int starId, FileId_t fileId, PointToPointCampusHelper *p2p, Ptr<const Packet> item);
  void initStarInfo(uint32_t starNumber, PointToPointHelper p2pStar, InternetStackHelper &stack);

  void Send (void);
  
  NetDeviceContainer devices_;

  
  std::vector<StarInfo> starInfoList_;
  Ipv4InterfaceContainer interfaces;

  FileCacheTable_t globalFileStore_;

  uint32_t fileCacheSize_;
  uint32_t spokeToStarHubDelay_;
  uint32_t hubToCentralServerDelay_;
  uint32_t numOfSpokesPerStar_;
  uint32_t numOfStars_;

  PointToPointHelper p2pServerHub_;
  PointToPointHelper p2pHubSpoke_;

  NodeContainer serverNode_;

  Ptr<UniformRandomVariable> randVar_;
  std::queue<FileId_t> missedTagList_;
  bool canLeafSendToHub_;
};

