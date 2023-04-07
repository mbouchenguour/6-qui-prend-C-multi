BEGIN {emplacement=0; min=99; nombre = 0}
{
	nombre = 0;
	for ( i=1; i <= NF; i++){
		if($i == 55)
			nombre += 7;
		else if(!($i % 10))
			nombre += 3;
		else if(!($i % 5))
			nombre += 2;
		else if(!($i % 11))
			nombre += 5;
		else
			nombre += 1;
	}
	if(nombre < min){
		emplacement = NR;
		min = nombre;
	}
		
	    
}

END{print emplacement}
