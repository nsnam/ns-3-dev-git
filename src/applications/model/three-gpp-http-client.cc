/*
 * Copyright (c) 2013 Magister Solutions
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#include "three-gpp-http-client.h"

#include "three-gpp-http-variables.h"

#include "ns3/address-utils.h"
#include "ns3/callback.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE("ThreeGppHttpClient");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(ThreeGppHttpClient);

ThreeGppHttpClient::ThreeGppHttpClient()
    : m_state{NOT_STARTED},
      m_socket{nullptr},
      m_objectBytesToBeReceived{0},
      m_objectClientTs{},
      m_objectServerTs{},
      m_embeddedObjectsToBeRequested{0},
      m_pageLoadStartTs{},
      m_numberEmbeddedObjectsRequested{0},
      m_numberBytesPage{0},
      m_httpVariables{CreateObject<ThreeGppHttpVariables>()},
      m_peerPort{}
{
    NS_LOG_FUNCTION(this);
}

// static
TypeId
ThreeGppHttpClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ThreeGppHttpClient")
            .SetParent<SourceApplication>()
            .AddConstructor<ThreeGppHttpClient>()
            .AddAttribute(
                "Variables",
                "Variable collection, which is used to control e.g. timing and HTTP request size.",
                PointerValue(),
                MakePointerAccessor(&ThreeGppHttpClient::m_httpVariables),
                MakePointerChecker<ThreeGppHttpVariables>())
            .AddAttribute("RemoteServerAddress",
                          "The address of the destination server.",
                          AddressValue(),
                          MakeAddressAccessor(&ThreeGppHttpClient::SetRemote),
                          MakeAddressChecker(),
                          TypeId::SupportLevel::DEPRECATED,
                          "Replaced by Remote in ns-3.44.")
            .AddAttribute("RemoteServerPort",
                          "The destination port of the outbound packets.",
                          UintegerValue(80), // the default HTTP port
                          MakeUintegerAccessor(&ThreeGppHttpClient::SetPort),
                          MakeUintegerChecker<uint16_t>(),
                          TypeId::SupportLevel::DEPRECATED,
                          "Replaced by Remote in ns-3.44.")
            .AddTraceSource("RxPage",
                            "A page has been received.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_rxPageTrace),
                            "ns3::ThreeGppHttpClient::RxPageTracedCallback")
            .AddTraceSource(
                "ConnectionEstablished",
                "Connection to the destination web server has been established.",
                MakeTraceSourceAccessor(&ThreeGppHttpClient::m_connectionEstablishedTrace),
                "ns3::ThreeGppHttpClient::TracedCallback")
            .AddTraceSource("ConnectionClosed",
                            "Connection to the destination web server is closed.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_connectionClosedTrace),
                            "ns3::ThreeGppHttpClient::TracedCallback")
            .AddTraceSource("Tx",
                            "General trace for sending a packet of any kind.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource(
                "TxMainObjectRequest",
                "Sent a request for a main object.",
                MakeTraceSourceAccessor(&ThreeGppHttpClient::m_txMainObjectRequestTrace),
                "ns3::Packet::TracedCallback")
            .AddTraceSource(
                "TxEmbeddedObjectRequest",
                "Sent a request for an embedded object.",
                MakeTraceSourceAccessor(&ThreeGppHttpClient::m_txEmbeddedObjectRequestTrace),
                "ns3::Packet::TracedCallback")
            .AddTraceSource("RxMainObjectPacket",
                            "A packet of main object has been received.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_rxMainObjectPacketTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("RxMainObject",
                            "Received a whole main object. Header is included.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_rxMainObjectTrace),
                            "ns3::ThreeGppHttpClient::TracedCallback")
            .AddTraceSource(
                "RxEmbeddedObjectPacket",
                "A packet of embedded object has been received.",
                MakeTraceSourceAccessor(&ThreeGppHttpClient::m_rxEmbeddedObjectPacketTrace),
                "ns3::Packet::TracedCallback")
            .AddTraceSource("RxEmbeddedObject",
                            "Received a whole embedded object. Header is included.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_rxEmbeddedObjectTrace),
                            "ns3::ThreeGppHttpClient::TracedCallback")
            .AddTraceSource("Rx",
                            "General trace for receiving a packet of any kind.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_rxTrace),
                            "ns3::Packet::PacketAddressTracedCallback")
            .AddTraceSource("RxDelay",
                            "General trace of delay for receiving a complete object.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_rxDelayTrace),
                            "ns3::Application::DelayAddressCallback")
            .AddTraceSource(
                "RxRtt",
                "General trace of round trip delay time for receiving a complete object.",
                MakeTraceSourceAccessor(&ThreeGppHttpClient::m_rxRttTrace),
                "ns3::Application::DelayAddressCallback")
            .AddTraceSource("StateTransition",
                            "Trace fired upon every HTTP client state transition.",
                            MakeTraceSourceAccessor(&ThreeGppHttpClient::m_stateTransitionTrace),
                            "ns3::Application::StateTransitionCallback");
    return tid;
}

void
ThreeGppHttpClient::SetRemote(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    if (!addr.IsInvalid())
    {
        m_peer = addr;
        if (m_peerPort)
        {
            SetPort(*m_peerPort);
        }
    }
}

void
ThreeGppHttpClient::SetPort(uint16_t port)
{
    NS_LOG_FUNCTION(this << port);
    if (m_peer.IsInvalid())
    {
        // save for later
        m_peerPort = port;
        return;
    }
    if (Ipv4Address::IsMatchingType(m_peer) || Ipv6Address::IsMatchingType(m_peer))
    {
        m_peer = addressUtils::ConvertToSocketAddress(m_peer, port);
    }
}

Ptr<Socket>
ThreeGppHttpClient::GetSocket() const
{
    return m_socket;
}

ThreeGppHttpClient::State_t
ThreeGppHttpClient::GetState() const
{
    return m_state;
}

std::string
ThreeGppHttpClient::GetStateString() const
{
    return GetStateString(m_state);
}

// static
std::string
ThreeGppHttpClient::GetStateString(ThreeGppHttpClient::State_t state)
{
    switch (state)
    {
    case NOT_STARTED:
        return "NOT_STARTED";
    case CONNECTING:
        return "CONNECTING";
    case EXPECTING_MAIN_OBJECT:
        return "EXPECTING_MAIN_OBJECT";
    case PARSING_MAIN_OBJECT:
        return "PARSING_MAIN_OBJECT";
    case EXPECTING_EMBEDDED_OBJECT:
        return "EXPECTING_EMBEDDED_OBJECT";
    case READING:
        return "READING";
    case STOPPED:
        return "STOPPED";
    default:
        NS_FATAL_ERROR("Unknown state");
        return "FATAL_ERROR";
    }
}

void
ThreeGppHttpClient::DoDispose()
{
    NS_LOG_FUNCTION(this);

    if (!Simulator::IsFinished())
    {
        StopApplication();
    }

    Application::DoDispose(); // Chain up.
}

void
ThreeGppHttpClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_state == NOT_STARTED)
    {
        m_httpVariables->Initialize();
        OpenConnection();
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for StartApplication().");
    }
}

void
ThreeGppHttpClient::StopApplication()
{
    NS_LOG_FUNCTION(this);

    SwitchToState(STOPPED);
    CancelAllPendingEvents();
    m_socket->Close();
    m_socket->SetConnectCallback(MakeNullCallback<void, Ptr<Socket>>(),
                                 MakeNullCallback<void, Ptr<Socket>>());
    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
}

void
ThreeGppHttpClient::ConnectionSucceededCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_state != CONNECTING)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ConnectionSucceeded().");
    }

    NS_ASSERT_MSG(m_socket == socket, "Invalid socket.");
    m_connectionEstablishedTrace(this);
    socket->SetRecvCallback(MakeCallback(&ThreeGppHttpClient::ReceivedDataCallback, this));
    NS_ASSERT(m_embeddedObjectsToBeRequested == 0);
    m_eventRequestMainObject = Simulator::ScheduleNow(&ThreeGppHttpClient::RequestMainObject, this);
}

void
ThreeGppHttpClient::ConnectionFailedCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    if (m_state == CONNECTING)
    {
        NS_LOG_ERROR("Client failed to connect to remote address " << m_peer);
    }
    else
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ConnectionFailed().");
    }
}

void
ThreeGppHttpClient::NormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    CancelAllPendingEvents();

    if (socket->GetErrno() != Socket::ERROR_NOTERROR)
    {
        NS_LOG_ERROR(this << " Connection has been terminated,"
                          << " error code: " << socket->GetErrno() << ".");
    }

    m_socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                MakeNullCallback<void, Ptr<Socket>>());

    m_connectionClosedTrace(this);
}

void
ThreeGppHttpClient::ErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    CancelAllPendingEvents();
    if (socket->GetErrno() != Socket::ERROR_NOTERROR)
    {
        NS_LOG_ERROR(this << " Connection has been terminated,"
                          << " error code: " << socket->GetErrno() << ".");
    }

    m_connectionClosedTrace(this);
}

void
ThreeGppHttpClient::ReceivedDataCallback(Ptr<Socket> socket)
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
                             << InetSocketAddress::ConvertFrom(from) << ".");
        }
        else if (Inet6SocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO(this << " A packet of " << packet->GetSize() << " bytes"
                             << " received from " << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                             << " port " << Inet6SocketAddress::ConvertFrom(from).GetPort() << " / "
                             << Inet6SocketAddress::ConvertFrom(from) << ".");
        }
#endif /* NS3_LOG_ENABLE */

        m_rxTrace(packet, from);

        switch (m_state)
        {
        case EXPECTING_MAIN_OBJECT:
            ReceiveMainObject(packet, from);
            break;
        case EXPECTING_EMBEDDED_OBJECT:
            ReceiveEmbeddedObject(packet, from);
            break;
        default:
            NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ReceivedData().");
            break;
        }
    }
}

