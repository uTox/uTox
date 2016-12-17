#!/usr/bin/zsh

#TODO use the author length + msglength from hexdump to gener
OFFSET=(14 150 15 22 110 24 17 47 98)

HEADER='1/8 "Vers  : %u \n" "    Time  : " 1/8 "%12u \n" 1/8  "    Author: %12u \n" 1/8 "    MsgLen: %12u \n" 1/1 "    Flags : %12u \n" 1/1 "    MSGTYP: %12u \n" 1/6 " \n"'
I=1
START=0
NUM=40

for len in $OFFSET; do
  hexdump -s $START -n $NUM -v -e $HEADER log2.txt
  ((START=START+40))
  ((START=START+len))
  ((I++))
done

hexdump -s $START -n $NUM -v -e $HEADER log2.txt
((START=START+40))
((START=START+len))
((I++))
