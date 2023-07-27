#ifndef SESSION_H
#define SESSION_H

#include "igtlSocket.h"
#include "igtlMessageHeader.h"

int Session(igtl::Socket * socket);

int ReceiveTransform(igtl::Socket * socket, igtl::MessageHeader * header);
int ReceivePosition(igtl::Socket * socket, igtl::MessageHeader * header);
int ReceiveImage(igtl::Socket * socket, igtl::MessageHeader * header);
int ReceiveStatus(igtl::Socket * socket, igtl::MessageHeader * header);


#if OpenIGTLink_PROTOCOL_VERSION >= 2
  int ReceivePoint(igtl::Socket * socket, igtl::MessageHeader * header);
  int ReceiveTrajectory(igtl::Socket * socket, igtl::MessageHeader::Pointer& header);
  int ReceiveString(igtl::Socket * socket, igtl::MessageHeader * header);
  int ReceiveBind(igtl::Socket * socket, igtl::MessageHeader * header);
  int ReceiveCapability(igtl::Socket * socket, igtl::MessageHeader * header);
#endif //OpenIGTLink_PROTOCOL_VERSION >= 2

#endif //
