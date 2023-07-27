/*=========================================================================

  Program:   OpenIGTLink -- Example for Data Receiving Client Program
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

//
// This program relays OpenIGTLink messages eschanged between a client host and a server host.
// The program creates two sockets:
//  - a 'client socket' to connect to the server host
//  - a 'server socket' to wait for connection from the client host
//
//  +------------------+              +------------------+              +------------------+
//  |                  |------------->|                  |------------->|                  |
//  |  Client Host     |              |  Relay Host      |              |  Server Host     |
//  |                  |<-------------|                  |<-------------|                  |
//  +------------------+              +------------------+              +------------------+
//                                  clicent            server
//                                  socket             socket


#include <iostream>
#include <iomanip>
#include <math.h>
#include <cstdlib>
#include <cstring>

#include "session.h"

#include "igtlServerSocket.h"
#include "igtlClientSocket.h"
#include "igtlMultiThreader.h"
#include "igtlOSUtil.h"


int ServerSession(igtl::Socket* socket, char* dest_hostname, int dest_port);

int main(int argc, char* argv[])
{
  //------------------------------------------------------------
  // Parse Arguments

  if (argc != 4) // check number of arguments
    {
    // If not correct, print usage
    std::cerr << " Usage: " << argv[0] << " <dest_hostname> <dest_port> <port>"    << std::endl;
    std::cerr << "    <dest_hostname> : IP or hostname of the destination host"                    << std::endl;
    std::cerr << "    <dest_port>     : Port # of the destination host (18944 in Slicer default)"   << std::endl;
    std::cerr << "    <port>          : Port # of this host (18944 in default)"   << std::endl;
    exit(0);
    }

  char*  dest_hostname = argv[1];
  int    dest_port     = atoi(argv[2]);
  int    port          = atoi(argv[3]);

  // Start a server to wait for connection from the client.
  igtl::ServerSocket::Pointer serverSocket;
  serverSocket = igtl::ServerSocket::New();
  int r = serverSocket->CreateServer(port);

  if (r < 0)
    {
    std::cerr << "Cannot create a server socket." << std::endl;
    exit(0);
    }

  igtl::Socket::Pointer socket;

  while (1)
    {
    //------------------------------------------------------------
    // Waiting for Connection
    socket = serverSocket->WaitForConnection(1000);

    if (socket.IsNotNull()) // if client connected
      {
      ServerSession(socket, dest_hostname, dest_port);
      //------------------------------------------------------------
      // Close connection (The example code never reaches to this section ...)
      socket->CloseSocket();
      }
    }
}


int ServerSession(igtl::Socket* serverSocket, char* dest_hostname, int dest_port)
{
  //------------------------------------------------------------
  // Establish Connection

  igtl::ClientSocket::Pointer clientSocket;
  clientSocket = igtl::ClientSocket::New();
  int r = clientSocket->ConnectToServer(dest_hostname, dest_port);

  if (r != 0)
    {
    std::cerr << "Cannot connect to the server." << std::endl;
    return 0;
    }

  igtl::Session::Pointer sessionUp = igtl::Session::New();
  igtl::Session::Pointer sessionDown = igtl::Session::New();
  igtl::MutexLock::Pointer clientLock = igtl::MutexLock::New();
  igtl::MutexLock::Pointer serverLock = igtl::MutexLock::New();

  sessionUp->SetSockets(clientSocket, serverSocket);
  sessionUp->SetMutexLocks(clientLock, serverLock);

  sessionDown->SetSockets(serverSocket, clientSocket);
  sessionDown->SetMutexLocks(serverLock, clientLock);

  sessionUp->Start();
  sessionDown->Start();

  // Monitor
  while(sessionUp->IsActive() && sessionDown->IsActive())
    {
    std::cerr << "Alive!" << std::endl;
    igtl::Sleep(500);
    }

  //threader->TerminateThread(tdup.threadID);
  //threader->TerminateThread(idup);
  //threader->TerminateThread(tddown.threadID);
  //threader->TerminateThread(iddown);
  //
  sessionUp->Stop();
  sessionDown->Stop();

  //------------------------------------------------------------
  // Close connection (The example code never reaches this section ...)

  clientSocket->CloseSocket();

}


