===============================
rio - minimalist http server 
===============================

rio is a minimalist http server that act as broker between browsers and your (python, ruby and others) workers

this project is a work in progress

Dependencies
------------

* zeromq (http://zeromq.org) 
* msgpack (http://msgpack.org)

Building
--------

Open terminal::
    
    git submodule init
    git submodule update
    make

Tesing
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

Experimental worker support
---------------------------

rio is designed to be plugged with workers at runtime, and to communicate really fast with them.

request format - rio to worker(s)

* [client_id, method, uri] as int, int, string


respose format - worker(s) to rio

* [client_id, content] as int, string



send me an email: felipecruz@loogica.net