void
ThreeGppHttpClient::OpenConnection()
{
    NS_LOG_FUNCTION(this);

    if (m_state != NOT_STARTED && m_state != EXPECTING_EMBEDDED_OBJECT &&
        m_state != PARSING_MAIN_OBJECT && m_state != READING)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for OpenConnection().");
    }

    m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
    NS_ABORT_MSG_IF(m_peer.IsInvalid(), "Remote address not properly set");
    if (!m_local.IsInvalid())
    {
        NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                         InetSocketAddress::IsMatchingType(m_local)) ||
                            (InetSocketAddress::IsMatchingType(m_peer) &&
                             Inet6SocketAddress::IsMatchingType(m_local)),
                        "Incompatible peer and local address IP version");
    }
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        const auto ret [[maybe_unused]] =
            m_local.IsInvalid() ? m_socket->Bind() : m_socket->Bind(m_local);
        NS_LOG_DEBUG(this << " Bind() return value= " << ret
                          << " GetErrNo= " << m_socket->GetErrno() << ".");

        const auto ipv4 = InetSocketAddress::ConvertFrom(m_peer).GetIpv4();
        const auto port = InetSocketAddress::ConvertFrom(m_peer).GetPort();
        NS_LOG_INFO(this << " Connecting to " << ipv4 << " port " << port << " / " << m_peer
                         << ".");
        m_socket->SetIpTos(m_tos);
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        const auto ret [[maybe_unused]] =
            m_local.IsInvalid() ? m_socket->Bind6() : m_socket->Bind(m_local);
        NS_LOG_DEBUG(this << " Bind6() return value= " << ret
                          << " GetErrNo= " << m_socket->GetErrno() << ".");

        const auto ipv6 = Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6();
        const auto port = Inet6SocketAddress::ConvertFrom(m_peer).GetPort();
        NS_LOG_INFO(this << " Connecting to " << ipv6 << " port " << port << " / " << m_peer
                         << ".");
    }
    else
    {
        NS_ASSERT_MSG(false, "Incompatible address type: " << m_peer);
    }

    const auto ret [[maybe_unused]] = m_socket->Connect(m_peer);
    NS_LOG_DEBUG(this << " Connect() return value= " << ret << " GetErrNo= " << m_socket->GetErrno()
                      << ".");

    NS_ASSERT_MSG(m_socket, "Failed creating socket.");

    SwitchToState(CONNECTING);

    m_socket->SetConnectCallback(
        MakeCallback(&ThreeGppHttpClient::ConnectionSucceededCallback, this),
        MakeCallback(&ThreeGppHttpClient::ConnectionFailedCallback, this));
    m_socket->SetCloseCallbacks(MakeCallback(&ThreeGppHttpClient::NormalCloseCallback, this),
                                MakeCallback(&ThreeGppHttpClient::ErrorCloseCallback, this));
    m_socket->SetRecvCallback(MakeCallback(&ThreeGppHttpClient::ReceivedDataCallback, this));
    m_socket->SetAttribute("MaxSegLifetime", DoubleValue(0.02)); // 20 ms.
}

