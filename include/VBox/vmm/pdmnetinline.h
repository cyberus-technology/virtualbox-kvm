/** @file
 * PDM - Networking Helpers, Inlined Code. (DEV,++)
 *
 * This is all inlined because it's too tedious to create 2-3 libraries to
 * contain it all (same bad excuse as for intnetinline.h).
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef VBOX_INCLUDED_vmm_pdmnetinline_h
#define VBOX_INCLUDED_vmm_pdmnetinline_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <VBox/log.h>
#include <VBox/types.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/net.h>
#include <iprt/string.h>


/** @defgroup grp_pdm_net_inline    The PDM Networking Helper APIs
 * @ingroup grp_pdm
 * @{
 */


/**
 * Checksum type.
 */
typedef enum PDMNETCSUMTYPE
{
    /** No checksum. */
    PDMNETCSUMTYPE_NONE = 0,
    /** Normal TCP checksum. */
    PDMNETCSUMTYPE_COMPLETE,
    /** Checksum on pseudo header (used with GSO). */
    PDMNETCSUMTYPE_PSEUDO,
    /** The usual 32-bit hack. */
    PDMNETCSUMTYPE_32_BIT_HACK = 0x7fffffff
} PDMNETCSUMTYPE;


/**
 * Validates the GSO context.
 *
 * @returns true if valid, false if not (not asserted or logged).
 * @param   pGso                The GSO context.
 * @param   cbGsoMax            The max size of the GSO context.
 * @param   cbFrame             The max size of the GSO frame (use to validate
 *                              the MSS).
 */
DECLINLINE(bool) PDMNetGsoIsValid(PCPDMNETWORKGSO pGso, size_t cbGsoMax, size_t cbFrame)
{
#define CHECK_COND_RETURN_FALSE(expr) if (RT_LIKELY(expr)) { /* likely */ } else return false
    PDMNETWORKGSOTYPE enmType;

    CHECK_COND_RETURN_FALSE(cbGsoMax >= sizeof(*pGso));

    enmType = (PDMNETWORKGSOTYPE)pGso->u8Type;
    CHECK_COND_RETURN_FALSE(enmType > PDMNETWORKGSOTYPE_INVALID && enmType < PDMNETWORKGSOTYPE_END);

    /* all types requires both headers. */
    CHECK_COND_RETURN_FALSE(pGso->offHdr1 >= sizeof(RTNETETHERHDR));
    CHECK_COND_RETURN_FALSE(pGso->offHdr2 > pGso->offHdr1);
    CHECK_COND_RETURN_FALSE(pGso->cbHdrsTotal > pGso->offHdr2);

    /* min size of the 1st header(s). */
    switch (enmType)
    {
        case PDMNETWORKGSOTYPE_IPV4_TCP:
        case PDMNETWORKGSOTYPE_IPV4_UDP:
            CHECK_COND_RETURN_FALSE((unsigned)pGso->offHdr2 - pGso->offHdr1 >= RTNETIPV4_MIN_LEN);
            break;
        case PDMNETWORKGSOTYPE_IPV6_TCP:
        case PDMNETWORKGSOTYPE_IPV6_UDP:
            CHECK_COND_RETURN_FALSE((unsigned)pGso->offHdr2 - pGso->offHdr1 >= RTNETIPV6_MIN_LEN);
            break;
        case PDMNETWORKGSOTYPE_IPV4_IPV6_TCP:
        case PDMNETWORKGSOTYPE_IPV4_IPV6_UDP:
            CHECK_COND_RETURN_FALSE((unsigned)pGso->offHdr2 - pGso->offHdr1 >= RTNETIPV4_MIN_LEN + RTNETIPV6_MIN_LEN);
            break;
        /* These two have been rejected above already, but we need to include them to avoid gcc warnings. */
        case PDMNETWORKGSOTYPE_INVALID:
        case PDMNETWORKGSOTYPE_END:
            break;
        /* No default case! Want gcc warnings. */
    }

    /* min size of the 2nd header. */
    switch (enmType)
    {
        case PDMNETWORKGSOTYPE_IPV4_TCP:
        case PDMNETWORKGSOTYPE_IPV6_TCP:
        case PDMNETWORKGSOTYPE_IPV4_IPV6_TCP:
            CHECK_COND_RETURN_FALSE((unsigned)pGso->cbHdrsTotal - pGso->offHdr2 >= RTNETTCP_MIN_LEN);
            break;
        case PDMNETWORKGSOTYPE_IPV4_UDP:
        case PDMNETWORKGSOTYPE_IPV6_UDP:
        case PDMNETWORKGSOTYPE_IPV4_IPV6_UDP:
            CHECK_COND_RETURN_FALSE((unsigned)pGso->cbHdrsTotal - pGso->offHdr2 >= RTNETUDP_MIN_LEN);
            break;
        /* These two have been rejected above already, but we need to include them to avoid gcc warnings. */
        case PDMNETWORKGSOTYPE_INVALID:
        case PDMNETWORKGSOTYPE_END:
            break;
        /* No default case! Want gcc warnings. */
    }

    /* There must be at more than one segment. */
    CHECK_COND_RETURN_FALSE(cbFrame > pGso->cbHdrsTotal);
    CHECK_COND_RETURN_FALSE(cbFrame - pGso->cbHdrsTotal >= pGso->cbMaxSeg);

    /* Make sure the segment size is enough to fit a UDP header. */
    CHECK_COND_RETURN_FALSE(enmType != PDMNETWORKGSOTYPE_IPV4_UDP || pGso->cbMaxSeg >= RTNETUDP_MIN_LEN);

    /* Make sure the segment size is not zero. */
    CHECK_COND_RETURN_FALSE(pGso->cbMaxSeg > 0);

    return true;
#undef CHECK_COND_RETURN_FALSE
}


