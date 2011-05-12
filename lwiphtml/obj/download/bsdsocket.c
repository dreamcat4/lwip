/* bsdsocket.c - PaulOS embedded operating system
   Copyright (C) 2002  Paul Sheer

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_NET

#include "mad.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "errno.h"
#include "file.h"
#include "list.h"
#include "sys/ioctl.h"

static int seg_sum (struct tcp_seg *c)
{
    int n = 0;
    while (c) {
	n += c->len;
	c = c->next;
    }
    return n;
}

static int seg_count (struct tcp_seg *c)
{
    int n = 0;
    while (c) {
	n++;
	c = c->next;
    }
    return n;
}

#if 0
void dump_pcb (struct tcp_pcb *pcb)
{
    printf ("next = 0x%x\n", (unsigned int) pcb->next);
    printf ("state = %d\n", (int) pcb->state);
    printf ("callback_arg = 0x%x\n", (unsigned int) pcb->callback_arg);
    printf ("accept = 0x%x\n", (unsigned int) pcb->accept);
    printf ("local_ip = 0x%x\n", (unsigned int) pcb->local_ip.addr);
    printf ("local_port = %d\n", (unsigned int) pcb->local_port);
    printf ("remote_ip = 0x%x\n", (unsigned int) pcb->remote_ip.addr);
    printf ("rcv_nxt = %d\n", (int) pcb->rcv_nxt);
    printf ("rcv_wnd = %d\n", (int) pcb->rcv_wnd);
    printf ("tmr = %d\n", (int) pcb->tmr);
    printf ("rtime = %d\n", (int) pcb->rtime);
    printf ("flags = %d\n", (int) pcb->flags);
    printf ("rttest = %d\n", (int) pcb->rttest);
    printf ("rtseq = %d\n", (int) pcb->rtseq);
    printf ("sa = %d\n", (int) pcb->sa);
    printf ("sv = %d\n", (int) pcb->sv);
    printf ("rto = %d\n", (int) pcb->rto);
    printf ("nrtx = %d\n", (int) pcb->nrtx);
    printf ("lastack = %d\n", (int) pcb->lastack);
    printf ("dupacks = %d\n", (int) pcb->dupacks);
    printf ("cwnd = %d\n", (int) pcb->cwnd);
    printf ("ssthresh = %d\n", (int) pcb->ssthresh);
    printf ("snd_nxt = %d\n", (int) pcb->snd_nxt);
    printf ("snd_max = %d\n", (int) pcb->snd_max);
    printf ("snd_wnd = %d\n", (int) pcb->snd_wnd);
    printf ("snd_wl1 = %d\n", (int) pcb->snd_wl1);
    printf ("snd_wl2 = %d\n", (int) pcb->snd_wl2);
    printf ("snd_lbb = %d\n", (int) pcb->snd_lbb);
    printf ("snd_buf = %d\n", (int) pcb->snd_buf);
    printf ("snd_snd_queuelen = %d\n", (int) pcb->snd_queuelen);
    printf ("sent = 0x%x\n", (unsigned int) pcb->sent);
    printf ("acked = %d\n", (int) pcb->acked);
    printf ("recv = 0x%x\n", (unsigned int) pcb->recv);
    printf ("recv_data = 0x%x, tot_len = %d\n", (unsigned int) pcb->recv_data, pcb->recv_data ? (int) pcb->recv_data->tot_len : 0);
    printf ("connected = 0x%x\n", (unsigned int) pcb->connected);
    printf ("poll = 0x%x\n", (unsigned int) pcb->poll);
    printf ("errf = 0x%x\n", (unsigned int) pcb->errf);
    printf ("polltmr = %d\n", (int) pcb->polltmr);
    printf ("pollinterval = %d\n", (int) pcb->pollinterval);
    printf ("unsent = %d, %d\n", seg_sum (pcb->unsent), seg_count (pcb->unsent));
    printf ("unacked = %d, %d\n", seg_sum (pcb->unacked), seg_count (pcb->unacked));
#if 0
    printf ("ooseg = %d\n", seg_sum (pcb->ooseg), seg_count (pcb->ooseg));
#endif
    printf ("\n");
};
#endif








static int lwip_error_to_errno (int v)
{
    const int e[] =
	{ 0, ENOMEM, ENOBUFS, ECONNABORTED, ECONNRESET, ESHUTDOWN, ENOTCONN, EINVAL, EINVAL, EHOSTUNREACH,
	EADDRINUSE
    };
    return e[-v];
}

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

LIST_TYPE_DECLARE (packet_item, char *data; int len;);
LIST_TYPE_DECLARE (incoming_item, struct socket_info *s;);

struct socket_info {
#define LISTENER_MAGIC	0x95de8a3b
#define STREAM_MAGIC	0x3fd740ab
    int type;
    int ref;			/* number of times dup'ed */
    struct tcp_pcb *pcb;
    union {
	struct {
	    int listener_queue_max_len;
	     LIST_DECLARE (incoming_list, incoming_item);	/* queued incoming connections for listening sockets */
	} listener;
	struct {
	    LIST_DECLARE (packet_list, packet_item);	/* queued incoming packets for connected sockets */
	    int in_packet_offset;
	    int shutdown_how;
	} stream;
    } d;
};

