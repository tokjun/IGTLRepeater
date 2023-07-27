#ifndef SESSION_H
#define SESSION_H

#include "igtlSocket.h"
#include "igtlMutexLock.h"
#include "igtlMessageHeader.h"


typedef struct {
  igtl::Socket * fromSocket;
  igtl::Socket * toSocket;
  igtl::MutexLock * fromLock;
  igtl::MutexLock * toLock;
} ThreadData;


int SessionThread(void* ptr);

int ReceiveTransform(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader * header);
int ReceivePosition(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader * header);
int ReceiveImage(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader * header);
int ReceiveStatus(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader * header);


#if OpenIGTLink_PROTOCOL_VERSION >= 2
  int ReceivePoint(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader * header);
  int ReceiveTrajectory(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader::Pointer& header);
  int ReceiveString(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader * header);
  int ReceiveBind(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader * header);
  int ReceiveCapability(igtl::Socket * fromSocket, igtl::Socket * toSocket, igtl::MessageHeader * header);
#endif //OpenIGTLink_PROTOCOL_VERSION >= 2

#endif //
