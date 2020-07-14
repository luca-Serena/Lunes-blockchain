#!/usr/bin/env bash

file="51-res.txt"
if [ -f $file ] ; then
    rm $file
fi

for i in outputs/*test*.txt ;do	
    #take all the last blocks, and find the one in greates position
    longest=$(cat $i | tail -n1000 | grep  "BR: " | cut -d "," -f6 | sort | tail -n1) #  $longest = position of the head of the main chain
    long=",$longest"                                                                  #  adding the comma for the research
    echo "$i $long"
                                                                                      # ID of the main chain head block
    id=$(cat $i | tail -n1000 | grep  "BR:"| cut -d "," -f5-6 | grep "$long" | cut -d "," -f1 |  tail -n1)
    rate=$(echo "$i" | cut -d'-' -f 3 | cut -d'.' -f1) #hashrate of the attacker
    attacker=37                                        #id of the node chosen as attacker
    chain=()
    id="_$id"                                          #underscore characted added to easily recognize the ID
    blocks= cat $i | grep "BS" > temp                  #shrinking the log to just BS records
    attackerTotals=$(cat "temp" | cut -d "," -f2 | grep $attacker | wc -l)      #total blocks mined by the attacker

    while [[ $id -ne "_-1" ]] ;do                      #iterating through the chain, from the last block to the root
    	tuple=$(cat "temp" | grep $id | head -n1)      #recovering the tuple of given ID
    	id=$(echo $tuple | cut -d "," -f4)
    	id="_$id"
    	chain[$j]=$(echo $tuple | cut -d "," -f2)      #save the creator of the block in the chain
    	j=$((j+1))
	done

	count=0
	for k in ${chain[@]}; do 
		if [ $k -eq $attacker ] ; then    
			count=$((count+1))                         #at the end it will represent the number of blocks in the chain mined by the attacker
		fi
	done
	echo $count
    if [ "$rate" -lt 10 ] ; then                       
        rate="0$rate"
    fi
    if [ "$rate" -lt 100 ] ; then
        rate="0$rate"
    fi
    echo "$rate $count $longest" >> $file              #hashrate of the attacker  -- number of blocks in the main chain mined by the attacker --  length of the longest chain
    #echo "$rate $count $attackerTotals" >> $file      #hashrate of the attacker  -- number of blocks in the main chain mined by the attacker --  number of blocks mined by the attacker
done
sort $file > "tmp.txt"
cat "tmp.txt" > $file
rm "tmp.txt" 
rm "temp"

