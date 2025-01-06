#!/bin/bash

while (( $# ))
do
    cd "$1" || exit 1
    gnome-terminal -- bash -c "${2:-}; exec bash"
    shift 2
    sleep 1
done

wait -n

exit 0
