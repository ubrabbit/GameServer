#!/bin/bash



UNAME=`uname -a`
echo $UNAME
if [[ $UNAME =~ "Darwin" ]];then
    echo "mac"

elif [[ $UNAME =~ "Centos" || $UNAME =~ "centos" ]];then
    echo "centos"
    bash install_env_centos.sh

elif [[ $UNAME =~ "Ubuntu" ]];then
    echo "ubuntu"
    bash install_env_ubuntu.sh

fi