void
ThreeGppHttpClient::RequestMainObject()
{
    NS_LOG_FUNCTION(this);

    if (m_state != CONNECTING && m_state != READING)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for RequestMainObject().");
    }

    ThreeGppHttpHeader header;
    header.SetContentLength(0); // Request does not need any content length.
    header.SetContentType(ThreeGppHttpHeader::MAIN_OBJECT);
    header.SetClientTs(Simulator::Now());

    const auto requestSize = m_httpVariables->GetRequestSize();
    auto packet = Create<Packet>(requestSize);
    packet->AddHeader(header);
    const auto packetSize = packet->GetSize();
    m_txMainObjectRequestTrace(packet);
    m_txTrace(packet);
    const auto actualBytes = m_socket->Send(packet);
    NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packet->GetSize() << " bytes,"
                      << " return value= " << actualBytes << ".");
    if (actualBytes != static_cast<int>(packetSize))
    {
        NS_LOG_ERROR(this << " Failed to send request for embedded object,"
                          << " GetErrNo= " << m_socket->GetErrno() << ","
                          << " waiting for another Tx opportunity.");
    }
    else
    {
        SwitchToState(EXPECTING_MAIN_OBJECT);
        m_pageLoadStartTs = Simulator::Now(); // start counting page loading time
    }
}

