/*
 * Copyright (c) 2013 Magister Solutions
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include "three-gpp-http-server.h"

#include "three-gpp-http-variables.h"

#include "ns3/address-utils.h"
#include "ns3/callback.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-socket.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE("ThreeGppHttpServer");

namespace ns3
{

// HTTP SERVER ////////////////////////////////////////////////////////////////

NS_OBJECT_ENSURE_REGISTERED(ThreeGppHttpServer);

ThreeGppHttpServer::ThreeGppHttpServer()
    : SinkApplication(HTTP_DEFAULT_PORT),
      m_state{NOT_STARTED},
      m_initialSocket{nullptr},
      m_txBuffer{Create<ThreeGppHttpServerTxBuffer>()},
      m_httpVariables{CreateObject<ThreeGppHttpVariables>()},
      m_optPort{},
      m_mtuSize{m_httpVariables->GetMtuSize()}
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO(this << " MTU size for this server application is " << m_mtuSize << " bytes.");
}

// static
TypeId
ThreeGppHttpServer::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ThreeGppHttpServer")
            .SetParent<SinkApplication>()
            .AddConstructor<ThreeGppHttpServer>()
            .AddAttribute("Variables",
                          "Variable collection, which is used to control e.g. processing and "
                          "object generation delays.",
                          PointerValue(),
                          MakePointerAccessor(&ThreeGppHttpServer::m_httpVariables),
                          MakePointerChecker<ThreeGppHttpVariables>())
            .AddAttribute("LocalAddress",
                          "The local address of the server, "
                          "i.e., the address on which to bind the Rx socket.",
                          AddressValue(),
                          MakeAddressAccessor(&ThreeGppHttpServer::SetLocal),
                          MakeAddressChecker(),
                          TypeId::SupportLevel::DEPRECATED,
                          "Replaced by Local in ns-3.44.")
            .AddAttribute("LocalPort",
                          "Port on which the application listen for incoming packets.",
                          UintegerValue(80), // the default HTTP port
                          MakeUintegerAccessor(&ThreeGppHttpServer::SetPort),
                          MakeUintegerChecker<uint16_t>(),
                          TypeId::SupportLevel::DEPRECATED,
                          "Replaced by Port in ns-3.44.")
            .AddAttribute("Tos",
                          "The Type of Service used to send packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&ThreeGppHttpServer::m_tos),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("Mtu",
                          "Maximum transmission unit (in bytes) of the TCP sockets "
                          "used in this application, excluding the compulsory 40 "
                          "bytes TCP header. Typical values are 1460 and 536 bytes. "
                          "The attribute is read-only because the value is randomly "
                          "determined.",
                          TypeId::ATTR_GET,
                          UintegerValue(),
                          MakeUintegerAccessor(&ThreeGppHttpServer::m_mtuSize),
                          MakeUintegerChecker<uint32_t>())
            .AddTraceSource(
                "ConnectionEstablished",
                "Connection to a remote web client has been established.",
                MakeTraceSourceAccessor(&ThreeGppHttpServer::m_connectionEstablishedTrace),
                "ns3::HttpServer::ConnectionEstablishedCallback")
            .AddTraceSource("MainObject",
                            "A main object has been generated.",
                            MakeTraceSourceAccessor(&ThreeGppHttpServer::m_mainObjectTrace),
                            "ns3::HttpServer::HttpObjectCallback")
            .AddTraceSource("EmbeddedObject",
                            "An embedded object has been generated.",
                            MakeTraceSourceAccessor(&ThreeGppHttpServer::m_embeddedObjectTrace),
                            "ns3::HttpServer::HttpObjectCallback")
            .AddTraceSource("Tx",
                            "A packet has been sent.",
                            MakeTraceSourceAccessor(&ThreeGppHttpServer::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("Rx",
                            "A packet has been received.",
                            MakeTraceSourceAccessor(&ThreeGppHttpServer::m_rxTrace),
                            "ns3::Packet::AddressTracedCallback")
            .AddTraceSource("RxWithAddresses",
                            "A packet has been received.",
                            MakeTraceSourceAccessor(&ThreeGppHttpServer::m_rxTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback")
            .AddTraceSource("RxDelay",
                            "A packet has been received with delay information.",
                            MakeTraceSourceAccessor(&ThreeGppHttpServer::m_rxDelayTrace),
                            "ns3::Application::DelayAddressCallback")
            .AddTraceSource("StateTransition",
                            "Trace fired upon every HTTP client state transition.",
                            MakeTraceSourceAccessor(&ThreeGppHttpServer::m_stateTransitionTrace),
                            "ns3::Application::StateTransitionCallback");
    return tid;
}

void
ThreeGppHttpServer::SetLocal(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    if (!addr.IsInvalid())
    {
        m_local = addr;
        if (m_optPort)
        {
            SetPort(*m_optPort);
        }
    }
}

void
ThreeGppHttpServer::SetPort(uint32_t port)
{
    NS_LOG_FUNCTION(this << port);
    if (port != INVALID_PORT)
    {
        m_port = port;
    }
    if (m_local.IsInvalid())
    {
        // save for later
        m_optPort = m_port;
        return;
    }
    if (Ipv4Address::IsMatchingType(m_local) || Ipv6Address::IsMatchingType(m_local))
    {
        m_local = addressUtils::ConvertToSocketAddress(m_local, m_port);
    }
}

void
ThreeGppHttpServer::SetMtuSize(uint32_t mtuSize)
{
    NS_LOG_FUNCTION(this << mtuSize);
    m_mtuSize = mtuSize;
}

Ptr<Socket>
ThreeGppHttpServer::GetSocket() const
{
    return m_initialSocket;
}

ThreeGppHttpServer::State_t
ThreeGppHttpServer::GetState() const
{
    return m_state;
}

std::string
ThreeGppHttpServer::GetStateString() const
{
    return GetStateString(m_state);
}

// static
std::string
ThreeGppHttpServer::GetStateString(ThreeGppHttpServer::State_t state)
{
    switch (state)
    {
    case NOT_STARTED:
        return "NOT_STARTED";
    case STARTED:
        return "STARTED";
    case STOPPED:
        return "STOPPED";
    default:
        NS_FATAL_ERROR("Unknown state");
        return "FATAL_ERROR";
    }
}

void
ThreeGppHttpServer::DoDispose()
{
    NS_LOG_FUNCTION(this);

    if (!Simulator::IsFinished())
    {
        StopApplication();
    }

    Application::DoDispose(); // Chain up.
}

void
ThreeGppHttpServer::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_state != NOT_STARTED)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for StartApplication().");
    }

    m_httpVariables->Initialize();
    if (!m_initialSocket)
    {
        // Find the current default MTU value of TCP sockets.
        Ptr<const ns3::AttributeValue> previousSocketMtu;
        const TypeId tcpSocketTid = TcpSocket::GetTypeId();
        for (uint32_t i = 0; i < tcpSocketTid.GetAttributeN(); i++)
        {
            TypeId::AttributeInformation attrInfo = tcpSocketTid.GetAttribute(i);
            if (attrInfo.name == "SegmentSize")
            {
                previousSocketMtu = attrInfo.initialValue;
            }
        }

        // Creating a TCP socket to connect to the server.
        m_initialSocket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        m_initialSocket->SetAttribute("SegmentSize", UintegerValue(m_mtuSize));

        NS_ABORT_MSG_IF(m_local.IsInvalid(), "Local address not properly set");
        if (InetSocketAddress::IsMatchingType(m_local))
        {
            const auto ipv4 [[maybe_unused]] = InetSocketAddress::ConvertFrom(m_local).GetIpv4();
            m_initialSocket->SetIpTos(m_tos); // Affects only IPv4 sockets.
            NS_LOG_INFO(this << " Binding on " << ipv4 << " port " << m_port << " / " << m_local
                             << ".");
        }
        else if (Inet6SocketAddress::IsMatchingType(m_local))
        {
            const auto ipv6 [[maybe_unused]] = Inet6SocketAddress::ConvertFrom(m_local).GetIpv6();
            NS_LOG_INFO(this << " Binding on " << ipv6 << " port " << m_port << " / " << m_local
                             << ".");
        }
        else
        {
            NS_ABORT_MSG("Incompatible local address");
        }

        auto ret [[maybe_unused]] = m_initialSocket->Bind(m_local);
        NS_LOG_DEBUG(this << " Bind() return value= " << ret
                          << " GetErrNo= " << m_initialSocket->GetErrno() << ".");

        ret = m_initialSocket->Listen();
        NS_LOG_DEBUG(this << " Listen () return value= " << ret
                          << " GetErrNo= " << m_initialSocket->GetErrno() << ".");

        NS_ASSERT_MSG(m_initialSocket, "Failed creating socket.");
        m_initialSocket->SetAcceptCallback(
            MakeCallback(&ThreeGppHttpServer::ConnectionRequestCallback, this),
            MakeCallback(&ThreeGppHttpServer::NewConnectionCreatedCallback, this));
        m_initialSocket->SetCloseCallbacks(
            MakeCallback(&ThreeGppHttpServer::NormalCloseCallback, this),
            MakeCallback(&ThreeGppHttpServer::ErrorCloseCallback, this));
        m_initialSocket->SetRecvCallback(
            MakeCallback(&ThreeGppHttpServer::ReceivedDataCallback, this));
        m_initialSocket->SetSendCallback(MakeCallback(&ThreeGppHttpServer::SendCallback, this));
    }

    SwitchToState(STARTED);
}

void
ThreeGppHttpServer::StopApplication()
{
    NS_LOG_FUNCTION(this);

    SwitchToState(STOPPED);

    // Close all accepted sockets.
    m_txBuffer->CloseAllSockets();

    // Stop listening.
    if (m_initialSocket)
    {
        m_initialSocket->Close();
        m_initialSocket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                           MakeNullCallback<void, Ptr<Socket>, const Address&>());
        m_initialSocket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                           MakeNullCallback<void, Ptr<Socket>>());
        m_initialSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_initialSocket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }
}

bool
ThreeGppHttpServer::ConnectionRequestCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);
    return true; // Unconditionally accept the connection request.
}

void
ThreeGppHttpServer::NewConnectionCreatedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);

    socket->SetCloseCallbacks(MakeCallback(&ThreeGppHttpServer::NormalCloseCallback, this),
                              MakeCallback(&ThreeGppHttpServer::ErrorCloseCallback, this));
    socket->SetRecvCallback(MakeCallback(&ThreeGppHttpServer::ReceivedDataCallback, this));
    socket->SetSendCallback(MakeCallback(&ThreeGppHttpServer::SendCallback, this));

    m_connectionEstablishedTrace(this, socket);
    m_txBuffer->AddSocket(socket);

    /*
     * A typical connection is established after receiving an empty (i.e., no
     * data) TCP packet with ACK flag. The actual data will follow in a separate
     * packet after that and will be received by ReceivedDataCallback().
     *
     * However, that empty ACK packet might get lost. In this case, we may
     * receive the first data packet right here already, because it also counts
     * as a new connection. The statement below attempts to fetch the data from
     * that packet, if any.
     */
    ReceivedDataCallback(socket);
}