/**
 * Returns the length of header for a particular segment/fragment.
 *
 * We cannot simply treat UDP header as a part of payload because we do not
 * want to modify the payload but still need to modify the checksum field in
 * UDP header. So we want to include UDP header when calculating the length
 * of headers in the first segment getting it copied to a temporary buffer
 * along with other headers.
 *
 * @returns Length of headers (including UDP header for the first fragment).
 * @param   pGso                The GSO context.
 * @param   iSeg                The segment index.
 */
DECLINLINE(uint8_t) pdmNetSegHdrLen(PCPDMNETWORKGSO pGso, uint32_t iSeg)
{
    return iSeg ? pGso->cbHdrsSeg : pGso->cbHdrsTotal;
}

/**
 * Returns the length of payload for a particular segment/fragment.
 *
 * The first segment does not contain UDP header. The size of UDP header is
 * determined as the difference between the total headers size and the size
 * used during segmentation.
 *
 * @returns Length of payload (including UDP header for the first fragment).
 * @param   pGso                The GSO context.
 * @param   iSeg                The segment that we're carving out (0-based).
 * @param   cSegs               The number of segments in the GSO frame.
 * @param   cbFrame             The size of the GSO frame.
 */
DECLINLINE(uint32_t) pdmNetSegPayloadLen(PCPDMNETWORKGSO pGso, uint32_t iSeg, uint32_t cSegs, uint32_t cbFrame)
{
    if (iSeg + 1 == cSegs)
        return cbFrame - iSeg * pGso->cbMaxSeg - pdmNetSegHdrLen(pGso, iSeg);
    return pGso->cbMaxSeg - (iSeg ? 0 : pGso->cbHdrsTotal - pGso->cbHdrsSeg);
}

/**
 * Calculates the number of segments a GSO frame will be segmented into.
 *
 * @returns Segment count.
 * @param   pGso                The GSO context.
 * @param   cbFrame             The GSO frame size (header proto + payload).
 */
DECLINLINE(uint32_t) PDMNetGsoCalcSegmentCount(PCPDMNETWORKGSO pGso, size_t cbFrame)
{
    size_t cbPayload;
    Assert(PDMNetGsoIsValid(pGso, sizeof(*pGso), cbFrame));
    cbPayload = cbFrame - pGso->cbHdrsSeg;
    return (uint32_t)((cbPayload + pGso->cbMaxSeg - 1) / pGso->cbMaxSeg);
}


/**
 * Used to find the IPv6 header when handling 4to6 tunneling.
 *
 * @returns Offset of the IPv6 header.
 * @param   pbSegHdrs           The headers / frame start.
 * @param   offIPv4Hdr          The offset of the IPv4 header.
 */
DECLINLINE(uint8_t) pgmNetGsoCalcIpv6Offset(uint8_t *pbSegHdrs, uint8_t offIPv4Hdr)
{
    PCRTNETIPV4 pIPv4Hdr = (PCRTNETIPV4)&pbSegHdrs[offIPv4Hdr];
    return offIPv4Hdr + pIPv4Hdr->ip_hl * 4;
}


/**
 * Update an UDP header after carving out a segment
 *
 * @param   u32PseudoSum        The pseudo checksum.
 * @param   pbSegHdrs           Pointer to the header bytes / frame start.
 * @param   offUdpHdr           The offset into @a pbSegHdrs of the UDP header.
 * @param   pbPayload           Pointer to the payload bytes.
 * @param   cbPayload           The amount of payload.
 * @param   cbHdrs              The size of all the headers.
 * @param   enmCsumType         Whether to checksum the payload, the pseudo
 *                              header or nothing.
 * @internal
 */
