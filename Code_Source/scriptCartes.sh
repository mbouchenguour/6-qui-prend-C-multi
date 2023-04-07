#!/bin/bash
nbJoueur=$1;
nbRobots=$2;

> cartes;
> ordreJoueur;

#On regarde si le fichier log n'existe pas
#On fait ceci car ce script est lancé à chaque manche ce qui permet d'éviter de supprimer le fichier log à chaque début de manche
if [ ! -s log ] 
then
  > log;
fi


for i in {1..104}
do  
  echo $i >> cartes;
done

echo "$(sort -R cartes)" > cartes

for (( i=1; i<=$nbJoueur; i++ ))
do  
  head -n 10 cartes > carteJoueur$i;
  echo "$(tail -n +11 cartes)" > cartes;
done

for (( i=(($nbJoueur+1)); i<=$nbJoueur+$nbRobots; i++ ))
do  
  head -n 10 cartes > carteRobot$i;
  echo "$(tail -n +11 cartes)" > cartes;
done

head -n 4 cartes > plateau;

rm cartes;






