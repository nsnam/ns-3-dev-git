/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

#include <iostream>

using namespace ns3;

/**
 * @ingroup network
 * A simple example of an Tag implementation
 */
class MyTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    void Print(std::ostream& os) const override;

    // these are our accessors to our tag structure
    /**
     * Set the tag value
     * @param value The tag value.
     */
    void SetSimpleValue(uint8_t value);
    /**
     * Get the tag value
     * @return the tag value.
     */
    uint8_t GetSimpleValue() const;

  private:
    uint8_t m_simpleValue; //!< tag value
};

TypeId
MyTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MyTag")
                            .SetParent<Tag>()
                            .AddConstructor<MyTag>()
                            .AddAttribute("SimpleValue",
                                          "A simple value",
                                          EmptyAttributeValue(),
                                          MakeUintegerAccessor(&MyTag::GetSimpleValue),
                                          MakeUintegerChecker<uint8_t>());
    return tid;
}

TypeId
MyTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MyTag::GetSerializedSize() const
{
    return 1;
}

void
MyTag::Serialize(TagBuffer i) const
{
    i.WriteU8(m_simpleValue);
}

void
MyTag::Deserialize(TagBuffer i)
{
    m_simpleValue = i.ReadU8();
}

void
MyTag::Print(std::ostream& os) const
{
    os << "v=" << (uint32_t)m_simpleValue;
}

void
MyTag::SetSimpleValue(uint8_t value)
{
    m_simpleValue = value;
}

uint8_t
MyTag::GetSimpleValue() const
{
    return m_simpleValue;
}

int
main(int argc, char* argv[])
{
    // create a tag.
    MyTag tag;
    tag.SetSimpleValue(0x56);

    // store the tag in a packet.
    Ptr<Packet> p = Create<Packet>(100);
    p->AddPacketTag(tag);

    // create a copy of the packet
    Ptr<Packet> aCopy = p->Copy();

    // read the tag from the packet copy
    MyTag tagCopy;
    p->PeekPacketTag(tagCopy);

    // the copy and the original are the same !
    NS_ASSERT(tagCopy.GetSimpleValue() == tag.GetSimpleValue());

    aCopy->PrintPacketTags(std::cout);
    std::cout << std::endl;

    return 0;
}
