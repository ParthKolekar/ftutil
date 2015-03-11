make

if [ $? == 0 ]
then
	echo "Success"
	echo "./ftutil -c starts client"
	echo "./ftutil -s starts server"
	echo "for extra functionality, please refer to help"
else
	echo "Something went wrong"
fi
