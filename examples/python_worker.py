import sys
import zmq
import msgpack

ident = sys.argv[1]

c = zmq.Context()
receiver = c.socket(zmq.PULL)
sender = c.socket(zmq.PUSH)

receiver.connect('tcp://127.0.0.1:5555')
sender.connect('tcp://127.0.0.1:4444')

count  = 0
while True:
    raw = receiver.recv()
    data = msgpack.unpackb(raw)
    content_type = "text/html"
    content = '<html><h1>rio</h1></html>'
    sender.send(msgpack.packb([data[0], len(content_type), content_type,
                               len(content), content]))
    count += 1
