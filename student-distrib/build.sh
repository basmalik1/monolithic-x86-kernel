#!/bin/bash

while getopts ":q" option; do
   case $option in
      q) # hides all non-warning/error, no gdb
         touch *.c *.h *.S
         touch interrupts/*
         make clean > /dev/null && make dep > /dev/null && touch Makefile.dep && touch bootimg && sudo make > /dev/null
         exit;;
   esac
done


make clean && make dep && sudo make && gdb bootimg -q  
