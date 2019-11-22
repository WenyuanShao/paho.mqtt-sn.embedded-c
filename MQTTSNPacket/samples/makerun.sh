#!/bin/bash

rm pub0sub1
./build
./pub0sub1 --host 161.253.78.154 --clientid test --server_port 1885 --client_port 11211 --qos 1
