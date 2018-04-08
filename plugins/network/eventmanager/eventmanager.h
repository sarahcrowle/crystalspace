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

#ifndef __CS_SOCKETMANAGER_H__
#define __CS_SOCKETMANAGER_H__

#include "inetwork/eventmanager.h"
#include "inetwork/socket.h"
#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "iutil/eventq.h"
#include "iutil/event.h"
#include "iutil/objreg.h"

#include "csutil/dirtyaccessarray.h"
#include "csutil/hashcomputer.h"
#include "csutil/refcount.h"
#include "csutil/set.h"
#include "csutil/scopedpointer.h"

using namespace CS::Network::Socket;
using namespace CS::Network::EventManager;
using CS::Utility::ScopedArrayPointer;

template<>
class csHashComputer<csRef<iAddress> >
{
public:
  static uint ComputeHash(csRef<iAddress> const &key)
  {
    return key->GetHash();
  }
};

CS_PLUGIN_NAMESPACE_BEGIN(EventManager)
{
  class Buffer : public csRefCount, public ScopedArrayPointer<char>
  {
  private:
    size_t size;

  public:
    Buffer(char *buffer, size_t size) :
      csRefCount(),
      ScopedArrayPointer<char>(buffer),
      size(size)
    {
    }

    size_t GetSize() const
    {
      return size;
    }
  };

  struct PendingPacket
  {
    csRef<Buffer> buffer;
    size_t processed;
  };

  struct ClientData
  {
    iAddress *address;
    PendingPacket *incoming;
    csArray<PendingPacket *> outgoing;
  };

  struct ServerData
  {
    // @@@TODO
  };

  class EventManager : public scfImplementation4<EventManager, iEventManager, iComponent, iEventHandler, iEventPlug>
  {
  private:
    typedef csHash<iAddress *, csRef<iSocket> > StreamHash;
    typedef csHash<ServerData, csRef<iSocket> > DatagramHash;
    typedef csHash<ClientData, csRef<iSocket> > ClientHash;
    typedef csHash<iSocket *, csRef<iAddress> > SocketHash;

    // main object registry
    iObjectRegistry* objectRegistry;

    // the queue we belong to
    csRef<iEventQueue> queue;

    // our outlet to post events
    csRef<iEventOutlet> outlet;

    // event name registry for name lookups
    csRef<iEventNameRegistry> nameRegistry;

    // callback if there is any
    csRef<iEventCallback> callback;

    // our socket manager
    csRef<iSocketManager> socketManager;

    // our clients
    ClientHash clients;

    // our stream servers
    StreamHash streamServers;

    // our datagram servers
    DatagramHash datagramServers;

    // address lookup table
    SocketHash lookup;

    // hash that holds pending requests
    csHash<csArray<PendingPacket *>, iSocket *> pending;

    // allowed events coming from upstream
    csSet<csEventID> remoteEvents;

    // event during which we perform processing
    csEventID processType;

    // enqueue a packet to be send to the client associated with data
    void SendPacket(Buffer *buffer, iSocket *socket, ClientData &data);

    // create an event from a buffer originating from the specified source
    void CreateEvent(char *buffer, size_t size, iAddress *source);

  public:
    EventManager(iBase *parent) : scfImplementationType(this, parent), processType(CS_EVENT_INVALID)
    {
    }

    virtual ~EventManager();

    // iComponent
    bool Initialize(iObjectRegistry *registry);

    // iEventManager
    iSocketManager *GetSocketManager() const
    {
      return socketManager;
    }

    void Process();
    void SetProcessEvent(csEventID processType);

    bool Subscribe(csEventID type, Source source);
    bool Subscribe(csEventID const types[], Source source);
    void Unsubscribe(csEventID type, Source source);
    void Unsubscribe(csEventID const types[], Source source);

    void Send(iEvent *e);

    void SetCallback(iEventCallback *callback);

    ConnectionID Connect(iAddress *address, Protocol p);
    void Disconnect(ConnectionID);

    ConnectionID Listen(iAddress *address, Protocol p);

    // iEventHandler
    bool HandleEvent(iEvent &e);

    // GenericName and GenericID
    CS_EVENTHANDLER_NAMES("Network Event Manager");

    // Generic/Instance Prc/Succ
    CS_EVENTHANDLER_NIL_CONSTRAINTS;

    // iEventPlug
    void EnableEvents(unsigned, bool)
    {
      return;
    }

    unsigned GetPotentiallyConflictingEvents()
    {
      return 0;
    }

    unsigned QueryEventPriority(unsigned iType)
    {
      return 0;
    }
  };
}
CS_PLUGIN_NAMESPACE_END(Socket)

#endif // __CS_SOCKETMANAGER_H__
