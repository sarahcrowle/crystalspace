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

#include "socket.h"
#include "iutil/comp.h"

using namespace CS::Network::Socket;

CS_PLUGIN_NAMESPACE_BEGIN(Socket)
{
  class SocketManager : public scfImplementation2<SocketManager, iSocketManager, iComponent>
  {
  private:
    Platform::Socket GetHandle(iSocket* socket) const;

  public:
    SocketManager(iBase*);
    virtual ~SocketManager();
    bool Initialize(iObjectRegistry *registry)
    {
      return true;
    }
    csPtr<iSocket> CreateSocket(Family family, Protocol protocol) const;
    csPtr<iAddress> Resolve(char const *host, char const *service, Family family, Protocol protocol) const;
    void Select(iSocketArray *read, iSocketArray *write) const;
    size_t GetSelectLimit() const;
  };
}
CS_PLUGIN_NAMESPACE_END(Socket)

#endif // __CS_SOCKETMANAGER_H__