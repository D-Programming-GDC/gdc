#!/bin/bash
#Things to add:
#-Check to make sure you can use command gdc
#-More error messages

ls -1 *.d | while read prog
do	
	echo -n "Compiling ${prog}..."
   /opt/gdc/bin/gdc ${prog} -o ${prog}.out
   echo "Done!"
   echo -e "${prog} output:\n"
   ./${prog}.out
   echo "---------"
done
