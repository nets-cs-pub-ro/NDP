{
    fname = "tmp/t/" $5 ".dat"
    if (e[$5]==1){
	print $0 >> fname
    }
    else {
	print $0 > fname
	e[$5] = 1;
    }	
}
