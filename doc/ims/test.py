import socket
import sys



serverAddr = "192.168.0.1"
serverPort = 5060

REGISTER = """REGISTER sip:one.att.net SIP/2.0\r\n\
Via: SIP/2.0/TCP 182.0.123.58:5060;branch=z9hG4bKLeyx3ZsUUnRM5mcCeQo69uP\r\n\
Max-Forwards: 70\r\n\
From: <sip:13800011103@one.att.net>;tag=4VWkdTHOF9.Odc\r\n\
To: <sip:13800011103@one.att.net>\r\n\
Call-ID: XP0VHy42Rr8GeaFMRdy@182.0.123.58\r\n\
CSeq: 1 REGISTER\r\n\
Contact: <sip:13800011103@182.0.123.58:5060>;+g.3gpp.smsip;+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";audio;video;+g-3gpp-ps-data-off="inactive";+sip.instance="<urn:gsma:imei:86539405-000273-0>"\r\n\
Allow: INVITE, CANCEL, BYE, ACK, REFER, NOTIFY, MESSAGE, INFO, PRACK, UPDATE, OPTIONS\r\n\
Authorization: Digest username="13800011103@private.att.net", realm="one.att.net", nonce="", uri="sip:one.att.net", response=""\r\n\
Expires: 600000\r\n\
Proxy-Require: sec-agree\r\n\
Require: sec-agree\r\n\
Supported: path, sec-agree\r\n\
P-Access-Network-Info: 3GPP-NR-TDD;utran-cell-id-3gpp=0010100313200001c001\r\n\
Security-Client: ipsec-3gpp;alg=hmac-md5-96;ealg=null;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-md5-96;ealg=des-ede3-cbc;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-md5-96;ealg=aes-cbc;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-sha-1-96;ealg=null;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-sha-1-96;ealg=des-ede3-cbc;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-sha-1-96;ealg=aes-cbc;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024\r\n\
Content-Length: 0\r\n\r\n"""

subscribe = """SUBSCRIBE sip:13800011103@one.att.net SIP/2.0\r\n\
Via: SIP/2.0/UDP 182.0.123.58:5060;branch=z9hG4bKB18mqR9Q7jiQfecpW0kA3\r\n\
Max-Forwards: 70\r\n\
From: <sip:13800011103@one.att.net>;tag=ZkTGAlN2H5\r\n\
To: <sip:13800011103@one.att.net>\r\n\
Call-ID: Nd3Rs8wJK6F2aG4ryvU9@182.0.123.58\r\n\
CSeq: 10052 SUBSCRIBE\r\n\
Contact: <sip:13800011103@182.0.123.58:5060>\r\n\
Accept: application/reginfo+xml\r\n\
Event: reg\r\n\
Expires: 600000\r\n\
Route: <sip:10.88.120.110:5060;lr>\r\n\
P-Access-Network-Info: 3GPP-NR-TDD;utran-cell-id-3gpp=0010100313200001c001\r\n\
P-Preferred-Identity: <sip:13800011103@one.att.net>\r\n\
Content-Length: 0\r\n\r\n"""

