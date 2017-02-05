#!/bin/bash

# Choose which optimization task to run
task="kick" # "walk"

# This script runs the simspark soccer simulator and an agent 

# Set the agent and monitor port randomly, to allow for multiple agents per machine
# Note: $RANDOM returns a value from 0 to 32767, ports <= 1024 are reserved for root 
# TODO: Instead of picking random ports purposely bind to available ports
SPARK_SERVERPORT=3200
SPARK_AGENTPORT=3100

echo -n "It is: "
date
echo -n "and I am on: "
hostname
echo "Agent port: $SPARK_AGENTPORT, Monitor port: $SPARK_SERVERPORT"

rcssserver3d --agent-port $SPARK_AGENTPORT --server-port $SPARK_SERVERPORT &
PID=$!
rcssmonitor3d  --server-port $SPARK_SERVERPORT &
#To view task while it runs uncomment the following line
#<roboviz_start_script> --serverPort=$SPARK_SERVERPORT &

sleep 5
TYPE=0
OUTPUT_FILE=output.txt


if [ $task == "kick" ]
then
    # FixedKick optimization task 
    ./agentspark --unum 2 --type $TYPE --paramsfile ./paramfiles/defaultParams.txt --paramsfile ./paramfiles/defaultParams_t$TYPE.txt --experimentout $OUTPUT_FILE --optimize fixedKickAgent --port $SPARK_AGENTPORT --mport $SPARK_SERVERPORT & 
fi

if [ $task == "walk" ]
then
# WalkForward optimization task
./agentspark --unum 2 --type $TYPE --paramsfile $DIR_SCRIPT/../paramfiles/defaultParams.txt --paramsfile $DIR_SCRIPT/../paramfiles/defaultParams_t$TYPE.txt --paramsfile $PARAMS_FILE --experimentout $OUTPUT_FILE --optimize walkForwardAgent --port $SPARK_AGENTPORT --mport $SPARK_SERVERPORT &
fi

AGENTPID=$!
sleep 3

maxWaitTimeSecs=300
total_wait_time=0

while [ ! -f $OUTPUT_FILE ] && [ $total_wait_time -lt $maxWaitTimeSecs ]
do 
  sleep 1
  total_wait_time=`expr $total_wait_time + 1`
done 

if [ ! -f $OUTPUT_FILE ]
then
  echo "Timed out while waiting for script to complete, current wait time is $total_wait_time seconds."
else
  echo "Completed with a wait time of $total_wait_time seconds."
fi

echo "Killing Simulator"
kill -s 2 $PID
echo "Killing Agent"
kill -s 2 $AGENTPID

sleep 2
echo "Finished"
