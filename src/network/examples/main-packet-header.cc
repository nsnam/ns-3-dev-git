#include "ns3/header.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

#include <iostream>

using namespace ns3;

/**
 * \ingroup network
 * A simple example of an Header implementation
 */
class MyHeader : public Header
{
  public:
    MyHeader();
    ~MyHeader() override;

    /**
     * Set the header data.
     * \param data The data.
     */
    void SetData(uint16_t data);
    /**
     * Get the header data.
     * \return The data.
     */
    uint16_t GetData() const;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    uint32_t GetSerializedSize() const override;

  private:
    uint16_t m_data; //!< Header data
};

MyHeader::MyHeader()
{
    // we must provide a public default constructor,
    // implicit or explicit, but never private.
}

MyHeader::~MyHeader()
{
}

TypeId
MyHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MyHeader").SetParent<Header>().AddConstructor<MyHeader>();
    return tid;
}

TypeId
MyHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MyHeader::Print(std::ostream& os) const
{
    // This method is invoked by the packet printing
    // routines to print the content of my header.
    // os << "data=" << m_data << std::endl;
    os << "data=" << m_data;
}

uint32_t
MyHeader::GetSerializedSize() const
{
    // we reserve 2 bytes for our header.
    return 2;
}

void
MyHeader::Serialize(Buffer::Iterator start) const
{
    // we can serialize two bytes at the start of the buffer.
    // we write them in network byte order.
    start.WriteHtonU16(m_data);
}

uint32_t
MyHeader::Deserialize(Buffer::Iterator start)
{
    // we can deserialize two bytes from the start of the buffer.
    // we read them in network byte order and store them
    // in host byte order.
    m_data = start.ReadNtohU16();

    // we return the number of bytes effectively read.
    return 2;
}

void
MyHeader::SetData(uint16_t data)
{
    m_data = data;
}

uint16_t
MyHeader::GetData() const
{
    return m_data;
}

int
main(int argc, char* argv[])
{
    // Enable the packet printing through Packet::Print command.
    Packet::EnablePrinting();

    // instantiate a header.
    MyHeader sourceHeader;
    sourceHeader.SetData(2);

    // instantiate a packet
    Ptr<Packet> p = Create<Packet>();

    // and store my header into the packet.
    p->AddHeader(sourceHeader);

    // print the content of my packet on the standard output.
    p->Print(std::cout);
    std::cout << std::endl;

    // you can now remove the header from the packet:
    MyHeader destinationHeader;
    p->RemoveHeader(destinationHeader);

    // and check that the destination and source
    // headers contain the same values.
    NS_ASSERT(sourceHeader.GetData() == destinationHeader.GetData());

    return 0;
}
