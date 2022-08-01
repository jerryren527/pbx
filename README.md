# Private Branch Exchange (PBX) Telephone System
This project is an implementation of a server that simulates the behavior of a PBX telephone system. 

# Introduction
This project is developed in C using sockets for interprocess communication. Multithreading safety and concurrency guarantees are ensured by using POSIX threads, mutexes, and semaphores. This project is course work for a Programming in C course at Stony Brook University.

The PBX (Private Branch Exchange) system allows calls to be placed between telephone units (TUs). 

### The PBX system can perform the following operations:

1) **Register** a TU in the system, and
2) **Unregister** a previously registered TU.

### A registered TU can perform the following operations:

1) **Pick up**. If the TU is ringing, then a connection is established with a calling TU. If the TU is not ringing, then the user hears a "dial tone" over the receiver.
2) **Hang up**. Any call in progress is disconnected.
3) **Dial** another registered TU. If the dialed TU is currently "on hook", then the dialed TU will start to ring, and the calling TU hears a "ring back". Otherwise, the dialed TU is "off hook", and a the calling TU hears a "busy signal."
4) **Chat** over the connection established when a calling TU dialed a called TU and the called TU picked up.

The TU operations and states (e.g., "on hook", "off hook", "dial tone", "ring back") are exactly analogous to a telephone system.

## Setup
This project is compiled and executed on the Linux Mint 20 Cinnamon operating system.

## Running the Project
Git clone this repository to your local computer running the Linux Mint 20 Cinnamon operating system. Compile the code using the *make* command, then run the PBX server with the following command:
```
$ make clean all
$ bin/pbx -p 9999
```

* In the example commands above, 9999 is a port number. Replace 9999 with any number above 1024.

In a new terminal window, use **telnet** to connect to the server:
```
$ telnet localhost 9999
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
ON HOOK 4
```

## Demo
https://user-images.githubusercontent.com/55968519/182228834-a2b4845e-b71c-4fb6-8681-5d2f48071eb9.mp4
