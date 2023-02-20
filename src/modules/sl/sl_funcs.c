/*
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "../../core/globals.h"
#include "../../core/forward.h"
#include "../../core/dprint.h"
#include "../../core/crypto/md5utils.h"
#include "../../core/msg_translator.h"
#include "../../core/udp_server.h"
#include "../../core/timer.h"
#include "../../core/mem/mem.h"
#include "../../core/mem/shm_mem.h"
#include "../../core/crc.h"
#include "../../core/dset.h"
#include "../../core/data_lump_rpl.h"
#include "../../core/action.h"
#include "../../core/config.h"
#include "../../core/tags.h"
#include "../../core/parser/parse_to.h"
#include "../../core/route.h"
#include "../../core/receive.h"
#include "../../core/onsend.h"
#include "../../core/kemi.h"
#include "sl_stats.h"
#include "sl_funcs.h"
#include "sl.h"


/* to-tag including pre-calculated and fixed part */
static char           sl_tag_buf[TOTAG_VALUE_LEN];
static str            sl_tag = {sl_tag_buf,TOTAG_VALUE_LEN};
/* from here, the variable prefix begins */
static char           *tag_suffix;
/* if we for this time did not send any stateless reply,
 * we do not filter */
static unsigned int  *sl_timeout;

static int _sl_filtered_ack_route = -1; /* default disabled */

static int _sl_evrt_local_response = -1; /* default disabled */

/* send path and flags in 3xx class reply */
int sl_rich_redirect = 0;

extern str _sl_event_callback_fl_ack;
extern str _sl_event_callback_lres_sent;

/*!
 * lookup sl event routes
 */
void sl_lookup_event_routes(void)
{
	_sl_filtered_ack_route=route_lookup(&event_rt, "sl:filtered-ack");
	if (_sl_filtered_ack_route>=0 && event_rt.rlist[_sl_filtered_ack_route]==0)
		_sl_filtered_ack_route=-1; /* disable */

	_sl_evrt_local_response = route_lookup(&event_rt, "sl:local-response");
	if (_sl_evrt_local_response>=0
			&& event_rt.rlist[_sl_evrt_local_response]==NULL)
		_sl_evrt_local_response = -1;
}

/*!
 * init sl internal structures
 */
int sl_startup()
{
	init_tags( sl_tag.s, &tag_suffix,
			"KAMAILIO-stateless",
			SL_TOTAG_SEPARATOR );

	/*timeout*/
	sl_timeout = (unsigned int*)shm_malloc(sizeof(unsigned int));
	if (!sl_timeout)
	{
		LM_ERR("no more free memory!\n");
		return -1;
	}
	*(sl_timeout)=get_ticks_raw();

	return 1;
}

/*!
 * free sl internal structures
 */
int sl_shutdown()
{
	if (sl_timeout)
		shm_free(sl_timeout);
	return 1;
}

/*!
 * get the To-tag for stateless reply
 */
int sl_get_reply_totag(struct sip_msg *msg, str *totag)
{
	if(msg==NULL || totag==NULL)
		return -1;
	calc_crc_suffix(msg, tag_suffix);
	*totag = sl_tag;
	return 1;
}

int magicAddStatusLine(struct sip_msg *msg, void* p)
{
	int len = 0;

	memcpy(p, "SIP/2.0 200 OK", strlen("SIP/2.0 200 OK")); // add status line
	p += strlen("SIP/2.0 200 OK");
	len += strlen("SIP/2.0 200 OK");

	memcpy(p, CRLF, CRLF_LEN );
	p += CRLF_LEN;
	len += CRLF_LEN;

	return len;
}

int magicAddEndHeaderLine(struct sip_msg *msg, void* p)
{
	int len = 0;

	memcpy(p, CRLF, CRLF_LEN );
	p += CRLF_LEN;
	len += CRLF_LEN;

	return len;
}

int magicAddFrom(struct sip_msg *msg, void* p)
{
	int len = 0;

	memcpy(p, msg->from->name.s, msg->from->name.len);
	p += msg->from->name.len;
	len += msg->from->name.len;

	memcpy(p, ": ", 2);
	p += 2;
	len += 2;

	memcpy(p, msg->from->body.s, msg->from->body.len);
	p += msg->from->body.len;
	len += msg->from->body.len;

	memcpy(p, "\r\n", 2 );
	p+=2;
	len += 2;

	return len;
}

