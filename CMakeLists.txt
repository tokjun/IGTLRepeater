PROJECT(igtlRepeater)

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

find_package(OpenIGTLink REQUIRED)

include(${OpenIGTLink_USE_FILE})

set(igtlRepeater_SOURCES
  session.cxx
  logger.cxx
  )

ADD_EXECUTABLE(igtlrepeater
  ${igtlRepeater_SOURCES}
  main.cxx
  )
TARGET_LINK_LIBRARIES(igtlrepeater OpenIGTLink)
