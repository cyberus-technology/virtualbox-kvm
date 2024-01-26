/** @file
 * NAT - slirp state/configuration.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef ___slirp_state_h
#define ___slirp_state_h

#include <iprt/req.h>
#include <iprt/critsect.h>

#define COUNTERS_INIT
#include "counters.h"

#include "ip_icmp.h"
#include "dnsproxy/dnsproxy.h"


/** Where to start DHCP IP number allocation. */
#define START_ADDR  15

/** DHCP Lease time. */
#define LEASE_TIME (24 * 3600)

/*
 * ARP cache this is naive implementaion of ARP
 * cache of mapping 4 byte IPv4 address to 6 byte
 * ethernet one.
 */
struct arp_cache_entry
{
    uint32_t ip;
    uint8_t ether[6];
    LIST_ENTRY(arp_cache_entry) list;
};
LIST_HEAD(arp_cache_head, arp_cache_entry);

/** TFTP session entry. */
struct dns_domain_entry
{
    char *dd_pszDomain;
    LIST_ENTRY(dns_domain_entry) dd_list;
};
LIST_HEAD(dns_domain_list_head, dns_domain_entry);

#ifdef VBOX_WITH_DNSMAPPING_IN_HOSTRESOLVER
typedef struct DNSMAPPINGENTRY
{
    /** Literal or pattern. */
    bool        fPattern;
    /** Host name or pattern to map. */
    char        *pszName;
    /** The IP Address. */
    uint32_t    u32IpAddress;
    /** List entry.  */
    STAILQ_ENTRY(DNSMAPPINGENTRY) MapList;
} DNSMAPPINGENTRY, *PDNSMAPPINGENTRY;
typedef STAILQ_HEAD(DNSMAPPINGHEAD, DNSMAPPINGENTRY) DNSMAPPINGHEAD;
#endif

struct dns_entry
{
    struct in_addr de_addr;
    TAILQ_ENTRY(dns_entry) de_list;
};
TAILQ_HEAD(dns_list_head, dns_entry);
TAILQ_HEAD(if_queue, mbuf);

struct port_forward_rule
{
    uint16_t proto;
    uint16_t host_port;
    uint16_t guest_port;
    struct in_addr guest_addr;
    struct in_addr bind_ip;
    int activated;
    struct socket *so;
    LIST_ENTRY(port_forward_rule) list;
};
LIST_HEAD(port_forward_rule_list, port_forward_rule);


#ifdef RT_OS_WINDOWS
struct pong;
TAILQ_HEAD(pong_tailq, pong);
#endif

/* forward declaration */
struct proto_handler;