void
ThreeGppHttpServer::NormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (socket == m_initialSocket)
    {
        if (m_state == STARTED)
        {
            NS_FATAL_ERROR("Initial listener socket shall not be closed"
                           << " when the server instance is still running.");
        }
    }
    else if (m_txBuffer->IsSocketAvailable(socket))
    {
        // The application should now prepare to close the socket.
        if (m_txBuffer->IsBufferEmpty(socket))
        {
            /*
             * Here we declare that we have nothing more to send and the socket
             * may be closed immediately.
             */
            socket->ShutdownSend();
            m_txBuffer->RemoveSocket(socket);
        }
        else
        {
            /*
             * Remember to close the socket later, whenever the buffer becomes
             * empty.
             */
            m_txBuffer->PrepareClose(socket);
        }
    }
}

void
ThreeGppHttpServer::ErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (socket == m_initialSocket)
    {
        if (m_state == STARTED)
        {
            NS_FATAL_ERROR("Initial listener socket shall not be closed"
                           << " when the server instance is still running.");
        }
    }
    else if (m_txBuffer->IsSocketAvailable(socket))
    {
        m_txBuffer->CloseSocket(socket);
    }
}

void
ThreeGppHttpServer::ReceivedDataCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Address from;
    while (auto packet = socket->RecvFrom(from))
    {
        if (packet->GetSize() == 0)
        {
            break; // EOF
        }

#ifdef NS3_LOG_ENABLE
        // Some log messages.
        if (InetSocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO(this << " A packet of " << packet->GetSize() << " bytes"
                             << " received from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                             << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " / "
                             << InetSocketAddress::ConvertFrom(from));
        }
        else if (Inet6SocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO(this << " A packet of " << packet->GetSize() << " bytes"
                             << " received from " << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                             << " port " << Inet6SocketAddress::ConvertFrom(from).GetPort() << " / "
                             << Inet6SocketAddress::ConvertFrom(from));
        }
#endif /* NS3_LOG_ENABLE */

        // Check the header. No need to remove it, since it is not a "real" header.
        ThreeGppHttpHeader httpHeader;
        packet->PeekHeader(httpHeader);

        // Fire trace sources.
        m_rxTrace(packet, from);
        m_rxTraceWithAddresses(packet, from, m_local);
        m_rxDelayTrace(Simulator::Now() - httpHeader.GetClientTs(), from);

        switch (httpHeader.GetContentType())
        {
        case ThreeGppHttpHeader::MAIN_OBJECT: {
            const auto processingDelay = m_httpVariables->GetMainObjectGenerationDelay();
            NS_LOG_INFO(this << " Will finish generating a main object in "
                             << processingDelay.As(Time::S) << ".");
            m_txBuffer->RecordNextServe(socket,
                                        Simulator::Schedule(processingDelay,
                                                            &ThreeGppHttpServer::ServeNewMainObject,
                                                            this,
                                                            socket),
                                        httpHeader.GetClientTs());
            break;
        }
        case ThreeGppHttpHeader::EMBEDDED_OBJECT: {
            const auto processingDelay = m_httpVariables->GetEmbeddedObjectGenerationDelay();
            NS_LOG_INFO(this << " Will finish generating an embedded object in "
                             << processingDelay.As(Time::S) << ".");
            m_txBuffer->RecordNextServe(
                socket,
                Simulator::Schedule(processingDelay,
                                    &ThreeGppHttpServer::ServeNewEmbeddedObject,
                                    this,
                                    socket),
                httpHeader.GetClientTs());
            break;
        }
        default:
            NS_FATAL_ERROR("Invalid packet.");
            break;
        }
    }
}

