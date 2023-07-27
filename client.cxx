/*=========================================================================

  Program:   OpenIGTLink -- Example for Data Receiving Client Program
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <iostream>
#include <iomanip>
#include <math.h>
#include <cstdlib>
#include <cstring>

#include "session.h"

#include "igtlClientSocket.h"


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

  //------------------------------------------------------------
  // Establish Connection

  igtl::ClientSocket::Pointer socket;
  socket = igtl::ClientSocket::New();
  int r = socket->ConnectToServer(dest_hostname, dest_port);

  if (r != 0)
    {
    std::cerr << "Cannot connect to the server." << std::endl;
    exit(0);
    }

  while (1)
    {
    Session(socket);
    }

  //------------------------------------------------------------
  // Close connection (The example code never reaches this section ...)

  socket->CloseSocket();

}