void
ThreeGppHttpClient::RequestEmbeddedObject()
{
    NS_LOG_FUNCTION(this);

    if (m_state != CONNECTING && m_state != PARSING_MAIN_OBJECT &&
        m_state != EXPECTING_EMBEDDED_OBJECT)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for RequestEmbeddedObject().");
    }

    if (m_embeddedObjectsToBeRequested == 0)
    {
        NS_LOG_WARN(this << " No embedded object to be requested.");
        return;
    }

    ThreeGppHttpHeader header;
    header.SetContentLength(0); // Request does not need any content length.
    header.SetContentType(ThreeGppHttpHeader::EMBEDDED_OBJECT);
    header.SetClientTs(Simulator::Now());

    const auto requestSize = m_httpVariables->GetRequestSize();
    auto packet = Create<Packet>(requestSize);
    packet->AddHeader(header);
    const auto packetSize = packet->GetSize();
    m_txEmbeddedObjectRequestTrace(packet);
    m_txTrace(packet);
    const auto actualBytes = m_socket->Send(packet);
    NS_LOG_DEBUG(this << " Send() packet " << packet << " of " << packet->GetSize() << " bytes,"
                      << " return value= " << actualBytes << ".");

    if (actualBytes != static_cast<int>(packetSize))
    {
        NS_LOG_ERROR(this << " Failed to send request for embedded object,"
                          << " GetErrNo= " << m_socket->GetErrno() << ","
                          << " waiting for another Tx opportunity.");
    }
    else
    {
        m_embeddedObjectsToBeRequested--;
        SwitchToState(EXPECTING_EMBEDDED_OBJECT);
    }
}

void
ThreeGppHttpClient::ReceiveMainObject(Ptr<Packet> packet, const Address& from)
{
    NS_LOG_FUNCTION(this << packet << from);

    if (m_state != EXPECTING_MAIN_OBJECT)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ReceiveMainObject().");
    }

    /*
     * In the following call to Receive(), #m_objectBytesToBeReceived *will*
     * be updated. #m_objectClientTs and #m_objectServerTs *may* be updated.
     * ThreeGppHttpHeader will be removed from the packet, if it is the first
     * packet of the object to be received; the header will be available in
     * #m_constructedPacketHeader.
     * #m_constructedPacket will also be updated.
     */
    Receive(packet);
    m_rxMainObjectPacketTrace(packet);

    if (m_objectBytesToBeReceived > 0)
    {
        /*
         * There are more packets of this main object, so just stay still
         * and wait until they arrive.
         */
        NS_LOG_INFO(this << " " << m_objectBytesToBeReceived << " byte(s)"
                         << " remains from this chunk of main object.");
        return;
    }

    /*
     * This is the last packet of this main object. Acknowledge the
     * reception of a whole main object
     */
    NS_LOG_INFO(this << " Finished receiving a main object.");
    m_rxMainObjectTrace(this, m_constructedPacket);

    if (!m_objectServerTs.IsZero())
    {
        m_rxDelayTrace(Simulator::Now() - m_objectServerTs, from);
        m_objectServerTs = MilliSeconds(0); // Reset back to zero.
    }

    if (!m_objectClientTs.IsZero())
    {
        m_rxRttTrace(Simulator::Now() - m_objectClientTs, from);
        m_objectClientTs = MilliSeconds(0); // Reset back to zero.
    }

    EnterParsingTime();
}

