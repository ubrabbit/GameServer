#!/bin/bash

#ps x | grep "test.sh" | grep -v "grep" | awk '{print $1}' | xargs -I{} kill {}
#ps x | grep "./socket_client" | grep -v "grep" | awk '{print $1}' | xargs -I{} kill {}

START_TIME=`date +%s`

COUNT=$1
if [ -z "${COUNT}" ]; then
    COUNT=2
fi

echo -n "" > statistics.txt

PROCESS_NUM="${COUNT}"
for ((I=1;I<=${PROCESS_NUM};I++))
do
    {
        timeout 30 ./socket_client
    }&
done
wait

STOP_TIME=`date +%s`
echo "TIME: $(expr ${STOP_TIME} - ${START_TIME})"

python3 statistics.py
