#include <iostream>
#include <iomanip>
#include <math.h>
#include <cstdlib>
#include <cstring>


#include "session.h"

#include "igtlMultiThreader.h"
#include "igtlOSUtil.h"
#include "igtlTimeStamp.h"

#include "igtlMessageHeader.h"
#include "igtlTransformMessage.h"
#include "igtlPositionMessage.h"
#include "igtlImageMessage.h"
#include "igtlClientSocket.h"
#include "igtlStatusMessage.h"

#if OpenIGTLink_PROTOCOL_VERSION >= 2
#include "igtlBindMessage.h"
#include "igtlSensorMessage.h"
#include "igtlPointMessage.h"
#include "igtlTrajectoryMessage.h"
#include "igtlStringMessage.h"
#include "igtlTrackingDataMessage.h"
#include "igtlQuaternionTrackingDataMessage.h"
#include "igtlCapabilityMessage.h"
#include "igtlTrackingDataMessage.h"
#endif // OpenIGTLink_PROTOCOL_VERSION >= 2


namespace igtl
{

//-----------------------------------------------------------------------------
Session::Session()
{
  this->Active   = 0;
  this->Threader = igtl::MultiThreader::New();

  this->fromSocket = NULL;
  this->toSocket = NULL;
  this->fromLock = NULL;
  this->toLock = NULL;

  this->logger = NULL;
}

//-----------------------------------------------------------------------------
Session::~Session()
{
}

//-----------------------------------------------------------------------------
void Session::PrintSelf(std::ostream& os) const
{
  this->Superclass::PrintSelf(os);
}

int Session::Start()
{
  if (!this->fromSocket)
    {
    std::cerr << "ERROR: no 'from' socket" << std::endl;
    return 0;
    }

  if (!this->toSocket)
    {
    std::cerr << "ERROR: no 'to' socket" << std::endl;
    return 0;
    }

  if (!this->fromLock)
    {
    std::cerr << "ERROR: no 'from' lock" << std::endl;
    return 0;
    }

  if (!this->toLock)
    {
    std::cerr << "ERROR: no 'to' lock" << std::endl;
    return 0;
    }

  if (!this->logger)
    {
    std::cerr << "ERROR: no logger" << std::endl;
    return 0;
    }

  if (this->Active == 0)
    {
    this->Active = 1;
    this->Threader->SpawnThread((igtl::ThreadFunctionType) &Session::MonitorThreadFunction, this);
    return 1;
    }
  else
    {
    std::cerr << "ERROR: the thread is already running" << std::endl;
    return 0;
    }
}


//-----------------------------------------------------------------------------
void Session::MonitorThreadFunction(void * ptr)
{

  igtl::MultiThreader::ThreadInfo* info =
    static_cast<igtl::MultiThreader::ThreadInfo*>(ptr);

  Session * con = static_cast<Session *>(info->UserData);
  con->id      = info->ThreadID;
  con->nThread = info->NumberOfThreads;

  std::cerr << "Starting Thread #" << con->id << std::endl;
  std::cerr << "MonitorThreadFunction() : Starting a thread..." << std::endl;

  while (con->Active)
    {
    int r;
    r = con->Process();
    if (r == 1)
      {
      std::cerr << "Connection closed by the 'from' host." << std::endl;
      con->Active = 0;
      }
    else if (r == 2)
      {
      std::cerr << "Connection closed by the 'to' host." << std::endl;
      con->Active = 0;
      }
    }

  con->Active = 0;
}


int Session::Process()
{
  // Return 0: Normal
  // Return 1: Closed by the 'from' host
  // Return 2: Closed by the 'to' host
  // Return 3: Size error

  // Create a message buffer to receive header
  igtl::MessageHeader::Pointer headerMsg;
  headerMsg = igtl::MessageHeader::New();

  //------------------------------------------------------------
  // Allocate a time stamp
  igtl::TimeStamp::Pointer tsMsg;
  tsMsg = igtl::TimeStamp::New();

  igtl::TimeStamp::Pointer tsSys;
  tsSys = igtl::TimeStamp::New();

  // Initialize receive buffer
  headerMsg->InitPack();

  // Receive generic header from the socket
  bool timeout(false);
  int headerLength = headerMsg->GetPackSize();

  igtlUint64 r = fromSocket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize(), timeout);
  if (r == 0)
    {
    //fromSocket->CloseSocket();
    return 1; // Connection closed by the 'from' socekt
    }