/** Main state/configuration structure for slirp NAT. */
typedef struct NATState
{
#define PROFILE_COUNTER(name, dsc)     STAMPROFILE Stat ## name
#define COUNTING_COUNTER(name, dsc)    STAMCOUNTER Stat ## name
#include "counters.h"
    /* Stuff from boot.c */
    void *pbootp_clients;
    const char *bootp_filename;
    /* Stuff from if.c */
    int if_mtu, if_mru;
    int if_comp;
    int if_maxlinkhdr;
    int if_queued;
    int if_thresh;
    /* Stuff from icmp.c */
    struct icmpstat_t icmpstat;
    /* Stuff from ip_input.c */
    struct ipstat_t ipstat;
    struct ipqhead ipq[IPREASS_NHASH];
    int maxnipq;    /* Administrative limit on # of reass queues*/
    int maxfragsperpacket; /* Maximum number of IPv4 fragments allowed per packet */
    int nipq; /* total number of reass queues */
    uint16_t ip_currid;
    /* Stuff from mbuf.c */
    /* Stuff from slirp.c */
    void *pvUser;
    uint32_t curtime;
    uint32_t time_fasttimo;
    uint32_t last_slowtimo;
    bool do_slowtimo;
    bool link_up;
    struct timeval tt;
    struct in_addr our_addr;
    struct in_addr alias_addr;
    struct in_addr special_addr;
    struct in_addr guest_addr_guess;

    int tcp_rcvspace;
    int tcp_sndspace;
    int socket_rcv;
    int socket_snd;
    int soMaxConn;
#ifdef RT_OS_WINDOWS
    ULONG (WINAPI * pfnGetAdaptersAddresses)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
#endif
    struct dns_list_head pDnsList;
    struct dns_domain_list_head pDomainList;
    uint32_t dnsgen;            /* XXX: merge with dnsLastUpdate? */
    struct in_addr tftp_server;
    struct in_addr loopback_addr;
    uint32_t dnsLastUpdate;
    uint32_t netmask;
    const uint8_t *slirp_ethaddr;
    char slirp_hostname[33];
    bool fPassDomain;
    struct in_addr bindIP;
    /* Stuff from tcp_input.c */
    struct socket tcb;

    struct socket *tcp_last_so;
    tcp_seq tcp_iss;
    /* Stuff from tcp_timer.c */
    struct tcpstat_t tcpstat;
    uint32_t tcp_now;
    int tcp_reass_qsize;
    int tcp_reass_maxqlen;
    int tcp_reass_maxseg;
    int tcp_reass_overflows;
    /* Stuff from tftp.c */
    void         *pvTftpSessions;
    int          cTftpSession;
    const char *tftp_prefix;
    /* Stuff from udp.c */
    struct udpstat_t udpstat;
    struct socket udb;
    struct socket *udp_last_so;

# ifndef RT_OS_WINDOWS
    /* counter of sockets needed for allocation enough room to
     * process sockets with poll/epoll
     *
     * NSOCK_INC/DEC should be injected before every
     * operation on socket queue (tcb, udb)
     */
    int nsock;
#  define NSOCK_INC() do {pData->nsock++;} while (0)
#  define NSOCK_DEC() do {pData->nsock--;} while (0)
#  define NSOCK_INC_EX(ex) do {ex->pData->nsock++;} while (0)
#  define NSOCK_DEC_EX(ex) do {ex->pData->nsock--;} while (0)
# else
#  define NSOCK_INC() do {} while (0)
#  define NSOCK_DEC() do {} while (0)
#  define NSOCK_INC_EX(ex) do {} while (0)
#  define NSOCK_DEC_EX(ex) do {} while (0)
# endif

    struct socket icmp_socket;
# if !defined(RT_OS_WINDOWS)
    struct icmp_storage icmp_msg_head;
    int cIcmpCacheSize;
    int iIcmpCacheLimit;
# else
    struct pong_tailq pongs_expected;
    struct pong_tailq pongs_received;
    size_t cbIcmpPending;
# endif

#if defined(RT_OS_WINDOWS)
# define VBOX_SOCKET_EVENT (pData->phEvents[VBOX_SOCKET_EVENT_INDEX])
    HANDLE phEvents[VBOX_EVENT_COUNT];
#endif
#ifdef zone_mbuf
# undef zone_mbuf
#endif
    uma_zone_t zone_mbuf;
#ifdef zone_clust
# undef zone_clust
#endif
    uma_zone_t zone_clust;
#ifdef zone_pack
# undef zone_pack
#endif
    uma_zone_t zone_pack;
#ifdef zone_jumbop
# undef zone_jumbop
#endif
    uma_zone_t zone_jumbop;
#ifdef zone_jumbo9
# undef zone_jumbo9
#endif
    uma_zone_t zone_jumbo9;
#ifdef zone_jumbo16
# undef zone_jumbo16
#endif
    uma_zone_t zone_jumbo16;
#ifdef zone_ext_refcnt
# undef zone_ext_refcnt
    int nmbclusters;                    /* limits number of mbuf clusters */
    int nmbjumbop;                      /* limits number of page size jumbo clusters */
    int nmbjumbo9;                      /* limits number of 9k jumbo clusters */
    int nmbjumbo16;                     /* limits number of 16k jumbo clusters */
    struct mbstat mbstat;
#endif
    uma_zone_t zone_ext_refcnt;
    /**
     * in (r89055) using of this behaviour has been changed and mean that Slirp
     * can't parse hosts strucutures/files to provide to guest host name-resolving
     * configuration, instead Slirp provides .{interface-number + 1}.3 as a nameserver
     * and proxies DNS queiries to Host's Name Resolver API.
     */
    bool fUseHostResolver;
    /**
     * Flag whether using the host resolver mode is permanent
     * because the user configured it that way.
     */
    bool fUseHostResolverPermanent;
    /* from dnsproxy/dnsproxy.h*/
    unsigned int authoritative_port;
    unsigned int authoritative_timeout;
    unsigned int recursive_port;
    unsigned int recursive_timeout;
    unsigned int stats_timeout;
    unsigned int port;

    unsigned long active_queries;
    unsigned long all_queries;
    unsigned long authoritative_queries;
    unsigned long recursive_queries;
    unsigned long removed_queries;
    unsigned long dropped_queries;
    unsigned long answered_queries;
    unsigned long dropped_answers;
    unsigned long late_answers;
    unsigned long hash_collisions;
    /*dnsproxy/dnsproxy.c*/
    unsigned short queryid;
    struct sockaddr_in authoritative_addr;
    struct sockaddr_in recursive_addr;
    int sock_query;
    int sock_answer;
    /* dnsproxy/hash.c */
#define HASHSIZE 10
#define HASH(id) (id & ((1 << HASHSIZE) - 1))
    struct request *request_hash[1 << HASHSIZE];
    /* this field control behaviour of DHCP server */
    bool fUseDnsProxy;
    /** Flag whether the guest can contact services on the host's
     * loopback interface (127.0.0.1/localhost). */
    bool fLocalhostReachable;

    LIST_HEAD(RT_NOTHING, libalias) instancehead;
    int    i32AliasMode;
    struct libalias *proxy_alias;
    LIST_HEAD(handler_chain, proto_handler) handler_chain;
    /** Critical R/W section to protect the handler chain list. */
    RTCRITSECTRW CsRwHandlerChain;
    struct port_forward_rule_list port_forward_rule_head;
    struct arp_cache_head arp_cache;
    /* libalis modules' handlers*/
    struct proto_handler *ftp_module;
    struct proto_handler *nbt_module;
#ifdef VBOX_WITH_NAT_SEND2HOME
    /* array of home addresses */
    struct sockaddr_in *pInSockAddrHomeAddress;
    /* size of pInSockAddrHomeAddress in elements */
    int cInHomeAddressSize;
#endif
#ifdef VBOX_WITH_DNSMAPPING_IN_HOSTRESOLVER
    DNSMAPPINGHEAD DNSMapNames;
    DNSMAPPINGHEAD DNSMapPatterns;
#endif
} NATState;


