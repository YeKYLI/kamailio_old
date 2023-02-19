

# REGISTER 

TCP

Status-Line: SIP/2.0 200 OK
Request-Line: REGISTER sip:one.att.net SIP/2.0


From: <sip:13800011103@one.att.net>;tag=4VWkdTHOF9.Odc
From: <sip:13800011103@one.att.net>;tag=4VWkdTHOF9.Odc

To: <sip:13800011103@one.att.net>;tag=5a8841e8-126bff1a-2bd25-7fc604e51418-6e78580a-13c4-7217
To: <sip:13800011103@one.att.net>

Call-ID: XP0VHy42Rr8GeaFMRdy@182.0.123.58
Call-ID: XP0VHy42Rr8GeaFMRdy@182.0.123.58

CSeq: 1 REGISTER
CSeq: 1 REGISTER

Via: SIP/2.0/TCP 182.0.123.58:5060;branch=z9hG4bKLeyx3ZsUUnRM5mcCeQo69uP
Via: SIP/2.0/TCP 182.0.123.58:5060;branch=z9hG4bKLeyx3ZsUUnRM5mcCeQo69uP

Contact: <sip:13800011103@182.0.123.58:5060>;expires=600000;audio;video;
+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";+g.3gpp.smsip="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";
+sip.instance="<urn:gsma:imei:86539

Contact: <sip:13800011103@182.0.123.58:5060>;+g.3gpp.smsip;
+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";
audio;video;
+g-3gpp-ps-data-off="inactive";
+sip.instance="<urn:gsma:imei:86539405-000273-0>"

--------------------------------------------------------------------------------------------------

// SUBSCRIBE
// UDP
// 添加一下TO的TAG
// 改变一下CONTACHT的内容

Status-Line: SIP/2.0 200 OK
SUBSCRIBE sip:13800011103@one.att.net SIP/2.0

From: <sip:13800011103@one.att.net>;tag=ZkTGAlN2H5
From: <sip:13800011103@one.att.net>;tag=ZkTGAlN2H5

To: <sip:13800011103@one.att.net>;tag=7c00dc87-28bf6e04-2bd25-7fc604e516c8-6e78580a-13c4-7217
To: <sip:13800011103@one.att.net>

Call-ID: Nd3Rs8wJK6F2aG4ryvU9@182.0.123.58
Call-ID: Nd3Rs8wJK6F2aG4ryvU9@182.0.123.58

CSeq: 10052 SUBSCRIBE
CSeq: 10052 SUBSCRIBE

Expires: 600000
Expires: 600000

Via: SIP/2.0/UDP 182.0.123.58:5060;branch=z9hG4bKB18mqR9Q7jiQfecpW0kA3
Via: SIP/2.0/UDP 182.0.123.58:5060;branch=z9hG4bKB18mqR9Q7jiQfecpW0kA3

Contact: <sip:13800011103@one.att.net>
Contact: <sip:13800011103@182.0.123.58:5060>

Content-Length: 0
Content-Length: 0

-------------------------------------------------------------------------------------------

// notify
// UDP
// 基本上根据订阅信息构建的
// 修改Expire的值

NOTIFY sip:13800011103@182.0.123.58:5060 SIP/2.0

From: <sip:13800011103@one.att.net>;tag=7c00dc87-28bf6e04-2bd25-7fc604e516c8-6e78580a-13c4-7217

To: <sip:13800011103@one.att.net>;tag=ZkTGAlN2H5

Call-ID: Nd3Rs8wJK6F2aG4ryvU9@182.0.123.58

CSeq: 1 NOTIFY

Via: SIP/2.0/UDP 10.88.120.110:5060;branch=z9hG4bK-5a9817e2-2bd25-ab2da0e-7fc605696238

Subscription-State: active;expires=599999

Event: reg

Max-Forwards: 70

Contact: <sip:13800011103@one.att.net>

Content-Length: 0

------------------------------------------------------------------------------------------------