  if (r != headerMsg->GetPackSize())
    {
    return 3;
    }

  unsigned char dummy[256];
  if (headerLength < 256)
    {
    memcpy(dummy, headerMsg->GetPackPointer(), headerLength);
    }
  else
    {
    std::cerr << "Large header" << std::endl;
    }

  // Deserialize the header
  headerMsg->Unpack();

  // Get time stamp
  igtlUint32 secMsg;
  igtlUint32 nanosecMsg;
  igtlUint32 secSys;
  igtlUint32 nanosecSys;

  headerMsg->GetTimeStamp(tsMsg);
  tsMsg->GetTimeStamp(&secMsg, &nanosecMsg);

  tsSys->GetTime();
  tsSys->GetTimeStamp(&secSys, &nanosecSys);

  std::stringstream ss;
  ss << this->Name << ", "
     << secSys << "." << std::setw(9) << std::setfill('0') << nanosecSys << ", "
     << secMsg << "." << std::setw(9) << std::setfill('0') << nanosecMsg << ", "
     << headerMsg->GetDeviceName() << ", " << headerMsg->GetDeviceType() << ", ";

  this->logger->Print(ss.str());

  // Check if the message type is blacklisted
  std::vector<std::string>::iterator it;
  for (it = this->blackList.begin(); it != this->blackList.end(); it ++)
    {
    if (strcmp(headerMsg->GetDeviceType(), (*it).c_str()) == 0)
      {
      std::stringstream ss;
      ss << "Blocked message type detected: " << headerMsg->GetDeviceType() << std::endl;
      igtlUint64 remain = headerMsg->GetBodySizeToRead();
      ss << "  Body size = " << remain << std::endl;
      igtlUint64 block  = 256;
      igtlUint64 n = 0;
      do
        {
        if (remain < block)
          {
          block = remain;
          }

        bool timeout(false);
        n = fromSocket->Receive(dummy, block, timeout, 0);
        if (n <= 0)
          {
          break;
          }
        remain -= n;
        }
      while (remain > 0);
      return 0;
      }
    }

  // Check data type and receive data body
  if (strcmp(headerMsg->GetDeviceType(), "TRANSFORM") == 0)
    {
    ReceiveTransform(headerMsg);
    }
  else if (strcmp(headerMsg->GetDeviceType(), "POSITION") == 0)
    {
    ReceivePosition(headerMsg);
    }
  else if (strcmp(headerMsg->GetDeviceType(), "IMAGE") == 0)
    {
    ReceiveImage(headerMsg);
    }
  else if (strcmp(headerMsg->GetDeviceType(), "STATUS") == 0)
    {
    ReceiveStatus(headerMsg);
    }
#if OpenIGTLink_PROTOCOL_VERSION >= 2
  else if (strcmp(headerMsg->GetDeviceType(), "POINT") == 0)
    {
    ReceivePoint(headerMsg);
    }
  else if (strcmp(headerMsg->GetDeviceType(), "TRAJ") == 0)
    {
    ReceiveTrajectory(headerMsg);
    }
  else if (strcmp(headerMsg->GetDeviceType(), "STRING") == 0)
    {
    ReceiveString(headerMsg);
    }
  else if (strcmp(headerMsg->GetDeviceType(), "BIND") == 0)
    {
    ReceiveBind(headerMsg);
    }
  else if (strcmp(headerMsg->GetDeviceType(), "CAPABILITY") == 0)
    {
    ReceiveCapability(headerMsg);
    }
  else if (strcmp(headerMsg->GetDeviceType(), "TDATA") == 0)
    {
    ReceiveTrackingData(headerMsg);
    }
#endif //OpenIGTLink_PROTOCOL_VERSION >= 2
  else
    {
    // if the data type is unknown, skip reading.
    //std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
    //std::cerr << "Size : " << headerMsg->GetBodySizeToRead() << std::endl;
    //
    toSocket->Send(dummy, headerLength);

    //fromSocket->Skip(headerMsg->GetBodySizeToRead(), 0);
    igtlUint64 remain = headerMsg->GetBodySizeToRead();
    igtlUint64 block  = 256;
    igtlUint64 n = 0;

    std::cerr << "Unrecognized data type: " << headerMsg->GetDeviceType() << std::endl;
    std::cerr << "Size: " << remain << std::endl;
    std::cerr << "Content: " << std::endl;
    for (int i = 0; i < remain; i ++)
      {
      std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)dummy[i] << " ";
      if (i % 16 == 15)
        {
        std::cerr << std::endl;
        }
      }

