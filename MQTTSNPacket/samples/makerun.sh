#!/bin/bash

rm pub0sub1
./build
./pub0sub1 --host 10.10.1.2 --clientid kkkkkkkkkk000000 --server_port 11211 --client_port 11211 --qos 0 --rate 200 --duration 3 --publisher
echo "./pub0sub1 --host 10.10.1.2 --clientid kkkkkkkkkk000000 --server_port 1884 --client_port 11211 --qos 1 --rate 200 --pubnum 100 --publisher"
