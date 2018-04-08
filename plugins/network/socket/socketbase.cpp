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

#include "socketbase.h"

CS_PLUGIN_NAMESPACE_BEGIN(Socket)
{
  namespace Platform
  {
#   ifdef CS_PLATFORM_WIN32
    extern Socket const invalidSocket = INVALID_SOCKET;

    // initialize winsocks version 2.2
    bool Initialize()
    {
      WSADATA wsaData;
      WORD wVers = MAKEWORD(2, 2);
      return WSAStartup(wVers, &wsaData) == 0;
    }

    // clean up winsocks
    void Cleanup()
    {
      WSACleanup();
    }

    // get a description for the last error that occured
    char *GetError()
    {
      int error = WSAGetLastError();
      char *message;

      // get standard message from windows
      FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		      nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message, 0, nullptr);

      // return the message
      return message;
    }

    // close a socket
    void Close(Socket s)
    {
      closesocket(s);
    }

    // IP printable to network conversion
    int InetPtoN(int family, char const *source, void *destination)
    {
      if(family == AF_INET)
      {
	int size = sizeof(sockaddr_in);
	return WSAStringToAddressA(const_cast<char *>(source), family, nullptr, reinterpret_cast<sockaddr *>(destination), &size);
      }
      else if(family == AF_INET6)
      {
	int size = sizeof(sockaddr_in6);
	return WSAStringToAddressA(const_cast<char *>(source), family, nullptr, reinterpret_cast<sockaddr *>(destination), &size);
      }
      else
      {
	return -1;
      }
    }

    // IP network to printable conversion
    char const *InetNtoP(int family, void const *source, char *destination, socklen_t destinationSize)
    {
      if(family == AF_INET)
      {
	DWORD size = destinationSize;
	if(WSAAddressToStringA(reinterpret_cast<sockaddr *>(const_cast<void *>(source)), sizeof(sockaddr_in), nullptr, destination, &size) == 0)
	{
	  return destination;
	}
	else
	{
	  return nullptr;
	}
      }
      else if(family == AF_INET6)
      {
	DWORD size = destinationSize;
	if(WSAAddressToStringA(reinterpret_cast<sockaddr *>(const_cast<void *>(source)), sizeof(sockaddr_in), nullptr, destination, &size) == 0)
	{
	  return destination;
	}
	else
	{
	  return nullptr;
	}
      }
      else
      {
	return nullptr;
      }
    }
#   else
    Socket const invalidSocket = -1;

    // no need for initialization on unix
    bool Initialize()
    {
        return true;
    }

    // no need for cleanup on unix
    void Cleanup()
    {
    }

    // get a description for the last error that occured
    char *GetError()
    {
      return strerror(errno);
    }

    // close a socket
    void Close(Socket s)
    {
      close(s);
    }

    // IP printable to network conversion
    int InetPtoN(int family, char const *source, void *destination)
    {
      return inet_pton(family, source, destination);
    }

    // IP network to printable conversion
    char const *InetNtoP(int family, void const *source, char *destination, socklen_t destinationSize)
    {
      return inet_ntop(family, source, destination, destinationSize);
    }
#   endif
  } // namespace Platform
}
CS_PLUGIN_NAMESPACE_END(Socket)
