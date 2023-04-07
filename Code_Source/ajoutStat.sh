#!/bin/bash
pseudo=$1;
gagne=$2;
points=$3;

if grep -q "^$pseudo" -w stats
then
    $(echo awk -f majStats.awk -v pseudo=$pseudo gagne=$gagne points=$points stats) > stats2 && mv stats2 stats;
else
    if (($gagne))
    then
    	sed -i '$a'"$pseudo\t\t\t1\t\t1\t\t0\t\t$points\t\t$points" stats;
    else
    	sed -i '$a'"$pseudo\t\t\t1\t\t0\t\t1\t\t$points\t\t$points" stats;
    fi
    
fi
