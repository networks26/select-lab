There are 3 dragons to fight, see `server.c`.

The main difference of this code with the code that is familiar to you is

* SELECT!!!

Do `man select`

Here is [my vpn code example](https://github.com/norayr/xoxovpn), and it is primitive, it is able to serve only one client.

I also had to set the socket to non-blocking, otherwise read operation would block, and my code would not work.

There is a solution, newer API in Unix systems (well, "newer", it's very old), that's select.

So we did fork(), now select() time. That's the one that uses sets.

Try to connect to your server with two clients, with three clients!

Using select() or poll() in a single-threaded event loop allows you to handle multiple connections from a single thread!

* You call select() (or poll()) on a set of file descriptors (the listening socket plus all the connected client sockets).

* select() blocks until at least one of those sockets becomes “ready” (e.g., it has data to be read, it can be written to, or there’s a new incoming connection on the listening socket).

* Once select() returns, you know exactly which socket(s) is/are ready; you then service them—performing accept(), recv(), send(), etc.

* After handling all ready sockets, you loop back and call select() again.

Because you’re never indefinitely blocked on just one client’s I/O, a single thread can effectively “multiplex” multiple connections. The thread only blocks inside select() (waiting for any activity on any socket), then rapidly responds to whichever socket(s) need service.

This model is often referred to as an event-driven, single-threaded architecture. When implementing a server you don’t necessarily need multi-threading but can still have concurrency across multiple sockets.



