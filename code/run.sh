#!/bin/sh
set -eu
cd "$(dirname "$0")"
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic scheduler.cpp -o scheduler
./scheduler --test
./scheduler --file data/basic.txt
./scheduler --random --seed 20261108 --summary
