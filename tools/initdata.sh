#!/bin/bash

until [[ `echo x$1` == 'x' ]]
do
    case $1 in
	-h) shift;
	    echo 'Initial data file generator, read data file (mass, position[3], velocity[3] per line; 7 columns)';
	    echo 'Usage: petar.init [options] [data file name]';
	    echo 'Options:';
	    echo '  -f: output file name (default: intput file name + ".input")';
	    echo '  -i: skip rows number (default: 0)';
	    echo '  -s: stellar evolution columns: base; bse; no (default: no)';
	    exit;;
	-f) shift; fout=$1; shift;;
	-i) shift; igline=$1; shift;;
	-s) shift; seflag=$1; shift;;
	*) fname=$1;shift;;
    esac
done

if [ ! -e $fname ] | [ -z $fname ] ; then
    echo 'Error, file name not provided' 
    exit
fi
[ -z $fout ] && fout=$fname.input
[ -z $igline ] && igline=0
[ -z $seflag ] && seflag=no

echo 'Transfer "'$fname$'" to PeTar input data file "'$fout'"'
echo 'Skip rows: '$igline
echo 'Add stellar evolution columns: '$seflag

n=`wc -l $fname|cut -d' ' -f1`
n=`expr $n - $igline`
if [[ $seflag != 'no' ]]; then
    if [[ $seflag == 'base' ]]; then
	read -p "Stellar radius (0): " radius
	[ -z $radius ] && radius=0
	awk -v n=$n -v ig=$igline 'BEGIN{print 0,n,0} {if(NR>ig) print $LINE,'$radius',0,0,0,0,0,NR-ig,0,0,0,0,0,0,0,0,0,0}' $fname >$fout
    elif [[ $seflag == 'bse' ]]; then
	read -p 'mass scale from NB to Msun (m[Msun] = m[NB]*mscale) (1.0): ' mscale
	[ -z $mscale ] && mscale=1.0
	awk -v n=$n -v ig=$igline -v ms=$mscale -v rs=$rscale 'BEGIN{print 0,n,0} {if(NR>ig) print $LINE, 0,0,0,0,0, 1,$1*ms,$1*ms,0.0,0.0,0.0,0.0, 0.0,NR-ig,0,0,0,0,0,0,0,0,0,0}' $fname >$fout
    else
	echo 'Error: unknown option for stellar evolution: '$seflag
    fi
else
    awk -v n=$n -v ig=$igline 'BEGIN{print 0,n,0} {if(NR>ig) print $LINE,0,NR-ig,0,0,0,0,0,0,0,0,0,0}' $fname >$fout
fi
