/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
//#include "udp-client-server-helper.h"
//#include "ns3/udp-server.h"
//#include "ns3/udp-client.h"
//#include "ns3/udp-trace-client.h"

#include "MyUdpClient-ServerHelper.h"

#include "MyUdpClient.h"

#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 
{
MyUdpClientHelper::MyUdpClientHelper ()
{
}

MyUdpClientHelper::MyUdpClientHelper (HubInfo* hub, ClientSenderType clientSenderType, Address address, uint16_t port)
{
  hub_ = hub;
  clientSenderType_ = clientSenderType;

  m_factory.SetTypeId (MyUdpClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void
MyUdpClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
MyUdpClientHelper::Install (NodeContainer c, uint32_t cliendIdStart)
{
  ApplicationContainer apps;
  uint32_t clientId = cliendIdStart;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i, clientId++)
  {
    Ptr<Node> node = *i;
    Ptr<MyUdpClient> client = m_factory.Create<MyUdpClient> ();
    client->setClientSenderType(clientSenderType_, clientId);    
    client->setHubHandle(hub_);
    node->AddApplication (client);
    apps.Add (client);
  }
  return apps;
}
} // namespace ns3