DECLINLINE(void) pdmNetGsoUpdateUdpHdr(uint32_t u32PseudoSum, uint8_t *pbSegHdrs, uint8_t offUdpHdr,
                                       uint8_t const *pbPayload, uint32_t cbPayload, uint8_t cbHdrs,
                                       PDMNETCSUMTYPE enmCsumType)
{
    PRTNETUDP pUdpHdr = (PRTNETUDP)&pbSegHdrs[offUdpHdr];
    pUdpHdr->uh_ulen  = RT_H2N_U16(cbPayload + cbHdrs - offUdpHdr);
    switch (enmCsumType)
    {
        case PDMNETCSUMTYPE_NONE:
            pUdpHdr->uh_sum = 0;
            break;
        case PDMNETCSUMTYPE_COMPLETE:
            pUdpHdr->uh_sum = RTNetUDPChecksum(u32PseudoSum, pUdpHdr);
            break;
        case PDMNETCSUMTYPE_PSEUDO:
            pUdpHdr->uh_sum = ~RTNetIPv4FinalizeChecksum(u32PseudoSum);
            break;
        default:
            NOREF(pbPayload);
            AssertFailed();
            break;
    }
}


/**
 * Update an UDP header after carving out an IP fragment
 *
 * @param   u32PseudoSum        The pseudo checksum.
 * @param   pbSegHdrs           Pointer to the header bytes copy
 * @param   pbFrame             Pointer to the frame start.
 * @param   offUdpHdr           The offset into @a pbSegHdrs of the UDP header.
 *
 * @internal
 */
DECLINLINE(void) pdmNetGsoUpdateUdpHdrUfo(uint32_t u32PseudoSum, uint8_t *pbSegHdrs, const uint8_t *pbFrame, uint8_t offUdpHdr)
{
    PCRTNETUDP pcUdpHdrOrig = (PCRTNETUDP)&pbFrame[offUdpHdr];
    PRTNETUDP  pUdpHdr = (PRTNETUDP)&pbSegHdrs[offUdpHdr];
    pUdpHdr->uh_sum = RTNetUDPChecksum(u32PseudoSum, pcUdpHdrOrig);
}


/**
 * Update a TCP header after carving out a segment.
 *
 * @param   u32PseudoSum        The pseudo checksum.
 * @param   pbSegHdrs           Pointer to the header bytes / frame start.
 * @param   offTcpHdr           The offset into @a pbSegHdrs of the TCP header.
 * @param   pbPayload           Pointer to the payload bytes.
 * @param   cbPayload           The amount of payload.
 * @param   offPayload          The offset into the payload that we're splitting
 *                              up.  We're ASSUMING that the payload follows
 *                              immediately after the TCP header w/ options.
 * @param   cbHdrs              The size of all the headers.
 * @param   fLastSeg            Set if this is the last segment.
 * @param   enmCsumType         Whether to checksum the payload, the pseudo
 *                              header or nothing.
 * @internal
 */
DECLINLINE(void) pdmNetGsoUpdateTcpHdr(uint32_t u32PseudoSum, uint8_t *pbSegHdrs, uint8_t offTcpHdr,
                                       uint8_t const *pbPayload, uint32_t cbPayload, uint32_t offPayload, uint8_t cbHdrs,
                                       bool fLastSeg, PDMNETCSUMTYPE enmCsumType)
{
    PRTNETTCP pTcpHdr = (PRTNETTCP)&pbSegHdrs[offTcpHdr];
    pTcpHdr->th_seq = RT_H2N_U32(RT_N2H_U32(pTcpHdr->th_seq) + offPayload);
    if (!fLastSeg)
        pTcpHdr->th_flags &= ~(RTNETTCP_F_FIN | RTNETTCP_F_PSH);
    switch (enmCsumType)
    {
        case PDMNETCSUMTYPE_NONE:
            pTcpHdr->th_sum = 0;
            break;
        case PDMNETCSUMTYPE_COMPLETE:
            pTcpHdr->th_sum = RTNetTCPChecksum(u32PseudoSum, pTcpHdr, pbPayload, cbPayload);
            break;
        case PDMNETCSUMTYPE_PSEUDO:
            pTcpHdr->th_sum = ~RTNetIPv4FinalizeChecksum(u32PseudoSum);
            break;
        default:
            NOREF(cbHdrs);
            AssertFailed();
            break;
    }
}