static void incoming_free (incoming_item * p)
{
    if (!p->s)
	return;
    if (p->s->pcb) {
	tcp_arg (p->s->pcb, NULL);
	tcp_sent (p->s->pcb, NULL);
	tcp_recv (p->s->pcb, NULL);
	tcp_err (p->s->pcb, NULL);
	tcp_close (p->s->pcb);
    }
    ldeleteall (p->s->d.stream.packet_list);
    free (p->s);
}

static void packet_free (packet_item * p)
{
    free (p->data);
}

static err_t _socket_sent (void *arg, struct tcp_pcb *pcb, u16_t len)
{
    struct socket_info *info = (struct socket_info *) arg;
    if (!pcb->unacked && !pcb->unsent) {
	tcp_close (pcb);
	tcp_sent (pcb, NULL);
	tcp_abort (pcb);	/* I like closure in a relationship */
	info->pcb = 0;
	return ERR_CLSD;
    }
    return ERR_OK;
}

static int _socket_close (struct tcp_pcb *pcb)
{
    if (pcb) {
	tcp_arg (pcb, NULL);
	tcp_sent (pcb, NULL);
	tcp_recv (pcb, NULL);
	tcp_err (pcb, NULL);
	if (!pcb->unacked && !pcb->unsent) {
	    tcp_close (pcb);
	    tcp_abort (pcb);	/* I like closure in a relationship */
	    return ERR_CLSD;
	} else {
	    tcp_sent (pcb, _socket_sent);
	}
    }
    return ERR_OK;
}

#if 0
static err_t _socket_poll (void *arg, struct tcp_pcb *pcb)
{
    if (arg == NULL) {
	tcp_close (pcb);
    }
#if 0
    else {
	send_data (pcb, (struct http_state *) arg);
    }
#endif

    return ERR_OK;
}
#endif

static void _socket_error (void *arg, err_t err)
{
    struct socket_info *info = (struct socket_info *) arg;
    ldeleteall (info->d.stream.packet_list);
printf ("_socket_error\n");
    info->pcb = 0;
}

static void _listener_error (void *arg, err_t err)
{
    struct socket_info *info = (struct socket_info *) arg;
    ldeleteall (info->d.listener.incoming_list);
printf ("_listener_error\n");
    info->pcb = 0;
}

static void *socket_dup (void *o)
{
    struct socket_info *info = (struct socket_info *) o;
    info->ref++;
    return info;
}

static void socket_close (void *o)
{
    struct socket_info *info = (struct socket_info *) o;
    if (info->ref) {
	info->ref--;
	return;
    }
    if (info->type == LISTENER_MAGIC) {
	ldeleteall (info->d.listener.incoming_list);
	_socket_close (info->pcb);
	info->pcb = 0;
    } else {
	ldeleteall (info->d.stream.packet_list);
	_socket_close (info->pcb);
	info->pcb = 0;
    }
    free (o);
}

static inline int _socket_write_space (struct socket_info *info)
{
#if 0
    if (stats.memp[MEMP_TCP_SEG].used > stats.memp[MEMP_TCP_SEG].avail / 2)
	return 0;
#else
//    if (smsc_packet_count () > 10)	/* FIXME: do we need this? */
//	return 0;
    if (mem_usage_percent (0) > 50)
	return 0;
#endif
    return tcp_sndbuf (info->pcb);
}

static int socket_write_space (void *o, int n)
{
    struct socket_info *info = (struct socket_info *) o;

    if (info->type != STREAM_MAGIC)
	return -1;

    if (!info->pcb)
	return -1;

    return _socket_write_space (info);
}

