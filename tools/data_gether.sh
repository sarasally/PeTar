#!/bin/bash

suffixes='esc group sse bse mosse mobse'

until [[ `echo x$1` == 'x' ]]
do
    case $1 in
	-h) shift;
	    echo 'Gether separated output data due to multiple MPI processes (file suffixes: '$suffixes')';
	    echo 'Usage: petar.data.gether [options] [data filename prefix]';
	    echo '       data filename prefix is defined by "petar -f", defaulted case is "data".'
	    echo 'Options:';
	    echo '  -f: output filename prefix (default: data filename prefix)';
	    echo '  -n: MPI processes number (default: auto detect)';
	    echo '  -i: before remove existing gethered files, ask first (default: no ask)';
	    exit;;
	-f) shift; fout=$1; shift;;
	-n) shift; nmpi=$1; shift;;
	-i) rmi=1; shift;;
	*) fname=$1;shift;;
    esac
done

if [ ! -e $fname ] | [ -z $fname ] ; then
    echo 'Error, file name not provided' 
    exit
fi
[ -z $fout ] && fout=$fname

echo 'data filename prefix: '$fout

flen=`expr ${#fname} + 2`
ls|egrep '^'$fname'.[0-9]+$' |sort -n -k 1.${flen} >$fout.snap.lst

for s in $suffixes
do
    file=$fname.$s
    echo $file
    if [ -e $file.0 ]; then
	if [ -e $fout.$s ]; then
	    if [ -z $rmi ]; then
		rm -f $fout.$s
	    else
		rm -i $fout.$s
	    fi
	fi
	
	echo 'gether '$file'.* to '$fout.$s
	if [ ! -z $nmpi ]; then
	    nend=`expr $nmpi - 1`
	    lst=`seq 0 $nend`
	    for i in $lst
	    do
		cat $file.$i >>$fout.$s
	    done
	else
	    flist=`ls |egrep $file'.[0-9]+$'`
	    echo $flist
	    cat $flist >$fout.$s
	fi
    fi
done

if [ -e $fout.group ]; then
    nmax=`awk '{print $2'} $fout.group|sort |tail -1`
    if [[ x$nmax != x ]]; then
	for ((i=2;i<=$nmax;i=i+1))
	do
	    echo 'get n_member= '$i' in '$fout.group' to '$fout.group.n$i
	    awk -v n=$i '{if ($2==n) print $LINE}' $fout.group >$fout.group.n$i
	done	    
    fi
fi

sse_opt='.sse .mosse'
for s in $sse_opt
do
    if [ -e $fout$s ]; then
	echo 'get '$s' type_change, sn_kick'
	egrep '^Type_change ' $fout$s |sed 's/Type_change//g' >$fout$s.type_change
	egrep '^SN_kick ' $fout$s |sed 's/SN_kick//g' >$fout$s.sn_kick
    fi
done

bse_opt='.bse .mobse'
for s in $bse_opt
do
    if [ -e $fout$s ]; then
	echo 'get '$s' type_change, sn_kick, dynamic_merge'
	egrep '^Dynamic_merge' $fout$s |sed 's/Dynamic_merge://g' >$fout$s.dynamic_merge.tmp
	awk '{if (NF==45) {for (i=1; i<=5; i++) printf("%s ", $i); printf("0 0 0 "); for (i=6; i<=NF; i++) printf("%s ", $i); printf("\n");} else print $LINE}' $fout$s.dynamic_merge.tmp > $fout$s.dynamic_merge
	rm -f $fout$s.dynamic_merge.tmp
	egrep '^SN_kick' $fout$s |sed 's/SN_kick//g' >$fout$s.sn_kick
	egrep -v '^(Dynamic_merge|SN_kick)' $fout$s |awk '{for (i=2;i<=NF;i++) printf("%s ", $i); printf("\n")}' >$fout$s.type_change
    fi
done