str getExpireFromSipMsg(struct sip_msg* msg, void* p)
{
	str output;
	output.s = NULL;
	output.len = 0;

	char* start = msg->buf;
	for(int i = 0; i < msg->len; i ++)
	{
		if(i + 7 >= msg->len) break;
		if(*(start + i) == 'E' && *(start + i + 1) == 'x' && *(start + i + 2) == 'p' 
		&& *(start + i + 3) == 'i' && *(start + i + 4) == 'r' && *(start + i + 5) == 'e'
        && *(start + i + 6) == 's' && *(start + i + 7) == ':')
		{
			output.s = msg->buf + i;
			int end = i + 7;
			while((end - 1) < msg->len)
			{
				if(*(start + end) == '\r' && *(start + end + 1) == '\n')
				{
					break;
				}
				end ++;
			}
			output.len = (end - i);
			
			break;
		}
	}

	return output;
}


str getContentLengthFromSipMsg(struct sip_msg* msg, void* p)
{
	str output;
	output.s = NULL;
	output.len = 0;

	char* start = msg->buf;
	for(int i = 0; i < msg->len; i ++)
	{
		if(i + 14 >= msg->len) break;
		if(*(start + i) == 'C' && *(start + i + 1) == 'o' && *(start + i + 2) == 'n' 
		&& *(start + i + 3) == 't' && *(start + i + 4) == 'e' && *(start + i + 5) == 'n'
        && *(start + i + 6) == 't' && *(start + i + 7) == '-' && *(start + i + 8) == 'L'
		&& *(start + i + 9) == 'e' && *(start + i + 10) == 'n' && *(start + i + 11) == 'g'
		&& *(start + i + 12) == 't' && *(start + i + 13) == 'h' && *(start + i + 14) == ':')
		{
			output.s = msg->buf + i;
			int end = i + 7;
			while((end - 1) < msg->len)
			{
				if(*(start + end) == '\r' && *(start + end + 1) == '\n')
				{
					break;
				}
				end ++;
			}
			output.len = (end - i);
			
			break;
		}
	}

	return output;
}

str getUriFromSipMsg(struct sip_msg *msg, void* p)
{
	str output;
	output.s = NULL;
	output.len = 0;

	char* start = msg->buf;
	for(int i = 0; i < msg->len; i ++)
	{
		if(*(start + i) == ' ')
		{
			output.s = (start + i + 1);

			for(int j = i + 1; j < msg->len; j ++)
			{
				if(*(start + j) == ' ')
				{
					output.len = j - i - 1;
					break;
				}
			}
			break;
		}
	}

	return output;

}

int magicAddTo(struct sip_msg *msg, void* p)
{
	int len = 0;
/*

	memcpy(p, msg->to->name.s, msg->to->len);
	p = p + msg->to->len;
	len = (len + msg->to->len);

	memcpy(p, CRLF, CRLF_LEN );
	p += CRLF_LEN;
	len += CRLF_LEN;


	return len;

*/
	memcpy(p, msg->to->name.s, msg->to->name.len);
	p += msg->to->name.len;
	len += msg->to->name.len;

	memcpy(p, ": ", 2);
	p += 2;
	len += 2;

	memcpy(p, msg->to->body.s, msg->to->body.len);
	p += msg->to->body.len;
	len += msg->to->body.len;

	memcpy(p, CRLF, CRLF_LEN );
	p+=CRLF_LEN;
	len += CRLF_LEN;

	return len;

}

int magicAddToWithTag(struct sip_msg *msg, void* p, char* tag, int tagNum)
{
	int len = 0;

	memcpy(p, msg->to->name.s, msg->to->name.len);
	p += msg->to->name.len;
	len += msg->to->name.len;

	memcpy(p, ": ", 2);
	p += 2;
	len += 2;

	memcpy(p, msg->to->body.s, msg->to->body.len);
	p += msg->to->body.len;
	len += msg->to->body.len;

	memcpy(p, (void*)tag, tagNum);
	p += tagNum;
	len += tagNum;

	memcpy(p, CRLF, CRLF_LEN );
	p+=CRLF_LEN;
	len += CRLF_LEN;

	return len;

}

