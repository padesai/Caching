#pragma once 

typedef uint32_t FileId_t;
typedef uint32_t NumOfAccesses_t;
typedef uint64_t TimeStamp_t;
typedef uint32_t StarId_t;
typedef uint32_t LeafId_t;

#include <vector>
#include <map>
#include <queue>
#include <cstdint>

extern uint32_t minFileId;
extern uint32_t maxFileId;
extern uint32_t minLeafFileId;
extern uint32_t maxLeafFileId;

extern uint32_t MaxPacketSize;
extern uint32_t maxPacketCount;
extern double interPacketTime;
extern uint32_t cacheSize;