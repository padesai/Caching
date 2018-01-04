#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <iostream>
#include "MyTag.h"

namespace ns3 
{

TypeId 
MyTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyTag")
    .SetParent<Tag> ()
    .AddConstructor<MyTag> ()
    .AddAttribute ("TagData",
                   "File Id",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&MyTag::GetTag),
                   MakeUintegerChecker<uint64_t> ())
  ;
  return tid;
}

TypeId 
MyTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
MyTag::GetSerializedSize (void) const
{
  return sizeof(tagValue);
}

void 
MyTag::Serialize (TagBuffer i) const
{
  i.WriteU64(tagValue);
}

void 
MyTag::Deserialize (TagBuffer i)
{
  tagValue = i.ReadU64();
}

void 
MyTag::Print (std::ostream &os) const
{
  os << " tagValue = " << tagValue << std::endl;
}

void 
MyTag::SetTag(uint64_t value)
{
  tagValue = value;
}

uint64_t 
MyTag::GetTag(void) const
{
  return tagValue;
}
}