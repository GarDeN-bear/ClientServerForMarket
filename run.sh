#!/bin/bash

while (( $# ))
do
    cd "$1" || exit 1
    if [ "$3" == "Server" ]; then
        ${2:-} &
    elif [ "$3" == "Client" ]; then
        ${2:-}
    fi
    shift 3
    sleep 1
done

wait -n
kill $(jobs -p) 2> /dev/null

exit 0
