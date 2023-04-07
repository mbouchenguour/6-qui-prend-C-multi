#!/bin/bash

compteur=1;
carteJoueur=$1;
max=0;
emplacement=0;
nbCarte=0;
nbTete=0;

while read X
do
     
    temp=$(echo $X | awk -v carteJoueur=$carteJoueur -f ranger.awk);
    if (( $max < $temp )) 
    then
        max=$temp;
        emplacement=$compteur;
    fi
    ((compteur++))
done < plateau;

if (( $emplacement != 0))
then
    nbCarte=$(echo $(awk 'NR=='$emplacement' {print NF}' plateau));
    nbTete=$(echo $(awk -f nbTete.awk -v emplacement=$emplacement plateau));
    $(awk -f placementCarte.awk -v carte=$carteJoueur -v emplacement=$emplacement plateau > plateau2 && mv plateau2 plateau);
fi

echo $emplacement;
echo $nbCarte;
echo $nbTete;