static int socket_write (void *o, unsigned char *s, int len)
{
    struct socket_info *info = (struct socket_info *) o;
    err_t err;

    if (info->type != STREAM_MAGIC) {
	errno = ENOTSOCK;
	return -1;
    }

    if (!info->pcb) {
	errno = ENOTCONN;
	return -1;
    }

    do {
/* explicit (i.e. one extra) */
	if (global_callback ()) {
	    errno = EINTR;
	    return -1;
	}
	if (!info->pcb)		/* socket closed */
	    return 0;
    } while (_socket_write_space (info) <= 0);

//dump_pcb (info->pcb);

    len = min (len, tcp_sndbuf (info->pcb));
    err = tcp_write (info->pcb, s, len, 1);

    if (err == ERR_OK) {
	if (!info->pcb->unacked)
	    tcp_output (info->pcb);
	return len;
    }
    errno = lwip_error_to_errno (err);
    return -1;
}

static int socket_write_queue (void *o)
{
    struct socket_info *info = (struct socket_info *) o;
    struct tcp_seg *unsent;
    int total = 0;
    if (info->type != STREAM_MAGIC)
	return -1;
    if (!info->pcb)
	return -1;
    for (unsent = info->pcb->unsent; unsent; unsent = unsent->next)
	total += unsent->len;
    return total;
}

static int socket_write_nonblock (void *o, unsigned char *s, int len)
{
    if (socket_write_space (o, 0))
	return socket_write (o, s, len);
    errno = EAGAIN;
    return -1;
}

err_t _socket_recv (void *arg, struct tcp_pcb * pcb, struct pbuf * p, err_t err)
{
    struct socket_info *info = (struct socket_info *) arg;
    if (err == ERR_OK && p == NULL) {
	err = _socket_close (info->pcb);
printf ("_socket_recv, error\n");
	info->pcb = 0;
	return err;
    }
    if (err == ERR_OK && p != NULL) {
	char *s;
	struct pbuf *q;
	struct packet_item *j;
/* record the packet in our list for later reading */
	lappend (info->d.stream.packet_list);
	j = ltail (info->d.stream.packet_list);
	s = j->data = (char *) malloc (p->tot_len);	/* FIXME: check return value */
	j->len = p->tot_len;
	for (q = p; q; q = q->next) {
	    memcpy (s, q->payload, q->len);
	    s += q->len;
	}
	pbuf_free (p);
    }
    return ERR_OK;
}

static int socket_read (void *o, unsigned char *s, int len)
{
    struct socket_info *info = (struct socket_info *) o;
    struct packet_item *j;
    int r = 0;

    if (info->type != STREAM_MAGIC) {
	errno = ENOTSOCK;
	return -1;
    }

    if (!info->pcb) {
	errno = ENOTCONN;
	return -1;
    }

    for (;;) {
/* explicit (i.e. always one extra callback) */
	if (global_callback ()) {
	    errno = EINTR;
	    return -1;
	}
	if (!info->pcb)		/* socket closed */
	    return 0;
	if ((j = lhead (info->d.stream.packet_list)))
	    if (j->len > 0)
		break;
    }

/* FIXME: quadruple check this algorithm */
    for (;;) {
	int c;
	c = min (len, j->len - info->d.stream.in_packet_offset);
	if (!c)
	    break;		/* out of space */
	memcpy (s, j->data + info->d.stream.in_packet_offset, c);
	s += c;
	r += c;
	len -= c;
	info->d.stream.in_packet_offset += c;
	if (info->d.stream.in_packet_offset == j->len) {
	    info->d.stream.in_packet_offset = 0;
	    ldeleteinc (info->d.stream.packet_list, j);
	    if (!j)
		break;		/* nothing left to process */
	}
    }

    tcp_recved (info->pcb, r);
    tcp_output (info->pcb);

    return r;
}

static int socket_read_avail (void *o, int n)
{
    packet_item *j;
    int total = 0;

    struct socket_info *info = (struct socket_info *) o;

    if (info->type == STREAM_MAGIC) {
	if (!info->pcb)
	    return -1;
	lsearchforward (info->d.stream.packet_list, j, total += j->len);
	return total;
    }

    total = lcount (info->d.listener.incoming_list);
/* return a count of the incoming connections */
    return total;
}

static int socket_read_nonblock (void *o, unsigned char *s, int len)
{
    if (socket_read_avail (o, 0))
	return socket_read (o, s, len);
    errno = EAGAIN;
    return -1;
}

