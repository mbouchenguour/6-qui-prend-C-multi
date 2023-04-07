#!/bin/bash

robot=$1;
bool=0;
emplacement=0;
compteur=1;
>tmp;
>tmp2;

#On filtre les cartes qui peuvent être posés, on stocke la valeur de la carte, l'emplacement, le nombre de carte sur l'emplacement et le nombre de tête de boeufs sur l'emplacement
while read carteRobot
do
    carte=0;
    temp=0;
    compteur=1;
    max=0;
    emplacement=0;
    while read cartesPlateau
    do
        temp=$(echo $cartesPlateau | awk -v carteJoueur=$carteRobot -f ranger.awk);
        if (( $max < $temp )) 
        then
            max=$temp;
            carte=$carteRobot;
            emplacement=$compteur;
            nbTete=$(echo $(awk -f nbTete.awk -v emplacement=$emplacement plateau));
        fi
        ((compteur++))
    done < plateau;

    if (( $max != 0))
    then
        echo $carte $emplacement $(awk 'NR=='$emplacement' {print NF}' plateau) $nbTete >> tmp;
    fi;
done < "carteRobot$robot";


#Si on a des cartes à posé
if [ -s tmp ]
then
	#On enleve les cartes qui se pose aux sixième emplacement
	while read carteRobot
	do
	    nbCarte=$(echo $carteRobot | awk '{print $3}');
	    if (( $nbCarte != 5 ))
	    then
		echo $carteRobot >> tmp2;
	    fi
	done < tmp;
	
	#Si on a des cartes qui ne pose pas au sixième emplacement
	if [ -s tmp2 ]
	then
		
		#On récupère la carte la plus sur à poser
		minEcart=120;
		carteFinale=0;
		while read carteRobot
		do
		    compteur=1;
		    emplacement=$(echo $carteRobot | awk '{print $2}');
		    while read cartePlateau
		    do
		    	cPlateauTemp=$(echo $cartePlateau | awk '{print $NF}');
		    	cRobotTemp=$(echo $carteRobot | awk '{print $1}');
		    	
		    	if (( $cRobotTemp <  $cPlateauTemp && $compteur != $emplacement ))
		    	then
		    		temp=$(( $cPlateauTemp - $cRobotTemp ));
		    		if (( $temp < $minEcart ))
		    		then
		    			carteFinale=$cRobotTemp;
		    			minEcart=$temp;
		    		fi
		    	fi
		    ((compteur++))
		    done < plateau;
		done < tmp2;
		
		#Si les cartes sont plus grandes que toutes les cartes, on récupère la plus grande
		if (( $carteFinale == 0 ))
		then
			while read carteRobot
			do
			    max=0;
			    carteRobotTemp=$(echo $carteRobot | awk '{print $1}');
			    if (( $carteRobotTemp > $max ))
			    then
			    	max=$carteRobotTemp;
			    	carteFinale=$carteRobotTemp;
			    fi
			done < tmp2;
		fi
		
	else
		#Sinon on pose la carte qui nous fera récupèrer le moins de tête de boeufs
		while read carteRobot
		do
		    min=120;
		    teteBoeuf=$(echo $carteRobot | awk '{print $4}');
		    if (( $teteBoeuf < $min ))
		    then
		    	carteFinale=$(echo $carteRobot | awk '{print $1}');
		    fi
		done < tmp;
	fi

#sinon
else
	
	#On pose la plus petite carte car plus compliqué a posé
	while read carteRobot
	do
	    min=120;
	    if (( $carteRobot < $min ))
	    then
	    	carteFinale=$carteRobot;
	    fi
	done < "carteRobot$robot";
fi

sed -i '/^'"$carteFinale"'$/d' "carteRobot$robot";

echo $carteFinale;

rm tmp;
rm tmp2;






