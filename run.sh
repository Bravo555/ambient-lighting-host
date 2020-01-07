#!/bin/sh

g++ main.cpp -Wall `pkg-config --cflags --libs x11` -ggdb3 && ./a.out
