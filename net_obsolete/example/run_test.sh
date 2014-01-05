#! /usr/bin/env bash

python ./client.py 192.168.58.103 8221 8432 100 &
python ./client.py 192.168.58.103 8221 8632 101 &
python ./client.py 192.168.58.103 8221 8832 102 &
python ./client.py 192.168.58.103 8221 9632 103 &


