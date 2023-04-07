{
	if($1 == pseudo)
	{
		$2 += 1;
		if(gagne)
			$3 += 1;
		else
			$4 +=1;
		$5 += points;
		$6 = $5 / $2;
		print $1 "\t\t\t\t" $2 "\t\t"  $3 "\t\t"  $4 "\t\t" $5 "\t\t"  $6;
	
	}
	else
		print $0;


}
