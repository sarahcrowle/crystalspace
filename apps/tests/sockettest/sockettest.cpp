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

#include "crystalspace.h"
#include "sockettest.h"
#include "csutil/sysfunc.h"
#include "iutil/threadmanager.h"

CS_IMPLEMENT_APPLICATION

using namespace CS::Network::Socket;

bool TCPTest::Initialize(iSocketManager* manager, CS::Network::Socket::Family f)
{
  server = manager->CreateSocket(f, CS_SOCKET_PROTOCOL_TCP);
  client = manager->CreateSocket(f, CS_SOCKET_PROTOCOL_TCP);

  if(!server.IsValid() || !client.IsValid())
  {
    csPrintf("tcp initialization: failed to created IPv4 TCP socket pair\n");
    return false;
  }

  address = manager->Resolve(nullptr, "12345", f, CS_SOCKET_PROTOCOL_TCP);

  if(!address.IsValid())
  {
    csPrintf("tcp initialization: failed to resolve local address\n");
    return false;
  }

  return true;
}

bool TCPTest::PrepareClient()
{
  return true;
}

bool TCPTest::PrepareServer()
{
  if(!server->Bind(address))
  {
    csPrintf("tcp server: failed to bind server: %s\n", server->GetLastError());
    return false;
  }

  if(!server->Listen(1))
  {
    csPrintf("tcp server: failed to listen with server: %s\n", server->GetLastError());
    return false;
  }

  return true;
}

THREADED_CALLABLE_IMPL(TCPTest, TestClient)
{
  if(!client->Connect(address))
  {
    csPrintf("tcp client: failed to connect to server: %s\n", client->GetLastError());
    return false;
  }

  csString sendTest("tcp client send test");
  csString recvTest("tcp server send test");

  if(client->Send(sendTest.GetData(), sendTest.Length()) != sendTest.Length())
  {
    csPrintf("tcp client: failed to send data: %s\n", client->GetLastError());
    client.Invalidate();
    return false;
  }

  CS_ALLOC_STACK_ARRAY(char,recvData,recvTest.Length()+1);
  recvData[recvTest.Length()] = 0;
  {
    size_t toReceive = recvTest.Length();
    char* buffer = recvData;
    while(toReceive > 0)
    {
      size_t received = client->Receive(recvData, recvTest.Length());
      if(received == static_cast<size_t>(-1))
      {
	csPrintf("tcp client: failed to receive data: %s\n", client->GetLastError());
	return false;
      }
      toReceive -= received;
      buffer += received;
    }
  }

  if(recvTest != recvData)
  {
    csPrintf("tcp client: received data (%s) didn't match expected data (%s)\n", recvTest.GetData(), recvData);
    return false;
  }

  client.Invalidate();

  return true;
}

THREADED_CALLABLE_IMPL(TCPTest, TestServer)
{
  serverClient = server->Accept();
  if(!serverClient.IsValid())
  {
    csPrintf("tcp server: failed to accept connection: %s\n", server->GetLastError());
    return false;
  }

  csString sendTest("tcp server send test");
  csString recvTest("tcp client send test");

  if(serverClient->Send(sendTest.GetData(), sendTest.Length()) != sendTest.Length())
  {
    csPrintf("tcp server: failed to send data: %s\n", serverClient->GetLastError());
    client.Invalidate();
    return false;
  }

  CS_ALLOC_STACK_ARRAY(char,recvData,recvTest.Length()+1);
  recvData[recvTest.Length()] = 0;
  {
    size_t toReceive = recvTest.Length();
    char* buffer = recvData;
    while(toReceive > 0)
    {
      size_t received = serverClient->Receive(recvData, recvTest.Length());
      if(received == static_cast<size_t>(-1))
      {
	csPrintf("tcp server: failed to receive data: %s\n", serverClient->GetLastError());
	return false;
      }
      toReceive -= received;
      buffer += received;
    }
  }

  if(recvTest != recvData)
  {
    csPrintf("tcp server: received data (%s) didn't match expected data (%s)\n", recvTest.GetData(), recvData);
    return false;
  }

  serverClient.Invalidate();
  server.Invalidate();

  return true;
}

