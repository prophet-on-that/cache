# Cache Server

This project is an implementation of a caching server and client,
written in C, which I have used to develop my understanding of
low-level network programming and gain more experience in the
language. The server uses a single-threaded, fixed-size hash table to
store data, and accepts requests to get and put data from the
client. The data format and (de)serialising is written from scratch.

I have used example files from [Beej's Guide to Network
Programming](https://beej.us/guide/bgnet/) as a base for the client
and the server.

## Building The Software

The software requires no dependencies apart from glibc. A Makefile is
provided: running `make` will build the client and server. `make test`
will run a small test suite.

## Memory Management Convention

The following conventions are used in the codebase to ease memory
management:

  * When calling a function prefixed with `create` or `out` that
    returns a pointer, the caller becomes its owner and is responsible
    for deallocation.
  * When calling a function, and pointer arguments prefixed by `take`
    take ownership of the pointer and will handle deallocation. On
    return from the function, the pointer argument must no longer be
    used. Moreover, a pointer to the stack should never be passed to
    such an argument.

This project has helped me understand the following situation in which
memory management in C is a pain. When incorporating data stored into
the heap into another struct, you either have the choice of copying
the data (straightforward to reason about but expensive) or take
ownership of the data (in C++, or in garbage-collected languages,
you'd use a shared pointer, but I haven't looked for a C
implementation, which would likely be limited anyway due to lack of
type system support). When a function takes ownership of a pointer,
you need some kind of naming convention (see above), or rigourous
documentation, so the caller knows not to deallocate (or even
reference) the pointer after the call returns. Additionally, using
stack-allocated data is a complication that would need to be handled
manually in each case, as we don't want to `free` such data!
