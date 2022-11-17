#!/bin/bash

#ps x | grep "test.sh" | grep -v "grep" | awk '{print $1}' | xargs -I{} kill {}
#ps x | grep "./socket_client" | grep -v "grep" | awk '{print $1}' | xargs -I{} kill {}

START_TIME=`date +%s`

PROCESS_NUM=20
for ((I=1;I<=${PROCESS_NUM};I++))
do
    {
        timeout 30 ./socket_client
    }&
done
wait

STOP_TIME=`date +%s`
echo "TIME: $(expr ${STOP_TIME} - ${START_TIME})"
