/*
    Copyright (C) 2013 by Matthieu Kraus

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "socketmanager.h"
#include <csgeom/math.h>

using namespace CS::Network::Socket;

CS_PLUGIN_NAMESPACE_BEGIN(Socket)
{
  SCF_IMPLEMENT_FACTORY(SocketManager);

  Platform::Socket SocketManager::GetHandle(iSocket* socket) const
  {
    switch(socket->GetFamily())
    {
    case CS_SOCKET_FAMILY_IP4:
      switch(socket->GetProtocol())
      {
      case CS_SOCKET_PROTOCOL_TCP:
	return static_cast<Socket<CS_SOCKET_FAMILY_IP4,CS_SOCKET_PROTOCOL_TCP> *>(socket)->GetSocket();
      case CS_SOCKET_PROTOCOL_UDP:
	return static_cast<Socket<CS_SOCKET_FAMILY_IP4,CS_SOCKET_PROTOCOL_UDP> *>(socket)->GetSocket();
      default:
	return Platform::invalidSocket;
      }
    case CS_SOCKET_FAMILY_IP6:
      switch(socket->GetProtocol())
      {
      case CS_SOCKET_PROTOCOL_TCP:
	return static_cast<Socket<CS_SOCKET_FAMILY_IP6,CS_SOCKET_PROTOCOL_TCP> *>(socket)->GetSocket();
      case CS_SOCKET_PROTOCOL_UDP:
	return static_cast<Socket<CS_SOCKET_FAMILY_IP6,CS_SOCKET_PROTOCOL_UDP> *>(socket)->GetSocket();
      default:
	return Platform::invalidSocket;
      }
      default:
	return Platform::invalidSocket;
    }

    return Platform::invalidSocket;
  }

  SocketManager::SocketManager(iBase *parent) : scfImplementationType(this, parent)
  {
    // perform one-time platform dependent socket library initialization
    Platform::Initialize();
  }

  SocketManager::~SocketManager()
  {
    // perform one-time platform dependent socket library cleanup
    Platform::Cleanup();
  }

  csPtr<iSocket> SocketManager::CreateSocket(Family family, Protocol protocol) const
  {
    csRef<iSocket> socket;
    switch(family)
    {
    case CS_SOCKET_FAMILY_IP4:
      {
	switch(protocol)
	{
	case CS_SOCKET_PROTOCOL_TCP:
	  socket.AttachNew(new Socket<CS_SOCKET_FAMILY_IP4,CS_SOCKET_PROTOCOL_TCP>);
	  break;

	case CS_SOCKET_PROTOCOL_UDP:
	  socket.AttachNew(new Socket<CS_SOCKET_FAMILY_IP4,CS_SOCKET_PROTOCOL_UDP>);
	  break;

	default:
	  CS_ASSERT(false);
	  break;
	}
	break;
      }

    case CS_SOCKET_FAMILY_IP6:
      {
	switch(protocol)
	{
	case CS_SOCKET_PROTOCOL_TCP:
	  socket.AttachNew(new Socket<CS_SOCKET_FAMILY_IP6,CS_SOCKET_PROTOCOL_TCP>);
	  break;

	case CS_SOCKET_PROTOCOL_UDP:
	  socket.AttachNew(new Socket<CS_SOCKET_FAMILY_IP6,CS_SOCKET_PROTOCOL_UDP>);
	  break;

	default:
	  CS_ASSERT(false);
	  break;
	}
	break;
      }

    default:
      CS_ASSERT(false);
      break;
    }

    return csPtr<iSocket>(socket);
  }

  csPtr<iAddress> SocketManager::Resolve(char const *host, char const *service, Family family, Protocol protocol) const
  {
    // get struct to hold hints
    addrinfo hints;

    // clear it
    memset(&hints, 0, sizeof(addrinfo));

    // set restrictions
    hints.ai_family = AddressLookup[family];
    hints.ai_socktype = TypeLookup[protocol];
    hints.ai_protocol = ProtocolLookup[protocol];

    // get struct to hold hints
    addrinfo *info = nullptr;

    // get info
    if(getaddrinfo(host, service, &hints, &info) != 0)
    {
      return csPtr<iAddress>(0);
    }
    else
    {
      // ensure our hints weren't ignored
      CS_ASSERT(info->ai_family == hints.ai_family);
      CS_ASSERT(info->ai_socktype == hints.ai_socktype);
      CS_ASSERT(info->ai_protocol == hints.ai_protocol);

      // create new address
      csRef<iAddress> result;
      if(family == CS_SOCKET_FAMILY_IP4)
      {
	// ensure result length matches
	CS_ASSERT(info->ai_addrlen == sizeof(Address<CS_SOCKET_FAMILY_IP4>::Type));

	// allocate address
	result.AttachNew(
	  new Address<CS_SOCKET_FAMILY_IP4>(
	    reinterpret_cast<Address<CS_SOCKET_FAMILY_IP4>::Type *>(info->ai_addr)));
      }
      else if(family == CS_SOCKET_FAMILY_IP6)
      {
	// ensure result length matches
	CS_ASSERT(info->ai_addrlen == sizeof(Address<CS_SOCKET_FAMILY_IP6>::Type));

	// allocate address
	result.AttachNew(
	  new Address<CS_SOCKET_FAMILY_IP6>(
	    reinterpret_cast<Address<CS_SOCKET_FAMILY_IP6>::Type *>(info->ai_addr)));
      }
      else
      {
	CS_ASSERT(false);
      }

      // free structure
      freeaddrinfo(info);

      // return result
      return csPtr<iAddress>(result);
    }

    return csPtr<iAddress>(nullptr);
  }

  void SocketManager::Select(iSocketArray *read, iSocketArray *write) const
  {
    // allocate and clear reading set
    fd_set readSet;
    FD_ZERO(&readSet);

    // get stack array to cache reading file descriptors
    CS_ALLOC_STACK_ARRAY(Platform::Socket,readFDs,read->GetSize());

    // fill reading set and cache array
    for(size_t i = 0; i < read->GetSize(); ++i)
    {
      // get file descriptor
      readFDs[i] = GetHandle(read->Get(i));

      // set it in the set
      FD_SET(readFDs[i], &readSet);
    }

    // allocate and clear writing set
    fd_set writeSet;
    FD_ZERO(&writeSet);

    // get stack array to cache writing file descriptors
    CS_ALLOC_STACK_ARRAY(Platform::Socket,writeFDs,write->GetSize());

    // fill writing set and cache array
    for(size_t i = 0; i < write->GetSize(); ++i)
    {
      // get file descriptor
      writeFDs[i] = GetHandle(write->Get(i));

      // set it in the set
      FD_SET(writeFDs[i], &writeSet);
    }

    // prepare timeout to signal we don't want to block
    timeval timeout;
    memset(&timeout, 0, sizeof(timeval));

    // select sockets
    size_t count = csMax(csMax(read->GetSize(), write->GetSize()), GetSelectLimit());
    if(select(static_cast<int>(count), &readSet, &writeSet, nullptr, &timeout) <= 0)
    {
      // timeout or error, nothing can be read or written
      read->Empty();
      write->Empty();
    }
    else
    {
      // select succeeded
      // remove sockets that can't be read from read array
      for(size_t i = 0; i < read->GetSize(); ++i)
      {
	if(!FD_ISSET(readFDs[i],&readSet))
	{
	  read->DeleteIndex(i);
	}
      }

      // remove sockets that can't be written from write array
      for(size_t i = 0; i < write->GetSize(); ++i)
      {
	if(!FD_ISSET(writeFDs[i],&writeSet))
	{
	  write->DeleteIndex(i);
	}
      }
    }
  }

  size_t SocketManager::GetSelectLimit() const
  {
    return static_cast<size_t>(FD_SETSIZE-1);
  }
}
CS_PLUGIN_NAMESPACE_END(Socket)
