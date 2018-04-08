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

#ifndef __INETWORK_SOCKET_H__
#define __INETWORK_SOCKET_H__

#include "cssysdef.h"

#include "iutil/array.h"
#include "csutil/scf.h"
#include "csutil/scf_implementation.h"

namespace CS
{
namespace Network
{
namespace Socket
{
/// Address families support by the socket implementation.
enum Family
{
  CS_SOCKET_FAMILY_IP4,
  CS_SOCKET_FAMILY_IP6
};

/// Transport protocols supported by the socket implementation.
enum Protocol
{
  CS_SOCKET_PROTOCOL_TCP,
  CS_SOCKET_PROTOCOL_UDP
};

/**
 * An endpoint to be used with an iSocket connection.
 * Obtained by resolving an address via iSocketManager.
 */
struct iAddress : public virtual iBase
{
  SCF_INTERFACE(iAddress, 0, 1, 0);

  virtual char const *GetAddress() const = 0;

  virtual int GetPort() const = 0;

  virtual Family GetFamily() const = 0;

  virtual bool IsSame(iAddress const *other) const = 0;

  virtual uint GetHash() const = 0;
};

/**
 * A socket connection obtained from iSocketManager.
 */
struct iSocket : public virtual iBase
{
  SCF_INTERFACE(iSocket, 0, 1, 0);

  /**
   * Bind this socket to a local address.
   * This is required before any communication can
   * can occur for UDP sockets and is also required
   * for TCP servers before any listening can occur.
   */
  virtual bool Bind(iAddress const *address) = 0;

  /**
   * Start to listen for incoming connections on the
   * the bound address. This is required before any
   * connections can be accepted for TCP servers.
   *
   * The queue size specifies how many incoming
   * connections that haven't been accepted, yet,
   * may be waiting in a queue without being rejected.
   */
  virtual bool Listen(int queueSize) = 0;

  /**
   * Accept a currently pending connection while
   * listening from them on a bound address.
   * 
   * This will block until a connection is ready to
   * be accepted if there's non currently waiting.
   * In order to check whether there's a connection
   * pending in the queue use iSocketManager::Select
   * and check whether this socket is ready to be read.
   *
   * If a pointer to a placeholder for an iAddress is passed
   * it'll hold the address of the peer that connected on
   * success.
   *
   * Returns a socket for the established connection on
   * success, else nullptr is returned.
   */
  virtual csPtr<iSocket> Accept(csRef<iAddress> *client = nullptr) = 0;

  /**
   * Connects to the peer specified by the passed iAddress.
   * 
   * This is required for TCP sockets before any data can be
   * transmitted.
   *
   * For UDP sockets connecting is optional and specifies
   * that data may only be sent to and received from the specified
   * peer.
   *
   * Returns true if a connection could be established and false
   * if an error occured.
   */
  virtual bool Connect(iAddress const *client) = 0;

  /**
   * Receive data from the peer this socket is connected to if
   * a connection was established.
   *
   * If no connection was established and this is a UDP socket
   * data from any peer will be read.
   *
   * The read data will be written to the buffer passed and at most
   * size bytes will be read.
   *
   * If a pointer to a placeholder for an iAddress was passed this
   * placeholder will be filled with information about the peer
   * from which the data was received.
   *
   * Returns the number of bytes actually received into the buffer
   * or size_t(-1) if an error occured.
   */
  virtual size_t Receive(char *buffer, size_t size, csRef<iAddress> *client = nullptr) = 0;

  /**
   * Sends at most size bytes of data read from buffer to a peer.
   * The peer the data is sent to is the one this socket is connected
   * to if this socket is connect and otherwise the one specified by
   * the passed address. It is an error to specify a peer if the socket
   * is connected or is a TCP socket and to not pass a peer if the socket
   * is not connected and a UDP socket.
   *
   * On success the actual numeber of bytes sent will be returned which will
   * not exceed size. Otherwise size_t(-1) is returned to indicate an error.
   */
  virtual size_t Send(char const *buffer, size_t size, iAddress *client = nullptr) = 0;

  /**
   * Obtain a textual description of the last error that occured.
   * Note that due to platform restrictions this error may not have
   * necessarily occured due to errors related to this socket.
   */
  virtual char const *GetLastError() const = 0;

  /**
   * Check whether this socket is ready to transmit data.
   * Returns true if it is, false otherwise.
   */
  virtual bool IsReady() const = 0;

  /**
   * Check whether this socket is associated with a peer.
   * Returns true if it is, false otherwise.
   */
  virtual bool IsConnected() const = 0;

  /**
   * Returns the address family this socket can work with.
   */
  virtual Family GetFamily() const = 0;

  /**
   * Returns the transport protocol this socket is using.
   */
  virtual Protocol GetProtocol() const = 0;
};

/// An array of sockets.
struct iSocketArray : public iArrayChangeAll<iSocket *>
{
  SCF_IARRAYCHANGEALL_INTERFACE(iSocketArray);
};

/**
 * The iSocketManager interface is the main interface for
 * socket usage. It's used to create sockets, resolve
 * addresses and select sockets which are ready for
 * processing.
 */
struct iSocketManager : public virtual iBase
{
  SCF_INTERFACE(iSocketManager, 0, 1, 0);

  /**
   * Create a socket for the specified address family and transport protocol.
   * Returns a new socket on success, nullptr otherwise.
   */
  virtual csPtr<iSocket> CreateSocket(Family family, Protocol protocol) const = 0;

  /**
   * Resolves an address for usage with sockets using the specified
   * address family and transport protocol.
   * 
   * The host may be either a formatted address suitable for the address family,
   * a name that resolves to such an address using name lookup or nullptr in which
   * case the local loopback address for this family will be used.
   *
   * The service name may either be the textual representation of an integer specifying
   * a service location at the specified address or a name that resolves to such an integer
   * using service name lookup procedures.
   *
   * If the host and the service could be resolved for the given address family and transport
   * protocol a valid address for usage with sockets of this type will be returned, otherwise
   * nullptr will be returned.
   */
  virtual csPtr<iAddress> Resolve(char const *host, char const *service, Family family, Protocol protocol) const = 0;

  /**
   * Selects sockets which are ready for processing from a given set of sockets.
   *
   * Sockets specified in read and write will be checked whether they can be
   * read or written respectively without blocking and only those who can will
   * remain in the arrays upon completion.
   *
   * Note that only a platform specific amount of sockets can be checked for
   * each processing type at once and while you may pass more to check any further
   * ones will be ignored and accordingly not remain in the ouput arrays.
   * You can query the amount of sockets that can be checked at once using GetSelectLimit().
   */
  virtual void Select(iSocketArray *read, iSocketArray *write) const = 0;

  /**
   * Get the amount of sockets that can be selected for reading and writing
   * respectively in a single call.
   */
  virtual size_t GetSelectLimit() const = 0;
};

} // Socket
} // Network
} // CS

#endif // __INETWORK_SOCKET_H__
