ps -ef | grep kama | awk ' { print $2 } ' | xargs sudo kill -9

sudo make all

sudo make install 

/build/sbin/kamailio -f doc/ims/test.cfg

python doc/ims/test.py 