/** Default IP time to live. */
#define ip_defttl IPDEFTTL

/** Number of permanent buffers in mbuf. */
#define mbuf_thresh 30

/** Use a fixed time before sending keepalive. */
#define tcp_keepidle TCPTV_KEEP_IDLE

/** Use a fixed interval between keepalive. */
#define tcp_keepintvl TCPTV_KEEPINTVL

/** Maximum idle time before timing out a connection. */
#define tcp_maxidle (TCPTV_KEEPCNT * tcp_keepintvl)

/** Default TCP socket options. */
#define so_options DO_KEEPALIVE

/** Default TCP MSS value. */
#define tcp_mssdflt TCP_MSS

/** Default TCP round trip time. */
#define tcp_rttdflt (TCPTV_SRTTDFLT / PR_SLOWHZ)

/** Enable RFC1323 performance enhancements.
 * @todo check if it really works, it was turned off before. */
#define tcp_do_rfc1323 1

/** TCP receive buffer size. */
#define tcp_rcvspace pData->tcp_rcvspace

/** TCP receive buffer size. */
#define tcp_sndspace pData->tcp_sndspace

/* TCP duplicate ACK retransmit threshold. */
#define tcprexmtthresh 3


#define bootp_filename pData->bootp_filename

#define if_mtu pData->if_mtu
#define if_mru pData->if_mru
#define if_comp pData->if_comp
#define if_maxlinkhdr pData->if_maxlinkhdr
#define if_queued pData->if_queued
#define if_thresh pData->if_thresh

