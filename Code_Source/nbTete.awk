BEGIN { nombre = 0}
{
	if(NR == emplacement)
	{
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
	}
}

END {print nombre}
