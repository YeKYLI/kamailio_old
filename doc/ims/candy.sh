
exit 1

make proper

make \
    PREFIX="/root/kamailio/build/"  \
    include_modules="ims_usrloc_pcscf ims_registrar_pcscf ims_ipsec_pcscf dialog ims_dialog xmlrpc db_mysql cdp cdp_avp ims_icscf presence ims_usrloc_scscf ims_registrar_scscf ims_auth ims_isc ims_charging"  \ 
    cfg

make all

make install

