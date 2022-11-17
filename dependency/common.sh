#!/bin/bash

function ErrorExit()
{
    if [ "$?" != "0" ]; then
        echo $1
        exit 1
    fi
}

cmake --version
CURREND_PWD=`pwd`
