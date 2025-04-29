#!/bin/bash

if [ "$1" = "debug" ] ; then
  echo "building in DEBUG mode.."
  gcc -Wall -std=c99 -DDEBUG_MODE -g -o main ../main.c -lraylib
else 
  echo "building in RELEASE mode.."
  gcc -Wall -std=c99 -g -o main ../main.c -lraylib
fi

echo "done"