    do
      {
      if (remain < block)
        {
        block = remain;
        }

      bool timeout(false);
      n = fromSocket->Receive(dummy, block, timeout, 0);
      toSocket->Send(dummy, n);
      if (n <= 0)
        {
        break;
        }
      remain -= n;
      }
    while (remain > 0);

    std::stringstream ss;
    ss << std::endl;
    if (this->logger.IsNotNull())
      {
      this->logger->Print(ss.str());
      }
    }

  return 0;
}


int Session::ReceiveTransform(igtl::MessageHeader * header)
{
  // Create a message buffer to receive transform data
  igtl::TransformMessage::Pointer transMsg;
  transMsg = igtl::TransformMessage::New();
  transMsg->SetMessageHeader(header);
  transMsg->AllocatePack();

  // Receive transform data from the socket
  bool timeout(false);
  this->fromSocket->Receive(transMsg->GetPackBodyPointer(), transMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = transMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    std::stringstream ss;

    // Retrive the transform data
    igtl::Matrix4x4 matrix;
    transMsg->GetMatrix(matrix);
    //igtl::PrintMatrix(matrix);
    ss << "Matrix=("
       << matrix[0][0] << ", " << matrix[1][0] << ", " << matrix[2][0] << ", " << matrix[3][0] << ", "
       << matrix[0][1] << ", " << matrix[1][1] << ", " << matrix[2][1] << ", " << matrix[3][1] << ", "
       << matrix[0][2] << ", " << matrix[1][2] << ", " << matrix[2][2] << ", " << matrix[3][2] << ", "
       << matrix[0][3] << ", " << matrix[1][3] << ", " << matrix[2][3] << ", " << matrix[3][3] << ")" << std::endl;

    this->logger->Print(ss.str());
    transMsg->Pack();
    toSocket->Send(transMsg->GetPackPointer(), transMsg->GetPackSize());

    return 1;
    }


  return 0;

}


int Session::ReceivePosition(igtl::MessageHeader * header)
{
  // Create a message buffer to receive transform data
  igtl::PositionMessage::Pointer positionMsg;
  positionMsg = igtl::PositionMessage::New();
  positionMsg->SetMessageHeader(header);
  positionMsg->AllocatePack();

  // Receive position position data from the socket
  bool timeout(false);
  this->fromSocket->Receive(positionMsg->GetPackBodyPointer(), positionMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = positionMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    // Retrive the transform data
    float position[3];
    float quaternion[4];

    positionMsg->GetPosition(position);
    positionMsg->GetQuaternion(quaternion);

    std::stringstream ss;
    ss << "position=(" << position[0] << ", " << position[1] << ", " << position[2] << "),"
       << "quaternion=(" << quaternion[0] << ", " << quaternion[1] << ", " << quaternion[2] << ", " << quaternion[3] << ")" << std::endl;

    this->logger->Print(ss.str());
    positionMsg->Pack();
    toSocket->Send(positionMsg->GetPackPointer(), positionMsg->GetPackSize());

    return 1;
    }

  return 0;

}