void
ThreeGppHttpClient::ReceiveEmbeddedObject(Ptr<Packet> packet, const Address& from)
{
    NS_LOG_FUNCTION(this << packet << from);

    if (m_state != EXPECTING_EMBEDDED_OBJECT)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ReceiveEmbeddedObject().");
    }

    /*
     * In the following call to Receive(), #m_objectBytesToBeReceived *will*
     * be updated. #m_objectClientTs and #m_objectServerTs *may* be updated.
     * ThreeGppHttpHeader will be removed from the packet, if it is the first
     * packet of the object to be received; the header will be available in
     * #m_constructedPacket, which will also be updated.
     */
    Receive(packet);
    m_rxEmbeddedObjectPacketTrace(packet);

    if (m_objectBytesToBeReceived > 0)
    {
        /*
         * There are more packets of this embedded object, so just stay
         * still and wait until they arrive.
         */
        NS_LOG_INFO(this << " " << m_objectBytesToBeReceived << " byte(s)"
                         << " remains from this chunk of embedded object");
        return;
    }

    /*
     * This is the last packet of this embedded object. Acknowledge
     * the reception of a whole embedded object
     */
    NS_LOG_INFO(this << " Finished receiving an embedded object.");
    m_rxEmbeddedObjectTrace(this, m_constructedPacket);

    if (!m_objectServerTs.IsZero())
    {
        m_rxDelayTrace(Simulator::Now() - m_objectServerTs, from);
        m_objectServerTs = MilliSeconds(0); // Reset back to zero.
    }

    if (!m_objectClientTs.IsZero())
    {
        m_rxRttTrace(Simulator::Now() - m_objectClientTs, from);
        m_objectClientTs = MilliSeconds(0); // Reset back to zero.
    }

    if (m_embeddedObjectsToBeRequested > 0)
    {
        NS_LOG_INFO(this << " " << m_embeddedObjectsToBeRequested
                         << " more embedded object(s) to be requested.");
        // Immediately request another using the existing connection.
        m_eventRequestEmbeddedObject =
            Simulator::ScheduleNow(&ThreeGppHttpClient::RequestEmbeddedObject, this);
    }
    else
    {
        /*
         * There is no more embedded object, the web page has been
         * downloaded completely. Now is the time to read it.
         */
        NS_LOG_INFO(this << " Finished receiving a web page.");
        FinishReceivingPage(); // trigger callback for page loading time
        EnterReadingTime();
    }
}

void
ThreeGppHttpClient::Receive(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    /* In a "real" HTTP message the message size is coded differently. The use of a header
     * is to avoid the burden of doing a real message parser.
     */
    bool firstPacket = false;

    if (m_objectBytesToBeReceived == 0)
    {
        // This is the first packet of the object.
        firstPacket = true;

        // Remove the header in order to calculate remaining data to be received.
        ThreeGppHttpHeader httpHeader;
        packet->RemoveHeader(httpHeader);

        m_objectBytesToBeReceived = httpHeader.GetContentLength();
        m_objectClientTs = httpHeader.GetClientTs();
        m_objectServerTs = httpHeader.GetServerTs();

        // Take a copy for constructed packet trace. Note that header is included.
        m_constructedPacket = packet->Copy();
        m_constructedPacket->AddHeader(httpHeader);
    }
    auto contentSize = packet->GetSize();
    m_numberBytesPage += contentSize; // increment counter of page size

    /* Note that the packet does not contain header at this point.
     * The content is purely raw data, which was the only intended data to be received.
     */
    if (m_objectBytesToBeReceived < contentSize)
    {
        NS_LOG_WARN(this << " The received packet (" << contentSize << " bytes of content)"
                         << " is larger than the content that we expected to receive ("
                         << m_objectBytesToBeReceived << " bytes).");
        // Stop expecting any more packet of this object.
        m_objectBytesToBeReceived = 0;
        m_constructedPacket = nullptr;
    }
    else
    {
        m_objectBytesToBeReceived -= contentSize;
        if (!firstPacket)
        {
            auto packetCopy = packet->Copy();
            m_constructedPacket->AddAtEnd(packetCopy);
        }
    }
}

void
ThreeGppHttpClient::EnterParsingTime()
{
    NS_LOG_FUNCTION(this);

    if (m_state != EXPECTING_MAIN_OBJECT)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for EnterParsingTime().");
    }

    const auto parsingTime = m_httpVariables->GetParsingTime();
    NS_LOG_INFO(this << " The parsing of this main object will complete in "
                     << parsingTime.As(Time::S) << ".");
    m_eventParseMainObject =
        Simulator::Schedule(parsingTime, &ThreeGppHttpClient::ParseMainObject, this);
    SwitchToState(PARSING_MAIN_OBJECT);
}