#define icmpstat pData->icmpstat

#define ipstat pData->ipstat
#define ipq pData->ipq
#define ip_currid pData->ip_currid

#define mbuf_alloced pData->mbuf_alloced
#define mbuf_max pData->mbuf_max
#define msize pData->msize
#define m_freelist pData->m_freelist
#define m_usedlist pData->m_usedlist

#define curtime pData->curtime
#define time_fasttimo pData->time_fasttimo
#define last_slowtimo pData->last_slowtimo
#define do_slowtimo pData->do_slowtimo
#define link_up pData->link_up
#define cUsers pData->cUsers
#define tt pData->tt
#define our_addr pData->our_addr
#ifndef VBOX_SLIRP_ALIAS
# define alias_addr pData->alias_addr
#else
# define handler_chain pData->handler_chain
#endif
#define dns_addr pData->dns_addr
#define loopback_addr pData->loopback_addr
#define slirp_hostname pData->slirp_hostname

#define tcb pData->tcb
#define tcp_last_so pData->tcp_last_so
#define tcp_iss pData->tcp_iss

#define tcpstat pData->tcpstat
#define tcp_now pData->tcp_now

#define tftp_prefix pData->tftp_prefix

#define udpstat pData->udpstat
#define udb pData->udb
#define udp_last_so pData->udp_last_so

#define maxfragsperpacket pData->maxfragsperpacket
#define maxnipq pData->maxnipq
#define nipq pData->nipq

#define tcp_reass_qsize pData->tcp_reass_qsize
#define tcp_reass_maxqlen pData->tcp_reass_maxqlen
#define tcp_reass_maxseg pData->tcp_reass_maxseg
#define tcp_reass_overflows pData->tcp_reass_overflows

#define queue_tcp_label tcb
#define queue_udp_label udb
#define VBOX_X2(x) x
#define VBOX_X(x) VBOX_X2(x)

#if 1

