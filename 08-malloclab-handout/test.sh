#!/bin/bash
    
make clean

make 

if [ $? -ne 0 ]; then
  echo -e "make failed"
  exit 1
fi

./mdriver -V