// INVITE
// TCP 是这里边最难的。。。。。
// 但是基本也是维持了一个TCP会话
// 在这里正好也挺好做的，，这里直接进程间通信，，阻塞在TCP进程里，直至完成全部业务

// INVITE STGE 1 | 1 发送 INVITE到 AF, AF立刻相应TRYING, 同时给 2 发送INVITE
// 第二阶段 | 监听2， 的响应， 100 TRYING ，不发，如果180 ring 就转发传给 1
// 第三阶段 | 如果200 OK，AF发送200 OK给1                 | 此时TCP服务结束
// 第四阶段 | UDP进程监听到1的 ACK， 然后转发给 2


------------------------------
// INVITE STGE 1

// to UE 2
// 100 Trying
// origin 

INVITE sip:13800011103@182.0.123.58:5060 SIP/2.0
Status-Line: SIP/2.0 100 Trying
INVITE tel:13800011103;phone-context=one.att.net SIP/2.0

From：<sip:13800011101@one.att.net>;tag=X6E3R9v
From：<sip:13800011101@one.att.net>;tag=X6E3R9v
From: <sip:13800011101@one.att.net>;tag=X6E3R9v

To: <sip:13800011103@182.0.123.58:5060>
To: <tel:13800011103;phone-context=one.att.net>
To: <tel:13800011103;phone-context=one.att.net>

Call-ID: 2eefbb81-fc08a76-2bd31-7fc60491c140-6e78580a-13c4-7217
Call-ID: 0rSzxKYZqGFRahidYb8I3Dc3doGY@182.0.123.56
Call-ID: 0rSzxKYZqGFRahidYb8I3Dc3doGY@182.0.123.56

CSeq: 1 INVITE
CSeq: 33302 INVITE
CSeq: 33302 INVITE

Via: SIP/2.0/UDP 10.88.120.110:5060;branch=z9hG4bK-65738974-2bd31-ab308a7-7fc605696d50
Via：SIP/2.0/TCP 182.0.123.56:5060;branch=z9hG4bKb4MiHkghekvemEcWZotV2Pm9uu.
Via：SIP/2.0/TCP 182.0.123.56:5060;branch=z9hG4bKb4MiHkghekvemEcWZotV2Pm9uu.

Max-Forwards: 70
Max-Forwards: 70

Accept: application/sdp, application/3gpp-ims+xml
Accept: application/sdp, application/3gpp-ims+xml

P-Preferred-Service: urn:urn-7:3gpp-service.ims.icsi.mmtel
P-Preferred-Service: urn:urn-7:3gpp-service.ims.icsi.mmtel

Accept-Contact: *;+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";video
Accept-Contact: *;+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";video

Allow: INVITE,CANCEL,BYE,ACK,REFER,NOTIFY,MESSAGE,INFO,PRACK,UPDATE,OPTIONS
Allow ：同上

P-Access-Network-Info: 3GPP-NR-TDD;utran-cell-id-3gpp=0010100313200001c001
同上

P-Early-Media: supported
同上

P-Preferred-Identity: <sip:13800011101@one.att.net>
same

Contact: <sip:10.88.120.110:5060;transport=UDP>
Contact: <sip:10.88.120.110:5060;transport=UDP>
<sip:13800011101@182.0.123.56:5060>;+g.3gpp.srvcc-alerting;+g.3gpp.mid-call;+g.3gpp.ps2cs-srvcc-orig-pre-alerting;+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";audio;video

Content-Type: application/sdp
same

Content-Length: 1466
Content-Length: 0
Content-Length: 1466

Session Description Protocol
same

------------------------------

// INVITE STGE 2

// 两个进程IPC通信

// 收到UE2的100Tring 不为所动
// 收到UE2 180 TRING 转发给 UE1, 依旧不为所动
// 收到UE2 200 OK后，转发给UE1 200OK
// 收到U1 ACK 后，转发给UE2 ACK，




------------------------------


// BYE
// TCP 这里正好是简单的流程

// 第一阶段 | 1发送BYE给AF， AF收到后给2发BYE
// 第二阶段 | 2发送200OK后，AF收到给1发送200 OK





