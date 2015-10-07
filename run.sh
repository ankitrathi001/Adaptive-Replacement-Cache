#!/bin/bash
gcc -o LRU.o LRU.c -lm
gcc -o ARC.o ARC.c -lm
./ARC.o 1024 P4.lis > P4_1024_ARC.txt
./ARC.o 2048 P4.lis > P4_2048_ARC.txt
./LRU.o 1024 P4.lis > P4_1024_LRU.txt
./LRU.o 2048 P4.lis > P4_2048_LRU.txt