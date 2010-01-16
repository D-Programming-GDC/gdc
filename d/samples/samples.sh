#!/bin/bash
#Things to add:
#-Check to make sure you can use command gdc
#-More error messages
echo -n "Compiling..."
/opt/gdc/bin/gdc hello.d -o hello
echo "Done!"
echo -e "Hello World output:\n"
./hello
echo -e "\nFinished."
