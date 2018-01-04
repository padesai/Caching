/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
//#include "udp-client.h"
#include "ns3/seq-ts-header.h"
#include <cstdlib>
#include <cstdio>
#include "MyTag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include <iostream>


#include "MyUdpClient.h"

#include "HubAndServerInfo.h"
namespace ns3 {



NS_LOG_COMPONENT_DEFINE ("MyUdpClient")
  ;
NS_OBJECT_ENSURE_REGISTERED (MyUdpClient)
  ;

TypeId
MyUdpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyUdpClient")
    .SetParent<Application> ()
    .AddConstructor<MyUdpClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&MyUdpClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&MyUdpClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&MyUdpClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&MyUdpClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&MyUdpClient::m_size),
                   MakeUintegerChecker<uint32_t> (12,1500))
  ;
  return tid;
}

MyUdpClient::MyUdpClient()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
}

MyUdpClient::~MyUdpClient ()
{
  NS_LOG_FUNCTION (this);
}

void MyUdpClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void MyUdpClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void MyUdpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void MyUdpClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
  {
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket (GetNode (), tid);
    if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
    {
        m_socket->Bind ();
        m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
    }
    else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
    {
      m_socket->Bind6 ();
      m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
    }
  }

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &MyUdpClient::Send, this);
}

void MyUdpClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}

void MyUdpClient::Send (void)
{
  switch (clientSenderType_)
  {
    case LEAF_TO_HUB_SENDER:
    {
      if (m_sent >= m_count)
      {
        return; // end the client
      }
      if (hub_->canLeafSendToHub())
      {
        uint64_t frameId = hub_->getRandomFrameId(senderId_);
        //std::cout << "MyUdpClient: Leaf request to server : " << frameId << std::endl;
        actualTransmit(frameId);
        return;
      }
    }
    case HUB_TO_SERVER_SENDER:
    {
      if (hub_->canHubSendToServer())
      {
        uint64_t frameId = hub_->getNextHubTxTag();  
       // std::cout << "MyUdpClient: Hub sending to server : " << frameId << std::endl;
        actualTransmit(frameId);
        return;
      }
    }
    break;
    case HUB_TO_LEAF_SENDER:
    {
      if (hub_->canHubSendToLeaf(senderId_))
      {
        uint64_t frameId = hub_->getHubToLeafResponseFileId(senderId_);  
        actualTransmit(frameId);
        return;
      }
    }
    break;
    case SERVER_TO_HUB_SENDER:
    {
      if (hub_->canServerSendToHub())
      {
        uint64_t frameId = hub_->getServerToHubResponseFileId();  
        actualTransmit(frameId);
        return;
      }
    }
    break;
  }

  m_sendEvent = Simulator::Schedule (m_interval, &MyUdpClient::Send, this);   
}

void MyUdpClient::actualTransmit(uint64_t frameId)
{
  Ptr <Node> node = GetNode ();
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header
  p->AddHeader (seqTs);

  MyTag tag;
  tag.SetTag(frameId);
  
  p->AddPacketTag(tag);

  //p->PrintPacketTags (std::cout);

  
  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
  {
    peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
  }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
  {
    peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
  }

  if ((m_socket->Send (p)) >= 0)
  {
    ++m_sent;
    NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
                                  << peerAddressStringStream.str () << " Uid: "
                                  << p->GetUid () << " Time: "
                                  << (Simulator::Now ()).GetSeconds ());

  }
  else
  {
    NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                        << peerAddressStringStream.str ());
  }

  m_sendEvent = Simulator::Schedule (m_interval, &MyUdpClient::Send, this);  
}

void MyUdpClient::setHubHandle(HubInfo* hub)
{
  hub_ = hub;
}

void MyUdpClient::setClientSenderType(ClientSenderType type, uint32_t senderId)
{
  clientSenderType_ = type;
  senderId_ = senderId;
}
} // Namespace ns3