int magicAddCallId(struct sip_msg *msg, void* p)
{
	int len = 0;

	memcpy(p, msg->callid->name.s, msg->callid->name.len);
	p += msg->callid->name.len;
	len += msg->callid->name.len;

	memcpy(p, ": ", 2);
	p += 2;
	len += 2;

	memcpy(p, msg->callid->body.s, msg->callid->body.len);
	p += msg->callid->body.len;
	len += msg->callid->body.len;

	memcpy(p, CRLF, CRLF_LEN );
	p+=CRLF_LEN;
	len += CRLF_LEN;

	return len;

}

int magicAddCSeq(struct sip_msg *msg, void* p)
{
	int len = 0;

	memcpy(p, msg->cseq->name.s, msg->cseq->name.len);
	p += msg->cseq->name.len;
	len += msg->cseq->name.len;

	memcpy(p, ": ", 2);
	p += 2;
	len += 2;

	memcpy(p, msg->cseq->body.s, msg->cseq->body.len);
	p += msg->cseq->body.len;
	len += msg->cseq->body.len;

	memcpy(p, CRLF, CRLF_LEN );
	p+=CRLF_LEN;
	len += CRLF_LEN;

	return len;

}

str getContactFromSipMsg(struct sip_msg* msg, void* p)
{
	str output;
	output.s = NULL;
	output.len = 0;

	char* start = msg->buf;
	for(int i = 0; i < msg->len; i ++)
	{
		if(i + 7 >= msg->len) break;
		if(*(start + i) == 'C' && *(start + i + 1) == 'o' && *(start + i + 2) == 'n' 
		&& *(start + i + 3) == 't' && *(start + i + 4) == 'a' && *(start + i + 5) == 'c'
		&& *(start + i + 6) == 't' && *(start + i + 7) == ':')
		{
			output.s = msg->buf + i;
			int end = i + 7;
			while((end - 1) < msg->len)
			{
				if(*(start + end) == '\r' && *(start + end + 1) == '\n')
				{
					break;
				}
				end ++;
			}
			output.len = (end - i);
			
			break;
		}
	}

	return output;
}

int magicAddVia(struct sip_msg *msg, void* p)
{
	int len = 0;

	memcpy(p, msg->h_via1->name.s, msg->h_via1->name.len);
	p += msg->h_via1->name.len;
	len += msg->h_via1->name.len;

	memcpy(p, ": ", 2);
	p += 2;
	len += 2;

	memcpy(p, msg->h_via1->body.s, msg->h_via1->body.len);
	p += msg->h_via1->body.len;
	len += msg->h_via1->body.len;

	memcpy(p, CRLF, CRLF_LEN );
	p+=CRLF_LEN;
	len += CRLF_LEN;

	return len;

}

int magicAddRegisterContact(struct sip_msg *msg, void* p)
{
	int len = 0;

	memcpy(p, msg->contact->name.s, msg->contact->name.len);
	p += msg->contact->name.len;
	len += msg->contact->name.len;

	memcpy(p, ": ", 2);
	p += 2;
	len += 2;

	memcpy(p, msg->contact->body.s, msg->contact->body.len);
	p += msg->contact->body.len;
	len += msg->contact->body.len;

	memcpy(p, CRLF, CRLF_LEN );
	p+=CRLF_LEN;
	len += CRLF_LEN;

	return len;

}

/*!
 * helper function for stateless reply
 */
