#!/bin/bash


. ./parsec_config.rc

my_lib="/home/gvaumour/Dev/apps/cache-simulator-pintools/obj-intel64/roeval_release.so"
pin_root="/home/gvaumour/Dev/apps/pin-3.2-81205-gcc-linux/pin"
pin_flags="-follow-execv -t "$my_lib


output_dir="/home/gvaumour/Dev/apps/cache-simulator-pintools/output/test/"
output_files="results.out config.ini log.out"

cd ../
make release

test="bodytrack"

a=0;
for i in "${benchs[@]}"
do
	if [ $i == $test ]; then

		cd $output_dir
		if [ ! -d $i ]; then	
			 mkdir $i;
		fi 
	
		cd ${directories[$a]}
		echo "Running "$i
		echo "[---------- Beginning of output ----------]"
		$pin_root $pin_flags -- ${cmd_medium[$a]}
		echo "[---------- End of output ----------]"

		mv $output_files $output_dir/$i/
	
	fi
	

	((a++));
done 