bool UDPTest::Initialize(iSocketManager* manager, CS::Network::Socket::Family f)
{
  server = manager->CreateSocket(f, CS_SOCKET_PROTOCOL_UDP);
  client = manager->CreateSocket(f, CS_SOCKET_PROTOCOL_UDP);

  if(!server.IsValid() || !client.IsValid())
  {
    csPrintf("udp initialization: failed to created IPv4 TCP socket pair\n");
    return false;
  }

  clientAddress = manager->Resolve(nullptr, "34521", f, CS_SOCKET_PROTOCOL_UDP);
  serverAddress = manager->Resolve(nullptr, "12543", f, CS_SOCKET_PROTOCOL_UDP);

  if(!clientAddress.IsValid() || !serverAddress.IsValid())
  {
    csPrintf("udp initialization: failed to resolve local addresses\n");
    return false;
  }

  return true;
}

bool UDPTest::PrepareClient()
{
  if(!client->Bind(clientAddress))
  {
    csPrintf("udp server: failed to bind server: %s\n", server->GetLastError());
    return false;
  }

  if(!client->Connect(serverAddress))
  {
    csPrintf("udp client: failed to connect to server: %s\n", client->GetLastError());
    return false;
  }

  return true;
}

bool UDPTest::PrepareServer()
{
  if(!server->Bind(serverAddress))
  {
    csPrintf("udp server: failed to bind server: %s\n", server->GetLastError());
    return false;
  }

  return true;
}

THREADED_CALLABLE_IMPL(UDPTest, TestClient)
{
  csString sendTest("udp client send test");
  csString recvTest("udp server send test");

  size_t sent = client->Send(sendTest.GetData(), sendTest.Length());
  if(sent != sendTest.Length())
  {
    csPrintf("udp client: failed to send data(%zu): %s\n", sent, client->GetLastError());
    client.Invalidate();
    return false;
  }

  CS_ALLOC_STACK_ARRAY(char,recvData,recvTest.Length()+1);
  recvData[recvTest.Length()] = 0;
  {
    size_t toReceive = recvTest.Length();
    char* buffer = recvData;
    while(toReceive > 0)
    {
      size_t received = client->Receive(recvData, recvTest.Length());
      if(received == static_cast<size_t>(-1))
      {
	csPrintf("udp client: failed to receive data: %s\n", client->GetLastError());
	return false;
      }
      toReceive -= received;
      buffer += received;
    }
  }

  if(recvTest != recvData)
  {
    csPrintf("udp client: received data (%s) didn't match expected data (%s)\n", recvTest.GetData(), recvData);
    return false;
  }

  client.Invalidate();
  return false;
}

THREADED_CALLABLE_IMPL(UDPTest, TestServer)
{
  csString sendTest("udp server send test");
  csString recvTest("udp client send test");

  size_t sent = server->Send(sendTest.GetData(), sendTest.Length(), clientAddress);
  if(sent != sendTest.Length())
  {
    csPrintf("udp server: failed to send data %zu: %s\n", sent, server->GetLastError());
    client.Invalidate();
    return false;
  }

  CS_ALLOC_STACK_ARRAY(char,recvData,recvTest.Length()+1);
  recvData[recvTest.Length()] = 0;
  {
    size_t toReceive = recvTest.Length();
    char* buffer = recvData;
    while(toReceive > 0)
    {
      size_t received = server->Receive(recvData, recvTest.Length());
      if(received == static_cast<size_t>(-1))
      {
	csPrintf("udp server: failed to receive data: %s\n", server->GetLastError());
	return false;
      }
      toReceive -= received;
      buffer += received;
    }
  }

  if(recvTest != recvData)
  {
    csPrintf("udp server: received data (%s) didn't match expected data (%s)\n", recvTest.GetData(), recvData);
    return false;
  }

  server.Invalidate();
  return false;
}