/**
 * Updates a IPv6 header after carving out a segment.
 *
 * @returns 32-bit intermediary checksum value for the pseudo header.
 * @param   pbSegHdrs           Pointer to the header bytes.
 * @param   offIpHdr            The offset into @a pbSegHdrs of the IP header.
 * @param   cbSegPayload        The amount of segmented payload.  Not to be
 *                              confused with the IP payload.
 * @param   cbHdrs              The size of all the headers.
 * @param   offPktHdr           Offset of the protocol packet header.  For the
 *                              pseudo header checksum calulation.
 * @param   bProtocol           The protocol type.  For the pseudo header.
 * @internal
 */
DECLINLINE(uint32_t) pdmNetGsoUpdateIPv6Hdr(uint8_t *pbSegHdrs, uint8_t offIpHdr, uint32_t cbSegPayload, uint8_t cbHdrs,
                                            uint8_t offPktHdr, uint8_t bProtocol)
{
    PRTNETIPV6 pIpHdr  = (PRTNETIPV6)&pbSegHdrs[offIpHdr];
    uint16_t cbPayload = (uint16_t)(cbHdrs - (offIpHdr + sizeof(RTNETIPV6)) + cbSegPayload);
    pIpHdr->ip6_plen   = RT_H2N_U16(cbPayload);
    return RTNetIPv6PseudoChecksumEx(pIpHdr, bProtocol, (uint16_t)(cbHdrs - offPktHdr + cbSegPayload));
}


/**
 * Updates a IPv4 header after carving out a segment.
 *
 * @returns 32-bit intermediary checksum value for the pseudo header.
 * @param   pbSegHdrs           Pointer to the header bytes.
 * @param   offIpHdr            The offset into @a pbSegHdrs of the IP header.
 * @param   cbSegPayload        The amount of segmented payload.
 * @param   iSeg                The segment index.
 * @param   cbHdrs              The size of all the headers.
 * @internal
 */
DECLINLINE(uint32_t) pdmNetGsoUpdateIPv4Hdr(uint8_t *pbSegHdrs, uint8_t offIpHdr, uint32_t cbSegPayload,
                                            uint32_t iSeg, uint8_t cbHdrs)
{
    PRTNETIPV4 pIpHdr = (PRTNETIPV4)&pbSegHdrs[offIpHdr];
    pIpHdr->ip_len    = RT_H2N_U16(cbHdrs - offIpHdr + cbSegPayload);
    pIpHdr->ip_id     = RT_H2N_U16(RT_N2H_U16(pIpHdr->ip_id) + iSeg);
    pIpHdr->ip_sum    = RTNetIPv4HdrChecksum(pIpHdr);
    return RTNetIPv4PseudoChecksum(pIpHdr);
}


/**
 * Updates a IPv4 header after carving out an IP fragment.
 *
 * @param   pbSegHdrs           Pointer to the header bytes.
 * @param   offIpHdr            The offset into @a pbSegHdrs of the IP header.
 * @param   cbSegPayload        The amount of segmented payload.
 * @param   offFragment         The offset of this fragment for reassembly.
 * @param   cbHdrs              The size of all the headers.
 * @param   fLastFragment       True if this is the last fragment of datagram.
 * @internal
 */
DECLINLINE(void) pdmNetGsoUpdateIPv4HdrUfo(uint8_t *pbSegHdrs, uint8_t offIpHdr, uint32_t cbSegPayload,
                                           uint32_t offFragment, uint8_t cbHdrs, bool fLastFragment)
{
    PRTNETIPV4 pIpHdr = (PRTNETIPV4)&pbSegHdrs[offIpHdr];
    pIpHdr->ip_len    = RT_H2N_U16(cbHdrs - offIpHdr + cbSegPayload);
    pIpHdr->ip_off    = RT_H2N_U16((offFragment / 8) | (fLastFragment ? 0 : RTNETIPV4_FLAGS_MF));
    pIpHdr->ip_sum    = RTNetIPv4HdrChecksum(pIpHdr);
}


/**
 * Carves out the specified segment in a destructive manner.
 *
 * This is for sequentially carving out segments and pushing them along for
 * processing or sending.  To avoid allocating a temporary buffer for
 * constructing the segment in, we trash the previous frame by putting the
 * header at the end of it.
 *
 * @returns Pointer to the segment frame that we've carved out.
 * @param   pGso                The GSO context data.
 * @param   pbFrame             Pointer to the GSO frame.
 * @param   cbFrame             The size of the GSO frame.
 * @param   pbHdrScatch         Pointer to a pGso->cbHdrs sized area where we
 *                              can save the original header prototypes on the
 *                              first call (@a iSeg is 0) and retrieve it on
 *                              susequent calls. (Just use a 256 bytes
 *                              buffer to make life easy.)
 * @param   iSeg                The segment that we're carving out (0-based).
 * @param   cSegs               The number of segments in the GSO frame.  Use
 *                              PDMNetGsoCalcSegmentCount to find this.
 * @param   pcbSegFrame         Where to return the size of the returned segment
 *                              frame.
 */
