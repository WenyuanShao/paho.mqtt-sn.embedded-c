#!/bin/bash

rm pub0sub1
./build
./pub0sub1 --host 127.0.0.1 --clientid kkkkkkkkkk000000 --server_port 1885 --client_port 11211 --qos 1 --rate 500 --pubnum 2000
