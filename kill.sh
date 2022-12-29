ps -ef | grep kama | awk ' { print $2 } ' | xargs kill -9