void
ThreeGppHttpServer::SendCallback(Ptr<Socket> socket, uint32_t availableBufferSize)
{
    NS_LOG_FUNCTION(this << socket << availableBufferSize);

    if (m_txBuffer->IsBufferEmpty(socket))
    {
        return;
    }

    const auto txBufferSize [[maybe_unused]] = m_txBuffer->GetBufferSize(socket);
    const auto actualSent [[maybe_unused]] = ServeFromTxBuffer(socket);

#ifdef NS3_LOG_ENABLE
    // Some log messages.
    if (actualSent < txBufferSize)
    {
        switch (m_txBuffer->GetBufferContentType(socket))
        {
        case ThreeGppHttpHeader::MAIN_OBJECT:
            NS_LOG_INFO(this << " Transmission of main object is suspended after " << actualSent
                             << " bytes.");
            break;
        case ThreeGppHttpHeader::EMBEDDED_OBJECT:
            NS_LOG_INFO(this << " Transmission of embedded object is suspended after " << actualSent
                             << " bytes.");
            break;
        default:
            NS_FATAL_ERROR("Invalid Tx buffer content type.");
            break;
        }
    }
    else
    {
        switch (m_txBuffer->GetBufferContentType(socket))
        {
        case ThreeGppHttpHeader::MAIN_OBJECT:
            NS_LOG_INFO(this << " Finished sending a whole main object.");
            break;
        case ThreeGppHttpHeader::EMBEDDED_OBJECT:
            NS_LOG_INFO(this << " Finished sending a whole embedded object.");
            break;
        default:
            NS_FATAL_ERROR("Invalid Tx buffer content type.");
            break;
        }
    }
#endif /* NS3_LOG_ENABLE */
}