DECLINLINE(void *) PDMNetGsoCarveSegmentQD(PCPDMNETWORKGSO pGso, uint8_t *pbFrame, size_t cbFrame, uint8_t *pbHdrScatch,
                                           uint32_t iSeg, uint32_t cSegs, uint32_t *pcbSegFrame)
{
    /*
     * Figure out where the payload is and where the header starts before we
     * do the protocol specific carving.
     *
     * UDP GSO uses IPv4 fragmentation, meaning that UDP header is present in
     * the first fragment only. When computing the total frame size of the
     * first fragment we need to use PDMNETWORKGSO::cbHdrsTotal instead of
     * PDMNETWORKGSO::cbHdrsSeg. In case of TCP GSO both cbHdrsTotal and
     * cbHdrsSeg have the same value, so it will work as well.
     */
    uint8_t * const pbSegHdrs    = pbFrame + pGso->cbMaxSeg * iSeg;
    uint8_t * const pbSegPayload = pbSegHdrs + pGso->cbHdrsSeg;
    uint32_t const  cbSegPayload = pdmNetSegPayloadLen(pGso, iSeg, cSegs, (uint32_t)cbFrame);
    uint32_t const  cbSegFrame   = cbSegPayload + (iSeg ? pGso->cbHdrsSeg : pGso->cbHdrsTotal);

    /*
     * Check assumptions (doing it after declaring the variables because of C).
     */
    Assert(iSeg < cSegs);
    Assert(cSegs == PDMNetGsoCalcSegmentCount(pGso, cbFrame));
    Assert(PDMNetGsoIsValid(pGso, sizeof(*pGso), cbFrame));

    /*
     * Copy the header and do the protocol specific massaging of it.
     */
    if (iSeg != 0)
        memcpy(pbSegHdrs, pbHdrScatch, pGso->cbHdrsSeg);
    else
        memcpy(pbHdrScatch, pbSegHdrs, pGso->cbHdrsSeg); /* There is no need to save UDP header */

    switch ((PDMNETWORKGSOTYPE)pGso->u8Type)
    {
        case PDMNETWORKGSOTYPE_IPV4_TCP:
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv4Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, iSeg, pGso->cbHdrsSeg),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, iSeg * pGso->cbMaxSeg,
                                  pGso->cbHdrsSeg, iSeg + 1 == cSegs, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_IPV4_UDP:
            if (iSeg == 0)
            {
                /* uh_ulen shall not exceed cbFrame - pGso->offHdr2 (offset of UDP header) */
                PRTNETUDP pUdpHdr = (PRTNETUDP)&pbFrame[pGso->offHdr2];
                Assert(pGso->offHdr2 + RT_UOFFSET_AFTER(RTNETUDP, uh_ulen) <= cbFrame);
                if ((unsigned)(pGso->offHdr2 + RT_BE2H_U16(pUdpHdr->uh_ulen)) > cbFrame)
                {
                    size_t cbUdp = cbFrame - pGso->offHdr2;
                    if (cbUdp >= UINT16_MAX)
                        pUdpHdr->uh_ulen = UINT16_MAX;
                    else
                        pUdpHdr->uh_ulen = RT_H2BE_U16((uint16_t)cbUdp);
                }
                /* uh_ulen shall be at least the size of UDP header */
                if (RT_BE2H_U16(pUdpHdr->uh_ulen) < sizeof(RTNETUDP))
                    pUdpHdr->uh_ulen = RT_H2BE_U16(sizeof(RTNETUDP));
                pdmNetGsoUpdateUdpHdrUfo(RTNetIPv4PseudoChecksum((PRTNETIPV4)&pbFrame[pGso->offHdr1]),
                                         pbSegHdrs, pbFrame, pGso->offHdr2);
            }
            pdmNetGsoUpdateIPv4HdrUfo(pbSegHdrs, pGso->offHdr1, cbSegPayload, iSeg * pGso->cbMaxSeg,
                                      pdmNetSegHdrLen(pGso, iSeg), iSeg + 1 == cSegs);
            break;
        case PDMNETWORKGSOTYPE_IPV6_TCP:
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv6Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, pGso->cbHdrsSeg,
                                                         pGso->offHdr2, RTNETIPV4_PROT_TCP),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, iSeg * pGso->cbMaxSeg,
                                  pGso->cbHdrsSeg, iSeg + 1 == cSegs, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_IPV6_UDP:
            pdmNetGsoUpdateUdpHdr(pdmNetGsoUpdateIPv6Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, pGso->cbHdrsSeg,
                                                         pGso->offHdr2, RTNETIPV4_PROT_UDP),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, pGso->cbHdrsSeg, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_IPV4_IPV6_TCP:
            pdmNetGsoUpdateIPv4Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, iSeg, pGso->cbHdrsSeg);
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv6Hdr(pbSegHdrs, pgmNetGsoCalcIpv6Offset(pbSegHdrs, pGso->offHdr1),
                                                         cbSegPayload, pGso->cbHdrsSeg, pGso->offHdr2, RTNETIPV4_PROT_TCP),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, iSeg * pGso->cbMaxSeg,
                                  pGso->cbHdrsSeg, iSeg + 1 == cSegs, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_IPV4_IPV6_UDP:
            pdmNetGsoUpdateIPv4Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, iSeg, pGso->cbHdrsSeg);
            pdmNetGsoUpdateUdpHdr(pdmNetGsoUpdateIPv6Hdr(pbSegHdrs, pgmNetGsoCalcIpv6Offset(pbSegHdrs, pGso->offHdr1),
                                                         cbSegPayload, pGso->cbHdrsSeg, pGso->offHdr2, RTNETIPV4_PROT_UDP),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, pGso->cbHdrsSeg, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_INVALID:
        case PDMNETWORKGSOTYPE_END:
            /* no default! wnat gcc warnings. */
            break;
    }

    *pcbSegFrame = cbSegFrame;
    return pbSegHdrs;
}


