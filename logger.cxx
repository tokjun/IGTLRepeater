/*=========================================================================

  Program:   NavX-OpenIGTLink Bridge
  Module:    $RCSfile: $
  Language:  C++
  Date:      $Date: $
  Version:   $Revision: $

  Copyright (c) Brigham and Women's Hospital. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#include <string.h>

#include "logger.h"

#include "igtlOSUtil.h"
#include "igtlTimeStamp.h"

namespace igtl
{

//-----------------------------------------------------------------------------
Logger::Logger()
{
  this->Mutex = igtl::MutexLock::New();
}

//-----------------------------------------------------------------------------
Logger::~Logger()
{
}

void Logger::Print(const std::string& msg)
{
  this->Mutex->Lock();
  std::cout << msg;
  this->Mutex->Unlock();
}

//-----------------------------------------------------------------------------
void Logger::PrintSelf(std::ostream& os) const
{
  this->Superclass::PrintSelf(os);
}



} // End of igtl namespace
