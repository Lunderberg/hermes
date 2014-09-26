hermes
======

This is a library designed for easy message passing between programs over IP/TCP using boost.

Any object that inherits from the Message class and implements a serialize function can be written.
An example of simple use is included.

Requirements
------------

- C++11
- libboost_system
- boost/asio, header-only library
- boost/preprocessor, header-only library