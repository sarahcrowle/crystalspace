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
#include "inetwork/socket.h"
#include "csutil/threadmanager.h"

using namespace CS::Network::Socket;

struct TCPTest : public ThreadedCallable<TCPTest>
{
  csRef<iSocket> client;
  csRef<iSocket> server;
  csRef<iSocket> serverClient;
  csRef<iAddress> address;
  iObjectRegistry* objReg;

  TCPTest(iObjectRegistry* objReg) : objReg(objReg)
  {
  }

  iObjectRegistry *GetObjectRegistry() const
  {
    return objReg;
  }

  bool Initialize(iSocketManager* manager, CS::Network::Socket::Family f);

  bool PrepareClient();
  bool PrepareServer();

  THREADED_CALLABLE_DECL(TCPTest, TestClient, csThreadReturn, THREADED, false, false);
  THREADED_CALLABLE_DECL(TCPTest, TestServer, csThreadReturn, THREADED, false, false);
};

struct UDPTest : public ThreadedCallable<UDPTest>
{
  csRef<iSocket> client;
  csRef<iAddress> clientAddress;
  csRef<iSocket> server;
  csRef<iAddress> serverAddress;
  iObjectRegistry* objReg;

  UDPTest(iObjectRegistry* objReg) : objReg(objReg)
  {
  }

  iObjectRegistry *GetObjectRegistry() const
  {
    return objReg;
  }

  bool Initialize(iSocketManager* manager, CS::Network::Socket::Family f);

  bool PrepareClient();
  bool PrepareServer();

  THREADED_CALLABLE_DECL(UDPTest, TestClient, csThreadReturn, THREADED, false, false);
  THREADED_CALLABLE_DECL(UDPTest, TestServer, csThreadReturn, THREADED, false, false);
};