int sl_reply_helper(struct sip_msg* msg, int code, char *reason, str *tag)
{
	str buf = {0, 0};
	str dset = {0, 0};
	struct dest_info dst;
	struct bookmark dummy_bm;
	int backup_mhomed, ret;
	str text;

	int backup_rt;
	struct run_act_ctx ctx;
	run_act_ctx_t *bctx;
	sr_kemi_eng_t *keng = NULL;
	str evname = str_init("sl:local-response");
	struct sip_msg pmsg;

	if (msg->first_line.u.request.method_value==METHOD_ACK)
		goto error;

	if(msg->msg_flags & FL_MSG_NOREPLY) {
		LM_INFO("message marked with no-reply flag\n");
		return -2;
	}

	init_dest_info(&dst);
	if (reply_to_via) {
		if (update_sock_struct_from_via(&dst.to, msg, msg->via1 )==-1)
		{
			LM_ERR("cannot lookup reply dst: %.*s\n",
					msg->via1->host.len, msg->via1->host.s);
			goto error;
		}
	} else update_sock_struct_from_ip(&dst.to, msg);

	/* if that is a redirection message, dump current message set to it */
	if (code>=300 && code<400) {
		dset.s=print_dset(msg, &dset.len, sl_rich_redirect);
		if (dset.s) {
			add_lump_rpl(msg, dset.s, dset.len, LUMP_RPL_HDR);
		}
	}

	text.s = reason;
	text.len = strlen(reason);

	/* add a to-tag if there is a To header field without it */
	// begin our magic
	/*
	if ( code>=180 &&
		(msg->to || (parse_headers(msg,HDR_TO_F, 0)!=-1 && msg->to))
		&& (get_to(msg)->tag_value.s==0 || get_to(msg)->tag_value.len==0) )
	{
		if(tag!=NULL && tag->s!=NULL) {
			buf.s = build_res_buf_from_sip_req(code, &text, tag,
						msg, (unsigned int*)&buf.len, &dummy_bm);
		} else {
			calc_crc_suffix( msg, tag_suffix );
			buf.s = build_res_buf_from_sip_req(code, &text, &sl_tag, msg,
					(unsigned int*)&buf.len, &dummy_bm);
		}
	} else {
		buf.s = build_res_buf_from_sip_req(code, &text, 0, msg,
				(unsigned int*)&buf.len, &dummy_bm);
	}
	if (!buf.s) {
		LM_DBG("response building failed\n");
		goto error;
	}
	*/

	// begin our magic
	// TEST CODE
/*
	void* temp = malloc(1000);
	void* p = temp;
	int len = 0;

	memcpy(p, "SIP/2.0 200 OK", strlen("SIP/2.0 200 OK"));
	p += strlen("SIP/2.0 200 OK");
	len += strlen("SIP/2.0 200 OK");
	memcpy(p, CRLF, CRLF_LEN );
	p+=CRLF_LEN;
	
	buf.s = temp;
	buf.len = strlen(temp);
*/


	void* temp = malloc(1000);
	void* p = temp;
	int len = 0;

// 订阅

// 发起电话

// 拒绝电话

	// 注册
	if (msg->first_line.u.request.method.len == 8)     /* REGISTER */
	{
		// Status-Line
		int ieLen = magicAddStatusLine(msg, p);
		p += ieLen;
		len += ieLen;

		// From
		ieLen = magicAddFrom(msg, p);
		p += ieLen;
		len += ieLen;

		// To
		// 后续tag会根据时间进行绑定，以确保每个tag都不一样
		char* tag = ";tag=5a8841e8-126bff1a-2bd25-7fc604e51418-6e78580a-13c4-7217";
		ieLen = magicAddToWithTag(msg, p, tag, 60);
		p += ieLen;
		len += ieLen;

		// Call-Id
		ieLen = magicAddCallId(msg, p);
		p += ieLen;
		len += ieLen;

		// CSeq
		ieLen = magicAddCSeq(msg, p);
		p += ieLen;
		len += ieLen;
		
		// via
		ieLen = magicAddVia(msg, p);
		p += ieLen;
		len += ieLen;

		// Contact
		str contact = getContactFromSipMsg(msg, p);
		memcpy(p, contact.s, contact.len);
		p += contact.len;
		len += contact.len;
		memcpy(p, ";expires=", 9); // ;
		p += 9;
		len += 9;
		str expires = getExpireFromSipMsg(msg, p); // Expires
		memcpy(p, expires.s + 9, expires.len - 9);
		p += expires.len - 9;
		len += expires.len - 9;
		memcpy(p, "\r\n", 2 );
	    p+=2;
	    len += 2;

		// Content-Length
		str content_Length = getContentLengthFromSipMsg(msg, p);
		memcpy(p, content_Length.s, content_Length.len);
		p += content_Length.len;
		len += content_Length.len;
		memcpy(p, "\r\n", 2 );
	    p+=2;
	    len += 2;


		// END
		memcpy(p, "\r\n", 2 );
	    p+=2;
	    len += 2;
	}

	else if (msg->first_line.u.request.method.len == 9) /* SUBSCRIBE*/
	{
		// Status-Line
		int ieLen = magicAddStatusLine(msg, p);
		p += ieLen;
		len += ieLen;

		// From
		ieLen = magicAddFrom(msg, p);
		p += ieLen;
		len += ieLen;

		// To
		// 后续tag会根据时间进行绑定，以确保每个tag都不一样
		char* tag = ";tag=7c00dc87-28bf6e04-2bd25-7fc604e516c8-6e78580a-13c4-7217";
		ieLen = magicAddToWithTag(msg, p, tag, 60);
		p += ieLen;
		len += ieLen;
		
		// Call-Id
		ieLen = magicAddCallId(msg, p);
		p += ieLen;
		len += ieLen;

		// CSeq
		ieLen = magicAddCSeq(msg, p);
		p += ieLen;
		len += ieLen;

		// Expires
		str expires = getExpireFromSipMsg(msg, p);
		memcpy(p, expires.s, expires.len);
		p += expires.len;
		len += expires.len;
		memcpy(p, "\r\n", 2 );
	    p+=2;
	    len += 2;

		// via
		ieLen = magicAddVia(msg, p);
		p += ieLen;
		len += ieLen;

		// Contact
		memcpy(p, "Contact: <", 10 );
	    p+=10;
	    len += 10;
		str contact = getUriFromSipMsg(msg, p);
		memcpy(p, contact.s, contact.len);
		p += contact.len;
		len += contact.len;
		memcpy(p, ">", 1);
	    p+=1;
	    len += 1;
		memcpy(p, "\r\n", 2 );
	    p+=2;
	    len += 2;

		// Content-Length
		str content_Length = getContentLengthFromSipMsg(msg, p);
		memcpy(p, content_Length.s, content_Length.len);
		p += content_Length.len;
		len += content_Length.len;
		memcpy(p, "\r\n", 2 );
	    p+=2;
	    len += 2;

		// END
		memcpy(p, "\r\n", 2 );
	    p+=2;
	    len += 2;
	}

	else;
	
	buf.s = temp;
	buf.len = len;

	

	
	sl_run_callbacks(SLCB_REPLY_READY, msg, code, reason, &buf, &dst);

	*(sl_timeout) = get_ticks_raw() + SL_RPL_WAIT_TIME;

	/* supress multhoming support when sending a reply back -- that makes sure
	 * that replies will come from where requests came in; good for NATs
	 * (there is no known use for mhomed for locally generated replies;
	 * note: forwarded cross-interface replies do benefit of mhomed!
	 */
	backup_mhomed=mhomed;
	mhomed=0;
	/* use for sending the received interface -bogdan*/
	dst.proto=msg->rcv.proto;
	dst.send_sock=msg->rcv.bind_address;
	dst.id=msg->rcv.proto_reserved1;
#ifdef USE_COMP
	dst.comp=msg->via1->comp_no;
#endif
	dst.send_flags=msg->rpl_send_flags;

	msg_send_buffer(&dst, buf.s, buf.len, 1);

	if(sip_check_fline(buf.s, buf.len) == 0)
		ret = msg_send_buffer(&dst, buf.s, buf.len, 0);
	else
		ret = msg_send_buffer(&dst, buf.s, buf.len, 1);

	mhomed=backup_mhomed;

	keng = sr_kemi_eng_get();
	if (_sl_evrt_local_response >= 0 || keng!=NULL
			|| sr_event_enabled(SREV_SIP_REPLY_OUT))
	{
		if (likely(build_sip_msg_from_buf(&pmsg, buf.s, buf.len,
				inc_msg_no()) == 0))
		{
			char *tmp = NULL;
			struct onsend_info onsnd_info;

			onsnd_info.to=&dst.to;
			onsnd_info.send_sock=dst.send_sock;
			onsnd_info.buf=buf.s;
			onsnd_info.len=buf.len;

			if (unlikely(!IS_SIP(msg))) {
				/* This is an HTTP reply...  So fudge in a CSeq into
				 * the parsed message message structure so that $rm will
				 * work in the route */
				hdr_field_t *hf;
				struct cseq_body *cseqb;
				char *tmp2;
				int len;
				int tsize;

				if ((hf = (hdr_field_t*) pkg_malloc(sizeof(struct hdr_field))) == NULL)
				{
					LM_ERR("out of package memory\n");
					goto event_route_error;
				}

				if ((cseqb = (struct cseq_body *) pkg_malloc(sizeof(struct cseq_body))) == NULL)
				{
					LM_ERR("out of package memory\n");
					pkg_free(hf);
					goto event_route_error;
				}

				tsize = sizeof(char)
						* (msg->first_line.u.request.method.len + 5);
				if ((tmp = (char *) pkg_malloc(tsize)) == NULL)
				{
					LM_ERR("out of package memory\n");
					pkg_free(cseqb);
					pkg_free(hf);
					goto event_route_error;
				}

				memset(hf, 0, sizeof(struct hdr_field));
				memset(cseqb, 0, sizeof(struct cseq_body));

				len = snprintf(tmp, tsize, "0 %.*s\r\n",
						msg->first_line.u.request.method.len,
						msg->first_line.u.request.method.s);
				if(len<0 || len>=tsize) {
					LM_ERR("failed to print the tmp cseq\n");
					pkg_free(tmp);
					pkg_free(cseqb);
					pkg_free(hf);
					goto event_route_error;
				}
				tmp2 = parse_cseq(tmp, &tmp[len], cseqb);

				hf->type = HDR_CSEQ_T;
				hf->body.s = tmp;
				hf->body.len = tmp2 - tmp;
				hf->parsed = cseqb;

				pmsg.parsed_flag|=HDR_CSEQ_F;
				pmsg.cseq = hf;
				if (pmsg.last_header==0) {
					pmsg.headers=hf;
					pmsg.last_header=hf;
				} else {
					pmsg.last_header->next=hf;
					pmsg.last_header=hf;
				}
			}

			if(IS_SIP(msg) && sr_event_enabled(SREV_SIP_REPLY_OUT)) {
				sr_event_param_t evp;
				memset(&evp, 0, sizeof(sr_event_param_t));
				evp.obuf = buf;
				evp.rcv = &msg->rcv;
				evp.dst = &dst;
				evp.req = msg;
				evp.rpl = &pmsg;
				evp.rplcode = code;
				evp.mode = 1;
				sr_event_exec(SREV_SIP_REPLY_OUT, &evp);
			}

			p_onsend=&onsnd_info;
			backup_rt = get_route_type();
			set_route_type(LOCAL_ROUTE);
			init_run_actions_ctx(&ctx);

			if(_sl_evrt_local_response>=0) {
				run_top_route(event_rt.rlist[_sl_evrt_local_response], &pmsg, 0);
			} else if (keng!=NULL) {
				bctx = sr_kemi_act_ctx_get();
				sr_kemi_act_ctx_set(&ctx);
				(void)sr_kemi_route(keng, msg, EVENT_ROUTE,
							&_sl_event_callback_lres_sent, &evname);
				sr_kemi_act_ctx_set(bctx);
			}
			set_route_type(backup_rt);
			p_onsend=0;

			if (tmp != NULL)
				pkg_free(tmp);

event_route_error:
			free_sip_msg(&pmsg);
		}
	}

	pkg_free(buf.s);

	if (ret<0) {
		goto error;
	}

	update_sl_stats(code);
	return 1;

error:
	update_sl_failures();
	return -1;
}

