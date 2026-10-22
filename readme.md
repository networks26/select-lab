There are **4 dragons** to fight, see `server.c`. The fourth one uses what you learned about signals in the previous lab.

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

## The four dragons

The first three are marked in `server.c`:

1. **Dragon #1 — TCP socket:** fix the protocol argument to `socket()`. Read `man 2 socket`.
2. **Dragon #2 — port number:** fix the port assigned in `sockaddr_in`. Remember network byte order. Read `man 3 htons` (or `man 3 byteorder`).
3. **Dragon #3 — XOR:** decrypt the bytes received from `client.c` using the same XOR key as the client, then print the decrypted message. Do not assume the encrypted bytes form a printable C string.

### Dragon #4 — Ctrl+C and closing sockets

You implemented signal handling in the **previous lab**. This time, add it yourself to the `select()` server. There is a handler skeleton and some hints in `server.c`, but the task is **not implemented**.

When you press **Ctrl+C** in the server terminal, your program must:

1. Catch `SIGINT` and request shutdown rather than terminate immediately with the default signal action.
2. Stop the main loop, including when the server is waiting inside `select()` with no clients sending anything.
3. Close **every connected client socket** and the **listening socket**, without closing a descriptor twice.
4. Exit and return to the shell. It must be possible to start the server again afterward.

Hints (look back at last week's `server.c`):

- Use `signal()` (or `sigaction()`) to register a handler for `SIGINT`. Look at `man 2 signal` and `man 7 signal`.
- Use a `volatile sig_atomic_t` flag so the signal handler can tell the main loop to stop. The starter code already declares `stop_requested` and `listening_fd` for you.
- Keep the handler short: set the flag, close the listening socket, and preserve `errno`. **Do not call `printf()` inside a signal handler**. The `close()` function is async-signal-safe. Leave closing *client* sockets to the ordinary cleanup code after the loop.
- A signal can interrupt `select()`: it then returns `-1` and `errno` may be `EINTR`. Handle this explicitly. Do **not** call `FD_ISSET()` on the result of a failed `select()`.
- If the handler closes the listening socket immediately before `select()` is called, `select()` may instead fail with `EBADF`. When shutdown was requested, this is not a fatal server error.
- The existing `while (1)` should not keep running after shutdown is requested. Check the shutdown flag again after `select()` returns, before accepting or reading sockets.

**Test it:** Start the server and connect **two or three clients** (use `nc` to observe the disconnections; the provided `client.c` does not read replies). Leave the clients connected and idle. Press Ctrl+C **in the server terminal**. Does the server return to the shell promptly? Do the client connections close? Can you start the server again on the same port?

Think about this: the operating system closes all file descriptors when a process terminates anyway. Why is handling `SIGINT` and closing the sockets deliberately still useful?

## What to submit

Submit your updated `server.c` with **all four dragons solved**. Keep the `client.c` and `makefile` in the repository so someone else can compile and test your work. In your README or a separate short text file, explain how you tested Ctrl+C with multiple connected clients.
