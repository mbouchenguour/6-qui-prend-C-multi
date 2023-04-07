{
    #Si on est pas dans l'emplacement souhaité, on réaffiche la ligne
    if(NR != emplacement){
        print $0;
    }
    else
    {
    	#Si la ligne à 5 cartes, on affiche seulement la carte
        if(NF == 5){
            print carte;
        }
        else
        {
            #Placement de la carte à la fin de la ligne
            if($NF < carte)
                print $0, carte;
            else
                print carte;		#Supprime la ligne puis affiche la carte, nécessaire lorsqu'un joueur saisie un emplacement
        }
            
    }
}