static void *socket_socket (void *param, int arg1, int arg2)
{
    struct socket_info *info;

    info = (struct socket_info *) malloc (sizeof (struct socket_info));
    if (!info) {
	errno = ENOMEM;
	return 0;
    }
    memset (info, 0, sizeof (struct socket_info));
    info->pcb = tcp_new ();
    if (!info->pcb) {
	errno = ENOMEM;
	free (info);
	return 0;
    }
    info->type = LISTENER_MAGIC;
    linit (info->d.listener.incoming_list, incoming_item, incoming_free);
    tcp_err (info->pcb, _listener_error);
    return (void *) info;
}

static err_t _socket_accept (void *arg, struct tcp_pcb *pcb, err_t err)
{
    struct socket_info *info = (struct socket_info *) arg;
    incoming_item *j;

    if (lcount (info->d.listener.incoming_list) >= info->d.listener.listener_queue_max_len)
	return ERR_ABRT;

    if (mem_usage_percent (0) > 25)
	return ERR_ABRT;

/* add a new connection to our list of pending incoming connections */
    lappend (info->d.listener.incoming_list);
    j = ltail (info->d.listener.incoming_list);
    j->s = (struct socket_info *) malloc (sizeof (struct socket_info));
    if (!j->s) {
	ldeleteinc (info->d.listener.incoming_list, j);
	return ERR_ABRT;
    }
    memset (j->s, 0, sizeof (struct socket_info));

/* setup list member structure */
    linit (j->s->d.stream.packet_list, packet_item, packet_free);
    j->s->type = STREAM_MAGIC;
    j->s->pcb = pcb;

/* tcp callbacks */
    tcp_arg (j->s->pcb, (void *) j->s);
    tcp_recv (j->s->pcb, _socket_recv);
    tcp_err (j->s->pcb, _socket_error);

    return ERR_OK;
}

static int socket_ioctl (void *o, int cmd, int *config)
{
    struct socket_info *info = (struct socket_info *) o;
    struct sockaddr_in *s = (struct sockaddr_in *) config;
    err_t r;
    switch (cmd) {
    case SIOBIND:
	if (info->type != LISTENER_MAGIC) {
	    errno = ENOTSOCK;
	    return -1;
	}
	r = tcp_bind (info->pcb, (struct ip_addr *) &s->sin_addr, ntohs (s->sin_port));
	if (r != ERR_OK) {
	    errno = lwip_error_to_errno (r);
	    return -1;
	}
	return 0;
    case SIOSHUTDOWN:
	if (info->type != STREAM_MAGIC) {
	    errno = ENOTSOCK;
	    return -1;
	}
	if (!info->pcb) {
	    errno = ENOTCONN;
	    return -1;
	}
	info->d.stream.shutdown_how |= 1 << *config;
	if (info->d.stream.shutdown_how >= (1 << *config)) {
	    _socket_close (info->pcb);
	    info->pcb = 0;
	}
	return 0;
    case SIOLISTEN:
	if (info->type != LISTENER_MAGIC) {
	    errno = ENOTSOCK;
	    return -1;
	}
	info->pcb = tcp_listen (info->pcb);
	info->d.listener.listener_queue_max_len = *config;
	tcp_arg (info->pcb, o);
	tcp_accept (info->pcb, _socket_accept);
	return 0;
    }
    errno = EINVAL;
    return -1;
}

static void *socket_accept (void *o, struct sockaddr *addr, int *addrlen)
{
    incoming_item *j;
    struct socket_info *info = (struct socket_info *) o;
    void *r;

    if (info->type != LISTENER_MAGIC) {
	errno = ENOTSOCK;
	return 0;
    }
    if (*addrlen != sizeof (struct sockaddr_in)) {
	errno = EFAULT;
	return 0;
    }

    j = lhead (info->d.listener.incoming_list);
    if (!j) {
	errno = EAGAIN;
	return 0;
    }
    while (!j->s->pcb) {
/* loop through sockets that closed before accept() could be called by the upper level application */
	ldeleteinc (info->d.listener.incoming_list, j);
	if (!j) {
	    errno = EAGAIN;
	    return 0;
	}
    }
    r = (void *) j->s;
    j->s = 0;			/* zero it so the structure doesn't get free'd */
    ldeleteinc (info->d.listener.incoming_list, j);	/* remove from listener queue */

    return r;
}

struct file file_socket = {
    socket_socket,		/* we use the open() method */
    socket_dup,
    socket_close,
    socket_ioctl,
    0,
    socket_write,
    socket_write,
    socket_write_space,
    socket_write_nonblock,
    0,
    socket_read,
    socket_read,
    socket_read_avail,
    socket_read_nonblock,
    socket_write_queue,
    0,
    0,
    socket_accept,
};

#endif	/* HAVE_NET */

