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

#ifndef __CS_SOCKET_BASE_H__
#define __CS_SOCKET_BASE_H__

#include "cssysdef.h"

#ifdef CS_PLATFORM_WIN32
// only include winsocks, not whole windows.h
# define _INC_WINDOWS
#define _WIN32_WINNT 0x0501		// Needed for mingw compile
// include winsocks
# include <winsock2.h>
# include <ws2tcpip.h>
// remove limited include again
# undef _INC_WINDOWS
#else
// system types
# include <sys/types.h>
// core BSD sockets
# include <sys/socket.h>
// IPPROTO_TCP/UDP
# include <netinet/in.h>
// ip address manipulation
# include <arpa/inet.h>
// name lookup
# include <netdb.h>
#endif

CS_PLUGIN_NAMESPACE_BEGIN(Socket)
{
  namespace Platform
  {
    // platform specific types
#   ifdef CS_PLATFORM_WIN32
      typedef SOCKET Socket;
#   else
      typedef int Socket;
#   endif

    extern Socket const invalidSocket;

    // function to perform one-time setup of the socket library if necessary
    bool Initialize();

    // function to perform cleanup once all socket usage is done if necessary
    void Cleanup();

    // get a description for the last error that occured
    char *GetError();

    // close a socket
    void Close(Socket);

    // IP printable to network conversion
    int InetPtoN(int family, char const *source, void *destination);

    // IP network to printable conversion
    char const *InetNtoP(int family, void const *source, char *destination, socklen_t size);
  } // namespace Platform
}
CS_PLUGIN_NAMESPACE_END(Socket)

#endif // __CS_SOCKET_BASE_H__