int Session::ReceiveImage(igtl::MessageHeader * header)
{
  // Create a message buffer to receive transform data
  igtl::ImageMessage::Pointer imgMsg;
  imgMsg = igtl::ImageMessage::New();
  imgMsg->SetMessageHeader(header);
  imgMsg->AllocatePack();

  // Receive transform data from the socket
  bool timeout(false);
  this->fromSocket->Receive(imgMsg->GetPackBodyPointer(), imgMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = imgMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    // Retrive the image data
    int   size[3];          // image dimension
    float spacing[3];       // spacing (mm/pixel)
    int   svsize[3];        // sub-volume size
    int   svoffset[3];      // sub-volume offset
    int   scalarType;       // scalar type
    int   endian;           // endian

    scalarType = imgMsg->GetScalarType();
    endian = imgMsg->GetEndian();
    imgMsg->GetDimensions(size);
    imgMsg->GetSpacing(spacing);
    imgMsg->GetSubVolume(svsize, svoffset);

    std::stringstream ss;
    ss << "Endian=" << endian << ", "
       << "Dimensions=("
       << size[0] << ", " << size[1] << ", " << size[2] << "), "
       << "Spacing=(" << spacing[0] << ", " << spacing[1] << ", " << spacing[2] << "), "
       << "SubVolumeDimension=(" << svsize[0] << ", " << svsize[1] << ", " << svsize[2] << "), "
       << "SubVolumeOffset=(" << svoffset[0] << ", " << svoffset[1] << ", " << svoffset[2] << ")" << std::endl;
    this->logger->Print(ss.str());

    imgMsg->Pack();
    toSocket->Send(imgMsg->GetPackPointer(), imgMsg->GetPackSize());

    return 1;
    }

  return 0;

}


int Session::ReceiveStatus(igtl::MessageHeader * header)
{
  // Create a message buffer to receive transform data
  igtl::StatusMessage::Pointer statusMsg;
  statusMsg = igtl::StatusMessage::New();
  statusMsg->SetMessageHeader(header);
  statusMsg->AllocatePack();

  // Receive transform data from the socket
  bool timeout(false);
  this->fromSocket->Receive(statusMsg->GetPackBodyPointer(), statusMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = statusMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    std::stringstream ss;

    ss << "Code=" << statusMsg->GetCode() << ", "
       << "SubCode=" << statusMsg->GetSubCode() << ", "
       << "ErrorName=" << statusMsg->GetErrorName() << ", "
       << "Status=" << statusMsg->GetStatusString() << std::endl;

    this->logger->Print(ss.str());

    statusMsg->Pack();
    toSocket->Send(statusMsg->GetPackPointer(), statusMsg->GetPackSize());

    }

  return 0;

}


#if OpenIGTLink_PROTOCOL_VERSION >= 2
int Session::ReceivePoint(igtl::MessageHeader * header)
{
  // Create a message buffer to receive transform data
  igtl::PointMessage::Pointer pointMsg;
  pointMsg = igtl::PointMessage::New();
  pointMsg->SetMessageHeader(header);
  pointMsg->AllocatePack();

  // Receive transform data from the socket
  bool timeout(false);
  this->fromSocket->Receive(pointMsg->GetPackBodyPointer(), pointMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = pointMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    std::stringstream ss;

    int nElements = pointMsg->GetNumberOfPointElement();
    for (int i = 0; i < nElements; i ++)
      {
      igtl::PointElement::Pointer pointElement;
      pointMsg->GetPointElement(i, pointElement);

      igtlUint8 rgba[4];
      pointElement->GetRGBA(rgba);

      igtlFloat32 pos[3];
      pointElement->GetPosition(pos);

      ss << "Element=" << i << ", "
         << "Name=" << pointElement->GetName() << ", "
         << "GroupName=" << pointElement->GetGroupName() << ", "
         << "RGBA=( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << "), "
         << "Position=(" << std::fixed << pos[0] << ", " << pos[1] << ", " << pos[2] << "), "
         << "Radius=" << std::fixed << pointElement->GetRadius() << ", "
         << "Owner=" << pointElement->GetOwner() << ", ";
      }
    ss << std::endl;
    this->logger->Print(ss.str());

    pointMsg->Pack();
    toSocket->Send(pointMsg->GetPackPointer(), pointMsg->GetPackSize());

    }

  return 1;
}

