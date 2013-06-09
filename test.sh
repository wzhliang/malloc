#!/bin/sh

for i in $(seq 10000); do
    if ! ./malloc; then
        exit
    fi
    echo "Test $i finished"
done
