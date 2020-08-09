## INTRODUCTION
UDP version of TCP reliable data transfer model, including protocols such as connection management, congestion control, and loss handling, implemented with [BSD sockets](https://www.keil.com/pack/doc/mw6/Network/html/using_network_sockets_bsd.html).


## COMPONENTS
[packet.h](https://github.com/lliu0809/rdt_web_server/blob/master/packet.h) contains informatin about the transfer packet, such as ACK Number, Sequence Number, flags and payload;<br/>

[server.cpp](https://github.com/lliu0809/rdt_web_server/blob/master/server.cpp) contains all the actions by the server. It opens a socket on a given port and waits for client connection. Upon connection, a three-way handshaking begins. The server stores  packets from the client with consecuetive ID numbers. When reveiving the FIN packet from client indicating the end of connection, it replies the client with an ACK and FIN packet. The server continues to run and wait for future connections after closing the previous one.<br/>

[client.cpp](https://github.com/lliu0809/rdt_web_server/blob/master/client.cpp) implents the client side. After finishing the transmission, it will send a FIN packet to the server. When it receives an ACK and FIN from the server, it will enter a 2 seconds countdown, after which closing the connection with the server.<br/>

## 
To run the system and begin file transfering, download [VirtualBox](https://www.virtualbox.org).
To build the files:
```
$ Makefile
```
To start the server:
```
$ ./server HOSTNAME
```
To start the client, connect to the server and transfer files to the server:
```
$ ./client HOSTNAME PORT FILE
```

To test data losses, use the [tc](https://man7.org/linux/man-pages/man8/tc.8.html) command.