int Session::ReceiveTrajectory(igtl::MessageHeader::Pointer& header)
{
  // Create a message buffer to receive transform data
  igtl::TrajectoryMessage::Pointer trajectoryMsg;
  trajectoryMsg = igtl::TrajectoryMessage::New();
  trajectoryMsg->SetMessageHeader(header);
  trajectoryMsg->AllocatePack();

  // Receive transform data from the socket
  bool timeout(false);
  this->fromSocket->Receive(trajectoryMsg->GetPackBodyPointer(), trajectoryMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = trajectoryMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    std::stringstream ss;

    int nElements = trajectoryMsg->GetNumberOfTrajectoryElement();
    for (int i = 0; i < nElements; i ++)
      {
      igtl::TrajectoryElement::Pointer trajectoryElement;
      trajectoryMsg->GetTrajectoryElement(i, trajectoryElement);

      igtlUint8 rgba[4];
      trajectoryElement->GetRGBA(rgba);

      igtlFloat32 entry[3];
      igtlFloat32 target[3];
      trajectoryElement->GetEntryPosition(entry);
      trajectoryElement->GetTargetPosition(target);

      ss << "Element #" << i << ", "
         << "Name=" << trajectoryElement->GetName() << ", "
         << "GroupName=" << trajectoryElement->GetGroupName() << ", "
         << "RGBA=( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << ", "
         << "EntryPt=( " << std::fixed << entry[0] << ", " << entry[1] << ", " << entry[2] << " )" << ", "
         << "TargetPt=( " << std::fixed << target[0] << ", " << target[1] << ", " << target[2] << " )" << ", "
         << "Radius=" << std::fixed << trajectoryElement->GetRadius() << ", "
         << "Owner=" << trajectoryElement->GetOwner() << ", ";
      }
    ss << std::endl;
    this->logger->Print(ss.str());

    trajectoryMsg->Pack();
    toSocket->Send(trajectoryMsg->GetPackPointer(), trajectoryMsg->GetPackSize());

    }

  return 1;
}


int Session::ReceiveString(igtl::MessageHeader * header)
{

  // Create a message buffer to receive transform data
  igtl::StringMessage::Pointer stringMsg;
  stringMsg = igtl::StringMessage::New();
  stringMsg->SetMessageHeader(header);
  stringMsg->AllocatePack();

  // Receive transform data from the socket
  bool timeout(false);
  this->fromSocket->Receive(stringMsg->GetPackBodyPointer(), stringMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = stringMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    std::stringstream ss;

    ss << "Encoding=" << stringMsg->GetEncoding() << ", "
       << "String=" << stringMsg->GetString() << std::endl;

    this->logger->Print(ss.str());

    stringMsg->Pack();
    this->toSocket->Send(stringMsg->GetPackPointer(), stringMsg->GetPackSize());

    }

  return 1;
}


int Session::ReceiveBind(igtl::MessageHeader * header)
{
  // Create a message buffer to receive transform data
  igtl::BindMessage::Pointer bindMsg;
  bindMsg = igtl::BindMessage::New();
  bindMsg->SetMessageHeader(header);
  bindMsg->AllocatePack();

  // Receive transform data from the socket
  bool timeout(false);
  this->fromSocket->Receive(bindMsg->GetPackBodyPointer(), bindMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = bindMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    std::stringstream ss;

    int n = bindMsg->GetNumberOfChildMessages();

    for (int i = 0; i < n; i ++)
      {
      if (strcmp(bindMsg->GetChildMessageType(i), "STRING") == 0)
        {
        igtl::StringMessage::Pointer stringMsg;
        stringMsg = igtl::StringMessage::New();
        bindMsg->GetChildMessage(i, stringMsg);
        stringMsg->Unpack(0);
        ss << "MessageType=STRING,"
           << "MessageNname=" << stringMsg->GetDeviceName() << ", "
           << "Encoding=" << stringMsg->GetEncoding() << ", "
           << "String=" << stringMsg->GetString() << ", ";
        }
      else if (strcmp(bindMsg->GetChildMessageType(i), "TRANSFORM") == 0)
        {
        igtl::TransformMessage::Pointer transMsg;
        transMsg = igtl::TransformMessage::New();
        bindMsg->GetChildMessage(i, transMsg);
        transMsg->Unpack(0);
        ss << "MessageType=TRANSFORM,"
           << "MessageName=" << transMsg->GetDeviceName() << ", ";
        igtl::Matrix4x4 matrix;
        transMsg->GetMatrix(matrix);
        //igtl::PrintMatrix(matrix);
        ss << "Matrix=("
           << matrix[0][0] << ", " << matrix[1][0] << ", " << matrix[2][0] << ", " << matrix[3][0] << ", "
           << matrix[0][1] << ", " << matrix[1][1] << ", " << matrix[2][1] << ", " << matrix[3][1] << ", "
           << matrix[0][2] << ", " << matrix[1][2] << ", " << matrix[2][2] << ", " << matrix[3][2] << ", "
           << matrix[0][3] << ", " << matrix[1][3] << ", " << matrix[2][3] << ", " << matrix[3][3] << "),";
        }
      }

    ss << std::endl;
    this->logger->Print(ss.str());
    bindMsg->Pack();
    this->toSocket->Send(bindMsg->GetPackPointer(), bindMsg->GetPackSize());

    }

  return 1;
}