/*! wrapper of sl_reply_helper - reason is charz, tag is null */
int sl_send_reply(struct sip_msg *msg, int code, char *reason)
{
	return sl_reply_helper(msg, code, reason, 0);
}

/*! wrapper of sl_reply_helper - reason is str, tag is null */
int sl_send_reply_str(struct sip_msg *msg, int code, str *reason)
{
	char *r;
	int ret;

	if(reason->s[reason->len-1]=='\0') {
		r = reason->s;
	} else {
		r = as_asciiz(reason);
		if (r == NULL)
		{
			LM_ERR("no pkg for reason phrase\n");
			return -1;
		}
	}

	ret = sl_reply_helper(msg, code, r, 0);

	if (r!=reason->s) pkg_free(r);
	return ret;
}

/*! wrapper of sl_reply_helper - reason is str, tag is str */
int sl_send_reply_dlg(struct sip_msg *msg, int code, str *reason, str *tag)
{
	char *r;
	int ret;

	if(reason->s[reason->len-1]=='\0') {
		r = reason->s;
	} else {
		r = as_asciiz(reason);
		if (r == NULL)
		{
			LM_ERR("no pkg for reason phrase\n");
			return -1;
		}
	}

	ret = sl_reply_helper(msg, code, r, tag);

	if (r!=reason->s) pkg_free(r);
	return ret;
}

