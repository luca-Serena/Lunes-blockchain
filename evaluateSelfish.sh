#!/usr/bin/env bash

file='selfish-res.txt'
if [ -f $file ] ; then
    rm $file
fi

for i in outputs/*test-Selfish-*.txt ;do	
    rate=$(echo "$i" | cut -d'-' -f 3 | cut -d'.' -f1)
    if [ "$rate" -lt 10 ] ; then
    	rate="0$rate"
    fi
    if [ "$rate" -lt 100 ] ; then
        rate="0$rate"
    fi
    succed=$(cat $i | grep -c "possible")
	succed=$(cat $i | tail -8 | head -1 | cut -d "," -f6)
	if [ -z "$succed" ] ; then
		succed=$(cat $i | tail -8 | head -1 | cut -d "," -f4)
    fi

    echo "$rate $succed" >> "selfish-res.txt"
done
sort $file > "tmp.txt"
cat "tmp.txt" > $file
rm "tmp.txt" 