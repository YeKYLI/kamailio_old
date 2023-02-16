
exit 1

make proper

# adjust PREFIX to the specific directory
make \
    PREFIX="/home/workspace/kamailio/build/"  \
    include_modules="ims_usrloc_pcscf ims_registrar_pcscf ims_ipsec_pcscf dialog ims_dialog xmlrpc db_mysql cdp cdp_avp ims_icscf presence ims_usrloc_scscf ims_registrar_scscf ims_auth ims_isc ims_charging"  \ 
    cfg

make all 

make install

# TEST command

## mac 
ssh silence@106.54.177.27 'sudo tcpdump -s0 -i any -c   10000 -nn -w - not port 22' | /Applications/Wireshark.app/Contents/MacOS/Wireshark -k -i -


