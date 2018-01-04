#ifndef MYTAG_H
#define MYTAG_H
#include "ns3/object-base.h"
#include "ns3/tag-buffer.h"
#include <stdint.h>

namespace ns3 
{

// define this class in a public header
class MyTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // these are our accessors to our tag structure
  void SetTag(uint64_t value);
  uint64_t GetTag (void) const;
private:
  uint64_t tagValue;
};
}
#endif