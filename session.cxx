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
  igtl::TimeStamp::Pointer ts;
  ts = igtl::TimeStamp::New();


  // Initialize receive buffer
  headerMsg->InitPack();

  // Receive generic header from the socket
  bool timeout(false);
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

  // Deserialize the header
  headerMsg->Unpack();

  // Get time stamp
  igtlUint32 sec;
  igtlUint32 nanosec;

  headerMsg->GetTimeStamp(ts);
  ts->GetTimeStamp(&sec, &nanosec);

  std::cerr << "Time stamp: "
            << sec << "." << std::setw(9) << std::setfill('0')
            << nanosec << std::endl;

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
#endif //OpenIGTLink_PROTOCOL_VERSION >= 2
  else
    {
    // if the data type is unknown, skip reading.
    std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
    std::cerr << "Size : " << headerMsg->GetBodySizeToRead() << std::endl;
    fromSocket->Skip(headerMsg->GetBodySizeToRead(), 0);
    }

  return 0;
}


int Session::ReceiveTransform(igtl::MessageHeader * header)
{
  std::cerr << "Receiving TRANSFORM data type." << std::endl;

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
    // Retrive the transform data
    igtl::Matrix4x4 matrix;
    transMsg->GetMatrix(matrix);
    igtl::PrintMatrix(matrix);

    transMsg->Pack();
    toSocket->Send(transMsg->GetPackPointer(), transMsg->GetPackSize());

    return 1;
    }


  return 0;

}


int Session::ReceivePosition(igtl::MessageHeader * header)
{
  std::cerr << "Receiving POSITION data type." << std::endl;

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

    std::cerr << "position   = (" << position[0] << ", " << position[1] << ", " << position[2] << ")" << std::endl;
    std::cerr << "quaternion = (" << quaternion[0] << ", " << quaternion[1] << ", "
              << quaternion[2] << ", " << quaternion[3] << ")" << std::endl << std::endl;

    positionMsg->Pack();
    toSocket->Send(positionMsg->GetPackPointer(), positionMsg->GetPackSize());

    return 1;
    }

  return 0;

}


int Session::ReceiveImage(igtl::MessageHeader * header)
{
  std::cerr << "Receiving IMAGE data type." << std::endl;

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

    std::cerr << "Device Name           : " << imgMsg->GetDeviceName() << std::endl;
    std::cerr << "Scalar Type           : " << scalarType << std::endl;
    std::cerr << "Endian                : " << endian << std::endl;
    std::cerr << "Dimensions            : ("
              << size[0] << ", " << size[1] << ", " << size[2] << ")" << std::endl;
    std::cerr << "Spacing               : ("
              << spacing[0] << ", " << spacing[1] << ", " << spacing[2] << ")" << std::endl;
    std::cerr << "Sub-Volume dimensions : ("
              << svsize[0] << ", " << svsize[1] << ", " << svsize[2] << ")" << std::endl;
    std::cerr << "Sub-Volume offset     : ("
              << svoffset[0] << ", " << svoffset[1] << ", " << svoffset[2] << ")" << std::endl;

    imgMsg->Pack();
    toSocket->Send(imgMsg->GetPackPointer(), imgMsg->GetPackSize());

    return 1;
    }

  return 0;

}


int Session::ReceiveStatus(igtl::MessageHeader * header)
{

  std::cerr << "Receiving STATUS data type." << std::endl;

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
    std::cerr << "========== STATUS ==========" << std::endl;
    std::cerr << " Code      : " << statusMsg->GetCode() << std::endl;
    std::cerr << " SubCode   : " << statusMsg->GetSubCode() << std::endl;
    std::cerr << " Error Name: " << statusMsg->GetErrorName() << std::endl;
    std::cerr << " Status    : " << statusMsg->GetStatusString() << std::endl;
    std::cerr << "============================" << std::endl;

    statusMsg->Pack();
    toSocket->Send(statusMsg->GetPackPointer(), statusMsg->GetPackSize());

    }

  return 0;

}