void
ThreeGppHttpServer::ServeNewMainObject(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    const auto objectSize = m_httpVariables->GetMainObjectSize();
    NS_LOG_INFO(this << " Main object to be served is " << objectSize << " bytes.");
    m_mainObjectTrace(objectSize);
    m_txBuffer->WriteNewObject(socket, ThreeGppHttpHeader::MAIN_OBJECT, objectSize);
    const auto actualSent = ServeFromTxBuffer(socket);

    if (actualSent < objectSize)
    {
        NS_LOG_INFO(this << " Transmission of main object is suspended after " << actualSent
                         << " bytes.");
    }
    else
    {
        NS_LOG_INFO(this << " Finished sending a whole main object.");
    }
}

void
ThreeGppHttpServer::ServeNewEmbeddedObject(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    const auto objectSize = m_httpVariables->GetEmbeddedObjectSize();
    NS_LOG_INFO(this << " Embedded object to be served is " << objectSize << " bytes.");
    m_embeddedObjectTrace(objectSize);
    m_txBuffer->WriteNewObject(socket, ThreeGppHttpHeader::EMBEDDED_OBJECT, objectSize);
    const auto actualSent = ServeFromTxBuffer(socket);

    if (actualSent < objectSize)
    {
        NS_LOG_INFO(this << " Transmission of embedded object is suspended after " << actualSent
                         << " bytes.");
    }
    else
    {
        NS_LOG_INFO(this << " Finished sending a whole embedded object.");
    }
}

