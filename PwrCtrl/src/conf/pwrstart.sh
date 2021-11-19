/ubi/local/apps/pwrctrl/pwrproc "/dev/null" &

while true
do
	num=`ps -ef | grep "pwrproc" | grep -v grep | wc -l`
	if [ $num -eq 0 ] ; then
		cd /ubi/local/apps/pwrctrl/
		./pwrproc "/dev/null" &
		cd -
	fi
	sleep 5
done