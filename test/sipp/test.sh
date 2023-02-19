sipp -sf UE_MO_RTP.xml -inf regsub.csv -nd -m 10 -aa -l 2 -r 1 -t t1 -i 127.0.0.1   -mi 127.0.0.1 -mp 7000 127.0.0.1:5060  -cid_str '%u-%p@%s' -max_socket 256
#sipp -sf UE_MO_RTP.xml -inf regsub.csv -nd -m 10 -aa -l 2 -r 1 -t un -i 127.0.0.1   -mi 127.0.0.1 -mp 7000 127.0.0.1:5060  -cid_str '%u-%p@%s' -max_socket 256


