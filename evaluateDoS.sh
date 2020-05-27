#!/usr/bin/env bash

file='dos-res.txt'
if [ -f $file ] ; then
    rm $file
fi

for i in outputs/*test-dos-*.txt ;do	
    rate=$(echo "$i" | cut -d'-' -f 3 | cut -d'.' -f1)
    if [ "$rate" -lt 10 ] ; then
    	rate="0$rate"
    fi
    received=$(cat $i | grep -c "TR:")                  #total transactions received
    ntests=$(cat $i | grep -c "victim:")                #total tests/epochs done
    receivedAverage=$(($received / $ntests))            #average transactions received per epoch
    receivedP=$(($receivedAverage * 100))
    attackers=$(cat $i| head -n 1 | cut -d " " -f1)     #number of attackers in the execution
    nodes=$(cat $i| head -n 1 | cut -d " " -f2)         #total number of nodes of the network
    potential_receivers=$(($nodes - $attackers - 1))    #nodes that potentially can receive the transaction
    percentage=$(echo $receivedP / $potential_receivers | bc -l)
    #percentage=$(echo $receivedP / $ntests / $potential_receivers * 100 | bc -l)
    #percentage=$(( bc <<< 'scale=2; $received / ($nodes - $attackers - 1) * 100' ))
    echo "$rate $received $ntests $receivedAverage $percentage" >> "dos-res.txt"
done
sort $file > "tmp.txt"
cat "tmp.txt" > $file
rm "tmp.txt" 
