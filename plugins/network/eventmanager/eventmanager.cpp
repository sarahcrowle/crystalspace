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

#include "cssysdef.h"
#include "eventmanager.h"
#include "csutil/csendian.h"
#include "csutil/cseventflattener.h"
#include "csutil/scfarray.h"

using namespace CS::Network::Socket;
using namespace CS::Network::EventManager;

CS_PLUGIN_NAMESPACE_BEGIN(EventManager)
{
  SCF_IMPLEMENT_FACTORY(EventManager);

  EventManager::~EventManager()
  {
    // unregister us as event listener
    queue->RemoveListener(this);

    // if we have a callback, let it know we're shutting down
    if(callback.IsValid())
    {
      callback->OnDestroy();
    }
  }

  bool EventManager::Initialize(iObjectRegistry *registry)
  {
    // keep a pointer to the registry
    objectRegistry = registry;

    // get event queue
    queue = csQueryRegistry<iEventQueue>(registry);

    // we need an event queue
    if(!queue.IsValid())
    {
      return false;
    }

    // create event outlet for us
    outlet = queue->CreateEventOutlet(this);

    // we cannot post events without an outlet
    if(!outlet.IsValid())
    {
      return false;
    }

    nameRegistry = csQueryRegistry<iEventNameRegistry>(registry);

    // we cannot lookup event relations without a name registry
    if(!nameRegistry.IsValid())
    {
      return false;
    }

    // register us as event listener
    queue->RegisterListener(this);

    // get socket manager
    socketManager = csQueryRegistry<iSocketManager>(registry);

    // networking without sockets... not really
    if(!socketManager.IsValid())
    {
      return false;
    }

    return true;
  }

  void EventManager::CreateEvent(char *buffer, size_t size, iAddress *source)
  {
    // create an event from our outlet
    csRef<iEvent> e = outlet->CreateEvent();

    // try to deserialize event from buffer
    if(csEventFlattener::Unflatten(objectRegistry, e, buffer, size) != csEventFlattenerErrorNone)
    {
      // failed to deserialize the event - bail
      return;
    }

    // do not allow remote sources to set the peer attribute
    e->Remove("peer");

    // set peer for this event
    if(!e->Add("peer", static_cast<ConnectionID>(source)))
    {
      // couldn't set peer - bail
      return;
    }

    // check whether this event belongs to a subcribed type
    for(csEventID id = e->Name; id != CS_EVENT_INVALID; id = nameRegistry->GetParentID(id))
    {
      if(remoteEvents.Contains(id))
      {
        // yes, post it
        outlet->Post(e);
      }
    }

    if(callback.IsValid())
    {
      // we aren't supposed to post those
      // let the callback know about it, anyway
      callback->OnUnhandledEvent(e);
    }
  }

  void EventManager::Process()
  {
    // check for pending connections
    {
      // get all stream servers for reading
      csArray<iSocket *> sockets(streamServers.GetSize());
      {
	StreamHash::GlobalIterator it = streamServers.GetIterator();
	while(it.HasNext())
	{
	  csRef<iSocket> socket;
	  it.Next(socket);

	  sockets.Push(socket);
	}
      }

      // select
      {
	// empty array for writing
	scfArray<iSocketArray> empty;

	// prepared sockets array for reading
	scfArrayWrap<iSocketArray, csArray<iSocket *> > socketsWrap(sockets);

	// execute
	socketManager->Select(&socketsWrap, &empty);
      }

      // accept connections
      for(size_t i = 0; i < sockets.GetSize(); ++i)
      {
	// accept pending connection
	csRef<iAddress> address;
	csRef<iSocket> socket = sockets[i]->Accept(&address);

	// put into our client hash
	ClientData &data = clients.GetOrCreate(socket);
	data.address = address;
        data.incoming = nullptr;

	// add to lookup table
	lookup.Put(address, socket);

	// let callback know we have a new client if there is any
	if(callback.IsValid())
	{
	  callback->OnConnect(static_cast<ConnectionID>(address));
	}
      }
    }

    // check for incoming packets on datagram servers
    {
      // @@@TODO
    }

    // try to finish pending outgoing requests and check for incoming packets
    {
      // get arrays for select
      csArray<iSocket *> read;
      csArray<iSocket *> write;

      // go over all clients and:
      // - add all clients to read to check for incoming packets
      // - add those with pending outgoing packets to the write set
      ClientHash::GlobalIterator it = clients.GetIterator();
      while(it.HasNext())
      {
	// get socket and client data
	csRef<iSocket> socket;
	ClientData &data = it.Next(socket);

	// add it to the read queue
	read.Push(socket);

	// add it to the write queue if it has pending packets
	if(!data.outgoing.IsEmpty())
	{
	  write.Push(socket);
	}
      }

      // select
      {
	scfArrayWrap<iSocketArray, csArray<iSocket*> > readWrap(read);
	scfArrayWrap<iSocketArray, csArray<iSocket*> > writeWrap(write);

	socketManager->Select(&readWrap, &writeWrap);
      }

      // reset iterator
      it.Reset();

      // go over all clients again and process them if they're ready
      // keep track which is the next to check for reading and writing
      size_t readCurr = 0;
      size_t writeCurr = 0;

      // temporary buffer for reading
      // @@@NOTE: this size may be quite excessive - it's the theoretical
      //          maximum MTU (not taking jumboframes into account)
      //          pratically a far smaller buffer would probably do
      size_t const bufferSize = 0xFFF;
      char *buffer = new char[bufferSize];
      while(it.HasNext())
      {
	csRef<iSocket> socket;
	ClientData &data = it.Next(socket);

	// check whether this socket is ready for reading
	if(readCurr < read.GetSize() && read[readCurr]  == socket)
	{
	  // it is, process it
	  ++readCurr;

	  // receive data
	  size_t received;
	  received = socket->Receive(buffer, bufferSize);

	  // check for error
          if(received == static_cast<size_t>(-1))
          {
            // read error - disconnect client and continue
            Disconnect(data.address);
            continue;
          }
	  if(received > 0)
	  {
	    // no error, continue
	    size_t processed = 0;
	    size_t left = received;

	    // try to finish pending request
	    if(data.incoming)
	    {
	      // datagrams are always transmitted as a whole
	      // so we cannot have a partially complete udp request
	      CS_ASSERT(socket->GetProtocol() != CS_SOCKET_PROTOCOL_UDP);

	      // get request
	      PendingPacket *&request = data.incoming;

	      // get it's current buffer position
	      char *reqBuffer = *request->buffer + request->processed;

	      // get the amount missing in this request
	      size_t missing = request->buffer->GetSize() - request->processed;

	      // get the amount we can process
	      size_t toProcess = csMin(missing, left);

	      // process it
	      memcpy(reqBuffer, buffer + processed, toProcess);
	      left -= toProcess;
	      processed += toProcess;

	      // check whether this request is done now
	      if(missing == toProcess)
	      {
		// create event
                CreateEvent(*request->buffer, request->buffer->GetSize(), data.address);

		// we're done with the request
		delete request;
		request = nullptr;
	      }
	      else
	      {
		// let the request know we did something
		request->processed += toProcess;
	      }
	    }

	    // process what's left
	    while(left > 0)
	    {
	      // read meta-data
	      uint16 size = csLittleEndian::Convert(*reinterpret_cast<uint16 *>(buffer + processed));
	      processed += sizeof(uint16);
	      left -= sizeof(uint16);

	      // check whether we have enough data left
	      if(size > left)
	      {
		// this mustn't happen for datagram sockets
		CS_ASSERT(socket->GetProtocol() != CS_SOCKET_PROTOCOL_UDP);

		// no, create a pending request
		PendingPacket *request = new PendingPacket;
		request->buffer.AttachNew(new Buffer(new char[size], size));

		// read what we can
		memcpy(*request->buffer, buffer + processed, left);
		request->processed = left;
		left = 0;

                // set as pending request
                data.incoming = request;
	      }
	      else
	      {
		// create event
		CreateEvent(buffer + processed, size, data.address);

		// update processed/left count
		processed += size;
		left -= size;
	      }
	    }
	  }
	}

	// check whether this socket is ready for writing
	if(writeCurr < write.GetSize() && write[writeCurr] == socket)
	{
	  // it is, process it
	  ++writeCurr;

	  while(!data.outgoing.IsEmpty())
	  {
	    // get request
	    PendingPacket *request = data.outgoing[0];

	    // get buffer and amount to send
	    char *reqBuffer = *request->buffer + request->processed;
	    size_t missing = request->buffer->GetSize() - request->processed;

	    // transmit data
	    size_t sent = socket->Send(reqBuffer, missing);

            // check for error
            if(sent == static_cast<size_t>(-1))
            {
              // write error - disconnect client and continue
              Disconnect(data.address);
              continue;
            }
	    // check whether we could send all
	    if(sent == missing)
	    {
	      // yes, we're done with the request and can continue with the next
	      // @@@NOTE: this assumes that we can write more if we could write everything
	      //          which may not be perfectly true
	      delete request;
	      data.outgoing.DeleteIndex(0);
	    }
	    else
	    {
	      // datagrams are always transmitted as a whole
	      // so we cannot have a partially complete udp request
	      CS_ASSERT(socket->GetProtocol() != CS_SOCKET_PROTOCOL_UDP);

	      // we couldn't write everything, so we stop here
	      request->processed += sent;
	      break;
	    }
	  }
	}
      }
      delete [] buffer;
    }
  }

  void EventManager::SetProcessEvent(csEventID type)
  {
    // unsubscribe from current process type
    queue->Unsubscribe(this, processType);

    // subscribe to new type
    queue->Subscribe(this, type);

    // set it as our processing type
    processType = type;
  }

  bool EventManager::Subscribe(csEventID type, Source source)
  {
    bool success = true;

    // check whether this type should be added as local source
    if(source & LOCAL)
    {
      // subscribe to it
      success = success && queue->Subscribe(this, type);
    }

    // check whether this type should be added as remote source
    if(source & REMOTE)
    {
      // add it to our set of accepted remote types
      remoteEvents.Add(type);
    }

    return success;
  }

  bool EventManager::Subscribe(csEventID const types[], Source source)
  {
    bool success = true;

    // check whether the specified types should be added as local source
    if(source & LOCAL)
    {
      // subscribe to those types with our queue
      success = success && queue->Subscribe(this, types);
    }

    // check whether the specified types should be added as remote source
    if(source & REMOTE)
    {
      // add them to our set of accepted types
      for(int i = 0; types[i] != CS_EVENTLIST_END; ++i)
      {
	remoteEvents.Add(types[i]);
      }
    }

    return success;
  }
  void EventManager::Unsubscribe(csEventID type, Source source)
  {
    // check whether we want to remove this type as local source
    // and it's not our process event
    if(source & LOCAL && type != processType)
    {
      // unsubscribe from it
      queue->Unsubscribe(this, type);
    }

    // check whether we want to remove this type as remote source
    if(source & REMOTE)
    {
      // remove it from our set
      remoteEvents.Delete(type);
    }
  }

  void EventManager::Unsubscribe(csEventID const types[], Source source)
  {
    // check whether we want to remove those types as local source
    if(source & LOCAL)
    {
      // unsubscribe from them
      queue->Unsubscribe(this, types);

      // check whether our process type is part of them
      for(int i = 0; types[i] != CS_EVENTLIST_END; ++i)
      {
	if(types[i] == processType)
	{
	  // it is, resubscribe to it
	  queue->Subscribe(this, processType);
	}
      }
    }

    // check whether we want to remove those types as remote source
    if(source & REMOTE)
    {
      // remove them from our set
      for(int i = 0; types[i] != CS_EVENTLIST_END; ++i)
      {
	remoteEvents.Delete(types[i]);
      }
    }
  }

  void EventManager::SendPacket(Buffer *buffer, iSocket *socket, ClientData &data)
  {
    // create a PendingPacket to be pushed on our queue
    PendingPacket *request = new PendingPacket;
    request->buffer = buffer;
    request->processed = 0;

    // add it to the queue
    data.outgoing.Push(request);
  }

  void EventManager::Send(iEvent *e)
  {
    // placeholder to hold target if there is any
    ConnectionID id;
    switch(e->Retrieve("peer", id))
    {
    case csEventErrNotFound:
      // event set as broadcast, no error
      // set id to invalid to indicate broadcast
      id = ConnectionInvalid;
      break;

    case csEventErrNone:
      // we got a client id - no error
      break;

    default:
      // attribute invalid, bail
      return;
    }

    // remove peer attribute if it exists
    e->Remove("peer");

    // keep track of flatten errors
    csEventFlattenerError error;

    // query required buffer size
    size_t eventSize;
    error = csEventFlattener::FlattenSize(objectRegistry, e, eventSize);

    // check whether an error occured
    if(error != csEventFlattenerErrorNone)
    {
      // we can't flatten this for some reason, bail
      return;
    }

    // calculate actual packet size (event size + metadata)
    size_t metaSize = sizeof(uint16);
    size_t packetSize = eventSize + metaSize;

    // get buffer to hold flattened event preceeded by it's size
    csRef<Buffer> buffer;
    buffer.AttachNew(new Buffer(new char[packetSize], packetSize));

    // set meta-data
    *reinterpret_cast<uint16 *>(static_cast<char *>(*buffer)) = csLittleEndian::Convert(static_cast<uint16>(eventSize)); // event size

    // flatten it
    error = csEventFlattener::Flatten(objectRegistry, e, *buffer + metaSize);

    // check for error
    if(error != csEventFlattenerErrorNone)
    {
      // failed to flatten - bail
      return;
    }

    // check whether this should go to a single target or should be broadcasted
    if(id != ConnectionInvalid)
    {
      // convert the id to the address pointer it is
      iAddress *address = static_cast<iAddress *>(id);

      // check whether we know about it
      if(!lookup.Contains(address))
      {
	// unknown address - bail
	return;
      }

      // get the socket associated with this address
      iSocket *socket = *lookup[address];

      // check whether it has an associated client
      if(!clients.Contains(socket))
      {
	// @@@TODO: handle connectionless server
	return;
      }

      // send this over the wire
      SendPacket(buffer, socket, *clients[socket]);
    }
    else
    {
      // send this to all clients
      ClientHash::GlobalIterator it = clients.GetIterator();
      while(it.HasNext())
      {
	// get socket an address
	csRef<iSocket> socket;
	ClientData &data = it.Next(socket);

	// send packet
	SendPacket(buffer, socket, data);
      }
    }
  }

  bool EventManager::HandleEvent(iEvent &e)
  {
    if(e.Name == processType)
    {
      Process();
    }
    else
    {
      Send(&e);
    }

    return true;
  }

  void EventManager::SetCallback(iEventCallback *c)
  {
    callback = c;
  }

  ConnectionID EventManager::Connect(iAddress *address, Protocol p)
  {
    // create a suitable socket
    csRef<iSocket> socket = socketManager->CreateSocket(address->GetFamily(), p);

    // check whether we could create it
    if(!socket.IsValid())
    {
      return ConnectionInvalid;
    }

    // connect to the target
    if(!socket->Connect(address))
    {
      return ConnectionInvalid;
    }

    // add socket to our client list and create packet queues
    ClientData &data = clients.GetOrCreate(socket);
    data.address = address;
    data.incoming = nullptr;

    // add socket to our address lookup table
    lookup.Put(address, socket);

    // create a connection id
    ConnectionID id = static_cast<ConnectionID>(address);

    // let the callback know we got a new client if there is any
    if(callback.IsValid())
    {
      callback->OnConnect(id);
    }

    return id;
  }

  void EventManager::Disconnect(ConnectionID id)
  {
    // cast id to the address pointer it is
    iAddress *address = static_cast<iAddress *>(id);

    // check whether we know it
    if(!lookup.Contains(address))
    {
      // nope - bail
      return;
    }

    // get the associated socket
    iSocket *socket = *lookup[address];

    // remove it as server/client resp.
    clients.DeleteAll(socket);
    if(socket->GetProtocol() == CS_SOCKET_PROTOCOL_TCP)
    {
      streamServers.DeleteAll(socket);
    }
    else
    {
      // @@@TODO
      datagramServers.DeleteAll(socket);
    }

    // remove it from lookup table
    lookup.DeleteAll(address);
  }

  ConnectionID EventManager::Listen(iAddress *address, Protocol p)
  {
    // create a suitable socket
    csRef<iSocket> socket = socketManager->CreateSocket(address->GetFamily(), p);

    // ensure it succeeded
    if(!socket.IsValid())
    {
      // couldn't create socket
      return ConnectionInvalid;
    }

    // try to bind it to the given address
    if(!socket->Bind(address))
    {
      // bind failed
      return ConnectionInvalid;
    }

    // try to listen for incoming connections
    // this is only necessary for stream servers as
    // datagram servers are connectionless
    if(p == CS_SOCKET_PROTOCOL_TCP && !socket->Listen(20))
    {
      // listen failed
      return ConnectionInvalid;
    }

    // put the socket in our server list
    if(p == CS_SOCKET_PROTOCOL_TCP)
    {
      streamServers.Put(socket, address);
    }
    else
    {
      (void) datagramServers.GetOrCreate(socket);
    }

    // add the address to our lookup table
    lookup.Put(address, socket);

    // create a connection id
    ConnectionID id = static_cast<ConnectionID>(address);

    // let the callback know we got a new server if there is any
    if(callback.IsValid())
    {
      callback->OnConnect(id);
    }

    return id;
  }
}
CS_PLUGIN_NAMESPACE_END(Socket)
