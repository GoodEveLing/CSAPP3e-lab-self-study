#!/bin/bash

index=$1

echo -e "---------------test tsh-------------\n"

make test$index

echo -e "---------------test tshref-------------\n"

./sdriver.pl -t trace$index.txt -s ./tsh -a "-p"