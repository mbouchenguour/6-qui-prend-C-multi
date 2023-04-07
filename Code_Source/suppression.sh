#!/bin/bash
nbJoueur=$1;
nbRobots=$2;

for (( i=1; i<=$nbJoueur; i++ ))
do  
  rm carteJoueur$i;
done

for (( i=(($nbJoueur+1)); i<=$nbJoueur+$nbRobots; i++ ))
do  
  rm carteRobot$i;
done

rm plateau;
rm ordreJoueur;
vim log -c "hardcopy > log.ps | q"; ps2pdf log.ps;
rm log.ps;
rm log;