/**
 * Carves out the specified segment in a non-destructive manner.
 *
 * The segment headers and segment payload is kept separate here.  The GSO frame
 * is still expected to be one linear chunk of data, but we don't modify any of
 * it.
 *
 * @returns The offset into the GSO frame of the payload.
 * @param   pGso                The GSO context data.
 * @param   pbFrame             Pointer to the GSO frame.  Used for retrieving
 *                              the header prototype and for checksumming the
 *                              payload.  The buffer is not modified.
 * @param   cbFrame             The size of the GSO frame.
 * @param   iSeg                The segment that we're carving out (0-based).
 * @param   cSegs               The number of segments in the GSO frame.  Use
 *                              PDMNetGsoCalcSegmentCount to find this.
 * @param   pbSegHdrs           Where to return the headers for the segment
 *                              that's been carved out.  The buffer must be at
 *                              least pGso->cbHdrs in size, using a 256 byte
 *                              buffer is a recommended simplification.
 * @param   pcbSegHdrs          Where to return the size of the returned
 *                              segment headers.
 * @param   pcbSegPayload       Where to return the size of the returned
 *                              segment payload.
 */
DECLINLINE(uint32_t) PDMNetGsoCarveSegment(PCPDMNETWORKGSO pGso, const uint8_t *pbFrame, size_t cbFrame,
                                           uint32_t iSeg, uint32_t cSegs, uint8_t *pbSegHdrs,
                                           uint32_t *pcbSegHdrs, uint32_t *pcbSegPayload)
{
    /*
     * Figure out where the payload is and where the header starts before we
     * do the protocol specific carving.
     */
    uint32_t const        cbSegHdrs    = pdmNetSegHdrLen(pGso, iSeg);
    uint8_t const * const pbSegPayload = pbFrame + cbSegHdrs + iSeg * pGso->cbMaxSeg;
    uint32_t const        cbSegPayload = pdmNetSegPayloadLen(pGso, iSeg, cSegs, (uint32_t)cbFrame);

    /*
     * Check assumptions (doing it after declaring the variables because of C).
     */
    Assert(iSeg < cSegs);
    Assert(cSegs == PDMNetGsoCalcSegmentCount(pGso, cbFrame));
    Assert(PDMNetGsoIsValid(pGso, sizeof(*pGso), cbFrame));

    /*
     * Copy the header and do the protocol specific massaging of it.
     */
    memcpy(pbSegHdrs, pbFrame, pGso->cbHdrsTotal); /* include UDP header */

    switch ((PDMNETWORKGSOTYPE)pGso->u8Type)
    {
        case PDMNETWORKGSOTYPE_IPV4_TCP:
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv4Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, iSeg, cbSegHdrs),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, iSeg * pGso->cbMaxSeg,
                                  cbSegHdrs, iSeg + 1 == cSegs, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_IPV4_UDP:
            if (iSeg == 0)
            {
                /* uh_ulen shall not exceed cbFrame - pGso->offHdr2 (offset of UDP header) */
                PRTNETUDP pUdpHdr = (PRTNETUDP)&pbFrame[pGso->offHdr2];
                Assert(pGso->offHdr2 + RT_UOFFSET_AFTER(RTNETUDP, uh_ulen) <= cbFrame);
                if ((unsigned)(pGso->offHdr2 + RT_BE2H_U16(pUdpHdr->uh_ulen)) > cbFrame)
                {
                    size_t cbUdp = cbFrame - pGso->offHdr2;
                    if (cbUdp >= UINT16_MAX)
                        pUdpHdr->uh_ulen = UINT16_MAX;
                    else
                        pUdpHdr->uh_ulen = RT_H2BE_U16((uint16_t)cbUdp);
                }
                /* uh_ulen shall be at least the size of UDP header */
                if (RT_BE2H_U16(pUdpHdr->uh_ulen) < sizeof(RTNETUDP))
                    pUdpHdr->uh_ulen = RT_H2BE_U16(sizeof(RTNETUDP));
                pdmNetGsoUpdateUdpHdrUfo(RTNetIPv4PseudoChecksum((PRTNETIPV4)&pbFrame[pGso->offHdr1]),
                                         pbSegHdrs, pbFrame, pGso->offHdr2);
            }
            pdmNetGsoUpdateIPv4HdrUfo(pbSegHdrs, pGso->offHdr1, cbSegPayload, iSeg * pGso->cbMaxSeg,
                                      cbSegHdrs, iSeg + 1 == cSegs);
            break;
        case PDMNETWORKGSOTYPE_IPV6_TCP:
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv6Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, cbSegHdrs,
                                                         pGso->offHdr2, RTNETIPV4_PROT_TCP),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, iSeg * pGso->cbMaxSeg,
                                  cbSegHdrs, iSeg + 1 == cSegs, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_IPV6_UDP:
            pdmNetGsoUpdateUdpHdr(pdmNetGsoUpdateIPv6Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, cbSegHdrs,
                                                         pGso->offHdr2, RTNETIPV4_PROT_UDP),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, cbSegHdrs, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_IPV4_IPV6_TCP:
            pdmNetGsoUpdateIPv4Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, iSeg, cbSegHdrs);
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv6Hdr(pbSegHdrs, pgmNetGsoCalcIpv6Offset(pbSegHdrs, pGso->offHdr1),
                                                         cbSegPayload, cbSegHdrs, pGso->offHdr2, RTNETIPV4_PROT_TCP),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, iSeg * pGso->cbMaxSeg,
                                  cbSegHdrs, iSeg + 1 == cSegs, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_IPV4_IPV6_UDP:
            pdmNetGsoUpdateIPv4Hdr(pbSegHdrs, pGso->offHdr1, cbSegPayload, iSeg, cbSegHdrs);
            pdmNetGsoUpdateUdpHdr(pdmNetGsoUpdateIPv6Hdr(pbSegHdrs, pgmNetGsoCalcIpv6Offset(pbSegHdrs, pGso->offHdr1),
                                                         cbSegPayload, cbSegHdrs, pGso->offHdr2, RTNETIPV4_PROT_UDP),
                                  pbSegHdrs, pGso->offHdr2, pbSegPayload, cbSegPayload, cbSegHdrs, PDMNETCSUMTYPE_COMPLETE);
            break;
        case PDMNETWORKGSOTYPE_INVALID:
        case PDMNETWORKGSOTYPE_END:
            /* no default! wnat gcc warnings. */
            break;
    }

    *pcbSegHdrs    = cbSegHdrs;
    *pcbSegPayload = cbSegPayload;
    return cbSegHdrs + iSeg * pGso->cbMaxSeg;
}