/*---------------------------------------------------------------------*
 * Main function
 *---------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
  iObjectRegistry* objReg = csInitializer::CreateEnvironment(argc, argv);
  if (!csInitializer::RequestPlugins(objReg,
	CS_REQUEST_VFS,
        CS_REQUEST_REPORTER,
        CS_REQUEST_REPORTERLISTENER,
        CS_REQUEST_PLUGIN("crystalspace.network.socket.manager", iSocketManager),
        CS_REQUEST_END))
  {
    csPrintf("Failed to initialize plugins!\n");
    return -1;
  }

  csRef<iSocketManager> socketManager = csQueryRegistry<iSocketManager>(objReg);
  if(!socketManager.IsValid())
  {
    csPrintf("failed to query socket manager\n");
    return -1;
  }

  csRef<iThreadManager> threadManager = csQueryRegistry<iThreadManager>(objReg);
  if(!threadManager.IsValid())
  {
    csPrintf("failed to query thread manager\n");
    return -1;
  }

  csPrintf("testing IPv4 TCP socket pair\n");
  {
    TCPTest test(objReg);

    if(!test.Initialize(socketManager, CS_SOCKET_FAMILY_IP4))
    {
      return -1;
    }

    if(!test.PrepareClient())
    {
      return -1;
    }

    if(!test.PrepareServer())
    {
      return -1;
    }

    csRefArray<iThreadReturn> returns;
    returns.Push(test.TestClient());
    returns.Push(test.TestServer());

    threadManager->Wait(returns);

    for(size_t i = 0; i < returns.GetSize(); ++i)
    {
      if(!returns[i]->IsFinished() || !returns[i]->WasSuccessful())
      {
	return -1;
      }
    }
  }

  csPrintf("testing IPv6 TCP socket pair\n");
  {
    TCPTest test(objReg);

    if(!test.Initialize(socketManager, CS_SOCKET_FAMILY_IP6))
    {
      return -1;
    }

    if(!test.PrepareClient())
    {
      return -1;
    }

    if(!test.PrepareServer())
    {
      return -1;
    }

    csRefArray<iThreadReturn> returns;
    returns.Push(test.TestClient());
    returns.Push(test.TestServer());

    threadManager->Wait(returns);

    for(size_t i = 0; i < returns.GetSize(); ++i)
    {
      if(!returns[i]->IsFinished() || !returns[i]->WasSuccessful())
      {
	return -1;
      }
    }
  }

  csPrintf("testing IPv4 UDP socket pair\n");
  {
    UDPTest test(objReg);

    if(!test.Initialize(socketManager, CS_SOCKET_FAMILY_IP4))
    {
      return -1;
    }

    if(!test.PrepareClient())
    {
      return -1;
    }

    if(!test.PrepareServer())
    {
      return -1;
    }

    csRefArray<iThreadReturn> returns;
    returns.Push(test.TestClient());
    returns.Push(test.TestServer());

    threadManager->Wait(returns);

    for(size_t i = 0; i < returns.GetSize(); ++i)
    {
      if(!returns[i]->IsFinished() || !returns[i]->WasSuccessful())
      {
	return -1;
      }
    }
  }

  csPrintf("testing IPv6 UDP socket pair\n");
  {
    UDPTest test(objReg);

    if(!test.Initialize(socketManager, CS_SOCKET_FAMILY_IP6))
    {
      return -1;
    }

    if(!test.PrepareClient())
    {
      return -1;
    }

    if(!test.PrepareServer())
    {
      return -1;
    }

    csRefArray<iThreadReturn> returns;
    returns.Push(test.TestClient());
    returns.Push(test.TestServer());

    threadManager->Wait(returns);

    for(size_t i = 0; i < returns.GetSize(); ++i)
    {
      if(!returns[i]->IsFinished() || !returns[i]->WasSuccessful())
      {
	return -1;
      }
    }
  }

  socketManager.Invalidate();
  threadManager.Invalidate();

  csInitializer::DestroyApplication(objReg);

  return 0;
}

