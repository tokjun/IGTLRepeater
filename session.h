#ifndef SESSION_H
#define SESSION_H

#include "igtlSocket.h"
#include "igtlMultiThreader.h"
#include "igtlMutexLock.h"
#include "igtlMessageHeader.h"
#include "logger.h"

namespace igtl
{

class IGTLCommon_EXPORT Session : public Object
{
public:

  igtlTypeMacro(igtl::Session, igtl::Object)
  igtlNewMacro(igtl::Session);

public:

  virtual const char * GetClassName() { return "Session"; };

  int Start();
  void Stop() { this->Active = 0; };

  inline int     IsActive()    { return this->Active; }

  void SetSockets(igtl::Socket * from, igtl::Socket * to)
  {
    this->fromSocket = from;
    this->toSocket   = to;
  };

  void SetMutexLocks(igtl::MutexLock * from, igtl::MutexLock * to)
  {
    this->fromLock = from;
    this->toLock   = to;
  };

  void SetLogger(igtl::Logger * logger)
  {
    this->logger = logger;
  };

  void SetName(const char * name)
  {
    this->Name = name;
  };

  void SetBlackList(std::vector<std::string> & blist)
  {
    this->blackList = blist;
  };


  static void    MonitorThreadFunction(void * ptr);

protected:

  Session();
  ~Session();

  void           PrintSelf(std::ostream& os) const;

  virtual int    Process();

  int ReceiveTransform(igtl::MessageHeader * header);
  int ReceivePosition(igtl::MessageHeader * header);
  int ReceiveImage(igtl::MessageHeader * header);
  int ReceiveStatus(igtl::MessageHeader * header);

#if OpenIGTLink_PROTOCOL_VERSION >= 2
  int ReceivePoint(igtl::MessageHeader * header);
  int ReceiveTrajectory(igtl::MessageHeader::Pointer& header);
  int ReceiveString(igtl::MessageHeader * header);
  int ReceiveBind(igtl::MessageHeader * header);
  int ReceiveCapability(igtl::MessageHeader * header);
  int ReceiveTrackingData(igtl::MessageHeader::Pointer& header);
#endif //OpenIGTLink_PROTOCOL_VERSION >= 2


protected:

  int            Active;  // 0: Not active; 1: Active; -1: exiting

  std::string    Name;

  igtl::MultiThreader::Pointer Threader;

  igtl::Socket * fromSocket;
  igtl::Socket * toSocket;
  igtl::MutexLock * fromLock;
  igtl::MutexLock * toLock;

  std::vector<std::string> blackList;

  igtl::Logger::Pointer logger;

  int id;
  int nThread;

};

}



#endif //