#if OpenIGTLink_PROTOCOL_VERSION >= 2
int Session::ReceivePoint(igtl::MessageHeader * header)
{

  std::cerr << "Receiving POINT data type." << std::endl;

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
    int nElements = pointMsg->GetNumberOfPointElement();
    for (int i = 0; i < nElements; i ++)
      {
      igtl::PointElement::Pointer pointElement;
      pointMsg->GetPointElement(i, pointElement);

      igtlUint8 rgba[4];
      pointElement->GetRGBA(rgba);

      igtlFloat32 pos[3];
      pointElement->GetPosition(pos);

      std::cerr << "========== Element #" << i << " ==========" << std::endl;
      std::cerr << " Name      : " << pointElement->GetName() << std::endl;
      std::cerr << " GroupName : " << pointElement->GetGroupName() << std::endl;
      std::cerr << " RGBA      : ( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << std::endl;
      std::cerr << " Position  : ( " << std::fixed << pos[0] << ", " << pos[1] << ", " << pos[2] << " )" << std::endl;
      std::cerr << " Radius    : " << std::fixed << pointElement->GetRadius() << std::endl;
      std::cerr << " Owner     : " << pointElement->GetOwner() << std::endl;
      std::cerr << "================================" << std::endl;
      }

    pointMsg->Pack();
    toSocket->Send(pointMsg->GetPackPointer(), pointMsg->GetPackSize());

    }

  return 1;
}

int Session::ReceiveTrajectory(igtl::MessageHeader::Pointer& header)
{

  std::cerr << "Receiving TRAJECTORY data type." << std::endl;

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

      std::cerr << "========== Element #" << i << " ==========" << std::endl;
      std::cerr << " Name      : " << trajectoryElement->GetName() << std::endl;
      std::cerr << " GroupName : " << trajectoryElement->GetGroupName() << std::endl;
      std::cerr << " RGBA      : ( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << std::endl;
      std::cerr << " Entry Pt  : ( " << std::fixed << entry[0] << ", " << entry[1] << ", " << entry[2] << " )" << std::endl;
      std::cerr << " Target Pt : ( " << std::fixed << target[0] << ", " << target[1] << ", " << target[2] << " )" << std::endl;
      std::cerr << " Radius    : " << std::fixed << trajectoryElement->GetRadius() << std::endl;
      std::cerr << " Owner     : " << trajectoryElement->GetOwner() << std::endl;
      std::cerr << "================================" << std::endl << std::endl;
      }

    trajectoryMsg->Pack();
    toSocket->Send(trajectoryMsg->GetPackPointer(), trajectoryMsg->GetPackSize());

    }

  return 1;
}


int Session::ReceiveString(igtl::MessageHeader * header)
{

  std::cerr << "Receiving STRING data type." << std::endl;

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
    std::cerr << "Encoding: " << stringMsg->GetEncoding() << "; "
              << "String: " << stringMsg->GetString() << std::endl;

    stringMsg->Pack();
    this->toSocket->Send(stringMsg->GetPackPointer(), stringMsg->GetPackSize());

    }

  return 1;
}


int Session::ReceiveBind(igtl::MessageHeader * header)
{

  std::cerr << "Receiving BIND data type." << std::endl;

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
    int n = bindMsg->GetNumberOfChildMessages();

    for (int i = 0; i < n; i ++)
      {
      if (strcmp(bindMsg->GetChildMessageType(i), "STRING") == 0)
        {
        igtl::StringMessage::Pointer stringMsg;
        stringMsg = igtl::StringMessage::New();
        bindMsg->GetChildMessage(i, stringMsg);
        stringMsg->Unpack(0);
        std::cerr << "Message type: STRING" << std::endl;
        std::cerr << "Message name: " << stringMsg->GetDeviceName() << std::endl;
        std::cerr << "Encoding: " << stringMsg->GetEncoding() << "; "
                  << "String: " << stringMsg->GetString() << std::endl;
        }
      else if (strcmp(bindMsg->GetChildMessageType(i), "TRANSFORM") == 0)
        {
        igtl::TransformMessage::Pointer transMsg;
        transMsg = igtl::TransformMessage::New();
        bindMsg->GetChildMessage(i, transMsg);
        transMsg->Unpack(0);
        std::cerr << "Message type: TRANSFORM" << std::endl;
        std::cerr << "Message name: " << transMsg->GetDeviceName() << std::endl;
        igtl::Matrix4x4 matrix;
        transMsg->GetMatrix(matrix);
        igtl::PrintMatrix(matrix);
        }
      }

    bindMsg->Pack();
    this->toSocket->Send(bindMsg->GetPackPointer(), bindMsg->GetPackSize());

    }

  return 1;
}


int Session::ReceiveCapability(igtl::MessageHeader * header)
{

  std::cerr << "Receiving CAPABILITY data type." << std::endl;

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
    int nTypes = capabilMsg->GetNumberOfTypes();
    for (int i = 0; i < nTypes; i ++)
      {
      std::cerr << "Typename #" << i << ": " << capabilMsg->GetType(i) << std::endl;
      }

    capabilMsg->Pack();
    this->toSocket->Send(capabilMsg->GetPackPointer(), capabilMsg->GetPackSize());

    }

  return 1;

}


#endif //OpenIGTLink_PROTOCOL_VERSION >= 2

} // End of igtl namespace
