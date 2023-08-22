# IGTLRepeater
A bridge to relay OpenIGTLink messages between two hosts for debugging.

## Prerequisite

The IGTLRepeater (in principle) works on Linux, Mac, and Windows. To build the software, the following tools are required:

- C++ compiler (g++, llvm c++, or Visual Studio)
- CMake

## Installation

### OpenIGTLink

The OpenIGTLink library must be built on the computer before comiling the IGTLRepeater. To install the OpenIGTLink library:

~~~~
$ cd <working directory>
$ git clone https://github.com/openigtlink/OpenIGTLink
$ mkdir OpenIGTLink-build
~~~~

You can configure the source code with CMake, from either CMake GUI or `ccmake` interface, or using the `cmake` command. The library can be compiled with the default parameters, though using the `Release` mode could improve the performance. If you are using either CMake GUI or ccamek interface,


| Where is the source code    | <working directory>/OpenIGTLink       |
|-----------------------------|---------------------------------------|
| Where to build the binaries | <working directory>/OpenIGTLink-build |




| Name             | Value                         |
|------------------|-------------------------------|
| CMAKE_BUILD_TYPE | Release                       |

then, click `Configure` followed by `Generate`.

If you are configuring with the `cmake` commmand:

~~~~
$ cd OpenIGTLink-build
$ cmake -DCMAKE_BUILD_TYPE:STRING=Relase ../OpenIGTLink
~~~~

Once the source code has been configured, run `make` (or build from Visual Studio).


### IGTLRepeater

Similarly, IGTLRepeater can be configured with CMake, and then built with the C++ compalier.

~~~~
$ cd <working directory>
$ git clone https://github.com/tokjun/IGTLRepeater
$ mkdir IGTLRepeater-build
~~~~

If using CMake GUI or `ccmake`, 


| Where is the source code    | <working directory>/IGTLRepeater       |
|-----------------------------|----------------------------------------|
| Where to build the binaries | <working directory>/IGTLRepeater-build |


| Name             | Value                                  |
|------------------|----------------------------------------|
| CMAKE_BUILD_TYPE | Release                                |
| OpenIGTLink_DIR  | <working directory>/OpenIGTLink-build  |


then, click `Configure` followed by `Generate`.

If you are configuring with the `cmake` commmand:

~~~~
$ cd IGTLRepeater-build
$ cmake -DCMAKE_BUILD_TYPE:STRING=Relase -DOpenIGTLink_DIR:PATH=<working directory>/OpenIGTLink-build ../IGTLRepeater
~~~~

Once the source code has been configured, run `make` (or build from Visual Studio).


## Usage

### Basics
We assume the following configuration:

~~~~
 +------------------+              +------------------+              +------------------+
 |                  |------------->|                  |------------->|                  |
 |  Client Host     |              |  Relay Host      |              |  Server Host     |
 |  192.168.0.2     |              |  192.168.0.3     |              |  192.168.0.4     |
 |                  |<-------------|                  |<-------------|                  |
 +------------------+              +------------------+              +------------------+
                               TCP Server                        TCP Server 
                               Port# 18945                       Port# 18944
~~~~
 
First, Open a terminal on the relay host, and run the following command to start the repeater:

~~~~
$ igtlrepeater 192.168.0.4 18944 18944
~~~~

Note that the first argument (`192.168.0.4`) is the IP or hostname of the server host. The second and third arguments (`18944` and `18945`) are the port numbers for the server host and the relay host, respectively. The port numbers can be the same, unless the relay and server hosts share a same port number.

Once the repeater has started, connect the client host to the server host using the IP and port number configured for the relay host. (`192.168.0.3` and `18945`, instead of `192.168.0.4` and `18944`.)


The console output from the repeater would look like:
~~~~
C->S, 1690476966.486063000, 0.000000000, LinearTransform, TRANSFORM, Matrix=(0.596679, 0.511415, -0.618408, 0, -0.650774, 0.759271, 0, 0, 0.46954, 0.402444, 0.785857, 0, 32.9847, 32.4072, 26.5837, 1)
C->S, 1690476966.511504000, 0.000000000, LinearTransform, TRANSFORM, Matrix=(0.596679, 0.511415, -0.618408, 0, -0.650774, 0.759271, 0, 0, 0.46954, 0.402444, 0.785857, 0, 31.9847, 32.4072, 26.5837, 1)
S->C, 1690476967.562868000, 0.000000000, LinearTransform_1, TRANSFORM, Matrix=(0.984808, 0, 0.173648, 0, 0, 1, 0, 0, -0.173648, 0, 0.984808, 0, 23.6354, 10, 4.16756, 1)
S->C, 1690476969.616718000, 0.000000000, LinearTransform_1, TRANSFORM, Matrix=(0.984808, 0, 0.173648, 0, 0, 1, 0, 0, -0.173648, 0, 0.984808, 0, 23.6354, 11, 4.16756, 1)
S->C, 1690476969.628399000, 0.000000000, LinearTransform_1, TRANSFORM, Matrix=(0.984808, 0, 0.173648, 0, 0, 1, 0, 0, -0.173648, 0, 0.984808, 0, 23.6354, 12
~~~~

Each line represents one OpenIGTLink message transferred from either the server or the client and is formatted in the CVS format. The first column (`C->S` or `S->C`) indicates the direction of the message, either 'Client to Server (C->S)' or 'Server to Client (S->C)'. The numbers in the second and thrid columns are the time stamps based on the system clock and the message header, respectively. The strings in the forth and fifth columns are the message name and the message type, respcetively. The rest of the line shows the content of the message, and the format depends on the message type.


## Blocking messages

The bridge allows the user to specify message types to be blocked using `-b` option. 

For example, if you want to prevent any RTS_TDATA message from being transmitted between the two hosts, run the bridge as follows (the IP and port numbers depend on your environment):

~~~~
$ igtlrepeater -b RTS_TDATA 192.168.0.4 18944 18944
~~~~






