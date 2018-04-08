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

#ifndef __CS_SOCKET_H__
#define __CS_SOCKET_H__

#include "socketbase.h"
#include "inetwork/socket.h"
#include "csutil/hashcomputer.h"
#include <cstring>

using namespace CS::Network::Socket;

CS_PLUGIN_NAMESPACE_BEGIN(Socket)
{
  namespace
  {
    static int const AddressLookup[2] = {AF_INET, AF_INET6};
    static int const DomainLookup[2] = {PF_INET, PF_INET6};
    static int const ProtocolLookup[2] = {IPPROTO_TCP, IPPROTO_UDP};
    static int const TypeLookup[2] = {SOCK_STREAM, SOCK_DGRAM};
  } // namespace

  template<Family family> struct Address;

  template<>
  class Address<CS_SOCKET_FAMILY_IP4>
    : public scfImplementation1<Address<CS_SOCKET_FAMILY_IP4>, iAddress>
  {
  public:
    typedef sockaddr_in Type;

  private:
    char address[INET_ADDRSTRLEN];
    int port;
    Type socketAddress;

  public:
    Address() : scfImplementationType(this)
    {
      memset(&socketAddress, 0, sizeof(Type));
    }

    Address(Type const *a) : scfImplementationType(this)
    {
      socketAddress = *a;
      Update();
    }

    void Update()
    {
      Platform::InetNtoP(AddressLookup[CS_SOCKET_FAMILY_IP4], &(socketAddress.sin_addr), address, INET_ADDRSTRLEN);
      port = ntohs(socketAddress.sin_port);
    }

    char const *GetAddress() const
    {
      return address;
    }

    int GetPort() const
    {
      return port;
    }

    Family GetFamily() const
    {
      return CS_SOCKET_FAMILY_IP4;
    }

    bool IsSame(iAddress const *other) const
    {
      // compare family
      if(other->GetFamily() != CS_SOCKET_FAMILY_IP4)
      {
	return false;
      }

      // cast to our type
      Address<CS_SOCKET_FAMILY_IP4> const* handle = static_cast<Address<CS_SOCKET_FAMILY_IP4> const *>(other);

      // compare port
      if(handle->port != port)
      {
	return false;
      }

      // compare address
      if(handle->socketAddress.sin_addr.s_addr != socketAddress.sin_addr.s_addr)
      {
	return false;
      }

      // same address
      return true;
    }
    
    uint GetHash() const
    {
      uint hash = csHashComputerStruct<in_addr>::ComputeHash(socketAddress.sin_addr);
      HashCombine(hash, port);
      return hash;
    }

    sockaddr const *GetStruct() const
    {
      return reinterpret_cast<sockaddr const *>(&socketAddress);
    }

    sockaddr *GetStruct()
    {
      return reinterpret_cast<sockaddr *>(&socketAddress);
    }
  };

  template<>
  struct Address<CS_SOCKET_FAMILY_IP6>
    : public scfImplementation1<Address<CS_SOCKET_FAMILY_IP6>, iAddress>
  {
  public:
    typedef sockaddr_in6 Type;

  private:
    char address[INET6_ADDRSTRLEN];
    int port;
    Type socketAddress;

  public:
    Address() : scfImplementationType(this)
    {
      memset(&socketAddress, 0, sizeof(Type));
    }

    Address(Type const *a) : scfImplementationType(this)
    {
      socketAddress = *a;
      Update();
    }

    void Update()
    {
      Platform::InetNtoP(AddressLookup[CS_SOCKET_FAMILY_IP6], &(socketAddress.sin6_addr), address, INET6_ADDRSTRLEN);
      port = ntohs(socketAddress.sin6_port);
    }

    char const *GetAddress() const
    {
      return address;
    }

    int GetPort() const
    {
      return port;
    }

    Family GetFamily() const
    {
      return CS_SOCKET_FAMILY_IP6;
    }

    bool IsSame(iAddress const *other) const
    {
      // compare family
      if(other->GetFamily() != CS_SOCKET_FAMILY_IP4)
      {
	return false;
      }

      // cast to our type
      Address<CS_SOCKET_FAMILY_IP6> const* handle = static_cast<Address<CS_SOCKET_FAMILY_IP6> const *>(other);

      // compare port
      if(handle->port != port)
      {
	return false;
      }

      // compare address
      if(memcmp(&handle->socketAddress.sin6_addr, &socketAddress.sin6_addr, sizeof(in6_addr)) != 0)
      {
	return false;
      }

      // same address
      return true;
    }

    uint GetHash() const
    {
      uint hash = csHashComputerStruct<in6_addr>::ComputeHash(socketAddress.sin6_addr);
      HashCombine(hash, port);
      return hash;
    }

    sockaddr const *GetStruct() const
    {
      return reinterpret_cast<sockaddr const *>(&socketAddress);
    }

    sockaddr *GetStruct()
    {
      return reinterpret_cast<sockaddr *>(&socketAddress);
    }
  };

  template<Family family, Protocol protocol>
  class Socket
    : public scfImplementation1<Socket<family, protocol>, iSocket>
  {
  private:
    // convenience typedef
    typedef Socket<family, protocol> ThisType;
    typedef Address<family> AddressType;

    bool ready;
    bool connected;
    Platform::Socket socket;

  public:
    Socket()
      : scfImplementation1<Socket<family, protocol>, iSocket>(this),
	ready(false), connected(false)
    {
      // create socket
      socket = ::socket(DomainLookup[family], TypeLookup[protocol], ProtocolLookup[protocol]);

      // ensure socket is valid
      CS_ASSERT(socket != Platform::invalidSocket);
    }

    Socket(Platform::Socket s)
      :	scfImplementation1<Socket<family, protocol>, iSocket>(this),
	ready(true), connected(true),
	socket(s)
    {
      // ensure socket is valid
      CS_ASSERT(socket != Platform::invalidSocket);
    }

    virtual ~Socket()
    {
      ready = false;
      connected = false;
      Platform::Close(socket);
    }

    bool Bind(iAddress const *address)
    {
      // ensure the address is for the right family
      CS_ASSERT(address->GetFamily() == family);

      // get internal address type
      AddressType const *socketAddress = static_cast<AddressType const *>(address);

      // try to bind
      if(bind(socket, socketAddress->GetStruct(), sizeof(typename AddressType::Type)) == 0)
      {
	// bind succeeded
	if(protocol == CS_SOCKET_PROTOCOL_UDP)
	{
	  // UDP sockets are ready when bound
	  ready = true;
	}
	return true;
      }

      // bind failed
      return false;
    }

    bool Listen(int queueSize)
    {
      // only stream protocols support active listening
      if(protocol != CS_SOCKET_PROTOCOL_TCP)
      {
	return false;
      }

      // try to listen for incoming connections
      if(listen(socket, queueSize) == 0)
      {
	// success
	return true;
      }

      // failed to listen
      return false;
    }

    csPtr<iSocket> Accept(csRef<iAddress> *client = nullptr)
    {
      // only tcp supports active listening
      if(protocol != CS_SOCKET_PROTOCOL_TCP)
      {
	CS_ASSERT(false);
	return csPtr<iSocket>(nullptr);
      }

      // try to accept a connection
      Platform::Socket child;
      // check whether we want to retrieve information about the client
      if(client)
      {
	// we do want to retrieve information about the client
	// create address
	AddressType *socketAddress = new AddressType;

	// placeholder for result size
	socklen_t size = sizeof(typename AddressType::Type);

	// accept connection
	child = accept(socket, socketAddress->GetStruct(), &size);

	// ensure the size returned matches our expected one
	CS_ASSERT(size == sizeof(typename AddressType::Type));

	// update address data
	socketAddress->Update();

	// attach address to passed reference
	client->AttachNew(socketAddress);
      }
      else
      {
	// accept connection
	child = accept(socket, nullptr, nullptr);
      }

      // check whether we actually got one
      if(child != Platform::invalidSocket)
      {
	// we did, create a new wrapper and return it
	csRef<iSocket> newSocket;
	newSocket.AttachNew(new ThisType(child));
	return csPtr<iSocket>(newSocket);
      }

      // getting a new connection failed
      return csPtr<iSocket>(nullptr);
    }

    bool Connect(iAddress const *client)
    {
      // @@@TODO: add possibility to unconnect udp sockets here
      // ensure the address is for the right family
      CS_ASSERT(client->GetFamily() == family);

      // get internal address type
      AddressType const *socketAddress = static_cast<AddressType const *>(client);

      // try to connect
      if(connect(socket, socketAddress->GetStruct(), sizeof(typename AddressType::Type)) == 0)
      {
	// connect succeeded
	if(protocol == CS_SOCKET_PROTOCOL_TCP)
	{
	  // TCP sockets are ready when connected
	  ready = true;
	}
	connected = true;
	return true;
      }

      // connect failed
      return false;
    }

    size_t Receive(char *buffer, size_t size, csRef<iAddress> *client = nullptr)
    {
      // check whether the socket is ready
      if(!ready)
      {
	// can't receive from a socket that isn't ready
	return static_cast<size_t>(-1);
      }

      // keep track how much we read
      int result = -1;

      // check whether information about the client should be filled
      if(client == nullptr)
      {
	// no, just receive normally
	result = recv(socket, buffer, static_cast<int>(size), 0);
      }
      else
      {
	// we do want to retrieve information about the client
	// create address
	AddressType *socketAddress = new AddressType;

	// placeholder for result size
	socklen_t size = sizeof(typename AddressType::Type);

	// receive data
	result = recvfrom(socket, buffer, size, 0, socketAddress->GetStruct(), &size);

	// ensure the size returned matches our expected one
	CS_ASSERT(size == sizeof(typename AddressType::Type));

	// update address data
	socketAddress->Update();

	// attach address to passed reference
	client->AttachNew(socketAddress);
      }

      // check for an error
      if(result < 0)
      {
	return static_cast<size_t>(-1);
      }

      // return number of bytes read
      return static_cast<size_t>(result);
    }

    size_t Send(char const *buffer, size_t size, iAddress *client = nullptr)
    {
      // check whether the socket is ready
      if(!ready)
      {
	// can't send on a socket that isn't ready
	return static_cast<size_t>(-1);
      }

      // stream sockets musn't specify a target
      // and must be connected
      if(protocol == CS_SOCKET_PROTOCOL_TCP)
      {
	if(client != nullptr || !connected)
	{
	  return static_cast<size_t>(-1);
	}
      }
      // non-stream sockets must either specify
      // a target or be connected
      else if(connected != (client == nullptr))
      {
	return static_cast<size_t>(-1);
      }

      // keep track how much data we could actually send
      int result = -1;

      // send message to the target
      if(client)
      {
	// ensure the address is for the right family
	CS_ASSERT(client->GetFamily() == family);

	// get internal address type
	AddressType const *socketAddress = static_cast<AddressType const *>(client);

	// send data to specified peer
#	ifdef CS_PLATFORM_WIN32
	// winsocks uses int instead of size_t for the buffer size
	result = sendto(socket, buffer, static_cast<int>(size), 0, socketAddress->GetStruct(), sizeof(typename AddressType::Type));
#	else
	result = sendto(socket, buffer, size, 0, socketAddress->GetStruct(), sizeof(typename AddressType::Type));
#	endif
      }
      else
      {
	// send data to connected peer
#	ifdef CS_PLATFORM_WIN32
	// winsocks uses int instead of size_t for the buffer size
	result = send(socket, buffer, static_cast<int>(size), 0);
#	else
	result = send(socket, buffer, size, 0);
#	endif
      }

      // check for error
      if(result < 0)
      {
	return static_cast<size_t>(-1);
      }

      // no error occured
      return static_cast<size_t>(result);
    }

    char const *GetLastError() const
    {
      // @@@TODO: we should handle errors we've thrown
      //          due to invalid input data here
      return Platform::GetError();
    }

    bool IsReady() const
    {
      return ready;
    }

    bool IsConnected() const
    {
      return connected;
    }

    Family GetFamily() const
    {
      return family;
    }

    Protocol GetProtocol() const
    {
      return protocol;
    }

    Platform::Socket GetSocket() const
    {
      return socket;
    }
  };
}
CS_PLUGIN_NAMESPACE_END(Socket)

#endif // __CS_SOCKET_H__