int sl_reply_error(struct sip_msg *msg )
{
	static char err_buf[MAX_REASON_LEN];
	int sip_error;
	int ret;

	if(msg->msg_flags & FL_MSG_NOREPLY) {
		LM_INFO("message marked with no-reply flag\n");
		return -2;
	}

	ret=err2reason_phrase( prev_ser_error, &sip_error,
		err_buf, sizeof(err_buf), "SL");
	if (ret>0) {
		sl_send_reply( msg, sip_error, err_buf );
		LM_ERR("stateless error reply used: %s\n", err_buf );
		return 1;
	} else {
		LM_ERR("err2reason failed\n");
		return -1;
	}
}



/* Returns:
 *  0  : ACK to a local reply
 * -1 : error
 *  1  : is not an ACK  or a non-local ACK
*/
int sl_filter_ACK(struct sip_msg *msg, unsigned int flags, void *bar )
{
	str *tag_str;
	run_act_ctx_t ctx;
	run_act_ctx_t *bctx;
	sr_kemi_eng_t *keng = NULL;
	str evname = str_init("sl:filtered-ack");

	if (msg->first_line.u.request.method_value!=METHOD_ACK)
		goto pass_it;

	/*check the timeout value*/
	if ( *(sl_timeout)<= get_ticks_raw() )
	{
		LM_DBG("too late to be a local ACK!\n");
		goto pass_it;
	}

	/*force to parse to header -> we need it for tag param*/
	if (parse_headers( msg, HDR_TO_F, 0 )==-1)
	{
		LM_ERR("unable to parse To header\n");
		return -1;
	}

	if (msg->to) {
		tag_str = &(get_to(msg)->tag_value);
		if ( tag_str->len==TOTAG_VALUE_LEN )
		{
			/* calculate the variable part of to-tag */
			calc_crc_suffix(msg, tag_suffix);
			/* test whether to-tag equal now */
			if (memcmp(tag_str->s,sl_tag.s,sl_tag.len)==0) {
				LM_DBG("SL local ACK found -> dropping it!\n" );
				update_sl_filtered_acks();
				sl_run_callbacks(SLCB_ACK_FILTERED, msg, 0, 0, 0, 0);
				keng = sr_kemi_eng_get();
				if(unlikely(_sl_filtered_ack_route>=0)) {
					run_top_route(event_rt.rlist[_sl_filtered_ack_route],
							msg, 0);
				} else if(keng!=NULL) {
					init_run_actions_ctx(&ctx);
					bctx = sr_kemi_act_ctx_get();
					sr_kemi_act_ctx_set(&ctx);
					(void)sr_kemi_route(keng, msg, EVENT_ROUTE,
							&_sl_event_callback_fl_ack, &evname);
					sr_kemi_act_ctx_set(bctx);
				}
				return 0;
			}
		}
	}

pass_it:
	return 1;
}

