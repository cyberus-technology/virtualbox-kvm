/* -*- indent-tabs-mode: nil; -*- */
#ifndef __VBOX_LWIP_LOG_H__
#define __VBOX_LWIP_LOG_H__

#include <VBox/log.h>

#ifdef LWIP_DEBUG
/*
 * All LWIP_DBG_* constants fit into a byte, so we use upper bits to
 * encode the VBox log group.
 *
 * Mapping between FOO_DEBUG and LOG_GROUP_LWIP_FOO is straightforward
 * except for IP4 where extra '4' was added to the group names to make
 * it possible to specify lwip_ip4* instead of lwip_ip*, where the
 * latter would enable both IP4 and IP6 logging.
 *
 * We ignore LWIP_DBG_STATE &c since in our scheme they would traslate
 * into additional log groups and require combinatorial explosion.  We
 * probably can use LWIP_DBG_TYPES_ON for finer selection if need be
 * (for internal debugging only, as it requires recompilation).
 *
 * Debug levels are mapped to RT debug levels so lwip's default level
 * ends up as RT's level4.  Non-default levels are currently not used
 * much in lwip sources, so enable l4 to get the logs.
 *
 * Caveat:  Slight snag.  The LOG_GROUP_LWIP_XXXX are enum values and
 *          the  lwIP XXXX_DEBUG macros are used in \#if XXXX_DEBUG
 *          tests around the place.  This make MSC raise complaint
 *          C4668, that e.g. 'LOG_GROUP_LWIP_IP4' is not defined as a
 *          preprocessor macro and therefore replaced with '0'.
 *          However, that works just fine because we or LWIP_DBG_ON so
 *          the test is true despite the warning. Thus the pragma
 *          below.
 */
# ifdef _MSC_VER
#  pragma warning(disable:4668)
# endif

# define LWIP_DEBUGF_LOG_GROUP_SHIFT 8
# define LWIP_DEBUGF_LOG_GROUP(_g) \
     (((_g) << LWIP_DEBUGF_LOG_GROUP_SHIFT) | LWIP_DBG_ON)

# define API_LIB_DEBUG    LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_API_LIB)
# define API_MSG_DEBUG    LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_API_MSG)
# define ETHARP_DEBUG     LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_ETHARP)
# define ICMP_DEBUG       LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_ICMP)
# define IGMP_DEBUG       LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_IGMP)
# define INET_DEBUG       LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_INET)
# define IP_DEBUG         LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_IP4)
# define IP_REASS_DEBUG   LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_IP4_REASS)
# define IP6_DEBUG        LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_IP6)
# define MEM_DEBUG        LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_MEM)
# define MEMP_DEBUG       LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_MEMP)
# define NETIF_DEBUG      LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_NETIF)
# define PBUF_DEBUG       LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_PBUF)
# define RAW_DEBUG        LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_RAW)
# define SOCKETS_DEBUG    LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_SOCKETS)
# define SYS_DEBUG        LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_SYS)
# define TCP_CWND_DEBUG   LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP_CWND)
# define TCP_DEBUG        LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP)
# define TCP_FR_DEBUG     LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP_FR)
# define TCP_INPUT_DEBUG  LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP_INPUT)
# define TCP_OUTPUT_DEBUG LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP_OUTPUT)
# define TCP_QLEN_DEBUG   LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP_QLEN)
# define TCP_RST_DEBUG    LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP_RST)
# define TCP_RTO_DEBUG    LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP_RTO)
# define TCP_WND_DEBUG    LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCP_WND)
# define TCPIP_DEBUG      LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TCPIP)
# define TIMERS_DEBUG     LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_TIMERS)
# define UDP_DEBUG        LWIP_DEBUGF_LOG_GROUP(LOG_GROUP_LWIP_UDP)

/*
 * The following symbols are for debugging of modules that are not
 * compiled in.  They are listed here for reference but there're no
 * log groups defined for them currently.
 */
# undef AUTOIP_DEBUG
# undef DHCP_DEBUG
# undef DNS_DEBUG
# undef PPP_DEBUG
# undef SLIP_DEBUG
# undef SNMP_MIB_DEBUG
# undef SNMP_MSG_DEBUG

# ifdef LOG_ENABLED

#  define LWIP_DEBUGF(_when, _args) \
     do { \
         const VBOXLOGGROUP _group = (_when) >> LWIP_DEBUGF_LOG_GROUP_SHIFT; \
         if (_group >= LOG_GROUP_DEFAULT) { \
             /* severe => l1; serious => l2; warning => l3; default => l4 */ \
             const unsigned int _level = 1U << (LWIP_DBG_MASK_LEVEL + 1 - ((_when) & LWIP_DBG_MASK_LEVEL)); \
             LogIt(_level, _group, _args);  \
         } \
     } while (0)

# else  /* !LOG_ENABLED */

#  define LWIP_DEBUGF(_when, _args) do { } while (0)

# endif /* !LOG_ENABLED */

#endif /* LWIP_DEBUG */
#endif /* !__VBOX_LWIP_LOG_H__ */