# define QSOCKET_LOCK(queue) do {} while (0)
# define QSOCKET_UNLOCK(queue) do {} while (0)
# define QSOCKET_LOCK_CREATE(queue) do {} while (0)
# define QSOCKET_LOCK_DESTROY(queue) do {} while (0)
# define QSOCKET_FOREACH(so, sonext, label)                              \
    for ((so)  = VBOX_X2(queue_ ## label ## _label).so_next;             \
         (so) != &(VBOX_X2(queue_ ## label ## _label));                  \
         (so) = (sonext))                                                \
    {                                                                    \
        (sonext) = (so)->so_next;                                        \
         Log5(("%s:%d Processing so:%R[natsock]\n", RT_GCC_EXTENSION __FUNCTION__, __LINE__, (so)));
# define CONTINUE(label) continue
# define CONTINUE_NO_UNLOCK(label) continue
# define LOOP_LABEL(label, so, sonext) /* empty*/
# define DO_TCP_OUTPUT(data, sotcb) tcp_output((data), (sotcb))
# define DO_TCP_INPUT(data, mbuf, size, so) tcp_input((data), (mbuf), (size), (so))
# define DO_TCP_CONNECT(data, so) tcp_connect((data), (so))
# define DO_SOREAD(ret, data, so, ifclose)                               \
    do {                                                                 \
        (ret) = soread((data), (so), (ifclose));                         \
    } while(0)
# define DO_SOWRITE(ret, data, so)                                       \
    do {                                                                 \
        (ret) = sowrite((data), (so));                                   \
    } while(0)
# define DO_SORECFROM(data, so) sorecvfrom((data), (so))
# define SOLOOKUP(so, label, src, sport, dst, dport)                                      \
    do {                                                                                  \
        (so) = solookup(&VBOX_X2(queue_ ## label ## _label), (src), (sport), (dst), (dport)); \
    } while (0)
# define DO_UDP_DETACH(data, so, ignored) udp_detach((data), (so))

#endif

#define TCP_OUTPUT(data, sotcb) DO_TCP_OUTPUT((data), (sotcb))
#define TCP_INPUT(data, mbuf, size, so) DO_TCP_INPUT((data), (mbuf), (size), (so))
#define TCP_CONNECT(data, so) DO_TCP_CONNECT((data), (so))
#define SOREAD(ret, data, so, ifclose) DO_SOREAD((ret), (data), (so), (ifclose))
#define SOWRITE(ret, data, so) DO_SOWRITE((ret), (data), (so))
#define SORECVFROM(data, so) DO_SORECFROM((data), (so))
#define UDP_DETACH(data, so, so_next) DO_UDP_DETACH((data), (so), (so_next))

/* dnsproxy/dnsproxy.c */
#define authoritative_port pData->authoritative_port
#define authoritative_timeout pData->authoritative_timeout
#define recursive_port pData->recursive_port
#define recursive_timeout pData->recursive_timeout
#define stats_timeout pData->stats_timeout
/* dnsproxy/hash.c */
#define dns_port pData->port
#define request_hash pData->request_hash
#define hash_collisions pData->hash_collisions
#define active_queries pData->active_queries
#define all_queries pData->all_queries
#define authoritative_queries pData->authoritative_queries
#define recursive_queries pData->recursive_queries
#define removed_queries pData->removed_queries
#define dropped_queries pData->dropped_queries
#define answered_queries pData->answered_queries
#define dropped_answers pData->dropped_answers
#define late_answers pData->late_answers

/* dnsproxy/dnsproxy.c */
#define queryid pData->queryid
#define authoritative_addr pData->authoritative_addr
#define recursive_addr pData->recursive_addr
#define sock_query pData->sock_query
#define sock_answer pData->sock_answer

#define instancehead pData->instancehead

#define nmbclusters    pData->nmbclusters
#define nmbjumbop  pData->nmbjumbop
#define nmbjumbo9  pData->nmbjumbo9
#define nmbjumbo16 pData->nmbjumbo16
#define mbstat pData->mbstat
#include "ext.h"
#undef zone_mbuf
#undef zone_clust
#undef zone_pack
#undef zone_jumbop
#undef zone_jumbo9
#undef zone_jumbo16
#undef zone_ext_refcnt
static inline uma_zone_t slirp_zone_pack(PNATState pData)
{
    return pData->zone_pack;
}
static inline uma_zone_t slirp_zone_jumbop(PNATState pData)
{
    return pData->zone_jumbop;
}
static inline uma_zone_t slirp_zone_jumbo9(PNATState pData)
{
    return pData->zone_jumbo9;
}
static inline uma_zone_t slirp_zone_jumbo16(PNATState pData)
{
    return pData->zone_jumbo16;
}
static inline uma_zone_t slirp_zone_ext_refcnt(PNATState pData)
{
    return pData->zone_ext_refcnt;
}
static inline uma_zone_t slirp_zone_mbuf(PNATState pData)
{
    return pData->zone_mbuf;
}
static inline uma_zone_t slirp_zone_clust(PNATState pData)
{
    return pData->zone_clust;
}
#ifndef VBOX_SLIRP_BSD
# define m_adj(m, len) m_adj(pData, (m), (len))
#endif

#endif /* !___slirp_state_h */