uint32_t
ThreeGppHttpServer::ServeFromTxBuffer(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_txBuffer->IsBufferEmpty(socket))
    {
        NS_LOG_LOGIC(this << " Tx buffer is empty. Not sending anything.");
        return 0;
    }

    const auto socketSize = socket->GetTxAvailable();
    NS_LOG_DEBUG(this << " Socket has " << socketSize << " bytes available for Tx.");

    // Get the number of bytes remaining to be sent.
    const auto txBufferSize = m_txBuffer->GetBufferSize(socket);

    // Compute the size of actual content to be sent; has to fit into the socket.
    // Note that header size is NOT counted as TxBuffer content. Header size is overhead.
    const auto contentSize = std::min(txBufferSize, socketSize - 22);
    auto packet = Create<Packet>(contentSize);
    auto packetSize = contentSize;
    if (packetSize == 0)
    {
        NS_LOG_LOGIC(this << " Socket size leads to packet size of zero; not sending anything.");
        return 0;
    }

    // If this is the first packet of an object, attach a header.
    if (!m_txBuffer->HasTxedPartOfObject(socket))
    {
        // Create header.
        ThreeGppHttpHeader httpHeader;
        httpHeader.SetContentLength(txBufferSize);
        httpHeader.SetContentType(m_txBuffer->GetBufferContentType(socket));
        // Using the client TS value as per the corresponding request packet.
        httpHeader.SetClientTs(m_txBuffer->GetClientTs(socket));
        httpHeader.SetServerTs(Simulator::Now());
        packet->AddHeader(httpHeader);
        packetSize += httpHeader.GetSerializedSize();

        NS_LOG_INFO(this << " Created packet " << packet << " of " << packetSize << " bytes."
                         << " The corresponding request came "
                         << (Simulator::Now() - httpHeader.GetClientTs()).As(Time::S) << " ago.");
    }
    else
    {
        NS_LOG_INFO(this << " Created packet " << packet << " of " << packetSize
                         << " bytes to be appended to a previous packet.");
    }

    // Send.
    const auto actualBytes = socket->Send(packet);
    NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packetSize << " bytes,"
                      << " return value= " << actualBytes << ".");
    m_txTrace(packet);

    if (actualBytes == static_cast<int>(packetSize))
    {
        // The packet goes through successfully.
        m_txBuffer->DepleteBufferSize(socket, contentSize);
        NS_LOG_INFO(this << " Remaining object to be sent " << m_txBuffer->GetBufferSize(socket)
                         << " bytes.");
        return packetSize;
    }
    else
    {
        NS_LOG_INFO(this << " Failed to send object, GetErrNo= " << socket->GetErrno()
                         << ", suspending transmission and waiting for another Tx opportunity.");
        return 0;
    }
}