/**
 * SL callbacks handling
 */

static sl_cbelem_t *_sl_cbelem_list = NULL;
static int _sl_cbelem_mask = 0;

void sl_destroy_callbacks_list(void)
{
	sl_cbelem_t *p1;
	sl_cbelem_t *p2;

	p1 = _sl_cbelem_list;
	while(p1) {
		p2 = p1;
		p1 = p1->next;
		pkg_free(p2);
	}
}

int sl_register_callback(sl_cbelem_t *cbe)
{
	sl_cbelem_t *p1;

	if(cbe==NULL) {
		LM_ERR("invalid parameter\n");
		return -1;
	}
	p1 = (sl_cbelem_t*)pkg_malloc(sizeof(sl_cbelem_t));

	if(p1==NULL) {
		LM_ERR("no more pkg\n");
		return -1;
	}

	memcpy(p1, cbe, sizeof(sl_cbelem_t));
	p1->next = _sl_cbelem_list;
	_sl_cbelem_list = p1;
	_sl_cbelem_mask |= cbe->type;

	return 0;
}

void sl_run_callbacks(unsigned int type, struct sip_msg *req,
		int code, char *reason, str *reply, struct dest_info *dst)
{
	sl_cbp_t param;
	sl_cbelem_t *p1;
	static str sreason;

	if(likely((_sl_cbelem_mask&type)==0))
		return;

	/* memset(&cbp, 0, sizeof(sl_cbp_t)); */
	param.type   = type;
	param.req    = req;
	param.code   = code;
	sreason.s    = reason;
	if(reason)
		sreason.len  = strlen(reason);
	else
		sreason.len  = 0;
	param.reason = &sreason;
	param.reply  = reply;
	param.dst    = dst;

	for(p1=_sl_cbelem_list; p1; p1=p1->next) {
		if (p1->type&type) {
			LM_DBG("execute callback for event type %d\n", type);
			param.cbp = p1->cbp;
			p1->cbf(&param);
		}
	}
}


