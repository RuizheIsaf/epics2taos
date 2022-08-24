#!/bin/bash
echo "hello world"
cd ../..
make clean && make
cd bin/linux-x86_64
./caMonitor ioctest