void
ThreeGppHttpServer::SwitchToState(ThreeGppHttpServer::State_t state)
{
    const auto oldState = GetStateString();
    const auto newState = GetStateString(state);
    NS_LOG_FUNCTION(this << oldState << newState);
    m_state = state;
    NS_LOG_INFO(this << " ThreeGppHttpServer " << oldState << " --> " << newState << ".");
    m_stateTransitionTrace(oldState, newState);
}

// HTTP SERVER TX BUFFER //////////////////////////////////////////////////////

ThreeGppHttpServerTxBuffer::ThreeGppHttpServerTxBuffer()
{
    NS_LOG_FUNCTION(this);
}

bool
ThreeGppHttpServerTxBuffer::IsSocketAvailable(Ptr<Socket> socket) const
{
    auto it = m_txBuffer.find(socket);
    return (it != m_txBuffer.end());
}

void
ThreeGppHttpServerTxBuffer::AddSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    NS_ASSERT_MSG(!IsSocketAvailable(socket),
                  this << " Cannot add socket " << socket
                       << " because it has already been added before.");

    TxBuffer_t txBuffer;
    txBuffer.txBufferContentType = ThreeGppHttpHeader::NOT_SET;
    txBuffer.txBufferSize = 0;
    txBuffer.isClosing = false;
    txBuffer.hasTxedPartOfObject = false;
    m_txBuffer.insert(std::pair<Ptr<Socket>, TxBuffer_t>(socket, txBuffer));
}

void
ThreeGppHttpServerTxBuffer::RemoveSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");

    if (!Simulator::IsExpired(it->second.nextServe))
    {
        NS_LOG_INFO(this << " Canceling a serving event which is due in "
                         << Simulator::GetDelayLeft(it->second.nextServe).As(Time::S) << ".");
        Simulator::Cancel(it->second.nextServe);
    }

    it->first->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                 MakeNullCallback<void, Ptr<Socket>>());
    it->first->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    it->first->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());

    m_txBuffer.erase(it);
}

void
ThreeGppHttpServerTxBuffer::CloseSocket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");

    if (!Simulator::IsExpired(it->second.nextServe))
    {
        NS_LOG_INFO(this << " Canceling a serving event which is due in "
                         << Simulator::GetDelayLeft(it->second.nextServe).As(Time::S) << ".");
        Simulator::Cancel(it->second.nextServe);
    }

    if (it->second.txBufferSize > 0)
    {
        NS_LOG_WARN(this << " Closing a socket where " << it->second.txBufferSize
                         << " bytes of transmission"
                         << " is still pending in the corresponding Tx buffer.");
    }

    it->first->Close();
    it->first->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                 MakeNullCallback<void, Ptr<Socket>>());
    it->first->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    it->first->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());

    m_txBuffer.erase(it);
}

