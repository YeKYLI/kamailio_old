# environment

## os ubuntu 20.04

## gcc 9.4.0

apt-get install gcc

## g++ 9.4.0

apt-get install g++

## make 4.2.1 

apt-get install make

## bison 3.5.1

apt-get install bison

## flex 2.6.4

apt-get install flex  

# build

make proper

make \
    PREFIX="/root/kamailio/build/" \
    include_modules="ims_usrloc_pcscf ims_registrar_pcscf ims_ipsec_pcscf dialog ims_dialog xmlrpc db_mysql cdp cdp_avp ims_icscf presence ims_usrloc_scscf ims_registrar_scscf ims_auth ims_isc ims_charging" \ 
    cfg

make all

make install

# run

## docker

sudo docker run -it --privileged  -p 5678:5678 -p 5679:22  -v /dev/log:/dev/log -v /home/ubuntu/test/sipp/:/root/sipp/ kamailio_dev:1.0


# gdb -x .gdb

!pkill kamailio

file src/kamailio

set args "-f" "/root/XG/src/kamailio/test.cfg"

set follow-fork-mode child

break main

run

# kamailio.cfg的运行机制

# sip注册流程怎么去写、、仿照之前的流程写即可

# 代码入库

git clone 

# 直接写注册流程的代码、这里直接仿照之前写的代码即可。。

这里可以用gdb调流程、调试代码

# 
