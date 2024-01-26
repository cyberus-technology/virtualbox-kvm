/* $Id: DevVGA-SVGA3d-dx.cpp $ */
/** @file
 * DevSVGA3d - VMWare SVGA device, 3D parts - Common code for DX backend interface.
 */

/*
 * Copyright (C) 2020-2023 Oracle and/or its affiliates.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_VMSVGA
#include <VBox/AssertGuest.h>
#include <iprt/errcore.h>
#include <VBox/log.h>
#include <VBox/vmm/pdmdev.h>

#include <iprt/assert.h>
#include <iprt/mem.h>

#include <VBoxVideo.h> /* required by DevVGA.h */

/* should go BEFORE any other DevVGA include to make all DevVGA.h config defines be visible */
#include "DevVGA.h"

#include "DevVGA-SVGA.h"
#include "DevVGA-SVGA3d.h"
#include "DevVGA-SVGA3d-internal.h"
#include "DevVGA-SVGA-internal.h"


/*
 * Helpers.
 */

static int dxMobWrite(PVMSVGAR3STATE pSvgaR3State, SVGAMobId mobid, uint32_t off, void const *pvData, uint32_t cbData)
{
    PVMSVGAMOB pMob = vmsvgaR3MobGet(pSvgaR3State, mobid);
    ASSERT_GUEST_RETURN(pMob, VERR_INVALID_STATE);

    return vmsvgaR3MobWrite(pSvgaR3State, pMob, off, pvData, cbData);
}


/*
 *
 * Command handlers.
 *
 */

int vmsvga3dDXUnbindContext(PVGASTATECC pThisCC, uint32_t cid, SVGADXContextMobFormat *pSvgaDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXBindContext, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, cid, &pDXContext);
    AssertRCReturn(rc, rc);

    /* Copy the host structure back to the guest memory. */
    memcpy(pSvgaDXContext, &pDXContext->svgaDXContext, sizeof(*pSvgaDXContext));

    return rc;
}


int vmsvga3dDXSwitchContext(PVGASTATECC pThisCC, uint32_t cid)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSwitchContext, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, cid, &pDXContext);
    AssertRCReturn(rc, rc);

    /* Notify the host backend that context is about to be switched. */
    rc = pSvgaR3State->pFuncsDX->pfnDXSwitchContext(pThisCC, pDXContext);
    if (rc == VINF_NOT_IMPLEMENTED || RT_FAILURE(rc))
        return rc;

    /** @todo Keep track of changes in the pipeline and apply only modified state. */
    /* It is not necessary to restore SVGADXContextMobFormat::shaderState::shaderResources
     * because they are applied by the backend before each Draw call.
     */
    #define DX_STATE_VS                0x00000001
    #define DX_STATE_PS                0x00000002
    #define DX_STATE_SAMPLERS          0x00000004
    #define DX_STATE_INPUTLAYOUT       0x00000008
    #define DX_STATE_TOPOLOGY          0x00000010
    #define DX_STATE_BLENDSTATE        0x00000080
    #define DX_STATE_DEPTHSTENCILSTATE 0x00000100
    #define DX_STATE_SOTARGETS         0x00000200
    #define DX_STATE_VIEWPORTS         0x00000400
    #define DX_STATE_SCISSORRECTS      0x00000800
    #define DX_STATE_RASTERIZERSTATE   0x00001000
    #define DX_STATE_RENDERTARGETS     0x00002000
    #define DX_STATE_GS                0x00004000
    uint32_t u32TrackedState = 0
        | DX_STATE_VS
        | DX_STATE_PS
        | DX_STATE_SAMPLERS
        | DX_STATE_INPUTLAYOUT
        | DX_STATE_TOPOLOGY
        | DX_STATE_BLENDSTATE
        | DX_STATE_DEPTHSTENCILSTATE
        | DX_STATE_SOTARGETS
        | DX_STATE_VIEWPORTS
        | DX_STATE_SCISSORRECTS
        | DX_STATE_RASTERIZERSTATE
        | DX_STATE_RENDERTARGETS
        | DX_STATE_GS
        ;

    LogFunc(("cid = %d, state = 0x%08X\n", cid, u32TrackedState));

    if (u32TrackedState & DX_STATE_VS)
    {
        u32TrackedState &= ~DX_STATE_VS;

        SVGA3dShaderType const shaderType = SVGA3D_SHADERTYPE_VS;

        uint32_t const idxShaderState = shaderType - SVGA3D_SHADERTYPE_MIN;
        SVGA3dShaderId shaderId = pDXContext->svgaDXContext.shaderState[idxShaderState].shaderId;

        rc = pSvgaR3State->pFuncsDX->pfnDXSetShader(pThisCC, pDXContext, shaderId, shaderType);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_PS)
    {
        u32TrackedState &= ~DX_STATE_PS;

        SVGA3dShaderType const shaderType = SVGA3D_SHADERTYPE_PS;

        uint32_t const idxShaderState = shaderType - SVGA3D_SHADERTYPE_MIN;
        SVGA3dShaderId shaderId = pDXContext->svgaDXContext.shaderState[idxShaderState].shaderId;

        rc = pSvgaR3State->pFuncsDX->pfnDXSetShader(pThisCC, pDXContext, shaderId, shaderType);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_GS)
    {
        u32TrackedState &= ~DX_STATE_GS;

        SVGA3dShaderType const shaderType = SVGA3D_SHADERTYPE_GS;

        uint32_t const idxShaderState = shaderType - SVGA3D_SHADERTYPE_MIN;
        SVGA3dShaderId shaderId = pDXContext->svgaDXContext.shaderState[idxShaderState].shaderId;

        rc = pSvgaR3State->pFuncsDX->pfnDXSetShader(pThisCC, pDXContext, shaderId, shaderType);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_SAMPLERS)
    {
        u32TrackedState &= ~DX_STATE_SAMPLERS;

        for (int i = SVGA3D_SHADERTYPE_MIN; i < SVGA3D_SHADERTYPE_MAX; ++i)
        {
            SVGA3dShaderType const shaderType = (SVGA3dShaderType)i;
            uint32_t const idxShaderState = shaderType - SVGA3D_SHADERTYPE_MIN;

            uint32_t startSampler = 0;
            uint32_t cSamplerId = SVGA3D_DX_MAX_SAMPLERS;
            SVGA3dSamplerId *paSamplerId = &pDXContext->svgaDXContext.shaderState[idxShaderState].samplers[0];

            rc = pSvgaR3State->pFuncsDX->pfnDXSetSamplers(pThisCC, pDXContext, startSampler, shaderType, cSamplerId, paSamplerId);
            AssertRC(rc);
        }
    }


    if (u32TrackedState & DX_STATE_INPUTLAYOUT)
    {
        u32TrackedState &= ~DX_STATE_INPUTLAYOUT;

        SVGA3dElementLayoutId const elementLayoutId = pDXContext->svgaDXContext.inputAssembly.layoutId;

        rc = pSvgaR3State->pFuncsDX->pfnDXSetInputLayout(pThisCC, pDXContext, elementLayoutId);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_TOPOLOGY)
    {
        u32TrackedState &= ~DX_STATE_TOPOLOGY;

        SVGA3dPrimitiveType const topology = (SVGA3dPrimitiveType)pDXContext->svgaDXContext.inputAssembly.topology;

        if (topology != SVGA3D_PRIMITIVE_INVALID)
            rc = pSvgaR3State->pFuncsDX->pfnDXSetTopology(pThisCC, pDXContext, topology);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_BLENDSTATE)
    {
        u32TrackedState &= ~DX_STATE_BLENDSTATE;

        SVGA3dBlendStateId const blendId = pDXContext->svgaDXContext.renderState.blendStateId;
        /* SVGADXContextMobFormat uses uint32_t array to store the blend factors, however they are in fact 32 bit floats. */
        float const *paBlendFactor = (float *)&pDXContext->svgaDXContext.renderState.blendFactor[0];
        uint32_t const sampleMask = pDXContext->svgaDXContext.renderState.sampleMask;

        rc = pSvgaR3State->pFuncsDX->pfnDXSetBlendState(pThisCC, pDXContext, blendId, paBlendFactor, sampleMask);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_DEPTHSTENCILSTATE)
    {
        u32TrackedState &= ~DX_STATE_DEPTHSTENCILSTATE;

        SVGA3dDepthStencilStateId const depthStencilId = pDXContext->svgaDXContext.renderState.depthStencilStateId;
        uint32_t const stencilRef = pDXContext->svgaDXContext.renderState.stencilRef;

        rc = pSvgaR3State->pFuncsDX->pfnDXSetDepthStencilState(pThisCC, pDXContext, depthStencilId, stencilRef);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_SOTARGETS)
    {
        u32TrackedState &= ~DX_STATE_SOTARGETS;

        uint32_t cSoTarget = SVGA3D_DX_MAX_SOTARGETS;
        SVGA3dSoTarget aSoTarget[SVGA3D_DX_MAX_SOTARGETS];
        for (uint32_t i = 0; i < SVGA3D_DX_MAX_SOTARGETS; ++i)
        {
            aSoTarget[i].sid = pDXContext->svgaDXContext.streamOut.targets[i];
            /** @todo Offset is not stored in svgaDXContext. Should it be stored elsewhere by the host? */
            aSoTarget[i].offset = 0;
            aSoTarget[i].sizeInBytes = 0;
        }

        rc = pSvgaR3State->pFuncsDX->pfnDXSetSOTargets(pThisCC, pDXContext, cSoTarget, aSoTarget);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_VIEWPORTS)
    {
        u32TrackedState &= ~DX_STATE_VIEWPORTS;

        uint32_t const cViewport = pDXContext->svgaDXContext.numViewports;
        SVGA3dViewport const *paViewport = &pDXContext->svgaDXContext.viewports[0];

        rc = pSvgaR3State->pFuncsDX->pfnDXSetViewports(pThisCC, pDXContext, cViewport, paViewport);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_SCISSORRECTS)
    {
        u32TrackedState &= ~DX_STATE_SCISSORRECTS;

        uint32_t const cRect = pDXContext->svgaDXContext.numScissorRects;
        SVGASignedRect const *paRect = &pDXContext->svgaDXContext.scissorRects[0];

        rc = pSvgaR3State->pFuncsDX->pfnDXSetScissorRects(pThisCC, pDXContext, cRect, paRect);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_RASTERIZERSTATE)
    {
        u32TrackedState &= ~DX_STATE_RASTERIZERSTATE;

        SVGA3dRasterizerStateId const rasterizerId = pDXContext->svgaDXContext.renderState.rasterizerStateId;

        rc = pSvgaR3State->pFuncsDX->pfnDXSetRasterizerState(pThisCC, pDXContext, rasterizerId);
        AssertRC(rc);
    }


    if (u32TrackedState & DX_STATE_RENDERTARGETS)
    {
        u32TrackedState &= ~DX_STATE_RENDERTARGETS;

        SVGA3dDepthStencilViewId const depthStencilViewId = (SVGA3dDepthStencilViewId)pDXContext->svgaDXContext.renderState.depthStencilViewId;
        uint32_t const cRenderTargetViewId = SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS;
        SVGA3dRenderTargetViewId const *paRenderTargetViewId = (SVGA3dRenderTargetViewId *)&pDXContext->svgaDXContext.renderState.renderTargetViewIds[0];

        rc = pSvgaR3State->pFuncsDX->pfnDXSetRenderTargets(pThisCC, pDXContext, depthStencilViewId, cRenderTargetViewId, paRenderTargetViewId);
        AssertRC(rc);
    }

    Assert(u32TrackedState == 0);

    return rc;
}


