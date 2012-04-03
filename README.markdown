===============================
rio - minimalist http server 
===============================

**this project is a work in progress**

rio is a minimalist http server that act as broker between browsers and your (python, ruby and others) workers.

If you're looking for similar projects, you should check:

* Mongrel2 (http://mongrel2.org/)
* zerogw (https://github.com/tailhook/zerogw)


External Dependencies and thanks to the Authors of:
------------

* zeromq (http://zeromq.org) 
* msgpack (http://msgpack.org)


Embedded libraries
-----------------

* khash from klib (https://github.com/attractivechaos/klib)


Building
--------

Open terminal::
    
    git submodule init
    git submodule update
    make

Testing
--------

Install CUnit (http://cunit.sourceforge.net)

Open terminal::
    
    make test

Debugging
--------

Open terminal::

    make debug
    sudo valgrind --leak-check=full ./rio
    

Running
-------

Open terminal::
    
    sudo ./rio

rio will serve your current directory files - (html, js, css, json, jpg, png)




Master -> Workers pattern
-------------------------

This branch will implement the master -> workers pattern. 

* **Master** - holds the runtime with listen socket, references to workers, catch signals and use zeromq PUB to send workers control messages. 

* **Worker** - a worker can have different roles. The initial idea is to have one worker reading and writing to external clients (http clients like browsers) and other worker sending and receiving payload from backend workers


Worker support
---------------------------

rio is designed to be plugged with backend workers at runtime, and to communicate really fast with them.

Request format - rio to worker(s)

* [client_id, method, uri] as int, int, string


Respose format - worker(s) to rio

* [client_id, content/type, content] as int, string(not implemented yet), string

A naive python worker

```python

import zmqpy

try:
    import msgpack
except Exception as e:
    import msgpack_pure as msgpack

c = zmqpy.Context().instance()
receiver = c.socket(zmqpy.PULL)
sender = c.socket(zmqpy.PUB)

receiver.connect('tcp://127.0.0.1:5555')
sender.connect('tcp://127.0.0.1:4444')

count  = 0
while True:
    raw = receiver.recv()
    data = msgpack.unpackb(raw)
    sender.send(msgpack.packb([data[0], '<html><h1>rio</h1></html>']))
```

A naive ruby worker could look like that

```ruby

require 'zmq'
require 'msgpack'

context = ZMQ::Context.new

worker_socket = context.socket ZMQ::PULL
sender_socket = context.socket ZMQ::PUB

worker_socket.connect "tcp://127.0.0.1:5555"
sender_socket.connect "tcp://127.0.0.1:4444"

count = 0

while true
    req = MessagePack.unpack worker_socket.recv
    count += 1
    id = req[0]
    uri = req[2]
    rep = MessagePack.pack([id, "<html><h1>RubyWorker #{count}</h1></html>"])
    sender_socket.send rep
end
```

TODO
----

* Finish workers synchronization
* Send http headers to external workers
* Refactor client struct - change name and use rio_buffer
* Implement REAL non-blocking behaviour - check all returns and errnos


send me an email: felipecruz@loogica.net