void
ThreeGppHttpClient::ParseMainObject()
{
    NS_LOG_FUNCTION(this);

    if (m_state != PARSING_MAIN_OBJECT)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for ParseMainObject().");
    }

    m_embeddedObjectsToBeRequested = m_httpVariables->GetNumOfEmbeddedObjects();
    // saving total number of embedded objects
    m_numberEmbeddedObjectsRequested = m_embeddedObjectsToBeRequested;
    NS_LOG_INFO(this << " Parsing has determined " << m_embeddedObjectsToBeRequested
                     << " embedded object(s) in the main object.");

    if (m_embeddedObjectsToBeRequested > 0)
    {
        /*
         * Immediately request the first embedded object using the
         * existing connection.
         */
        m_eventRequestEmbeddedObject =
            Simulator::ScheduleNow(&ThreeGppHttpClient::RequestEmbeddedObject, this);
    }
    else
    {
        /*
         * There is no embedded object in the main object. So sit back and
         * enjoy the plain web page.
         */
        NS_LOG_INFO(this << " Finished receiving a web page.");
        FinishReceivingPage(); // trigger callback for page loading time
        EnterReadingTime();
    }
}

void
ThreeGppHttpClient::EnterReadingTime()
{
    NS_LOG_FUNCTION(this);

    if (m_state != EXPECTING_EMBEDDED_OBJECT && m_state != PARSING_MAIN_OBJECT)
    {
        NS_FATAL_ERROR("Invalid state " << GetStateString() << " for EnterReadingTime().");
    }

    const auto readingTime = m_httpVariables->GetReadingTime();
    NS_LOG_INFO(this << " Client will finish reading this web page in " << readingTime.As(Time::S)
                     << ".");

    // Schedule a request of another main object once the reading time expires.
    m_eventRequestMainObject =
        Simulator::Schedule(readingTime, &ThreeGppHttpClient::RequestMainObject, this);
    SwitchToState(READING);
}

void
ThreeGppHttpClient::CancelAllPendingEvents()
{
    NS_LOG_FUNCTION(this);

    if (!Simulator::IsExpired(m_eventRequestMainObject))
    {
        NS_LOG_INFO(this << " Canceling RequestMainObject() which is due in "
                         << Simulator::GetDelayLeft(m_eventRequestMainObject).As(Time::S) << ".");
        Simulator::Cancel(m_eventRequestMainObject);
    }

    if (!Simulator::IsExpired(m_eventRequestEmbeddedObject))
    {
        NS_LOG_INFO(this << " Canceling RequestEmbeddedObject() which is due in "
                         << Simulator::GetDelayLeft(m_eventRequestEmbeddedObject).As(Time::S)
                         << ".");
        Simulator::Cancel(m_eventRequestEmbeddedObject);
    }

    if (!Simulator::IsExpired(m_eventParseMainObject))
    {
        NS_LOG_INFO(this << " Canceling ParseMainObject() which is due in "
                         << Simulator::GetDelayLeft(m_eventParseMainObject).As(Time::S) << ".");
        Simulator::Cancel(m_eventParseMainObject);
    }
}

void
ThreeGppHttpClient::SwitchToState(ThreeGppHttpClient::State_t state)
{
    const auto oldState = GetStateString();
    const auto newState = GetStateString(state);
    NS_LOG_FUNCTION(this << oldState << newState);

    if ((state == EXPECTING_MAIN_OBJECT) || (state == EXPECTING_EMBEDDED_OBJECT))
    {
        if (m_objectBytesToBeReceived > 0)
        {
            NS_FATAL_ERROR("Cannot start a new receiving session if the previous object ("
                           << m_objectBytesToBeReceived
                           << " bytes) is not completely received yet.");
        }
    }

    m_state = state;
    NS_LOG_INFO(this << " HttpClient " << oldState << " --> " << newState << ".");
    m_stateTransitionTrace(oldState, newState);
}

void
ThreeGppHttpClient::FinishReceivingPage()
{
    m_rxPageTrace(this,
                  Simulator::Now() - m_pageLoadStartTs,
                  m_numberEmbeddedObjectsRequested,
                  m_numberBytesPage);
    // Reset counter variables.
    m_numberEmbeddedObjectsRequested = 0;
    m_numberBytesPage = 0;
}

} // namespace ns3
