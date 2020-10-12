#!/bin/bash

rm pub0sub1
./build
./pub0sub1 --host 10.10.1.2 --clientid kkkkkkkkkk000001 --server_port 1885 --client_port 11212 --qos 1 --rate 200 --pubnum 10
