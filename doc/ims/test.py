import socket
import sys



serverAddr = "192.168.0.1"
serverPort = 5060

REGISTER = """REGISTER sip:one.att.net SIP/2.0
Via: SIP/2.0/TCP 182.0.123.58:5060;branch=z9hG4bKLeyx3ZsUUnRM5mcCeQo69uP
Max-Forwards: 70
From: <sip:13800011103@one.att.net>;tag=4VWkdTHOF9.Odc
To: <sip:13800011103@one.att.net>
Call-ID: XP0VHy42Rr8GeaFMRdy@182.0.123.58
CSeq: 1 REGISTER
Contact: <sip:13800011103@182.0.123.58:5060>;+g.3gpp.smsip;+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";audio;video;+g-3gpp-ps-data-off="inactive";+sip.instance="<urn:gsma:imei:86539405-000273-0>"
Allow: INVITE, CANCEL, BYE, ACK, REFER, NOTIFY, MESSAGE, INFO, PRACK, UPDATE, OPTIONS
Authorization: Digest username="13800011103@private.att.net", realm="one.att.net", nonce="", uri="sip:one.att.net", response=""
Expires: 600000
Proxy-Require: sec-agree
Require: sec-agree
Supported: path, sec-agree
P-Access-Network-Info: 3GPP-NR-TDD;utran-cell-id-3gpp=0010100313200001c001
Security-Client: ipsec-3gpp;alg=hmac-md5-96;ealg=null;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-md5-96;ealg=des-ede3-cbc;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-md5-96;ealg=aes-cbc;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-sha-1-96;ealg=null;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-sha-1-96;ealg=des-ede3-cbc;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024,ipsec-3gpp;alg=hmac-sha-1-96;ealg=aes-cbc;spi-c=1262;spi-s=1263;port-c=7023;port-s=7024
Content-Length: 0

"""

subscribe = """SUBSCRIBE sip:13800011103@one.att.net SIP/2.0
Via: SIP/2.0/UDP 182.0.123.58:5060;branch=z9hG4bKB18mqR9Q7jiQfecpW0kA3
Max-Forwards: 70
From: <sip:13800011103@one.att.net>;tag=ZkTGAlN2H5
To: <sip:13800011103@one.att.net>
Call-ID: Nd3Rs8wJK6F2aG4ryvU9@182.0.123.58
CSeq: 10052 SUBSCRIBE
Contact: <sip:13800011103@182.0.123.58:5060>
Accept: application/reginfo+xml
Event: reg
Expires: 600000
Route: <sip:10.88.120.110:5060;lr>
P-Access-Network-Info: 3GPP-NR-TDD;utran-cell-id-3gpp=0010100313200001c001
P-Preferred-Identity: <sip:13800011103@one.att.net>
Content-Length: 0

"""

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


message = format(REGISTER)
tcpSock.send(message)

message = format(subscribe)
udpSock.sendto(message, server_address)




tcpSock.close()
