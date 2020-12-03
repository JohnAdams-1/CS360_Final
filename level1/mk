#!/bin/bash

if [ -z "$1" ]
then
    echo "[mk]: Must pass a disk name to execute."
    exit -1
fi

cp $1 "$1_cpy"
echo "[mk]: Copied $1 to $1_cpy"

echo "[mk]: Compiling project..."
gcc -g main.c -o project

echo "[mk]: Executing compiled project with $1_cpy"
./project "$1_cpy"

echo "[mk]: Removing $1_cpy and project"
rm "$1_cpy" project