int Session::ReceiveCapability(igtl::MessageHeader * header)
{
  // Create a message buffer to receive transform data
  igtl::CapabilityMessage::Pointer capabilMsg;
  capabilMsg = igtl::CapabilityMessage::New();
  capabilMsg->SetMessageHeader(header);
  capabilMsg->AllocatePack();

  // Receive transform data from the socket
  bool timeout(false);
  this->fromSocket->Receive(capabilMsg->GetPackBodyPointer(), capabilMsg->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = capabilMsg->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    std::stringstream ss;

    int nTypes = capabilMsg->GetNumberOfTypes();
    for (int i = 0; i < nTypes; i ++)
      {
      ss << "ID=" << i << ", "
         << "TYPE=" << capabilMsg->GetType(i) << ", ";
      }

    ss << std::endl;
    this->logger->Print(ss.str());
    capabilMsg->Pack();
    this->toSocket->Send(capabilMsg->GetPackPointer(), capabilMsg->GetPackSize());

    }

  return 1;

}

int Session::ReceiveTrackingData(igtl::MessageHeader::Pointer& header)
{
  //------------------------------------------------------------
  // Allocate TrackingData Message Class

  igtl::TrackingDataMessage::Pointer trackingData;
  trackingData = igtl::TrackingDataMessage::New();
  trackingData->SetMessageHeader(header);
  trackingData->AllocatePack();

  // Receive body from the socket
  bool timeout(false);
  fromSocket->Receive(trackingData->GetPackBodyPointer(), trackingData->GetPackBodySize(), timeout);

  // Deserialize the transform data
  // If you want to skip CRC check, call Unpack() without argument.
  int c = trackingData->Unpack(1);

  if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
    {
    std::stringstream ss;

    int nElements = trackingData->GetNumberOfTrackingDataElements();
    for (int i = 0; i < nElements; i ++)
      {
      igtl::TrackingDataElement::Pointer trackingElement;
      trackingData->GetTrackingDataElement(i, trackingElement);

      igtl::Matrix4x4 matrix;
      trackingElement->GetMatrix(matrix);

      ss << "ElementID=" << i << ", "
         << "Name=" << trackingElement->GetName() << ", "
         << "Type=" << (int) trackingElement->GetType() << ", ";

      ss << "Matrix=("
         << matrix[0][0] << ", " << matrix[1][0] << ", " << matrix[2][0] << ", " << matrix[3][0] << ", "
         << matrix[0][1] << ", " << matrix[1][1] << ", " << matrix[2][1] << ", " << matrix[3][1] << ", "
         << matrix[0][2] << ", " << matrix[1][2] << ", " << matrix[2][2] << ", " << matrix[3][2] << ", "
         << matrix[0][3] << ", " << matrix[1][3] << ", " << matrix[2][3] << ", " << matrix[3][3] << "),";

      }

    ss << std::endl;
    this->logger->Print(ss.str());
    trackingData->Pack();
    this->toSocket->Send(trackingData->GetPackPointer(), trackingData->GetPackSize());
    }
  else
    {
    this->logger->Print("Invalid TrackingData message.");
    }

  return 1;
}



#endif //OpenIGTLink_PROTOCOL_VERSION >= 2

} // End of igtl namespace
