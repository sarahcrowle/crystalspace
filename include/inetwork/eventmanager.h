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

#ifndef __INETWORK_EVENTMANAGER_H__
#define __INETWORK_EVENTMANAGER_H__

#include "cssysdef.h"

#include "inetwork/socket.h"

#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "iutil/eventnames.h"

struct iEvent;

namespace CS
{
namespace Network
{
namespace EventManager
{
using namespace Socket;

typedef void* ConnectionID;
ConnectionID const ConnectionInvalid = static_cast<ConnectionID>(nullptr);

/**
 * Callback class to derive from if you want to receive notifications
 * about various happening during operation of the event manager.
 */
struct iEventCallback : public virtual iBase
{
  SCF_INTERFACE(iEventCallback, 0, 1, 0);

  /**
   * Called each time an event is received from a peer that
   * wasn't subscribed to for remote sources.
   * @param event The event that was received with attribute "peer"
   *              set to the ConnectionID of the remote source.
   */
  virtual void OnUnhandledEvent(iEvent *e) = 0;
  /**
   * Called each time a new remote source is discovered.
   * @param connection The ConnectionID of the new remote source.
   */
  virtual void OnConnect(ConnectionID connection) = 0;
  /**
   * Called each time the connection to a remote source is lost.
   * @param connection The ConnectionID of remote source that was lost.
   */
  virtual void OnDisconnect(ConnectionID connection) = 0;
  /**
   * Called upon destruction of the event manager.
   */
  virtual void OnDestroy() = 0;
};

/**
 * Possible sources for events.
 * LOCAL specifies the application running the event manager.
 * REMOTE specifies remote sources.
 * BIDIRECTIONAL specifies both LOCAL and REMOTE.
 */
enum Source
{
  LOCAL = 1,
  REMOTE = 2,
  BIDIRECTIONAL = 3
};

/**
 * Interface to the network event manager.
 * This class allows you to transmit events from and to remote peers.
 * In order to do so you can either let it subscribe to certain
 * event types which are broadcasted to all known peers unless
 * the "peer" attribute is set which should be a valid ConnectionID
 * for a known peer - in that case the event is only transmitted to
 * the specified peer - or manually send events for which the same
 * rule applies.
 *
 * Additionally you can subscribe to event types which are to be posted
 * to the event queue upon receival and manually handle unsubscribed events
 * via a callback.
 *
 * The event manager can act as client and server or a mixture of both
 * depending on whether you're going to listen (as server) or connect
 * to a remote server (as client).
 */ 
struct iEventManager : public virtual iBase
{
  SCF_INTERFACE(iEventManager, 0, 1, 0);

  /**
   * Get the socket manager associated with the event manager
   * for the purpose of resolving addresses.
   */
  virtual iSocketManager *GetSocketManager() const = 0;

  /**
   * Manually process events. During processing an attempt
   * is made to accept new connections, receive events from
   * remote sources and send previously queued events to be
   * transmitted to remote peers.
   */
  virtual void Process() = 0;

  /**
   * This function allows you to optionally specify an event
   * type which the event manager will subscribe to locally
   * and during whose processing it'll perform event processing.
   * @see Process
   */
  virtual void SetProcessEvent(csEventID processType) = 0;

  /**
   * Convenience function to only subscribe to a single event type.
   */
  virtual bool Subscribe(csEventID type, Source source) = 0;

  /**
   * Subscribe to the specified event types for the given sources.
   * @param types An array of event types which should be subscribed
   *              terminated by CS_EVENTLIST_END.
   * @param source The source for which those event types should be
   *               subscribed. @see Source
   */
  virtual bool Subscribe(csEventID const types[], Source source) = 0;

  /**
   * Convenience function to only unsubscribe from a single event type.
   */
  virtual void Unsubscribe(csEventID type, Source source) = 0;

  /**
   * Unsubscribe from the specified event types for the given sources.
   * @param types An array of event types which should be unsubscribed
   *              terminated by CS_EVENTLIST_END.
   * @param source The source for which those event types should be
   *               subscribed. @see Source
   */
  virtual void Unsubscribe(csEventID const types[], Source source) = 0;

  /**
   * Manually send an event to remote peers.
   */
  virtual void Send(iEvent *e) = 0;

  /**
   * Set the callback which should receive notifications during operation
   * of the event manager.
   */
  virtual void SetCallback(iEventCallback *callback) = 0;

  /**
   * Connect to a remote server running on the specified address
   * using the specified protocol.
   *
   * Addresses are resolved using iSocketManager.
   */
  virtual ConnectionID Connect(iAddress *address, Protocol p) = 0;

  /**
   * Terminate the connection identified by the given id.
   */
  virtual void Disconnect(ConnectionID) = 0;

  /**
   * Listen for remote peers on the given address using the specified
   * protocol.
   */
  virtual ConnectionID Listen(iAddress *address, Protocol p) = 0;
};

} // EventManager
} // Network
} // CS

#endif // __INETWORK_EVENTMANAGER_H__
