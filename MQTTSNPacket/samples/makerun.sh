#!/bin/bash

rm pub0sub1
./build
./pub0sub1 --host 10.10.1.2 --clientid kkkkkkkkkk000000 --server_port 1883 --client_port 11211 --qos 1