/**
 * Create a new 3D DX context.
 *
 * @returns VBox status code.
 * @param   pThisCC         The VGA/VMSVGA state for ring-3.
 * @param   cid             Context id to be created.
 */
int vmsvga3dDXDefineContext(PVGASTATECC pThisCC, uint32_t cid)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineContext, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;

    LogFunc(("cid %d\n", cid));

    AssertReturn(cid < SVGA3D_MAX_CONTEXT_IDS, VERR_INVALID_PARAMETER);

    if (cid >= p3dState->cDXContexts)
    {
        /* Grow the array. */
        uint32_t cNew = RT_ALIGN(cid + 15, 16);
        void *pvNew = RTMemRealloc(p3dState->papDXContexts, sizeof(p3dState->papDXContexts[0]) * cNew);
        AssertReturn(pvNew, VERR_NO_MEMORY);
        p3dState->papDXContexts = (PVMSVGA3DDXCONTEXT *)pvNew;
        while (p3dState->cDXContexts < cNew)
        {
            pDXContext = (PVMSVGA3DDXCONTEXT)RTMemAllocZ(sizeof(*pDXContext));
            AssertReturn(pDXContext, VERR_NO_MEMORY);
            pDXContext->cid = SVGA3D_INVALID_ID;
            p3dState->papDXContexts[p3dState->cDXContexts++] = pDXContext;
        }
    }
    /* If one already exists with this id, then destroy it now. */
    if (p3dState->papDXContexts[cid]->cid != SVGA3D_INVALID_ID)
        vmsvga3dDXDestroyContext(pThisCC, cid);

    pDXContext = p3dState->papDXContexts[cid];
    memset(pDXContext, 0, sizeof(*pDXContext));

    /* 0xFFFFFFFF (SVGA_ID_INVALID) is a better initial value than 0 for most of svgaDXContext fields. */
    memset(&pDXContext->svgaDXContext, 0xFF, sizeof(pDXContext->svgaDXContext));
    pDXContext->svgaDXContext.inputAssembly.topology = SVGA3D_PRIMITIVE_INVALID;
    pDXContext->svgaDXContext.numViewports = 0;
    pDXContext->svgaDXContext.numScissorRects = 0;
    pDXContext->cid = cid;

    /* Init the backend specific data. */
    rc = pSvgaR3State->pFuncsDX->pfnDXDefineContext(pThisCC, pDXContext);

    /* Cleanup on failure. */
    if (RT_FAILURE(rc))
        vmsvga3dDXDestroyContext(pThisCC, cid);

    return rc;
}


int vmsvga3dDXDestroyContext(PVGASTATECC pThisCC, uint32_t cid)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyContext, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, cid, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDestroyContext(pThisCC, pDXContext);

    RT_ZERO(*pDXContext);
    pDXContext->cid = SVGA3D_INVALID_ID;

    return rc;
}