invite = """INVITE tel:13800011103;phone-context=one.att.net SIP/2.0\r\n\
Via: SIP/2.0/TCP 182.0.123.56:5060;branch=z9hG4bKb4MiHkghekvemEcWZotV2Pm9uu.\r\n\
Max-Forwards: 70\r\n\
From: <sip:13800011101@one.att.net>;tag=X6E3R9v\r\n\
To: <tel:13800011103;phone-context=one.att.net>\r\n\
Call-ID: 0rSzxKYZqGFRahidYb8I3Dc3doGY@182.0.123.56\r\n\
CSeq: 33302 INVITE\r\n\
Contact: <sip:13800011101@182.0.123.56:5060>;+g.3gpp.srvcc-alerting;+g.3gpp.mid-call;+g.3gpp.ps2cs-srvcc-orig-pre-alerting;+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";audio;video\r\n\
Accept: application/sdp, application/3gpp-ims+xml\r\n\
Allow: INVITE, CANCEL, BYE, ACK, REFER, NOTIFY, MESSAGE, INFO, PRACK, UPDATE, OPTIONS\r\n\
Route: <sip:10.88.120.110:5060;lr>\r\n\
Supported: timer,replaces,100rel,precondition,histinfo,199\r\n\
P-Access-Network-Info: 3GPP-NR-TDD;utran-cell-id-3gpp=0010100313200001c001\r\n\
P-Preferred-Service: urn:urn-7:3gpp-service.ims.icsi.mmtel\r\n\
P-Early-Media: supported\r\n\
Accept-Contact: *;+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";video\r\n\
P-Preferred-Identity: <sip:13800011101@one.att.net>\r\n\
Content-Type: application/sdp\r\n\
Content-Length: 1466\r\n\

v=0\r\n\
o=SPRD-IMS-UE 678599293 678599293 IN IP4 182.0.123.56\r\n\
s=-\r\n\
c=IN IP4 182.0.123.56\r\n\
b=AS:1002\r\n\
b=RS:8600\r\n\
b=RR:8000\r\n\
t=0 0\r\n\
m=audio 59000 RTP/AVP 96 98 100 110 112 101 103\r\n\
b=AS:42\r\n\
b=RS:600\r\n\
b=RR:2000\r\n\
a=rtpmap:96 EVS/16000\r\n\
a=fmtp:96 br=5.9-24.4;bw=nb-wb;max-red=0\r\n\
a=rtpmap:98 AMR-WB/16000\r\n\
a=fmtp:98 mode-change-capability=2;max-red=0\r\n\
a=rtpmap:100 AMR-WB/16000\r\n\
a=fmtp:100 octet-align=1;mode-change-capability=2;max-red=0\r\n\
a=rtpmap:110 AMR/8000\r\n\
a=fmtp:110 mode-change-capability=2;max-red=0\r\n\
a=rtpmap:112 AMR/8000\r\n\
a=fmtp:112 octet-align=1;mode-change-capability=2;max-red=0\r\n\
a=rtpmap:101 telephone-event/16000\r\n\
a=fmtp:101 0-15\r\n\
a=rtpmap:103 telephone-event/8000\r\n\
a=fmtp:103 0-15\r\n\
a=ptime:20\r\n\
a=maxptime:240\r\n\
a=sendrecv\r\n\
a=curr:qos local none\r\n\
a=curr:qos remote none\r\n\
a=des:qos mandatory local sendrecv\r\n\
a=des:qos optional remote sendrecv\r\n\
m=video 60000 RTP/AVP 104 105\r\n\
b=AS:960\r\n\
b=RS:8000\r\n\
b=RR:6000\r\n\
a=tcap:1 RTP/AVPF\r\n\
a=pcfg:1 t=1\r\n\
a=rtcp-fb:* nack\r\n\
a=rtcp-fb:* nack pli\r\n\
a=rtcp-fb:* ccm tmmbr\r\n\
a=rtcp-fb:* ccm fir\r\n\
a=rtpmap:104 H264/90000\r\n\
a=fmtp:104 profile-level-id=42C01E;packetization-mode=1;sprop-parameter-sets=Z0LAHukDwKJALCIRqA==,aM48gA==\r\n\
a=rtpmap:105 H264/90000\r\n\
a=fmtp:105 profile-level-id=42C01E;packetization-mode=0;sprop-parameter-sets=Z0LAHukDwKJALCIRqA==,aM48gA==\r\n\
a=extmap:5 urn:3gpp:video-orientation\r\n\
a=sendrecv\r\n\
a=curr:qos local none\r\n\
a=curr:qos remote none\r\n\
a=des:qos mandatory local sendrecv\r\n\
a=des:qos optional remote sendrecv\r\n"""

message = format(subscribe)

## udp client
udpSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#udpSock.sendto(message, server_address)

## tcp client
tcpSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address=(serverAddr, serverPort)
tcpSock.connect((serverAddr, serverPort))
#tcpSock.send(message)
#tcpSock.close()


#message = format(REGISTER)
#tcpSock.send(message)

#message = format(subscribe)
#udpSock.sendto(message, server_address)

## test invite
message = format(invite)
tcpSock.send(message)

exit




tcpSock.close()