void
ThreeGppHttpServerTxBuffer::CloseAllSockets()
{
    NS_LOG_FUNCTION(this);

    for (auto& [socket, buffer] : m_txBuffer)
    {
        if (!Simulator::IsExpired(buffer.nextServe))
        {
            NS_LOG_INFO(this << " Canceling a serving event which is due in "
                             << Simulator::GetDelayLeft(buffer.nextServe).As(Time::S) << ".");
            Simulator::Cancel(buffer.nextServe);
        }

        socket->Close();
        socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                  MakeNullCallback<void, Ptr<Socket>>());
        socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        socket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }

    m_txBuffer.clear();
}

bool
ThreeGppHttpServerTxBuffer::IsBufferEmpty(Ptr<Socket> socket) const
{
    const auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.cend(), "Socket " << socket << " cannot be found.");
    return (it->second.txBufferSize == 0);
}

Time
ThreeGppHttpServerTxBuffer::GetClientTs(Ptr<Socket> socket) const
{
    const auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.cend(), "Socket " << socket << " cannot be found.");
    return it->second.clientTs;
}

ThreeGppHttpHeader::ContentType_t
ThreeGppHttpServerTxBuffer::GetBufferContentType(Ptr<Socket> socket) const
{
    const auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.cend(), "Socket " << socket << " cannot be found.");
    return it->second.txBufferContentType;
}

uint32_t
ThreeGppHttpServerTxBuffer::GetBufferSize(Ptr<Socket> socket) const
{
    const auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.cend(), "Socket " << socket << " cannot be found.");
    return it->second.txBufferSize;
}

bool
ThreeGppHttpServerTxBuffer::HasTxedPartOfObject(Ptr<Socket> socket) const
{
    const auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.cend(), "Socket " << socket << " cannot be found");
    return it->second.hasTxedPartOfObject;
}

void
ThreeGppHttpServerTxBuffer::WriteNewObject(Ptr<Socket> socket,
                                           ThreeGppHttpHeader::ContentType_t contentType,
                                           uint32_t objectSize)
{
    NS_LOG_FUNCTION(this << socket << contentType << objectSize);

    NS_ASSERT_MSG(contentType != ThreeGppHttpHeader::NOT_SET,
                  "Unable to write an object without a proper Content-Type.");
    NS_ASSERT_MSG(objectSize > 0, "Unable to write a zero-sized object.");

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    NS_ASSERT_MSG(it->second.txBufferSize == 0,
                  "Cannot write to Tx buffer of socket "
                      << socket << " until the previous content has been completely sent.");
    it->second.txBufferContentType = contentType;
    it->second.txBufferSize = objectSize;
    it->second.hasTxedPartOfObject = false;
}

void
ThreeGppHttpServerTxBuffer::RecordNextServe(Ptr<Socket> socket,
                                            const EventId& eventId,
                                            const Time& clientTs)
{
    NS_LOG_FUNCTION(this << socket << clientTs.As(Time::S));

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    it->second.nextServe = eventId;
    it->second.clientTs = clientTs;
}

void
ThreeGppHttpServerTxBuffer::DepleteBufferSize(Ptr<Socket> socket, uint32_t amount)
{
    NS_LOG_FUNCTION(this << socket << amount);

    NS_ASSERT_MSG(amount > 0, "Unable to consume zero bytes.");

    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    NS_ASSERT_MSG(it->second.txBufferSize >= amount,
                  "The requested amount is larger than the current buffer size.");
    it->second.txBufferSize -= amount;
    it->second.hasTxedPartOfObject = true;

    if (it->second.isClosing && (it->second.txBufferSize == 0))
    {
        /*
         * The peer has earlier issued a close request and we have now waited
         * until all the existing data are pushed into the socket. Now we close
         * the socket explicitly.
         */
        CloseSocket(socket);
    }
}

void
ThreeGppHttpServerTxBuffer::PrepareClose(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    auto it = m_txBuffer.find(socket);
    NS_ASSERT_MSG(it != m_txBuffer.end(), "Socket " << socket << " cannot be found.");
    it->second.isClosing = true;
}

} // namespace ns3