int vmsvga3dDXBindContext(PVGASTATECC pThisCC, uint32_t cid, SVGADXContextMobFormat *pSvgaDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXBindContext, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, cid, &pDXContext);
    AssertRCReturn(rc, rc);

    if (pSvgaDXContext)
       memcpy(&pDXContext->svgaDXContext, pSvgaDXContext, sizeof(*pSvgaDXContext));

    rc = pSvgaR3State->pFuncsDX->pfnDXBindContext(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXReadbackContext(PVGASTATECC pThisCC, uint32_t idDXContext, SVGADXContextMobFormat *pSvgaDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXReadbackContext, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXReadbackContext(pThisCC, pDXContext);
    if (RT_SUCCESS(rc))
        memcpy(pSvgaDXContext, &pDXContext->svgaDXContext, sizeof(*pSvgaDXContext));
    return rc;
}


int vmsvga3dDXInvalidateContext(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXInvalidateContext, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXInvalidateContext(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXSetSingleConstantBuffer(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetSingleConstantBuffer const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetSingleConstantBuffer, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(pCmd->slot < SVGA3D_DX_MAX_CONSTBUFFERS, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->type >= SVGA3D_SHADERTYPE_MIN && pCmd->type < SVGA3D_SHADERTYPE_MAX, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t const idxShaderState = pCmd->type - SVGA3D_SHADERTYPE_MIN;
    SVGA3dConstantBufferBinding *pCBB = &pDXContext->svgaDXContext.shaderState[idxShaderState].constantBuffers[pCmd->slot];
    pCBB->sid           = pCmd->sid;
    pCBB->offsetInBytes = pCmd->offsetInBytes;
    pCBB->sizeInBytes   = pCmd->sizeInBytes;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetSingleConstantBuffer(pThisCC, pDXContext, pCmd->slot, pCmd->type, pCmd->sid, pCmd->offsetInBytes, pCmd->sizeInBytes);
    return rc;
}


int vmsvga3dDXSetShaderResources(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetShaderResources const *pCmd, uint32_t cShaderResourceViewId, SVGA3dShaderResourceViewId const *paShaderResourceViewId)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetShaderResources, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(pCmd->startView < SVGA3D_DX_MAX_SRVIEWS, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(cShaderResourceViewId <= SVGA3D_DX_MAX_SRVIEWS - pCmd->startView, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->type >= SVGA3D_SHADERTYPE_MIN && pCmd->type < SVGA3D_SHADERTYPE_MAX, VERR_INVALID_PARAMETER);
    for (uint32_t i = 0; i < cShaderResourceViewId; ++i)
        ASSERT_GUEST_RETURN(   paShaderResourceViewId[i] < pDXContext->cot.cSRView
                            || paShaderResourceViewId[i] == SVGA3D_INVALID_ID, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t const idxShaderState = pCmd->type - SVGA3D_SHADERTYPE_MIN;
    for (uint32_t i = 0; i < cShaderResourceViewId; ++i)
    {
        SVGA3dShaderResourceViewId const shaderResourceViewId = paShaderResourceViewId[i];
        pDXContext->svgaDXContext.shaderState[idxShaderState].shaderResources[pCmd->startView + i] = shaderResourceViewId;
    }

    rc = pSvgaR3State->pFuncsDX->pfnDXSetShaderResources(pThisCC, pDXContext, pCmd->startView, pCmd->type, cShaderResourceViewId, paShaderResourceViewId);
    return rc;
}


int vmsvga3dDXSetShader(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetShader const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetShader, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(   pCmd->shaderId < pDXContext->cot.cShader
                        || pCmd->shaderId == SVGA_ID_INVALID, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->type >= SVGA3D_SHADERTYPE_MIN && pCmd->type < SVGA3D_SHADERTYPE_MAX, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t const idxShaderState = pCmd->type - SVGA3D_SHADERTYPE_MIN;
    pDXContext->svgaDXContext.shaderState[idxShaderState].shaderId = pCmd->shaderId;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetShader(pThisCC, pDXContext, pCmd->shaderId, pCmd->type);
    return rc;
}


int vmsvga3dDXSetSamplers(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetSamplers const *pCmd, uint32_t cSamplerId, SVGA3dSamplerId const *paSamplerId)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetSamplers, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(pCmd->startSampler < SVGA3D_DX_MAX_SAMPLERS, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(cSamplerId <= SVGA3D_DX_MAX_SAMPLERS - pCmd->startSampler, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->type >= SVGA3D_SHADERTYPE_MIN && pCmd->type < SVGA3D_SHADERTYPE_MAX, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t const idxShaderState = pCmd->type - SVGA3D_SHADERTYPE_MIN;
    for (uint32_t i = 0; i < cSamplerId; ++i)
    {
        SVGA3dSamplerId const samplerId = paSamplerId[i];
        ASSERT_GUEST_RETURN(   samplerId < pDXContext->cot.cSampler
                            || samplerId == SVGA_ID_INVALID, VERR_INVALID_PARAMETER);
        pDXContext->svgaDXContext.shaderState[idxShaderState].samplers[pCmd->startSampler + i] = samplerId;
    }
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXSetSamplers(pThisCC, pDXContext, pCmd->startSampler, pCmd->type, cSamplerId, paSamplerId);
    return rc;
}


#ifdef DUMP_BITMAPS
static void vmsvga3dDXDrawDumpRenderTargets(PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, const char *pszPrefix = NULL)
{
    for (uint32_t i = 0; i < SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS; ++i)
    {
        if (pDXContext->svgaDXContext.renderState.renderTargetViewIds[i] != SVGA3D_INVALID_ID)
        {
            SVGACOTableDXRTViewEntry *pRTViewEntry = &pDXContext->cot.paRTView[pDXContext->svgaDXContext.renderState.renderTargetViewIds[i]];
            Log(("Dump RT[%u] sid = %u rtvid = %u\n", i, pRTViewEntry->sid, pDXContext->svgaDXContext.renderState.renderTargetViewIds[i]));

            SVGA3dSurfaceImageId image;
            image.sid = pRTViewEntry->sid;
            image.face = 0;
            image.mipmap = 0;
            VMSVGA3D_MAPPED_SURFACE map;
            int rc = vmsvga3dSurfaceMap(pThisCC, &image, NULL, VMSVGA3D_SURFACE_MAP_READ, &map);
            if (RT_SUCCESS(rc))
            {
                vmsvga3dMapWriteBmpFile(&map, pszPrefix ? pszPrefix : "rt-");
                vmsvga3dSurfaceUnmap(pThisCC, &image, &map, /* fWritten =  */ false);
            }
            else
                Log(("Map failed %Rrc\n", rc));
        }
    }
}
#endif

int vmsvga3dDXDraw(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDraw const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDraw, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDraw(pThisCC, pDXContext, pCmd->vertexCount, pCmd->startVertexLocation);
#ifdef DUMP_BITMAPS
    vmsvga3dDXDrawDumpRenderTargets(pThisCC, pDXContext);
#endif
    return rc;
}


int vmsvga3dDXDrawIndexed(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDrawIndexed const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDrawIndexed, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDrawIndexed(pThisCC, pDXContext, pCmd->indexCount, pCmd->startIndexLocation, pCmd->baseVertexLocation);
#ifdef DUMP_BITMAPS
    vmsvga3dDXDrawDumpRenderTargets(pThisCC, pDXContext);
#endif
    return rc;
}


int vmsvga3dDXDrawInstanced(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDrawInstanced const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDrawInstanced, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDrawInstanced(pThisCC, pDXContext,
             pCmd->vertexCountPerInstance, pCmd->instanceCount, pCmd->startVertexLocation, pCmd->startInstanceLocation);
#ifdef DUMP_BITMAPS
    vmsvga3dDXDrawDumpRenderTargets(pThisCC, pDXContext);
#endif
    return rc;
}


int vmsvga3dDXDrawIndexedInstanced(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDrawIndexedInstanced const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDrawIndexedInstanced, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDrawIndexedInstanced(pThisCC, pDXContext,
             pCmd->indexCountPerInstance, pCmd->instanceCount, pCmd->startIndexLocation, pCmd->baseVertexLocation, pCmd->startInstanceLocation);
#ifdef DUMP_BITMAPS
    vmsvga3dDXDrawDumpRenderTargets(pThisCC, pDXContext);
#endif
    return rc;
}


int vmsvga3dDXDrawAuto(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDrawAuto, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDrawAuto(pThisCC, pDXContext);
#ifdef DUMP_BITMAPS
    vmsvga3dDXDrawDumpRenderTargets(pThisCC, pDXContext);
#endif
    return rc;
}


int vmsvga3dDXSetInputLayout(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dElementLayoutId elementLayoutId)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetInputLayout, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(   elementLayoutId == SVGA3D_INVALID_ID
                        || elementLayoutId < pDXContext->cot.cElementLayout, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pDXContext->svgaDXContext.inputAssembly.layoutId = elementLayoutId;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetInputLayout(pThisCC, pDXContext, elementLayoutId);
    return rc;
}


int vmsvga3dDXSetVertexBuffers(PVGASTATECC pThisCC, uint32_t idDXContext, uint32_t startBuffer, uint32_t cVertexBuffer, SVGA3dVertexBuffer const *paVertexBuffer)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetVertexBuffers, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(startBuffer < SVGA3D_DX_MAX_VERTEXBUFFERS, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(cVertexBuffer <= SVGA3D_DX_MAX_VERTEXBUFFERS - startBuffer, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    for (uint32_t i = 0; i < cVertexBuffer; ++i)
    {
        uint32_t const idxVertexBuffer = startBuffer + i;

        pDXContext->svgaDXContext.inputAssembly.vertexBuffers[idxVertexBuffer].bufferId = paVertexBuffer[i].sid;
        pDXContext->svgaDXContext.inputAssembly.vertexBuffers[idxVertexBuffer].stride = paVertexBuffer[i].stride;
        pDXContext->svgaDXContext.inputAssembly.vertexBuffers[idxVertexBuffer].offset = paVertexBuffer[i].offset;
    }

    rc = pSvgaR3State->pFuncsDX->pfnDXSetVertexBuffers(pThisCC, pDXContext, startBuffer, cVertexBuffer, paVertexBuffer);
    return rc;
}


int vmsvga3dDXSetIndexBuffer(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetIndexBuffer const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetIndexBuffer, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    pDXContext->svgaDXContext.inputAssembly.indexBufferSid = pCmd->sid;
    pDXContext->svgaDXContext.inputAssembly.indexBufferOffset = pCmd->offset;
    pDXContext->svgaDXContext.inputAssembly.indexBufferFormat = pCmd->format;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetIndexBuffer(pThisCC, pDXContext, pCmd->sid, pCmd->format, pCmd->offset);
    return rc;
}


int vmsvga3dDXSetTopology(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dPrimitiveType topology)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetTopology, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(topology >= SVGA3D_PRIMITIVE_MIN && topology < SVGA3D_PRIMITIVE_MAX, VERR_INVALID_PARAMETER);

    pDXContext->svgaDXContext.inputAssembly.topology = topology;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetTopology(pThisCC, pDXContext, topology);
    return rc;
}


int vmsvga3dDXSetRenderTargets(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dDepthStencilViewId depthStencilViewId, uint32_t cRenderTargetViewId, SVGA3dRenderTargetViewId const *paRenderTargetViewId)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetRenderTargets, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(   depthStencilViewId < pDXContext->cot.cDSView
                        || depthStencilViewId == SVGA_ID_INVALID, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(cRenderTargetViewId <= SVGA3D_MAX_RENDER_TARGETS, VERR_INVALID_PARAMETER);
    for (uint32_t i = 0; i < cRenderTargetViewId; ++i)
        ASSERT_GUEST_RETURN(   paRenderTargetViewId[i] < pDXContext->cot.cRTView
                            || paRenderTargetViewId[i] == SVGA_ID_INVALID, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pDXContext->svgaDXContext.renderState.depthStencilViewId = depthStencilViewId;
    for (uint32_t i = 0; i < cRenderTargetViewId; ++i)
        pDXContext->svgaDXContext.renderState.renderTargetViewIds[i] = paRenderTargetViewId[i];

    /* Remember how many render target slots must be set. */
    pDXContext->cRenderTargets = RT_MAX(pDXContext->cRenderTargets, cRenderTargetViewId);

    rc = pSvgaR3State->pFuncsDX->pfnDXSetRenderTargets(pThisCC, pDXContext, depthStencilViewId, cRenderTargetViewId, paRenderTargetViewId);
    return rc;
}


int vmsvga3dDXSetBlendState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetBlendState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetBlendState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dBlendStateId const blendId = pCmd->blendId;

    ASSERT_GUEST_RETURN(   blendId == SVGA3D_INVALID_ID
                        || blendId < pDXContext->cot.cBlendState, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pDXContext->svgaDXContext.renderState.blendStateId = blendId;
    /* SVGADXContextMobFormat uses uint32_t array to store the blend factors, however they are in fact 32 bit floats. */
    memcpy(pDXContext->svgaDXContext.renderState.blendFactor, pCmd->blendFactor, sizeof(pDXContext->svgaDXContext.renderState.blendFactor));
    pDXContext->svgaDXContext.renderState.sampleMask = pCmd->sampleMask;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetBlendState(pThisCC, pDXContext, blendId, pCmd->blendFactor, pCmd->sampleMask);
    return rc;
}


int vmsvga3dDXSetDepthStencilState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetDepthStencilState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetDepthStencilState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dDepthStencilStateId const depthStencilId = pCmd->depthStencilId;

    ASSERT_GUEST_RETURN(   depthStencilId == SVGA3D_INVALID_ID
                        || depthStencilId < pDXContext->cot.cDepthStencil, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pDXContext->svgaDXContext.renderState.depthStencilStateId = depthStencilId;
    pDXContext->svgaDXContext.renderState.stencilRef = pCmd->stencilRef;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetDepthStencilState(pThisCC, pDXContext, depthStencilId, pCmd->stencilRef);
    return rc;
}


int vmsvga3dDXSetRasterizerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dRasterizerStateId rasterizerId)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetRasterizerState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(   rasterizerId == SVGA3D_INVALID_ID
                        || rasterizerId < pDXContext->cot.cRasterizerState, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pDXContext->svgaDXContext.renderState.rasterizerStateId = rasterizerId;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetRasterizerState(pThisCC, pDXContext, rasterizerId);
    return rc;
}


int vmsvga3dDXDefineQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineQuery const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineQuery, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dQueryId const queryId = pCmd->queryId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paQuery, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(queryId < pDXContext->cot.cQuery, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->type >= SVGA3D_QUERYTYPE_MIN && pCmd->type < SVGA3D_QUERYTYPE_MAX, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* Cleanup the current query. */
    pSvgaR3State->pFuncsDX->pfnDXDestroyQuery(pThisCC, pDXContext, queryId);

    SVGACOTableDXQueryEntry *pEntry = &pDXContext->cot.paQuery[queryId];
    pEntry->type   = pCmd->type;
    pEntry->state  = SVGADX_QDSTATE_IDLE;
    pEntry->flags  = pCmd->flags;
    pEntry->mobid  = SVGA_ID_INVALID;
    pEntry->offset = 0;

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineQuery(pThisCC, pDXContext, queryId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyQuery const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyQuery, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dQueryId const queryId = pCmd->queryId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paQuery, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(queryId < pDXContext->cot.cQuery, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pSvgaR3State->pFuncsDX->pfnDXDestroyQuery(pThisCC, pDXContext, queryId);

    /* Cleanup COTable entry.*/
    SVGACOTableDXQueryEntry *pEntry = &pDXContext->cot.paQuery[queryId];
    pEntry->type   = SVGA3D_QUERYTYPE_INVALID;
    pEntry->state  = SVGADX_QDSTATE_INVALID;
    pEntry->flags  = 0;
    pEntry->mobid  = SVGA_ID_INVALID;
    pEntry->offset = 0;

    return rc;
}


int vmsvga3dDXBindQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXBindQuery const *pCmd, PVMSVGAMOB pMob)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dQueryId const queryId = pCmd->queryId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paQuery, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(queryId < pDXContext->cot.cQuery, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXQueryEntry *pEntry = &pDXContext->cot.paQuery[queryId];
    pEntry->mobid = vmsvgaR3MobId(pMob);

    return rc;
}


int vmsvga3dDXSetQueryOffset(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetQueryOffset const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dQueryId const queryId = pCmd->queryId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paQuery, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(queryId < pDXContext->cot.cQuery, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXQueryEntry *pEntry = &pDXContext->cot.paQuery[queryId];
    pEntry->offset = pCmd->mobOffset;

    return rc;
}


int vmsvga3dDXBeginQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXBeginQuery const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXBeginQuery, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dQueryId const queryId = pCmd->queryId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paQuery, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(queryId < pDXContext->cot.cQuery, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXQueryEntry *pEntry = &pDXContext->cot.paQuery[queryId];
    Assert(pEntry->state == SVGADX_QDSTATE_IDLE || pEntry->state == SVGADX_QDSTATE_PENDING || pEntry->state == SVGADX_QDSTATE_FINISHED);
    if (pEntry->state != SVGADX_QDSTATE_ACTIVE)
    {
        rc = pSvgaR3State->pFuncsDX->pfnDXBeginQuery(pThisCC, pDXContext, queryId);
        if (RT_SUCCESS(rc))
        {
            pEntry->state = SVGADX_QDSTATE_ACTIVE;

            /* Update the guest status of the query. */
            uint32_t const u32 = SVGA3D_QUERYSTATE_PENDING;
            dxMobWrite(pSvgaR3State, pEntry->mobid, pEntry->offset, &u32, sizeof(u32));
        }
        else
        {
            uint32_t const u32 = SVGA3D_QUERYSTATE_FAILED;
            dxMobWrite(pSvgaR3State, pEntry->mobid, pEntry->offset, &u32, sizeof(u32));
        }
    }
    return rc;
}


static int dxEndQuery(PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dQueryId queryId, SVGACOTableDXQueryEntry *pEntry)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;

    int rc = VINF_SUCCESS;
    if (pEntry->state == SVGADX_QDSTATE_ACTIVE || pEntry->state == SVGADX_QDSTATE_IDLE)
    {
        pEntry->state = SVGADX_QDSTATE_PENDING;

        uint32_t u32QueryState;
        SVGADXQueryResultUnion queryResult;
        uint32_t cbQuery = 0; /* Actual size of query data returned by backend. */
        rc = pSvgaR3State->pFuncsDX->pfnDXEndQuery(pThisCC, pDXContext, queryId, &queryResult, &cbQuery);
        if (RT_SUCCESS(rc))
        {
            /* Write the result after SVGA3dQueryState. */
            dxMobWrite(pSvgaR3State, pEntry->mobid, pEntry->offset + sizeof(uint32_t), &queryResult, cbQuery);

            u32QueryState = SVGA3D_QUERYSTATE_SUCCEEDED;
        }
        else
            u32QueryState = SVGA3D_QUERYSTATE_FAILED;

        dxMobWrite(pSvgaR3State, pEntry->mobid, pEntry->offset, &u32QueryState, sizeof(u32QueryState));

        if (RT_SUCCESS(rc))
            pEntry->state = SVGADX_QDSTATE_FINISHED;
    }
    else
        AssertStmt(pEntry->state == SVGADX_QDSTATE_FINISHED, rc = VERR_INVALID_STATE);

    return rc;
}


int vmsvga3dDXEndQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXEndQuery const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXEndQuery, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dQueryId const queryId = pCmd->queryId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paQuery, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(queryId < pDXContext->cot.cQuery, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXQueryEntry *pEntry = &pDXContext->cot.paQuery[queryId];
    rc = dxEndQuery(pThisCC, pDXContext, queryId, pEntry);
    return rc;
}


int vmsvga3dDXReadbackQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXReadbackQuery const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dQueryId const queryId = pCmd->queryId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paQuery, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(queryId < pDXContext->cot.cQuery, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* The device does not cache queries. So this is a NOP. */

    return rc;
}


int vmsvga3dDXSetPredication(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetPredication const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetPredication, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dQueryId const queryId = pCmd->queryId;

    ASSERT_GUEST_RETURN(   queryId == SVGA3D_INVALID_ID
                        || queryId < pDXContext->cot.cQuery, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXSetPredication(pThisCC, pDXContext, queryId, pCmd->predicateValue);
    return rc;
}


int vmsvga3dDXSetSOTargets(PVGASTATECC pThisCC, uint32_t idDXContext, uint32_t cSoTarget, SVGA3dSoTarget const *paSoTarget)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetSOTargets, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(cSoTarget <= SVGA3D_DX_MAX_SOTARGETS, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /** @todo Offset is not stored in svgaDXContext. Should it be stored elsewhere? */
    for (uint32_t i = 0; i < SVGA3D_DX_MAX_SOTARGETS; ++i)
        pDXContext->svgaDXContext.streamOut.targets[i] = i < cSoTarget ? paSoTarget[i].sid : SVGA3D_INVALID_ID;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetSOTargets(pThisCC, pDXContext, cSoTarget, paSoTarget);
    return rc;
}


int vmsvga3dDXSetViewports(PVGASTATECC pThisCC, uint32_t idDXContext, uint32_t cViewport, SVGA3dViewport const *paViewport)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetViewports, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(cViewport <= SVGA3D_DX_MAX_VIEWPORTS, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pDXContext->svgaDXContext.numViewports = (uint8_t)cViewport;
    for (uint32_t i = 0; i < cViewport; ++i)
        pDXContext->svgaDXContext.viewports[i] = paViewport[i];

    rc = pSvgaR3State->pFuncsDX->pfnDXSetViewports(pThisCC, pDXContext, cViewport, paViewport);
    return rc;
}


int vmsvga3dDXSetScissorRects(PVGASTATECC pThisCC, uint32_t idDXContext, uint32_t cRect, SVGASignedRect const *paRect)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetScissorRects, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(cRect <= SVGA3D_DX_MAX_SCISSORRECTS, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pDXContext->svgaDXContext.numScissorRects = (uint8_t)cRect;
    for (uint32_t i = 0; i < cRect; ++i)
        pDXContext->svgaDXContext.scissorRects[i] = paRect[i];

    rc = pSvgaR3State->pFuncsDX->pfnDXSetScissorRects(pThisCC, pDXContext, cRect, paRect);
    return rc;
}


int vmsvga3dDXClearRenderTargetView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXClearRenderTargetView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXClearRenderTargetView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dRenderTargetViewId const renderTargetViewId = pCmd->renderTargetViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paRTView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(renderTargetViewId < pDXContext->cot.cRTView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXClearRenderTargetView(pThisCC, pDXContext, renderTargetViewId, &pCmd->rgba);
    return rc;
}


int vmsvga3dDXClearDepthStencilView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXClearDepthStencilView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXClearDepthStencilView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dDepthStencilViewId const depthStencilViewId = pCmd->depthStencilViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paDSView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(depthStencilViewId < pDXContext->cot.cDSView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXClearDepthStencilView(pThisCC, pDXContext, pCmd->flags, depthStencilViewId, pCmd->depth, (uint8_t)pCmd->stencil);
    return rc;
}


int vmsvga3dDXPredCopyRegion(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXPredCopyRegion const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXPredCopyRegion, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    /** @todo Memcpy if both resources do not have the hardware resource. */

    rc = pSvgaR3State->pFuncsDX->pfnDXPredCopyRegion(pThisCC, pDXContext, pCmd->dstSid, pCmd->dstSubResource, pCmd->srcSid, pCmd->srcSubResource, &pCmd->box);
    return rc;
}


int vmsvga3dDXPredCopy(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXPredCopy const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXPredCopy, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXPredCopy(pThisCC, pDXContext, pCmd->dstSid, pCmd->srcSid);
    return rc;
}


int vmsvga3dDXPresentBlt(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXPresentBlt const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXPresentBlt, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXPresentBlt(pThisCC, pDXContext,
                                                 pCmd->dstSid, pCmd->destSubResource, &pCmd->boxDest,
                                                 pCmd->srcSid, pCmd->srcSubResource, &pCmd->boxSrc, pCmd->mode);
    return rc;
}


int vmsvga3dDXGenMips(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXGenMips const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXGenMips, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dShaderResourceViewId const shaderResourceViewId = pCmd->shaderResourceViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paSRView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(shaderResourceViewId < pDXContext->cot.cSRView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXGenMips(pThisCC, pDXContext, shaderResourceViewId);
    return rc;
}


int vmsvga3dDXDefineShaderResourceView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineShaderResourceView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineShaderResourceView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dShaderResourceViewId const shaderResourceViewId = pCmd->shaderResourceViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paSRView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(shaderResourceViewId < pDXContext->cot.cSRView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXSRViewEntry *pEntry = &pDXContext->cot.paSRView[shaderResourceViewId];
    pEntry->sid               = pCmd->sid;
    pEntry->format            = pCmd->format;
    pEntry->resourceDimension = pCmd->resourceDimension;
    pEntry->desc              = pCmd->desc;

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineShaderResourceView(pThisCC, pDXContext, shaderResourceViewId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyShaderResourceView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyShaderResourceView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyShaderResourceView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dShaderResourceViewId const shaderResourceViewId = pCmd->shaderResourceViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paSRView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(shaderResourceViewId < pDXContext->cot.cSRView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXSRViewEntry *pEntry = &pDXContext->cot.paSRView[shaderResourceViewId];
    RT_ZERO(*pEntry);

    rc = pSvgaR3State->pFuncsDX->pfnDXDestroyShaderResourceView(pThisCC, pDXContext, shaderResourceViewId);
    return rc;
}


int vmsvga3dDXDefineRenderTargetView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineRenderTargetView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineRenderTargetView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dRenderTargetViewId const renderTargetViewId = pCmd->renderTargetViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paRTView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(renderTargetViewId < pDXContext->cot.cRTView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXRTViewEntry *pEntry = &pDXContext->cot.paRTView[renderTargetViewId];
    pEntry->sid               = pCmd->sid;
    pEntry->format            = pCmd->format;
    pEntry->resourceDimension = pCmd->resourceDimension;
    pEntry->desc              = pCmd->desc;

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineRenderTargetView(pThisCC, pDXContext, renderTargetViewId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyRenderTargetView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyRenderTargetView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyRenderTargetView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dRenderTargetViewId const renderTargetViewId = pCmd->renderTargetViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paRTView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(renderTargetViewId < pDXContext->cot.cRTView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXRTViewEntry *pEntry = &pDXContext->cot.paRTView[renderTargetViewId];
    RT_ZERO(*pEntry);

    for (uint32_t i = 0; i < SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS; ++i)
    {
        if (pDXContext->svgaDXContext.renderState.renderTargetViewIds[i] == renderTargetViewId)
            pDXContext->svgaDXContext.renderState.renderTargetViewIds[i] = SVGA_ID_INVALID;
    }

    rc = pSvgaR3State->pFuncsDX->pfnDXDestroyRenderTargetView(pThisCC, pDXContext, renderTargetViewId);
    return rc;
}


int vmsvga3dDXDefineDepthStencilView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineDepthStencilView_v2 const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineDepthStencilView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dDepthStencilViewId const depthStencilViewId = pCmd->depthStencilViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paDSView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(depthStencilViewId < pDXContext->cot.cDSView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXDSViewEntry *pEntry = &pDXContext->cot.paDSView[depthStencilViewId];
    pEntry->sid               = pCmd->sid;
    pEntry->format            = pCmd->format;
    pEntry->resourceDimension = pCmd->resourceDimension;
    pEntry->mipSlice          = pCmd->mipSlice;
    pEntry->firstArraySlice   = pCmd->firstArraySlice;
    pEntry->arraySize         = pCmd->arraySize;
    pEntry->flags             = pCmd->flags;

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineDepthStencilView(pThisCC, pDXContext, depthStencilViewId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyDepthStencilView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyDepthStencilView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyDepthStencilView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dDepthStencilViewId const depthStencilViewId = pCmd->depthStencilViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paDSView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(depthStencilViewId < pDXContext->cot.cDSView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXDSViewEntry *pEntry = &pDXContext->cot.paDSView[depthStencilViewId];
    RT_ZERO(*pEntry);

    rc = pSvgaR3State->pFuncsDX->pfnDXDestroyDepthStencilView(pThisCC, pDXContext, depthStencilViewId);
    return rc;
}


int vmsvga3dDXDefineElementLayout(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dElementLayoutId elementLayoutId, uint32_t cDesc, SVGA3dInputElementDesc const *paDesc)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineElementLayout, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(pDXContext->cot.paElementLayout, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(elementLayoutId < pDXContext->cot.cElementLayout, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXElementLayoutEntry *pEntry = &pDXContext->cot.paElementLayout[elementLayoutId];
    pEntry->elid = elementLayoutId;
    pEntry->numDescs = RT_MIN(cDesc, RT_ELEMENTS(pEntry->descs));
    memcpy(pEntry->descs, paDesc, pEntry->numDescs * sizeof(pEntry->descs[0]));

#ifdef LOG_ENABLED
    Log6(("Element layout %d: slot off fmt class step reg\n", pEntry->elid));
    for (uint32_t i = 0; i < pEntry->numDescs; ++i)
    {
        Log6(("  [%u]: %u 0x%02X %d %u %u %u\n",
              i,
              pEntry->descs[i].inputSlot,
              pEntry->descs[i].alignedByteOffset,
              pEntry->descs[i].format,
              pEntry->descs[i].inputSlotClass,
              pEntry->descs[i].instanceDataStepRate,
              pEntry->descs[i].inputRegister
            ));
    }
#endif

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineElementLayout(pThisCC, pDXContext, elementLayoutId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyElementLayout(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyElementLayout const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyElementLayout, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dElementLayoutId const elementLayoutId = pCmd->elementLayoutId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paElementLayout, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(elementLayoutId < pDXContext->cot.cElementLayout, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pSvgaR3State->pFuncsDX->pfnDXDestroyElementLayout(pThisCC, pDXContext, elementLayoutId);

    SVGACOTableDXElementLayoutEntry *pEntry = &pDXContext->cot.paElementLayout[elementLayoutId];
    RT_ZERO(*pEntry);
    pEntry->elid = SVGA3D_INVALID_ID;

    return rc;
}


int vmsvga3dDXDefineBlendState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineBlendState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineBlendState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dBlendStateId const blendId = pCmd->blendId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paBlendState, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(blendId < pDXContext->cot.cBlendState, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXBlendStateEntry *pEntry = &pDXContext->cot.paBlendState[blendId];
    pEntry->alphaToCoverageEnable  = pCmd->alphaToCoverageEnable;
    pEntry->independentBlendEnable = pCmd->independentBlendEnable;
    memcpy(pEntry->perRT, pCmd->perRT, sizeof(pEntry->perRT));

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineBlendState(pThisCC, pDXContext, blendId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyBlendState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyBlendState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyBlendState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dBlendStateId const blendId = pCmd->blendId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paBlendState, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(blendId < pDXContext->cot.cBlendState, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pSvgaR3State->pFuncsDX->pfnDXDestroyBlendState(pThisCC, pDXContext, blendId);

    SVGACOTableDXBlendStateEntry *pEntry = &pDXContext->cot.paBlendState[blendId];
    RT_ZERO(*pEntry);

    return rc;
}


int vmsvga3dDXDefineDepthStencilState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineDepthStencilState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineDepthStencilState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dDepthStencilStateId const depthStencilId = pCmd->depthStencilId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paDepthStencil, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(depthStencilId < pDXContext->cot.cDepthStencil, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXDepthStencilEntry *pEntry = &pDXContext->cot.paDepthStencil[depthStencilId];
    pEntry->depthEnable             = pCmd->depthEnable;
    pEntry->depthWriteMask          = pCmd->depthWriteMask;
    pEntry->depthFunc               = pCmd->depthFunc;
    pEntry->stencilEnable           = pCmd->stencilEnable;
    pEntry->frontEnable             = pCmd->frontEnable;
    pEntry->backEnable              = pCmd->backEnable;
    pEntry->stencilReadMask         = pCmd->stencilReadMask;
    pEntry->stencilWriteMask        = pCmd->stencilWriteMask;

    pEntry->frontStencilFailOp      = pCmd->frontStencilFailOp;
    pEntry->frontStencilDepthFailOp = pCmd->frontStencilDepthFailOp;
    pEntry->frontStencilPassOp      = pCmd->frontStencilPassOp;
    pEntry->frontStencilFunc        = pCmd->frontStencilFunc;

    pEntry->backStencilFailOp       = pCmd->backStencilFailOp;
    pEntry->backStencilDepthFailOp  = pCmd->backStencilDepthFailOp;
    pEntry->backStencilPassOp       = pCmd->backStencilPassOp;
    pEntry->backStencilFunc         = pCmd->backStencilFunc;

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineDepthStencilState(pThisCC, pDXContext, depthStencilId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyDepthStencilState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyDepthStencilState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyDepthStencilState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dDepthStencilStateId const depthStencilId = pCmd->depthStencilId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paDepthStencil, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(depthStencilId < pDXContext->cot.cDepthStencil, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pSvgaR3State->pFuncsDX->pfnDXDestroyDepthStencilState(pThisCC, pDXContext, depthStencilId);

    SVGACOTableDXDepthStencilEntry *pEntry = &pDXContext->cot.paDepthStencil[depthStencilId];
    RT_ZERO(*pEntry);

    return rc;
}


int vmsvga3dDXDefineRasterizerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineRasterizerState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineRasterizerState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dRasterizerStateId const rasterizerId = pCmd->rasterizerId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paRasterizerState, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(rasterizerId < pDXContext->cot.cRasterizerState, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXRasterizerStateEntry *pEntry = &pDXContext->cot.paRasterizerState[rasterizerId];
    pEntry->fillMode              = pCmd->fillMode;
    pEntry->cullMode              = pCmd->cullMode;
    pEntry->frontCounterClockwise = pCmd->frontCounterClockwise;
    pEntry->provokingVertexLast   = pCmd->provokingVertexLast;
    pEntry->depthBias             = pCmd->depthBias;
    pEntry->depthBiasClamp        = pCmd->depthBiasClamp;
    pEntry->slopeScaledDepthBias  = pCmd->slopeScaledDepthBias;
    pEntry->depthClipEnable       = pCmd->depthClipEnable;
    pEntry->scissorEnable         = pCmd->scissorEnable;
    pEntry->multisampleEnable     = pCmd->multisampleEnable;
    pEntry->antialiasedLineEnable = pCmd->antialiasedLineEnable;
    pEntry->lineWidth             = pCmd->lineWidth;
    pEntry->lineStippleEnable     = pCmd->lineStippleEnable;
    pEntry->lineStippleFactor     = pCmd->lineStippleFactor;
    pEntry->lineStipplePattern    = pCmd->lineStipplePattern;
    pEntry->forcedSampleCount     = 0; /** @todo Not in pCmd. */
    RT_ZERO(pEntry->mustBeZero);

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineRasterizerState(pThisCC, pDXContext, rasterizerId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyRasterizerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyRasterizerState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyRasterizerState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dRasterizerStateId const rasterizerId = pCmd->rasterizerId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paRasterizerState, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(rasterizerId < pDXContext->cot.cRasterizerState, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXDestroyRasterizerState(pThisCC, pDXContext, rasterizerId);

    SVGACOTableDXRasterizerStateEntry *pEntry = &pDXContext->cot.paRasterizerState[rasterizerId];
    RT_ZERO(*pEntry);

    return rc;
}


int vmsvga3dDXDefineSamplerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineSamplerState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineSamplerState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dSamplerId const samplerId = pCmd->samplerId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paSampler, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(samplerId < pDXContext->cot.cSampler, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXSamplerEntry *pEntry = &pDXContext->cot.paSampler[samplerId];
    pEntry->filter         = pCmd->filter;
    pEntry->addressU       = pCmd->addressU;
    pEntry->addressV       = pCmd->addressV;
    pEntry->addressW       = pCmd->addressW;
    pEntry->mipLODBias     = pCmd->mipLODBias;
    pEntry->maxAnisotropy  = pCmd->maxAnisotropy;
    pEntry->comparisonFunc = pCmd->comparisonFunc;
    pEntry->borderColor    = pCmd->borderColor;
    pEntry->minLOD         = pCmd->minLOD;
    pEntry->maxLOD         = pCmd->maxLOD;

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineSamplerState(pThisCC, pDXContext, samplerId, pEntry);
    return rc;
}


int vmsvga3dDXDestroySamplerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroySamplerState const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroySamplerState, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dSamplerId const samplerId = pCmd->samplerId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paSampler, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(samplerId < pDXContext->cot.cSampler, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pSvgaR3State->pFuncsDX->pfnDXDestroySamplerState(pThisCC, pDXContext, samplerId);

    SVGACOTableDXSamplerEntry *pEntry = &pDXContext->cot.paSampler[samplerId];
    RT_ZERO(*pEntry);

    return rc;
}


int vmsvga3dDXDefineShader(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineShader const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineShader, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dShaderId const shaderId = pCmd->shaderId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paShader, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(shaderId < pDXContext->cot.cShader, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->type >= SVGA3D_SHADERTYPE_MIN && pCmd->type < SVGA3D_SHADERTYPE_MAX, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->sizeInBytes >= 8, VERR_INVALID_PARAMETER); /* Version Token + Length Token. */
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* Cleanup the current shader. */
    pSvgaR3State->pFuncsDX->pfnDXDestroyShader(pThisCC, pDXContext, shaderId);

    SVGACOTableDXShaderEntry *pEntry = &pDXContext->cot.paShader[shaderId];
    pEntry->type          = pCmd->type;
    pEntry->sizeInBytes   = pCmd->sizeInBytes;
    pEntry->offsetInBytes = 0;
    pEntry->mobid         = SVGA_ID_INVALID;

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineShader(pThisCC, pDXContext, shaderId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyShader(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyShader const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyShader, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dShaderId const shaderId = pCmd->shaderId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paShader, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(shaderId < pDXContext->cot.cShader, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pSvgaR3State->pFuncsDX->pfnDXDestroyShader(pThisCC, pDXContext, shaderId);

    /* Cleanup COTable entries.*/
    SVGACOTableDXShaderEntry *pEntry = &pDXContext->cot.paShader[shaderId];
    pEntry->type          = SVGA3D_SHADERTYPE_INVALID;
    pEntry->sizeInBytes   = 0;
    pEntry->offsetInBytes = 0;
    pEntry->mobid         = SVGA_ID_INVALID;

    /** @todo Destroy shaders on context and backend deletion. */
    return rc;
}


static int dxBindShader(DXShaderInfo *pShaderInfo, PVMSVGAMOB pMob, SVGACOTableDXShaderEntry const *pEntry, void const *pvShaderBytecode)
{
    /* How many bytes the MOB can hold. */
    uint32_t const cbMob = vmsvgaR3MobSize(pMob) - pEntry->offsetInBytes;
    ASSERT_GUEST_RETURN(cbMob >= pEntry->sizeInBytes, VERR_INVALID_PARAMETER);
    AssertReturn(pEntry->sizeInBytes >= 8, VERR_INTERNAL_ERROR); /* Host ensures this in DefineShader. */

    int rc = DXShaderParse(pvShaderBytecode, pEntry->sizeInBytes, pShaderInfo);
    if (RT_SUCCESS(rc))
    {
        /* Get the length of the shader bytecode. */
        uint32_t const *pau32Token = (uint32_t *)pvShaderBytecode; /* Tokens */
        uint32_t const cToken = pau32Token[1]; /* Length of the shader in tokens. */
        ASSERT_GUEST_RETURN(cToken <= pEntry->sizeInBytes / 4, VERR_INVALID_PARAMETER);

        /* Check if the shader contains SVGA3dDXSignatureHeader and signature entries after the bytecode.
         * If they are not there (Linux guest driver does not provide them), then it is fine
         * and the signatures generated by DXShaderParse will be used.
         */
        uint32_t cbSignaturesAvail = pEntry->sizeInBytes - cToken * 4; /* How many bytes for signatures are available. */
        if (cbSignaturesAvail >= sizeof(SVGA3dDXSignatureHeader))
        {
            cbSignaturesAvail -= sizeof(SVGA3dDXSignatureHeader);

            SVGA3dDXSignatureHeader const *pSignatureHeader = (SVGA3dDXSignatureHeader *)((uint8_t *)pvShaderBytecode + cToken * 4);
            if (pSignatureHeader->headerVersion == SVGADX_SIGNATURE_HEADER_VERSION_0)
            {
                ASSERT_GUEST_RETURN(   pSignatureHeader->numInputSignatures <= RT_ELEMENTS(pShaderInfo->aInputSignature)
                                    && pSignatureHeader->numOutputSignatures <= RT_ELEMENTS(pShaderInfo->aOutputSignature)
                                    && pSignatureHeader->numPatchConstantSignatures <= RT_ELEMENTS(pShaderInfo->aPatchConstantSignature),
                                    VERR_INVALID_PARAMETER);

                uint32_t const cSignature = pSignatureHeader->numInputSignatures
                                          + pSignatureHeader->numOutputSignatures
                                          + pSignatureHeader->numPatchConstantSignatures;
                uint32_t const cbSignature = cSignature * sizeof(SVGA3dDXSignatureEntry);
                ASSERT_GUEST_RETURN(cbSignaturesAvail >= cbSignature, VERR_INVALID_PARAMETER);

                /* The shader does not need guesswork. */
                pShaderInfo->fGuestSignatures = true;

                /* Copy to DXShaderInfo. */
                uint8_t const *pu8Signatures = (uint8_t *)&pSignatureHeader[1];
                pShaderInfo->cInputSignature = pSignatureHeader->numInputSignatures;
                memcpy(pShaderInfo->aInputSignature, pu8Signatures, pSignatureHeader->numInputSignatures * sizeof(SVGA3dDXSignatureEntry));

                pu8Signatures += pSignatureHeader->numInputSignatures * sizeof(SVGA3dDXSignatureEntry);
                pShaderInfo->cOutputSignature = pSignatureHeader->numOutputSignatures;
                memcpy(pShaderInfo->aOutputSignature, pu8Signatures, pSignatureHeader->numOutputSignatures * sizeof(SVGA3dDXSignatureEntry));

                pu8Signatures += pSignatureHeader->numOutputSignatures * sizeof(SVGA3dDXSignatureEntry);
                pShaderInfo->cPatchConstantSignature = pSignatureHeader->numPatchConstantSignatures;
                memcpy(pShaderInfo->aPatchConstantSignature, pu8Signatures, pSignatureHeader->numPatchConstantSignatures * sizeof(SVGA3dDXSignatureEntry));

                /* Sort must be called before GenerateSemantics which assigns attribute indices
                 * based on the order of attributes.
                 */
                DXShaderSortSignatures(pShaderInfo);
                DXShaderGenerateSemantics(pShaderInfo);
            }
        }
    }

    return rc;
}


int vmsvga3dDXBindShader(PVGASTATECC pThisCC, SVGA3dCmdDXBindShader const *pCmd, PVMSVGAMOB pMob)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXBindShader, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, pCmd->cid, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(pCmd->shid < pDXContext->cot.cShader, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    /* 'type' and 'sizeInBytes' has been already initialized by DefineShader. */
    SVGACOTableDXShaderEntry *pEntry = &pDXContext->cot.paShader[pCmd->shid];
    //pEntry->type;
    //pEntry->sizeInBytes;
    pEntry->offsetInBytes = pCmd->offsetInBytes;
    pEntry->mobid         = vmsvgaR3MobId(pMob);

    if (pMob)
    {
        /* Bind a mob to the shader. */

        /* Create a memory pointer for the MOB, which is accessible by host. */
        rc = vmsvgaR3MobBackingStoreCreate(pSvgaR3State, pMob, vmsvgaR3MobSize(pMob));
        if (RT_SUCCESS(rc))
        {
            /* Get pointer to the shader bytecode. This will also verify the offset. */
            void const *pvShaderBytecode = vmsvgaR3MobBackingStorePtr(pMob, pEntry->offsetInBytes);
            ASSERT_GUEST_RETURN(pvShaderBytecode, VERR_INVALID_PARAMETER);

            /* Get the shader and optional signatures from the MOB. */
            DXShaderInfo shaderInfo;
            RT_ZERO(shaderInfo);
            rc = dxBindShader(&shaderInfo, pMob, pEntry, pvShaderBytecode);
            if (RT_SUCCESS(rc))
            {
                /* pfnDXBindShader makes a copy of shaderInfo on success. */
                rc = pSvgaR3State->pFuncsDX->pfnDXBindShader(pThisCC, pDXContext, pCmd->shid, &shaderInfo);
            }
            AssertRC(rc);

            /** @todo Backing store is not needed anymore in any case? */
            if (RT_FAILURE(rc))
            {
                DXShaderFree(&shaderInfo);

                vmsvgaR3MobBackingStoreDelete(pSvgaR3State, pMob);
            }
        }
    }
    else
    {
        /* Unbind. */
        /** @todo Nothing to do here but release the MOB? */
        vmsvgaR3MobBackingStoreDelete(pSvgaR3State, pMob);
    }

    return rc;
}


int vmsvga3dDXDefineStreamOutput(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineStreamOutput const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineStreamOutput, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dStreamOutputId const soid = pCmd->soid;

    ASSERT_GUEST_RETURN(pDXContext->cot.paStreamOutput, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(soid < pDXContext->cot.cStreamOutput, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->numOutputStreamEntries < SVGA3D_MAX_DX10_STREAMOUT_DECLS, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXStreamOutputEntry *pEntry = &pDXContext->cot.paStreamOutput[soid];
    pEntry->numOutputStreamEntries = pCmd->numOutputStreamEntries;
    memcpy(pEntry->decl, pCmd->decl, sizeof(pEntry->decl));
    memcpy(pEntry->streamOutputStrideInBytes, pCmd->streamOutputStrideInBytes, sizeof(pEntry->streamOutputStrideInBytes));
    pEntry->rasterizedStream = 0; // Apparently invalid in this command: pCmd->rasterizedStream;
    pEntry->numOutputStreamStrides = 0;
    pEntry->mobid = SVGA_ID_INVALID;
    pEntry->offsetInBytes = 0;
    pEntry->usesMob = 0;
    pEntry->pad0 = 0;
    pEntry->pad1 = 0;
    RT_ZERO(pEntry->pad2);

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineStreamOutput(pThisCC, pDXContext, soid, pEntry);
    return rc;
}


int vmsvga3dDXDestroyStreamOutput(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyStreamOutput const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyStreamOutput, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dStreamOutputId const soid = pCmd->soid;

    ASSERT_GUEST_RETURN(pDXContext->cot.paStreamOutput, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(soid < pDXContext->cot.cStreamOutput, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXDestroyStreamOutput(pThisCC, pDXContext, soid);

    SVGACOTableDXStreamOutputEntry *pEntry = &pDXContext->cot.paStreamOutput[soid];
    RT_ZERO(*pEntry);
    pEntry->mobid = SVGA_ID_INVALID;

    return rc;
}


int vmsvga3dDXSetStreamOutput(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetStreamOutput const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetStreamOutput, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dStreamOutputId const soid = pCmd->soid;

    ASSERT_GUEST_RETURN(   soid == SVGA_ID_INVALID
                        || soid < pDXContext->cot.cStreamOutput, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    pDXContext->svgaDXContext.streamOut.soid = soid;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetStreamOutput(pThisCC, pDXContext, soid);
    return rc;
}


static int dxSetOrGrowCOTable(PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, PVMSVGAMOB pMob,
                              SVGACOTableType type, uint32_t validSizeInBytes, bool fGrow)
{
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    int rc = VINF_SUCCESS;

    ASSERT_GUEST_RETURN(type < RT_ELEMENTS(pDXContext->aCOTMobs), VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t cbCOT;
    if (pMob)
    {
        /* Bind a new mob to the COTable. */
        cbCOT = vmsvgaR3MobSize(pMob);

        ASSERT_GUEST_RETURN(validSizeInBytes <= cbCOT, VERR_INVALID_PARAMETER);
        RT_UNTRUSTED_VALIDATED_FENCE();

        /* Create a memory pointer, which is accessible by host. */
        rc = vmsvgaR3MobBackingStoreCreate(pSvgaR3State, pMob, validSizeInBytes);
    }
    else
    {
        /* Unbind. */
        validSizeInBytes = 0;
        cbCOT = 0;
        vmsvgaR3MobBackingStoreDelete(pSvgaR3State, pDXContext->aCOTMobs[type]);
    }

    uint32_t cEntries = 0;
    uint32_t cValidEntries = 0;
    if (RT_SUCCESS(rc))
    {
        static uint32_t const s_acbEntry[SVGA_COTABLE_MAX] =
        {
            sizeof(SVGACOTableDXRTViewEntry),
            sizeof(SVGACOTableDXDSViewEntry),
            sizeof(SVGACOTableDXSRViewEntry),
            sizeof(SVGACOTableDXElementLayoutEntry),
            sizeof(SVGACOTableDXBlendStateEntry),
            sizeof(SVGACOTableDXDepthStencilEntry),
            sizeof(SVGACOTableDXRasterizerStateEntry),
            sizeof(SVGACOTableDXSamplerEntry),
            sizeof(SVGACOTableDXStreamOutputEntry),
            sizeof(SVGACOTableDXQueryEntry),
            sizeof(SVGACOTableDXShaderEntry),
            sizeof(SVGACOTableDXUAViewEntry),
        };

        cEntries = cbCOT / s_acbEntry[type];
        cValidEntries = validSizeInBytes / s_acbEntry[type];
    }

    if (RT_SUCCESS(rc))
    {
        if (   fGrow
            && pDXContext->aCOTMobs[type]
            && cValidEntries)
        {
            /* Copy entries from the current mob to the new mob. */
            void const *pvSrc = vmsvgaR3MobBackingStorePtr(pDXContext->aCOTMobs[type], 0);
            void *pvDst = vmsvgaR3MobBackingStorePtr(pMob, 0);
            if (pvSrc && pvDst)
                memcpy(pvDst, pvSrc, validSizeInBytes);
            else
                AssertFailedStmt(rc = VERR_INVALID_STATE);
        }
    }

    if (RT_SUCCESS(rc))
    {
        pDXContext->aCOTMobs[type] = pMob;

        void *pvCOT = vmsvgaR3MobBackingStorePtr(pMob, 0);
        switch (type)
        {
            case SVGA_COTABLE_RTVIEW:
                pDXContext->cot.paRTView          = (SVGACOTableDXRTViewEntry *)pvCOT;
                pDXContext->cot.cRTView           = cEntries;
                break;
            case SVGA_COTABLE_DSVIEW:
                pDXContext->cot.paDSView          = (SVGACOTableDXDSViewEntry *)pvCOT;
                pDXContext->cot.cDSView           = cEntries;
                break;
            case SVGA_COTABLE_SRVIEW:
                pDXContext->cot.paSRView          = (SVGACOTableDXSRViewEntry *)pvCOT;
                pDXContext->cot.cSRView           = cEntries;
                break;
            case SVGA_COTABLE_ELEMENTLAYOUT:
                pDXContext->cot.paElementLayout   = (SVGACOTableDXElementLayoutEntry *)pvCOT;
                pDXContext->cot.cElementLayout    = cEntries;
                break;
            case SVGA_COTABLE_BLENDSTATE:
                pDXContext->cot.paBlendState      = (SVGACOTableDXBlendStateEntry *)pvCOT;
                pDXContext->cot.cBlendState       = cEntries;
                break;
            case SVGA_COTABLE_DEPTHSTENCIL:
                pDXContext->cot.paDepthStencil    = (SVGACOTableDXDepthStencilEntry *)pvCOT;
                pDXContext->cot.cDepthStencil     = cEntries;
                break;
            case SVGA_COTABLE_RASTERIZERSTATE:
                pDXContext->cot.paRasterizerState = (SVGACOTableDXRasterizerStateEntry *)pvCOT;
                pDXContext->cot.cRasterizerState  = cEntries;
                break;
            case SVGA_COTABLE_SAMPLER:
                pDXContext->cot.paSampler         = (SVGACOTableDXSamplerEntry *)pvCOT;
                pDXContext->cot.cSampler          = cEntries;
                break;
            case SVGA_COTABLE_STREAMOUTPUT:
                pDXContext->cot.paStreamOutput    = (SVGACOTableDXStreamOutputEntry *)pvCOT;
                pDXContext->cot.cStreamOutput     = cEntries;
                break;
            case SVGA_COTABLE_DXQUERY:
                pDXContext->cot.paQuery           = (SVGACOTableDXQueryEntry *)pvCOT;
                pDXContext->cot.cQuery            = cEntries;
                break;
            case SVGA_COTABLE_DXSHADER:
                pDXContext->cot.paShader          = (SVGACOTableDXShaderEntry *)pvCOT;
                pDXContext->cot.cShader           = cEntries;
                break;
            case SVGA_COTABLE_UAVIEW:
                pDXContext->cot.paUAView          = (SVGACOTableDXUAViewEntry *)pvCOT;
                pDXContext->cot.cUAView           = cEntries;
                break;
            case SVGA_COTABLE_MAX: break; /* Compiler warning */
        }
    }
    else
        vmsvgaR3MobBackingStoreDelete(pSvgaR3State, pMob);

    /* Notify the backend. */
    if (RT_SUCCESS(rc))
        rc = pSvgaR3State->pFuncsDX->pfnDXSetCOTable(pThisCC, pDXContext, type, cValidEntries);

    return rc;
}


int vmsvga3dDXSetCOTable(PVGASTATECC pThisCC, SVGA3dCmdDXSetCOTable const *pCmd, PVMSVGAMOB pMob)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetCOTable, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, pCmd->cid, &pDXContext);
    AssertRCReturn(rc, rc);
    RT_UNTRUSTED_VALIDATED_FENCE();

    return dxSetOrGrowCOTable(pThisCC, pDXContext, pMob, pCmd->type, pCmd->validSizeInBytes, false);
}


int vmsvga3dDXReadbackCOTable(PVGASTATECC pThisCC, SVGA3dCmdDXReadbackCOTable const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, pCmd->cid, &pDXContext);
    AssertRCReturn(rc, rc);
    RT_UNTRUSTED_VALIDATED_FENCE();

    ASSERT_GUEST_RETURN(pCmd->type < RT_ELEMENTS(pDXContext->aCOTMobs), VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    PVMSVGAMOB pMob = pDXContext->aCOTMobs[pCmd->type];
    rc = vmsvgaR3MobBackingStoreWriteToGuest(pSvgaR3State, pMob);
    return rc;
}


int vmsvga3dDXBufferCopy(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXBufferCopy, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXBufferCopy(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXSurfaceCopyAndReadback(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSurfaceCopyAndReadback, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXSurfaceCopyAndReadback(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXMoveQuery(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXMoveQuery, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXMoveQuery(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXBindAllQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXBindAllQuery const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    RT_NOREF(idDXContext);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, pCmd->cid, &pDXContext);
    AssertRCReturn(rc, rc);

    for (uint32_t i = 0; i < pDXContext->cot.cQuery; ++i)
    {
        SVGACOTableDXQueryEntry *pEntry = &pDXContext->cot.paQuery[i];
        if (pEntry->type != SVGA3D_QUERYTYPE_INVALID)
            pEntry->mobid = pCmd->mobid;
    }

    return rc;
}


int vmsvga3dDXReadbackAllQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXReadbackAllQuery const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    RT_NOREF(idDXContext);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, pCmd->cid, &pDXContext);
    AssertRCReturn(rc, rc);

    /* "Read back cached states from the device if they exist."
     * The device does not cache queries. So this is a NOP.
     */
    RT_NOREF(pDXContext);

    return rc;
}


int vmsvga3dDXBindAllShader(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXBindAllShader, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXBindAllShader(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXHint(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXHint, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXHint(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXBufferUpdate(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXBufferUpdate, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXBufferUpdate(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXSetConstantBufferOffset(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetConstantBufferOffset const *pCmd, SVGA3dShaderType type)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetSingleConstantBuffer, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(pCmd->slot < SVGA3D_DX_MAX_CONSTBUFFERS, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    uint32_t const idxShaderState = type - SVGA3D_SHADERTYPE_MIN;
    SVGA3dConstantBufferBinding *pCBB = &pDXContext->svgaDXContext.shaderState[idxShaderState].constantBuffers[pCmd->slot];

    /* Only 'offsetInBytes' is updated. */
    // pCBB->sid;
    pCBB->offsetInBytes = pCmd->offsetInBytes;
    // pCBB->sizeInBytes;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetSingleConstantBuffer(pThisCC, pDXContext, pCmd->slot, type, pCBB->sid, pCBB->offsetInBytes, pCBB->sizeInBytes);
    return rc;
}


int vmsvga3dDXCondBindAllShader(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXCondBindAllShader, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXCondBindAllShader(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dScreenCopy(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnScreenCopy, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnScreenCopy(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXGrowCOTable(PVGASTATECC pThisCC, SVGA3dCmdDXGrowCOTable const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetCOTable, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, pCmd->cid, &pDXContext);
    AssertRCReturn(rc, rc);

    PVMSVGAMOB pMob = vmsvgaR3MobGet(pSvgaR3State, pCmd->mobid);
    return dxSetOrGrowCOTable(pThisCC, pDXContext, pMob, pCmd->type, pCmd->validSizeInBytes, true);
}


int vmsvga3dIntraSurfaceCopy(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdIntraSurfaceCopy const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnIntraSurfaceCopy, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnIntraSurfaceCopy(pThisCC, pDXContext, pCmd->surface, pCmd->box);
    return rc;
}


int vmsvga3dDXResolveCopy(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXResolveCopy, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXResolveCopy(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXPredResolveCopy(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXPredResolveCopy, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXPredResolveCopy(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXPredConvertRegion(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXPredConvertRegion, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXPredConvertRegion(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXPredConvert(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXPredConvert, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXPredConvert(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dWholeSurfaceCopy(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnWholeSurfaceCopy, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnWholeSurfaceCopy(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXDefineUAView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineUAView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDefineUAView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dUAViewId const uaViewId = pCmd->uaViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paUAView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(uaViewId < pDXContext->cot.cUAView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXUAViewEntry *pEntry = &pDXContext->cot.paUAView[uaViewId];
    pEntry->sid               = pCmd->sid;
    pEntry->format            = pCmd->format;
    pEntry->resourceDimension = pCmd->resourceDimension;
    pEntry->desc              = pCmd->desc;
    pEntry->structureCount    = 0;

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineUAView(pThisCC, pDXContext, uaViewId, pEntry);
    return rc;
}


int vmsvga3dDXDestroyUAView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyUAView const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDestroyUAView, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dUAViewId const uaViewId = pCmd->uaViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paUAView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(uaViewId < pDXContext->cot.cUAView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXUAViewEntry *pEntry = &pDXContext->cot.paUAView[uaViewId];
    RT_ZERO(*pEntry);

    rc = pSvgaR3State->pFuncsDX->pfnDXDestroyUAView(pThisCC, pDXContext, uaViewId);
    return rc;
}


int vmsvga3dDXClearUAViewUint(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXClearUAViewUint const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXClearUAViewUint, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dUAViewId const uaViewId = pCmd->uaViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paUAView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(uaViewId < pDXContext->cot.cUAView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXClearUAViewUint(pThisCC, pDXContext, uaViewId, pCmd->value.value);
    return rc;
}


int vmsvga3dDXClearUAViewFloat(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXClearUAViewFloat const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXClearUAViewFloat, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dUAViewId const uaViewId = pCmd->uaViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paUAView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(uaViewId < pDXContext->cot.cUAView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXClearUAViewFloat(pThisCC, pDXContext, uaViewId, pCmd->value.value);
    return rc;
}


int vmsvga3dDXCopyStructureCount(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXCopyStructureCount const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXCopyStructureCount, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dUAViewId const uaViewId = pCmd->srcUAViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paUAView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(uaViewId < pDXContext->cot.cUAView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnDXCopyStructureCount(pThisCC, pDXContext, uaViewId, pCmd->destSid, pCmd->destByteOffset);
    return rc;
}


int vmsvga3dDXSetUAViews(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetUAViews const *pCmd, uint32_t cUAViewId, SVGA3dUAViewId const *paUAViewId)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetUAViews, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(pCmd->uavSpliceIndex <= SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(cUAViewId <= SVGA3D_DX11_1_MAX_UAVIEWS, VERR_INVALID_PARAMETER);
    for (uint32_t i = 0; i < cUAViewId; ++i)
        ASSERT_GUEST_RETURN(   paUAViewId[i] < pDXContext->cot.cUAView
                            || paUAViewId[i] == SVGA3D_INVALID_ID, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    for (uint32_t i = 0; i < cUAViewId; ++i)
    {
        SVGA3dUAViewId const uaViewId = paUAViewId[i];
        pDXContext->svgaDXContext.uaViewIds[i] = uaViewId;
    }
    pDXContext->svgaDXContext.uavSpliceIndex = pCmd->uavSpliceIndex;

    rc = pSvgaR3State->pFuncsDX->pfnDXSetUAViews(pThisCC, pDXContext, pCmd->uavSpliceIndex, cUAViewId, paUAViewId);
    return rc;
}


int vmsvga3dDXDrawIndexedInstancedIndirect(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDrawIndexedInstancedIndirect const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDrawIndexedInstancedIndirect, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDrawIndexedInstancedIndirect(pThisCC, pDXContext, pCmd->argsBufferSid, pCmd->byteOffsetForArgs);
    return rc;
}


int vmsvga3dDXDrawInstancedIndirect(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDrawInstancedIndirect const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDrawInstancedIndirect, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDrawInstancedIndirect(pThisCC, pDXContext, pCmd->argsBufferSid, pCmd->byteOffsetForArgs);
    return rc;
}


int vmsvga3dDXDispatch(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDispatch const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDispatch, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDispatch(pThisCC, pDXContext, pCmd->threadGroupCountX, pCmd->threadGroupCountY, pCmd->threadGroupCountZ);
    return rc;
}


int vmsvga3dDXDispatchIndirect(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXDispatchIndirect, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXDispatchIndirect(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dWriteZeroSurface(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnWriteZeroSurface, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnWriteZeroSurface(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dHintZeroSurface(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnHintZeroSurface, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnHintZeroSurface(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXTransferToBuffer(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXTransferToBuffer, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXTransferToBuffer(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXSetStructureCount(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetStructureCount const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dUAViewId const uaViewId = pCmd->uaViewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paUAView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(uaViewId < pDXContext->cot.cUAView, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXUAViewEntry *pEntry = &pDXContext->cot.paUAView[uaViewId];
    pEntry->structureCount = pCmd->structureCount;

    return VINF_SUCCESS;
}


int vmsvga3dLogicOpsBitBlt(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnLogicOpsBitBlt, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnLogicOpsBitBlt(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dLogicOpsTransBlt(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnLogicOpsTransBlt, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnLogicOpsTransBlt(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dLogicOpsStretchBlt(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnLogicOpsStretchBlt, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnLogicOpsStretchBlt(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dLogicOpsColorFill(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnLogicOpsColorFill, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnLogicOpsColorFill(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dLogicOpsAlphaBlend(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnLogicOpsAlphaBlend, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnLogicOpsAlphaBlend(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dLogicOpsClearTypeBlend(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnLogicOpsClearTypeBlend, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnLogicOpsClearTypeBlend(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXSetCSUAViews(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetCSUAViews const *pCmd, uint32_t cUAViewId, SVGA3dUAViewId const *paUAViewId)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetCSUAViews, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    ASSERT_GUEST_RETURN(pCmd->startIndex < SVGA3D_DX11_1_MAX_UAVIEWS, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(cUAViewId <= SVGA3D_DX11_1_MAX_UAVIEWS - pCmd->startIndex, VERR_INVALID_PARAMETER);
    for (uint32_t i = 0; i < cUAViewId; ++i)
        ASSERT_GUEST_RETURN(   paUAViewId[i] < pDXContext->cot.cUAView
                            || paUAViewId[i] == SVGA3D_INVALID_ID, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    for (uint32_t i = 0; i < cUAViewId; ++i)
    {
        SVGA3dUAViewId const uaViewId = paUAViewId[i];
        pDXContext->svgaDXContext.csuaViewIds[pCmd->startIndex + i] = uaViewId;
    }

    rc = pSvgaR3State->pFuncsDX->pfnDXSetCSUAViews(pThisCC, pDXContext, pCmd->startIndex, cUAViewId, paUAViewId);
    return rc;
}


int vmsvga3dDXSetMinLOD(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetMinLOD, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXSetMinLOD(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXDefineStreamOutputWithMob(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineStreamOutputWithMob const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dStreamOutputId const soid = pCmd->soid;

    ASSERT_GUEST_RETURN(pDXContext->cot.paStreamOutput, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(soid < pDXContext->cot.cStreamOutput, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(pCmd->numOutputStreamEntries < SVGA3D_MAX_STREAMOUT_DECLS, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXStreamOutputEntry *pEntry = &pDXContext->cot.paStreamOutput[soid];
    pEntry->numOutputStreamEntries = pCmd->numOutputStreamEntries;
    RT_ZERO(pEntry->decl);
    memcpy(pEntry->streamOutputStrideInBytes, pCmd->streamOutputStrideInBytes, sizeof(pEntry->streamOutputStrideInBytes));
    pEntry->rasterizedStream = pCmd->rasterizedStream;
    pEntry->numOutputStreamStrides = pCmd->numOutputStreamStrides;
    pEntry->mobid = SVGA_ID_INVALID;
    pEntry->offsetInBytes = 0;
    pEntry->usesMob = 1;
    pEntry->pad0 = 0;
    pEntry->pad1 = 0;
    RT_ZERO(pEntry->pad2);

    rc = pSvgaR3State->pFuncsDX->pfnDXDefineStreamOutput(pThisCC, pDXContext, soid, pEntry);
    return rc;
}


int vmsvga3dDXSetShaderIface(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXSetShaderIface, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXSetShaderIface(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXBindStreamOutput(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXBindStreamOutput const *pCmd)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);
    SVGA3dStreamOutputId const soid = pCmd->soid;

    ASSERT_GUEST_RETURN(pDXContext->cot.paStreamOutput, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(soid < pDXContext->cot.cStreamOutput, VERR_INVALID_PARAMETER);
    RT_UNTRUSTED_VALIDATED_FENCE();

    SVGACOTableDXStreamOutputEntry *pEntry = &pDXContext->cot.paStreamOutput[soid];

    ASSERT_GUEST_RETURN(pCmd->sizeInBytes >= pEntry->numOutputStreamEntries * sizeof(SVGA3dStreamOutputDeclarationEntry), VERR_INVALID_PARAMETER);
    ASSERT_GUEST(pEntry->usesMob);

    pEntry->mobid = pCmd->mobid;
    pEntry->offsetInBytes = pCmd->offsetInBytes;
    pEntry->usesMob = 1;

    return VINF_SUCCESS;
}


int vmsvga3dSurfaceStretchBltNonMSToMS(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnSurfaceStretchBltNonMSToMS, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnSurfaceStretchBltNonMSToMS(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dDXBindShaderIface(PVGASTATECC pThisCC, uint32_t idDXContext)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnDXBindShaderIface, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    rc = pSvgaR3State->pFuncsDX->pfnDXBindShaderIface(pThisCC, pDXContext);
    return rc;
}


int vmsvga3dVBDXClearRenderTargetViewRegion(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdVBDXClearRenderTargetViewRegion const *pCmd, uint32_t cRect, SVGASignedRect const *paRect)
{
    int rc;
    PVMSVGAR3STATE const pSvgaR3State = pThisCC->svga.pSvgaR3State;
    AssertReturn(pSvgaR3State->pFuncsDX && pSvgaR3State->pFuncsDX->pfnVBDXClearRenderTargetViewRegion, VERR_INVALID_STATE);
    PVMSVGA3DSTATE p3dState = pThisCC->svga.p3dState;
    AssertReturn(p3dState, VERR_INVALID_STATE);

    PVMSVGA3DDXCONTEXT pDXContext;
    rc = vmsvga3dDXContextFromCid(p3dState, idDXContext, &pDXContext);
    AssertRCReturn(rc, rc);

    SVGA3dRenderTargetViewId const renderTargetViewId = pCmd->viewId;

    ASSERT_GUEST_RETURN(pDXContext->cot.paRTView, VERR_INVALID_STATE);
    ASSERT_GUEST_RETURN(renderTargetViewId < pDXContext->cot.cRTView, VERR_INVALID_PARAMETER);
    ASSERT_GUEST_RETURN(cRect <= 65536, VERR_INVALID_PARAMETER); /* Arbitrary limit. */
    RT_UNTRUSTED_VALIDATED_FENCE();

    rc = pSvgaR3State->pFuncsDX->pfnVBDXClearRenderTargetViewRegion(pThisCC, pDXContext, renderTargetViewId, &pCmd->color, cRect, paRect);
    return rc;
}