/**
 * Prepares the GSO frame for direct use without any segmenting.
 *
 * @param   pGso                The GSO context.
 * @param   pvFrame             The frame to prepare.
 * @param   cbFrame             The frame size.
 * @param   enmCsumType         Whether to checksum the payload, the pseudo
 *                              header or nothing.
 */
DECLINLINE(void) PDMNetGsoPrepForDirectUse(PCPDMNETWORKGSO pGso, void *pvFrame, size_t cbFrame, PDMNETCSUMTYPE enmCsumType)
{
    /*
     * Figure out where the payload is and where the header starts before we
     * do the protocol bits.
     */
    uint8_t * const pbHdrs    = (uint8_t *)pvFrame;
    uint8_t * const pbPayload = pbHdrs  + pGso->cbHdrsTotal;
    uint32_t  const cbFrame32 = (uint32_t)cbFrame;
    uint32_t  const cbPayload = cbFrame32 - pGso->cbHdrsTotal;

    /*
     * Check assumptions (doing it after declaring the variables because of C).
     */
    Assert(PDMNetGsoIsValid(pGso, sizeof(*pGso), cbFrame));

    /*
     * Get down to busienss.
     */
    switch ((PDMNETWORKGSOTYPE)pGso->u8Type)
    {
        case PDMNETWORKGSOTYPE_IPV4_TCP:
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv4Hdr(pbHdrs, pGso->offHdr1, cbFrame32 - pGso->cbHdrsTotal, 0, pGso->cbHdrsTotal),
                                  pbHdrs, pGso->offHdr2, pbPayload, cbPayload, 0, pGso->cbHdrsTotal, true, enmCsumType);
            break;
        case PDMNETWORKGSOTYPE_IPV4_UDP:
            pdmNetGsoUpdateUdpHdr(pdmNetGsoUpdateIPv4Hdr(pbHdrs, pGso->offHdr1, cbFrame32 - pGso->cbHdrsTotal, 0, pGso->cbHdrsTotal),
                                  pbHdrs, pGso->offHdr2, pbPayload, cbPayload, pGso->cbHdrsTotal, enmCsumType);
            break;
        case PDMNETWORKGSOTYPE_IPV6_TCP:
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv6Hdr(pbHdrs, pGso->offHdr1, cbPayload, pGso->cbHdrsTotal,
                                                         pGso->offHdr2, RTNETIPV4_PROT_TCP),
                                  pbHdrs, pGso->offHdr2, pbPayload, cbPayload, 0, pGso->cbHdrsTotal, true, enmCsumType);
            break;
        case PDMNETWORKGSOTYPE_IPV6_UDP:
            pdmNetGsoUpdateUdpHdr(pdmNetGsoUpdateIPv6Hdr(pbHdrs, pGso->offHdr1, cbPayload, pGso->cbHdrsTotal,
                                                         pGso->offHdr2, RTNETIPV4_PROT_UDP),
                                  pbHdrs, pGso->offHdr2, pbPayload, cbPayload, pGso->cbHdrsTotal, enmCsumType);
            break;
        case PDMNETWORKGSOTYPE_IPV4_IPV6_TCP:
            pdmNetGsoUpdateIPv4Hdr(pbHdrs, pGso->offHdr1, cbPayload, 0, pGso->cbHdrsTotal);
            pdmNetGsoUpdateTcpHdr(pdmNetGsoUpdateIPv6Hdr(pbHdrs, pgmNetGsoCalcIpv6Offset(pbHdrs, pGso->offHdr1),
                                                         cbPayload, pGso->cbHdrsTotal, pGso->offHdr2, RTNETIPV4_PROT_TCP),
                                  pbHdrs, pGso->offHdr2, pbPayload, cbPayload, 0, pGso->cbHdrsTotal, true, enmCsumType);
            break;
        case PDMNETWORKGSOTYPE_IPV4_IPV6_UDP:
            pdmNetGsoUpdateIPv4Hdr(pbHdrs, pGso->offHdr1, cbPayload, 0, pGso->cbHdrsTotal);
            pdmNetGsoUpdateUdpHdr(pdmNetGsoUpdateIPv6Hdr(pbHdrs, pgmNetGsoCalcIpv6Offset(pbHdrs, pGso->offHdr1),
                                                         cbPayload, pGso->cbHdrsTotal, pGso->offHdr2, RTNETIPV4_PROT_UDP),
                                  pbHdrs, pGso->offHdr2, pbPayload, cbPayload, pGso->cbHdrsTotal, enmCsumType);
            break;
        case PDMNETWORKGSOTYPE_INVALID:
        case PDMNETWORKGSOTYPE_END:
            /* no default! wnat gcc warnings. */
            break;
    }
}


/**
 * Gets the GSO type name string.
 *
 * @returns Pointer to read only name string.
 * @param   enmType             The type.
 */
DECLINLINE(const char *) PDMNetGsoTypeName(PDMNETWORKGSOTYPE enmType)
{
    switch (enmType)
    {
        case PDMNETWORKGSOTYPE_IPV4_TCP:        return "TCPv4";
        case PDMNETWORKGSOTYPE_IPV6_TCP:        return "TCPv6";
        case PDMNETWORKGSOTYPE_IPV4_UDP:        return "UDPv4";
        case PDMNETWORKGSOTYPE_IPV6_UDP:        return "UDPv6";
        case PDMNETWORKGSOTYPE_IPV4_IPV6_TCP:   return "4to6TCP";
        case PDMNETWORKGSOTYPE_IPV4_IPV6_UDP:   return "4to6UDP";
        case PDMNETWORKGSOTYPE_INVALID:         return "invalid";
        case PDMNETWORKGSOTYPE_END:             return "end";
    }
    return "bad-gso-type";
}

/** @} */

#endif /* !VBOX_INCLUDED_vmm_pdmnetinline_h */

