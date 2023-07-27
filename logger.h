#ifndef LOGGER_H_
#define LOGGER_H_

/*=========================================================================

  Program:   IGTL Repeater
  Language:  C++

  Copyright (c) Junichi Tokuda. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <string>

#include "igtlWin32Header.h"
#include "igtlMultiThreader.h"
#include "igtlImageMessage.h"
#include "igtlMutexLock.h"

namespace igtl
{

class IGTLCommon_EXPORT Logger : public Object
{
public:

  static const char*  StatusString[];

  igtlTypeMacro(igtl::Logger, igtl::Object)
  igtlNewMacro(igtl::Logger);

public:

  virtual const char * GetClassName() { return "Base"; };

  void Print(std::string& msg);

protected:

  Logger();
  ~Logger();

  void           PrintSelf(std::ostream& os) const;

protected:

  igtl::MutexLock::Pointer Mutex;
};

}

#endif // LOGGER_H_
