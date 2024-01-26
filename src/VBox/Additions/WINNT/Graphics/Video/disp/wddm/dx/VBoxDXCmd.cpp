/* $Id: VBoxDXCmd.cpp $ */
/** @file
 * VirtualBox D3D user mode driver utilities.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

#include <iprt/alloc.h>
#include <iprt/errcore.h>
#include <iprt/initterm.h>
#include <VBox/log.h>

#include <iprt/win/windows.h>
#include <iprt/win/d3dkmthk.h>

#include <d3d10umddi.h>

#include "VBoxDX.h"
#include "VBoxDXCmd.h"


#define SET_CMD_FIELD(_aName) cmd->_aName = _aName


int vgpu10DefineBlendState(PVBOXDX_DEVICE pDevice,
                           SVGA3dBlendStateId blendId,
                           uint8 alphaToCoverageEnable,
                           uint8 independentBlendEnable,
                           const SVGA3dDXBlendStatePerRT *perRT)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_BLEND_STATE,
                                             sizeof(SVGA3dCmdDXDefineBlendState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineBlendState *cmd = (SVGA3dCmdDXDefineBlendState *)pvCmd;
    SET_CMD_FIELD(blendId);
    SET_CMD_FIELD(alphaToCoverageEnable);
    SET_CMD_FIELD(independentBlendEnable);
    memcpy(cmd->perRT, perRT, sizeof(cmd->perRT));
    cmd->pad0 = 0;

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyBlendState(PVBOXDX_DEVICE pDevice,
                            SVGA3dBlendStateId blendId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_BLEND_STATE,
                                             sizeof(SVGA3dCmdDXDestroyBlendState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyBlendState *cmd = (SVGA3dCmdDXDestroyBlendState *)pvCmd;
    SET_CMD_FIELD(blendId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineDepthStencilState(PVBOXDX_DEVICE pDevice,
                                  SVGA3dDepthStencilStateId depthStencilId,
                                  uint8_t depthEnable,
                                  SVGA3dDepthWriteMask depthWriteMask,
                                  SVGA3dComparisonFunc depthFunc,
                                  uint8 stencilEnable,
                                  uint8 frontEnable,
                                  uint8 backEnable,
                                  uint8 stencilReadMask,
                                  uint8 stencilWriteMask,
                                  uint8 frontStencilFailOp,
                                  uint8 frontStencilDepthFailOp,
                                  uint8 frontStencilPassOp,
                                  SVGA3dComparisonFunc frontStencilFunc,
                                  uint8 backStencilFailOp,
                                  uint8 backStencilDepthFailOp,
                                  uint8 backStencilPassOp,
                                  SVGA3dComparisonFunc backStencilFunc)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_STATE,
                                             sizeof(SVGA3dCmdDXDefineDepthStencilState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineDepthStencilState *cmd = (SVGA3dCmdDXDefineDepthStencilState *)pvCmd;
    SET_CMD_FIELD(depthStencilId);
    SET_CMD_FIELD(depthEnable);
    SET_CMD_FIELD(depthWriteMask);
    SET_CMD_FIELD(depthFunc);
    SET_CMD_FIELD(stencilEnable);
    SET_CMD_FIELD(frontEnable);
    SET_CMD_FIELD(backEnable);
    SET_CMD_FIELD(stencilReadMask);
    SET_CMD_FIELD(stencilWriteMask);
    SET_CMD_FIELD(frontStencilFailOp);
    SET_CMD_FIELD(frontStencilDepthFailOp);
    SET_CMD_FIELD(frontStencilPassOp);
    SET_CMD_FIELD(frontStencilFunc);
    SET_CMD_FIELD(backStencilFailOp);
    SET_CMD_FIELD(backStencilDepthFailOp);
    SET_CMD_FIELD(backStencilPassOp);
    SET_CMD_FIELD(backStencilFunc);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyDepthStencilState(PVBOXDX_DEVICE pDevice,
                                   SVGA3dDepthStencilStateId depthStencilId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_STATE,
                                             sizeof(SVGA3dCmdDXDestroyDepthStencilState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyDepthStencilState *cmd = (SVGA3dCmdDXDestroyDepthStencilState *)pvCmd;
    SET_CMD_FIELD(depthStencilId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineRasterizerState(PVBOXDX_DEVICE pDevice,
                                SVGA3dRasterizerStateId rasterizerId,
                                uint8 fillMode,
                                SVGA3dCullMode cullMode,
                                uint8 frontCounterClockwise,
                                uint8 provokingVertexLast,
                                int32 depthBias,
                                float depthBiasClamp,
                                float slopeScaledDepthBias,
                                uint8 depthClipEnable,
                                uint8 scissorEnable,
                                SVGA3dMultisampleRastEnable multisampleEnable,
                                uint8 antialiasedLineEnable,
                                float lineWidth,
                                uint8 lineStippleEnable,
                                uint8 lineStippleFactor,
                                uint16 lineStipplePattern)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_RASTERIZER_STATE,
                                             sizeof(SVGA3dCmdDXDefineRasterizerState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineRasterizerState *cmd = (SVGA3dCmdDXDefineRasterizerState *)pvCmd;
    SET_CMD_FIELD(rasterizerId);
    SET_CMD_FIELD(fillMode);
    SET_CMD_FIELD(cullMode);
    SET_CMD_FIELD(frontCounterClockwise);
    SET_CMD_FIELD(provokingVertexLast);
    SET_CMD_FIELD(depthBias);
    SET_CMD_FIELD(depthBiasClamp);
    SET_CMD_FIELD(slopeScaledDepthBias);
    SET_CMD_FIELD(depthClipEnable);
    SET_CMD_FIELD(scissorEnable);
    SET_CMD_FIELD(multisampleEnable);
    SET_CMD_FIELD(antialiasedLineEnable);
    SET_CMD_FIELD(lineWidth);
    SET_CMD_FIELD(lineStippleEnable);
    SET_CMD_FIELD(lineStippleFactor);
    SET_CMD_FIELD(lineStipplePattern);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyRasterizerState(PVBOXDX_DEVICE pDevice,
                                 SVGA3dRasterizerStateId rasterizerId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_RASTERIZER_STATE,
                                             sizeof(SVGA3dCmdDXDestroyRasterizerState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyRasterizerState *cmd = (SVGA3dCmdDXDestroyRasterizerState *)pvCmd;
    SET_CMD_FIELD(rasterizerId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineSamplerState(PVBOXDX_DEVICE pDevice,
                             SVGA3dSamplerId samplerId,
                             SVGA3dFilter filter,
                             uint8 addressU,
                             uint8 addressV,
                             uint8 addressW,
                             float mipLODBias,
                             uint8 maxAnisotropy,
                             SVGA3dComparisonFunc comparisonFunc,
                             SVGA3dRGBAFloat borderColor,
                             float minLOD,
                             float maxLOD)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_SAMPLER_STATE,
                                             sizeof(SVGA3dCmdDXDefineSamplerState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineSamplerState *cmd = (SVGA3dCmdDXDefineSamplerState *)pvCmd;
    SET_CMD_FIELD(samplerId);
    SET_CMD_FIELD(filter);
    SET_CMD_FIELD(addressU);
    SET_CMD_FIELD(addressV);
    SET_CMD_FIELD(addressW);
    cmd->pad0 = 0;
    SET_CMD_FIELD(mipLODBias);
    SET_CMD_FIELD(maxAnisotropy);
    SET_CMD_FIELD(comparisonFunc);
    cmd->pad1 = 0;
    SET_CMD_FIELD(borderColor);
    SET_CMD_FIELD(minLOD);
    SET_CMD_FIELD(maxLOD);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroySamplerState(PVBOXDX_DEVICE pDevice,
                              SVGA3dSamplerId samplerId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_SAMPLER_STATE,
                                             sizeof(SVGA3dCmdDXDestroySamplerState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroySamplerState *cmd = (SVGA3dCmdDXDestroySamplerState *)pvCmd;
    SET_CMD_FIELD(samplerId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineElementLayout(PVBOXDX_DEVICE pDevice,
                              SVGA3dElementLayoutId elementLayoutId,
                              uint32_t cElements,
                              SVGA3dInputElementDesc *paDesc)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_ELEMENTLAYOUT,
                                             sizeof(SVGA3dCmdDXDefineElementLayout)
                                             + cElements * sizeof(SVGA3dInputElementDesc));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineElementLayout *cmd = (SVGA3dCmdDXDefineElementLayout *)pvCmd;
    SET_CMD_FIELD(elementLayoutId);
    memcpy(&cmd[1], paDesc, cElements * sizeof(SVGA3dInputElementDesc));

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyElementLayout(PVBOXDX_DEVICE pDevice,
                               SVGA3dElementLayoutId elementLayoutId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_ELEMENTLAYOUT,
                                             sizeof(SVGA3dCmdDXDestroyElementLayout));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyElementLayout *cmd = (SVGA3dCmdDXDestroyElementLayout *)pvCmd;
    SET_CMD_FIELD(elementLayoutId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetInputLayout(PVBOXDX_DEVICE pDevice,
                         SVGA3dElementLayoutId elementLayoutId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_INPUT_LAYOUT,
                                             sizeof(SVGA3dCmdDXSetInputLayout));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetInputLayout *cmd = (SVGA3dCmdDXSetInputLayout *)pvCmd;
    SET_CMD_FIELD(elementLayoutId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetBlendState(PVBOXDX_DEVICE pDevice,
                        SVGA3dBlendStateId blendId,
                        const float blendFactor[4],
                        uint32 sampleMask)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_BLEND_STATE,
                                             sizeof(SVGA3dCmdDXSetBlendState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetBlendState *cmd = (SVGA3dCmdDXSetBlendState *)pvCmd;
    SET_CMD_FIELD(blendId);
    memcpy(cmd->blendFactor, blendFactor, sizeof(blendFactor));
    SET_CMD_FIELD(sampleMask);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetDepthStencilState(PVBOXDX_DEVICE pDevice,
                               SVGA3dDepthStencilStateId depthStencilId,
                               uint32 stencilRef)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_DEPTHSTENCIL_STATE,
                                             sizeof(SVGA3dCmdDXSetDepthStencilState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetDepthStencilState *cmd = (SVGA3dCmdDXSetDepthStencilState *)pvCmd;
    SET_CMD_FIELD(depthStencilId);
    SET_CMD_FIELD(stencilRef);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetRasterizerState(PVBOXDX_DEVICE pDevice,
                             SVGA3dRasterizerStateId rasterizerId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_RASTERIZER_STATE,
                                             sizeof(SVGA3dCmdDXSetRasterizerState));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetRasterizerState *cmd = (SVGA3dCmdDXSetRasterizerState *)pvCmd;
    SET_CMD_FIELD(rasterizerId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetSamplers(PVBOXDX_DEVICE pDevice,
                      uint32 startSampler,
                      SVGA3dShaderType type,
                      uint32_t numSamplers,
                      const SVGA3dSamplerId *paSamplerIds)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_SAMPLERS,
                                             sizeof(SVGA3dCmdDXSetSamplers)
                                             + numSamplers * sizeof(SVGA3dSamplerId));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetSamplers *cmd = (SVGA3dCmdDXSetSamplers *)pvCmd;
    SET_CMD_FIELD(startSampler);
    SET_CMD_FIELD(type);
    memcpy(&cmd[1], paSamplerIds, numSamplers * sizeof(SVGA3dSamplerId));

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetTopology(PVBOXDX_DEVICE pDevice,
                      SVGA3dPrimitiveType topology)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_TOPOLOGY,
                                             sizeof(SVGA3dCmdDXSetTopology));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetTopology *cmd = (SVGA3dCmdDXSetTopology *)pvCmd;
    SET_CMD_FIELD(topology);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10Draw(PVBOXDX_DEVICE pDevice,
               uint32 vertexCount,
               uint32 startVertexLocation)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DRAW,
                                             sizeof(SVGA3dCmdDXDraw));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDraw *cmd = (SVGA3dCmdDXDraw *)pvCmd;
    SET_CMD_FIELD(vertexCount);
    SET_CMD_FIELD(startVertexLocation);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10DrawIndexed(PVBOXDX_DEVICE pDevice,
                      uint32 indexCount,
                      uint32 startIndexLocation,
                      int32 baseVertexLocation)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DRAW_INDEXED,
                                             sizeof(SVGA3dCmdDXDrawIndexed));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDrawIndexed *cmd = (SVGA3dCmdDXDrawIndexed *)pvCmd;
    SET_CMD_FIELD(indexCount);
    SET_CMD_FIELD(startIndexLocation);
    SET_CMD_FIELD(baseVertexLocation);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10DrawInstanced(PVBOXDX_DEVICE pDevice,
                        uint32 vertexCountPerInstance,
                        uint32 instanceCount,
                        uint32 startVertexLocation,
                        uint32 startInstanceLocation)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DRAW_INSTANCED,
                                             sizeof(SVGA3dCmdDXDrawInstanced));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDrawInstanced *cmd = (SVGA3dCmdDXDrawInstanced *)pvCmd;
    SET_CMD_FIELD(vertexCountPerInstance);
    SET_CMD_FIELD(instanceCount);
    SET_CMD_FIELD(startVertexLocation);
    SET_CMD_FIELD(startInstanceLocation);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10DrawIndexedInstanced(PVBOXDX_DEVICE pDevice,
                               uint32 indexCountPerInstance,
                               uint32 instanceCount,
                               uint32 startIndexLocation,
                               int32 baseVertexLocation,
                               uint32 startInstanceLocation)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DRAW_INDEXED_INSTANCED,
                                             sizeof(SVGA3dCmdDXDrawIndexedInstanced));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDrawIndexedInstanced *cmd = (SVGA3dCmdDXDrawIndexedInstanced *)pvCmd;
    SET_CMD_FIELD(indexCountPerInstance);
    SET_CMD_FIELD(instanceCount);
    SET_CMD_FIELD(startIndexLocation);
    SET_CMD_FIELD(baseVertexLocation);
    SET_CMD_FIELD(startInstanceLocation);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10DrawAuto(PVBOXDX_DEVICE pDevice)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DRAW_AUTO,
                                             sizeof(SVGA3dCmdDXDrawAuto));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDrawAuto *cmd = (SVGA3dCmdDXDrawAuto *)pvCmd;
    cmd->pad0 = 0;

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10SetViewports(PVBOXDX_DEVICE pDevice,
                       uint32_t cViewports,
                       const D3D10_DDI_VIEWPORT *paViewports)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_VIEWPORTS,
                                             sizeof(SVGA3dCmdDXSetViewports) + cViewports * sizeof(SVGA3dViewport));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetViewports *cmd = (SVGA3dCmdDXSetViewports *)pvCmd;
    cmd->pad0 = 0;

    SVGA3dViewport *paSvgaViewports = (SVGA3dViewport *)&cmd[1];
    for (uint32_t i = 0; i < cViewports; ++i)
    {
        SVGA3dViewport *d = &paSvgaViewports[i];
        const D3D10_DDI_VIEWPORT *s = &paViewports[i];
        d->x        = s->TopLeftX;
        d->y        = s->TopLeftY;
        d->width    = s->Width;
        d->height   = s->Height;
        d->minDepth = s->MinDepth;
        d->maxDepth = s->MaxDepth;
    }

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10SetScissorRects(PVBOXDX_DEVICE pDevice,
                          uint32_t cRects,
                          const D3D10_DDI_RECT *paRects)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_SCISSORRECTS,
                                             sizeof(SVGA3dCmdDXSetScissorRects) + cRects * sizeof(SVGASignedRect));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetScissorRects *cmd = (SVGA3dCmdDXSetScissorRects *)pvCmd;
    cmd->pad0 = 0;

    SVGASignedRect *paSvgaRects = (SVGASignedRect *)&cmd[1];
    for (uint32_t i = 0; i < cRects; ++i)
    {
        SVGASignedRect *d = &paSvgaRects[i];
        const D3D10_DDI_RECT *s = &paRects[i];
        d->left   = s->left;
        d->top    = s->top;
        d->right  = s->right;
        d->bottom = s->bottom;
    }

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10DefineShader(PVBOXDX_DEVICE pDevice,
                       SVGA3dShaderId shaderId,
                       SVGA3dShaderType type,
                       uint32_t sizeInBytes)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_SHADER,
                                             sizeof(SVGA3dCmdDXDefineShader));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineShader *cmd = (SVGA3dCmdDXDefineShader *)pvCmd;
    SET_CMD_FIELD(shaderId);
    SET_CMD_FIELD(type);
    SET_CMD_FIELD(sizeInBytes);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineStreamOutputWithMob(PVBOXDX_DEVICE pDevice,
                                    SVGA3dStreamOutputId soid,
                                    uint32 numOutputStreamEntries,
                                    uint32 numOutputStreamStrides,
                                    const uint32 *streamOutputStrideInBytes,
                                    uint32 rasterizedStream)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_STREAMOUTPUT_WITH_MOB,
                                             sizeof(SVGA3dCmdDXDefineStreamOutputWithMob));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineStreamOutputWithMob *cmd = (SVGA3dCmdDXDefineStreamOutputWithMob *)pvCmd;
    SET_CMD_FIELD(soid);
    SET_CMD_FIELD(numOutputStreamEntries);
    SET_CMD_FIELD(numOutputStreamStrides);
    for (unsigned i = 0; i < numOutputStreamStrides; ++i)
        cmd->streamOutputStrideInBytes[i] = streamOutputStrideInBytes[i];
    for (unsigned i = numOutputStreamStrides; i < SVGA3D_DX_MAX_SOTARGETS; ++i)
        cmd->streamOutputStrideInBytes[i] = 0;
    SET_CMD_FIELD(rasterizedStream);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10BindStreamOutput(PVBOXDX_DEVICE pDevice,
                           SVGA3dStreamOutputId soid,
                           D3DKMT_HANDLE hAllocation,
                           uint32 offsetInBytes,
                           uint32 sizeInBytes)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_BIND_STREAMOUTPUT,
                                             sizeof(SVGA3dCmdDXBindStreamOutput), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXBindStreamOutput *cmd = (SVGA3dCmdDXBindStreamOutput *)pvCmd;
    SET_CMD_FIELD(soid);
    cmd->mobid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(offsetInBytes);
    SET_CMD_FIELD(sizeInBytes);

    vboxDXStorePatchLocation(pDevice, &cmd->mobid, VBOXDXALLOCATIONTYPE_CO,
                             hAllocation, offsetInBytes, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetStreamOutput(PVBOXDX_DEVICE pDevice,
                          SVGA3dStreamOutputId soid)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_STREAMOUTPUT,
                                             sizeof(SVGA3dCmdDXSetStreamOutput), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetStreamOutput *cmd = (SVGA3dCmdDXSetStreamOutput *)pvCmd;
    SET_CMD_FIELD(soid);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyShader(PVBOXDX_DEVICE pDevice,
                        SVGA3dShaderId shaderId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_SHADER,
                                             sizeof(SVGA3dCmdDXDestroyShader), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyShader *cmd = (SVGA3dCmdDXDestroyShader *)pvCmd;
    SET_CMD_FIELD(shaderId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10BindShader(PVBOXDX_DEVICE pDevice,
                     uint32_t shid,
                     D3DKMT_HANDLE hAllocation,
                     uint32_t offsetInBytes)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_BIND_SHADER,
                                             sizeof(SVGA3dCmdDXBindShader), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXBindShader *cmd = (SVGA3dCmdDXBindShader *)pvCmd;
    cmd->cid = SVGA3D_INVALID_ID; /** @todo Add a patch or modify the command in the miniport? */
    SET_CMD_FIELD(shid);
    cmd->mobid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(offsetInBytes);

    vboxDXStorePatchLocation(pDevice, &cmd->mobid, VBOXDXALLOCATIONTYPE_SHADERS,
                             hAllocation, offsetInBytes, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10SetShader(PVBOXDX_DEVICE pDevice,
                    SVGA3dShaderId shaderId,
                    SVGA3dShaderType type)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_SHADER,
                                             sizeof(SVGA3dCmdDXSetShader));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetShader *cmd = (SVGA3dCmdDXSetShader *)pvCmd;
    SET_CMD_FIELD(shaderId);
    SET_CMD_FIELD(type);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10SetVertexBuffers(PVBOXDX_DEVICE pDevice,
                           uint32_t startBuffer,
                           uint32_t numBuffers,
                           D3DKMT_HANDLE *paAllocations,
                           const UINT *paStrides,
                           const UINT *paOffsets)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS,
                                             sizeof(SVGA3dCmdDXSetVertexBuffers)
                                             + numBuffers * sizeof(SVGA3dVertexBuffer),
                                             numBuffers);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetVertexBuffers *cmd = (SVGA3dCmdDXSetVertexBuffers *)pvCmd;
    SET_CMD_FIELD(startBuffer);

    SVGA3dVertexBuffer *pVertexBuffer = (SVGA3dVertexBuffer *)&cmd[1];
    for (unsigned i = 0; i < numBuffers; ++i, ++pVertexBuffer)
    {
        pVertexBuffer->sid    = SVGA3D_INVALID_ID;
        pVertexBuffer->stride = paStrides[i];
        pVertexBuffer->offset = paOffsets[i];
        vboxDXStorePatchLocation(pDevice, &pVertexBuffer->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                                 paAllocations[i], 0, false);
    }

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10SetIndexBuffer(PVBOXDX_DEVICE pDevice,
                         D3DKMT_HANDLE hAllocation,
                         SVGA3dSurfaceFormat format,
                         uint32_t offset)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_INDEX_BUFFER,
                                             sizeof(SVGA3dCmdDXSetIndexBuffer), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetIndexBuffer *cmd = (SVGA3dCmdDXSetIndexBuffer *)pvCmd;
    cmd->sid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(format);
    SET_CMD_FIELD(offset);
    vboxDXStorePatchLocation(pDevice, &cmd->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, false);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10SoSetTargets(PVBOXDX_DEVICE pDevice,
                       uint32_t numTargets,
                       D3DKMT_HANDLE *paAllocations,
                       uint32_t *paOffsets,
                       uint32_t *paSizes)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_SOTARGETS,
                                             sizeof(SVGA3dCmdDXSetSOTargets)
                                             + numTargets * sizeof(SVGA3dSoTarget),
                                             numTargets);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetSOTargets *cmd = (SVGA3dCmdDXSetSOTargets *)pvCmd;
    cmd->pad0 = 0;

    SVGA3dSoTarget *pSoTarget = (SVGA3dSoTarget *)&cmd[1];
    for (unsigned i = 0; i < numTargets; ++i, ++pSoTarget)
    {
        pSoTarget->sid         = SVGA3D_INVALID_ID;
        pSoTarget->offset      = paOffsets[i];
        pSoTarget->sizeInBytes = paSizes[i];
        vboxDXStorePatchLocation(pDevice, &pSoTarget->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                                 paAllocations[i], 0, true);
    }

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineShaderResourceView(PVBOXDX_DEVICE pDevice,
                                   SVGA3dShaderResourceViewId shaderResourceViewId,
                                   D3DKMT_HANDLE hAllocation,
                                   SVGA3dSurfaceFormat format,
                                   SVGA3dResourceType resourceDimension,
                                   SVGA3dShaderResourceViewDesc const *pDesc)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_SHADERRESOURCE_VIEW,
                                             sizeof(SVGA3dCmdDXDefineShaderResourceView), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineShaderResourceView *cmd = (SVGA3dCmdDXDefineShaderResourceView *)pvCmd;
    SET_CMD_FIELD(shaderResourceViewId);
    cmd->sid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(format);
    SET_CMD_FIELD(resourceDimension);
    cmd->desc = *pDesc;
    vboxDXStorePatchLocation(pDevice, &cmd->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, false);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10GenMips(PVBOXDX_DEVICE pDevice,
                  SVGA3dShaderResourceViewId shaderResourceViewId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_GENMIPS,
                                             sizeof(SVGA3dCmdDXGenMips));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXGenMips *cmd = (SVGA3dCmdDXGenMips *)pvCmd;
    SET_CMD_FIELD(shaderResourceViewId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyShaderResourceView(PVBOXDX_DEVICE pDevice,
                                    SVGA3dShaderResourceViewId shaderResourceViewId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_SHADERRESOURCE_VIEW,
                                             sizeof(SVGA3dCmdDXDestroyShaderResourceView));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyShaderResourceView *cmd = (SVGA3dCmdDXDestroyShaderResourceView *)pvCmd;
    SET_CMD_FIELD(shaderResourceViewId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineRenderTargetView(PVBOXDX_DEVICE pDevice,
                                 SVGA3dRenderTargetViewId renderTargetViewId,
                                 D3DKMT_HANDLE hAllocation,
                                 SVGA3dSurfaceFormat format,
                                 SVGA3dResourceType resourceDimension,
                                 SVGA3dRenderTargetViewDesc const *pDesc)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_RENDERTARGET_VIEW,
                                             sizeof(SVGA3dCmdDXDefineRenderTargetView), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineRenderTargetView *cmd = (SVGA3dCmdDXDefineRenderTargetView *)pvCmd;
    SET_CMD_FIELD(renderTargetViewId);
    cmd->sid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(format);
    SET_CMD_FIELD(resourceDimension);
    cmd->desc = *pDesc;
    vboxDXStorePatchLocation(pDevice, &cmd->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ClearRenderTargetView(PVBOXDX_DEVICE pDevice,
                                SVGA3dRenderTargetViewId renderTargetViewId,
                                const float rgba[4])
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_CLEAR_RENDERTARGET_VIEW,
                                             sizeof(SVGA3dCmdDXClearRenderTargetView));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXClearRenderTargetView *cmd = (SVGA3dCmdDXClearRenderTargetView *)pvCmd;
    SET_CMD_FIELD(renderTargetViewId);
    cmd->rgba.value[0] = rgba[0];
    cmd->rgba.value[1] = rgba[1];
    cmd->rgba.value[2] = rgba[2];
    cmd->rgba.value[3] = rgba[3];

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ClearRenderTargetViewRegion(PVBOXDX_DEVICE pDevice,
                                      SVGA3dRenderTargetViewId viewId,
                                      const float color[4],
                                      const D3D10_DDI_RECT *paRects,
                                      uint32_t cRects)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_VB_DX_CLEAR_RENDERTARGET_VIEW_REGION,
                                             sizeof(SVGA3dCmdVBDXClearRenderTargetViewRegion) + cRects * sizeof(SVGASignedRect));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdVBDXClearRenderTargetViewRegion *cmd = (SVGA3dCmdVBDXClearRenderTargetViewRegion *)pvCmd;
    SET_CMD_FIELD(viewId);
    cmd->reserved = 0;
    cmd->color.value[0] = color[0];
    cmd->color.value[1] = color[1];
    cmd->color.value[2] = color[2];
    cmd->color.value[3] = color[3];

    SVGASignedRect *paSvgaRects = (SVGASignedRect *)&cmd[1];
    for (uint32_t i = 0; i < cRects; ++i)
    {
        SVGASignedRect *d = &paSvgaRects[i];
        const D3D10_DDI_RECT *s = &paRects[i];
        d->left   = s->left;
        d->top    = s->top;
        d->right  = s->right;
        d->bottom = s->bottom;
    }

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}

int vgpu10DestroyRenderTargetView(PVBOXDX_DEVICE pDevice,
                                  SVGA3dRenderTargetViewId renderTargetViewId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_RENDERTARGET_VIEW,
                                             sizeof(SVGA3dCmdDXDestroyRenderTargetView));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyRenderTargetView *cmd = (SVGA3dCmdDXDestroyRenderTargetView *)pvCmd;
    SET_CMD_FIELD(renderTargetViewId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineDepthStencilView(PVBOXDX_DEVICE pDevice,
                                 SVGA3dDepthStencilViewId depthStencilViewId,
                                 D3DKMT_HANDLE hAllocation,
                                 SVGA3dSurfaceFormat format,
                                 SVGA3dResourceType resourceDimension,
                                 uint32 mipSlice,
                                 uint32 firstArraySlice,
                                 uint32 arraySize,
                                 SVGA3DCreateDSViewFlags flags)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_VIEW_V2,
                                             sizeof(SVGA3dCmdDXDefineDepthStencilView_v2), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineDepthStencilView_v2 *cmd = (SVGA3dCmdDXDefineDepthStencilView_v2 *)pvCmd;
    SET_CMD_FIELD(depthStencilViewId);
    cmd->sid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(format);
    SET_CMD_FIELD(resourceDimension);
    SET_CMD_FIELD(mipSlice);
    SET_CMD_FIELD(firstArraySlice);
    SET_CMD_FIELD(arraySize);
    SET_CMD_FIELD(flags);
    vboxDXStorePatchLocation(pDevice, &cmd->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ClearDepthStencilView(PVBOXDX_DEVICE pDevice,
                                uint16 flags,
                                uint16 stencil,
                                SVGA3dDepthStencilViewId depthStencilViewId,
                                float depth)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_CLEAR_DEPTHSTENCIL_VIEW,
                                             sizeof(SVGA3dCmdDXClearDepthStencilView));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXClearDepthStencilView *cmd = (SVGA3dCmdDXClearDepthStencilView *)pvCmd;
    SET_CMD_FIELD(flags);
    SET_CMD_FIELD(stencil);
    SET_CMD_FIELD(depthStencilViewId);
    SET_CMD_FIELD(depth);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyDepthStencilView(PVBOXDX_DEVICE pDevice,
                                  SVGA3dDepthStencilViewId depthStencilViewId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_VIEW,
                                             sizeof(SVGA3dCmdDXDestroyDepthStencilView));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyDepthStencilView *cmd = (SVGA3dCmdDXDestroyDepthStencilView *)pvCmd;
    SET_CMD_FIELD(depthStencilViewId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetRenderTargets(PVBOXDX_DEVICE pDevice,
                           SVGA3dDepthStencilViewId depthStencilViewId,
                           uint32_t numRTVs,
                           uint32_t numClearSlots,
                           uint32_t *paRenderTargetViewIds)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_RENDERTARGETS,
                                             sizeof(SVGA3dCmdDXSetRenderTargets)
                                             + (numRTVs + numClearSlots) * sizeof(SVGA3dRenderTargetViewId));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetRenderTargets *cmd = (SVGA3dCmdDXSetRenderTargets *)pvCmd;
    SET_CMD_FIELD(depthStencilViewId);

    SVGA3dRenderTargetViewId *dst = (SVGA3dRenderTargetViewId *)&cmd[1];
    memcpy(dst, paRenderTargetViewIds, numRTVs * sizeof(SVGA3dRenderTargetViewId));
    dst += numRTVs;

    for (unsigned i = 0; i < numClearSlots; ++i)
        *dst++ = SVGA3D_INVALID_ID;

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetShaderResources(PVBOXDX_DEVICE pDevice,
                             SVGA3dShaderType type,
                             uint32 startView,
                             uint32_t numViews,
                             uint32_t *paViewIds)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_SHADER_RESOURCES,
                                             sizeof(SVGA3dCmdDXSetShaderResources)
                                             + numViews * sizeof(SVGA3dShaderResourceViewId));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetShaderResources *cmd = (SVGA3dCmdDXSetShaderResources *)pvCmd;
    SET_CMD_FIELD(startView);
    SET_CMD_FIELD(type);
    memcpy(&cmd[1], paViewIds, numViews * sizeof(SVGA3dShaderResourceViewId));

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetSingleConstantBuffer(PVBOXDX_DEVICE pDevice,
                                  uint32 slot,
                                  SVGA3dShaderType type,
                                  D3DKMT_HANDLE hAllocation,
                                  uint32 offsetInBytes,
                                  uint32 sizeInBytes)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_SINGLE_CONSTANT_BUFFER,
                                             sizeof(SVGA3dCmdDXSetSingleConstantBuffer), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetSingleConstantBuffer *cmd = (SVGA3dCmdDXSetSingleConstantBuffer *)pvCmd;
    SET_CMD_FIELD(slot);
    SET_CMD_FIELD(type);
    cmd->sid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(offsetInBytes);
    SET_CMD_FIELD(sizeInBytes);
    vboxDXStorePatchLocation(pDevice, &cmd->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, false);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10UpdateSubResource(PVBOXDX_DEVICE pDevice,
                            D3DKMT_HANDLE hAllocation,
                            uint32 subResource,
                            const SVGA3dBox *pBox)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_UPDATE_SUBRESOURCE,
                                             sizeof(SVGA3dCmdDXUpdateSubResource), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXUpdateSubResource *cmd = (SVGA3dCmdDXUpdateSubResource *)pvCmd;
    cmd->sid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(subResource);
    cmd->box = *pBox;
    vboxDXStorePatchLocation(pDevice, &cmd->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ReadbackSubResource(PVBOXDX_DEVICE pDevice,
                              D3DKMT_HANDLE hAllocation,
                              uint32 subResource)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_READBACK_SUBRESOURCE,
                                             sizeof(SVGA3dCmdDXReadbackSubResource), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXReadbackSubResource *cmd = (SVGA3dCmdDXReadbackSubResource *)pvCmd;
    cmd->sid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(subResource);

    /* fWriteOperation == true should make sure that DXGK waits until the command is completed
     * before getting the allocation data in pfnLockCb.
     */
    vboxDXStorePatchLocation(pDevice, &cmd->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10TransferFromBuffer(PVBOXDX_DEVICE pDevice,
                             D3DKMT_HANDLE hSrcAllocation,
                             uint32 srcOffset,
                             uint32 srcPitch,
                             uint32 srcSlicePitch,
                             D3DKMT_HANDLE hDstAllocation,
                             uint32 destSubResource,
                             SVGA3dBox const &destBox)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_TRANSFER_FROM_BUFFER,
                                             sizeof(SVGA3dCmdDXTransferFromBuffer), 2);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXTransferFromBuffer *cmd = (SVGA3dCmdDXTransferFromBuffer *)pvCmd;
    cmd->srcSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(srcOffset);
    SET_CMD_FIELD(srcPitch);
    SET_CMD_FIELD(srcSlicePitch);
    cmd->destSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(destSubResource);
    SET_CMD_FIELD(destBox);
    vboxDXStorePatchLocation(pDevice, &cmd->srcSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hSrcAllocation, 0, false);
    vboxDXStorePatchLocation(pDevice, &cmd->destSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hDstAllocation, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ResourceCopyRegion(PVBOXDX_DEVICE pDevice,
                             D3DKMT_HANDLE hDstAllocation,
                             uint32 dstSubResource,
                             uint32 dstX,
                             uint32 dstY,
                             uint32 dstZ,
                             D3DKMT_HANDLE hSrcAllocation,
                             uint32 srcSubResource,
                             SVGA3dBox const &srcBox)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_PRED_COPY_REGION,
                                             sizeof(SVGA3dCmdDXPredCopyRegion), 2);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXPredCopyRegion *cmd = (SVGA3dCmdDXPredCopyRegion *)pvCmd;
    cmd->dstSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(dstSubResource);
    cmd->srcSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(srcSubResource);
    cmd->box.x    = dstX;
    cmd->box.y    = dstY;
    cmd->box.z    = dstZ;
    cmd->box.w    = srcBox.w;
    cmd->box.h    = srcBox.h;
    cmd->box.d    = srcBox.d;
    cmd->box.srcx = srcBox.x;
    cmd->box.srcy = srcBox.y;
    cmd->box.srcz = srcBox.z;

    vboxDXStorePatchLocation(pDevice, &cmd->dstSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hDstAllocation, 0, true);
    vboxDXStorePatchLocation(pDevice, &cmd->srcSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hSrcAllocation, 0, false);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ResourceCopy(PVBOXDX_DEVICE pDevice,
                       D3DKMT_HANDLE hDstAllocation,
                       D3DKMT_HANDLE hSrcAllocation)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_PRED_COPY,
                                             sizeof(SVGA3dCmdDXPredCopy), 2);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXPredCopy *cmd = (SVGA3dCmdDXPredCopy *)pvCmd;
    cmd->dstSid = SVGA3D_INVALID_ID;
    cmd->srcSid = SVGA3D_INVALID_ID;

    vboxDXStorePatchLocation(pDevice, &cmd->dstSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hDstAllocation, 0, true);
    vboxDXStorePatchLocation(pDevice, &cmd->srcSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hSrcAllocation, 0, false);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10MobFence64(PVBOXDX_DEVICE pDevice,
                     uint64 value,
                     D3DKMT_HANDLE hAllocation,
                     uint32 mobOffset)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_MOB_FENCE_64,
                                             sizeof(SVGA3dCmdDXMobFence64), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXMobFence64 *cmd = (SVGA3dCmdDXMobFence64 *)pvCmd;
    SET_CMD_FIELD(value);
    cmd->mobId = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(mobOffset);

    vboxDXStorePatchLocation(pDevice, &cmd->mobId, VBOXDXALLOCATIONTYPE_CO,
                             hAllocation, mobOffset, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineQuery(PVBOXDX_DEVICE pDevice,
                      SVGA3dQueryId queryId,
                      SVGA3dQueryType type,
                      SVGA3dDXQueryFlags flags)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_QUERY,
                                             sizeof(SVGA3dCmdDXDefineQuery), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineQuery *cmd = (SVGA3dCmdDXDefineQuery *)pvCmd;
    SET_CMD_FIELD(queryId);
    SET_CMD_FIELD(type);
    SET_CMD_FIELD(flags);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyQuery(PVBOXDX_DEVICE pDevice,
                       SVGA3dQueryId queryId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_QUERY,
                                             sizeof(SVGA3dCmdDXDestroyQuery), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyQuery *cmd = (SVGA3dCmdDXDestroyQuery *)pvCmd;
    SET_CMD_FIELD(queryId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10BindQuery(PVBOXDX_DEVICE pDevice,
                    SVGA3dQueryId queryId,
                    D3DKMT_HANDLE hAllocation)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_BIND_QUERY,
                                             sizeof(SVGA3dCmdDXBindQuery), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXBindQuery *cmd = (SVGA3dCmdDXBindQuery *)pvCmd;
    SET_CMD_FIELD(queryId);
    cmd->mobid = SVGA3D_INVALID_ID;

    vboxDXStorePatchLocation(pDevice, &cmd->mobid, VBOXDXALLOCATIONTYPE_CO,
                             hAllocation, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetQueryOffset(PVBOXDX_DEVICE pDevice,
                         SVGA3dQueryId queryId,
                         uint32 mobOffset)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_QUERY_OFFSET,
                                             sizeof(SVGA3dCmdDXSetQueryOffset), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetQueryOffset *cmd = (SVGA3dCmdDXSetQueryOffset *)pvCmd;
    SET_CMD_FIELD(queryId);
    SET_CMD_FIELD(mobOffset);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10BeginQuery(PVBOXDX_DEVICE pDevice,
                     SVGA3dQueryId queryId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_BEGIN_QUERY,
                                             sizeof(SVGA3dCmdDXBeginQuery), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXBeginQuery *cmd = (SVGA3dCmdDXBeginQuery *)pvCmd;
    SET_CMD_FIELD(queryId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10EndQuery(PVBOXDX_DEVICE pDevice,
                   SVGA3dQueryId queryId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_END_QUERY,
                                             sizeof(SVGA3dCmdDXEndQuery), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXEndQuery *cmd = (SVGA3dCmdDXEndQuery *)pvCmd;
    SET_CMD_FIELD(queryId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ReadbackQuery(PVBOXDX_DEVICE pDevice,
                        SVGA3dQueryId queryId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_READBACK_QUERY,
                                             sizeof(SVGA3dCmdDXReadbackQuery), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXReadbackQuery *cmd = (SVGA3dCmdDXReadbackQuery *)pvCmd;
    SET_CMD_FIELD(queryId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetPredication(PVBOXDX_DEVICE pDevice,
                         SVGA3dQueryId queryId,
                         uint32 predicateValue)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_PREDICATION,
                                             sizeof(SVGA3dCmdDXSetPredication));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetPredication *cmd = (SVGA3dCmdDXSetPredication *)pvCmd;
    SET_CMD_FIELD(queryId);
    SET_CMD_FIELD(predicateValue);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DefineUAView(PVBOXDX_DEVICE pDevice,
                       SVGA3dUAViewId uaViewId,
                       D3DKMT_HANDLE hAllocation,
                       SVGA3dSurfaceFormat format,
                       SVGA3dResourceType resourceDimension,
                       const SVGA3dUAViewDesc &desc)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DEFINE_UA_VIEW,
                                             sizeof(SVGA3dCmdDXDefineUAView), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDefineUAView *cmd = (SVGA3dCmdDXDefineUAView *)pvCmd;
    SET_CMD_FIELD(uaViewId);
    cmd->sid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(format);
    SET_CMD_FIELD(resourceDimension);
    SET_CMD_FIELD(desc);
    vboxDXStorePatchLocation(pDevice, &cmd->sid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DestroyUAView(PVBOXDX_DEVICE pDevice,
                        SVGA3dUAViewId uaViewId)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DESTROY_UA_VIEW,
                                             sizeof(SVGA3dCmdDXDestroyUAView), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDestroyUAView *cmd = (SVGA3dCmdDXDestroyUAView *)pvCmd;
    SET_CMD_FIELD(uaViewId);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ClearUAViewUint(PVBOXDX_DEVICE pDevice,
                          SVGA3dUAViewId uaViewId,
                          const uint32 value[4])
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_CLEAR_UA_VIEW_UINT,
                                             sizeof(SVGA3dCmdDXClearUAViewUint), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXClearUAViewUint *cmd = (SVGA3dCmdDXClearUAViewUint *)pvCmd;
    SET_CMD_FIELD(uaViewId);
    cmd->value.value[0] = value[0];
    cmd->value.value[1] = value[1];
    cmd->value.value[2] = value[2];
    cmd->value.value[3] = value[3];

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10ClearUAViewFloat(PVBOXDX_DEVICE pDevice,
                           SVGA3dUAViewId uaViewId,
                           const float value[4])
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_CLEAR_UA_VIEW_FLOAT,
                                             sizeof(SVGA3dCmdDXClearUAViewFloat), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXClearUAViewFloat *cmd = (SVGA3dCmdDXClearUAViewFloat *)pvCmd;
    SET_CMD_FIELD(uaViewId);
    cmd->value.value[0] = value[0];
    cmd->value.value[1] = value[1];
    cmd->value.value[2] = value[2];
    cmd->value.value[3] = value[3];

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetCSUAViews(PVBOXDX_DEVICE pDevice,
                       uint32 startIndex,
                       uint32 numViews,
                       const SVGA3dUAViewId *paViewIds)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_CS_UA_VIEWS,
                                             sizeof(SVGA3dCmdDXSetCSUAViews)
                                             + numViews * sizeof(SVGA3dUAViewId));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetCSUAViews *cmd = (SVGA3dCmdDXSetCSUAViews *)pvCmd;
    SET_CMD_FIELD(startIndex);
    memcpy(&cmd[1], paViewIds, numViews * sizeof(SVGA3dUAViewId));

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetUAViews(PVBOXDX_DEVICE pDevice,
                     uint32 uavSpliceIndex,
                     uint32 numViews,
                     const SVGA3dUAViewId *paViewIds)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_UA_VIEWS,
                                             sizeof(SVGA3dCmdDXSetUAViews)
                                             + numViews * sizeof(SVGA3dUAViewId));
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetUAViews *cmd = (SVGA3dCmdDXSetUAViews *)pvCmd;
    SET_CMD_FIELD(uavSpliceIndex);
    memcpy(&cmd[1], paViewIds, numViews * sizeof(SVGA3dUAViewId));

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10SetStructureCount(PVBOXDX_DEVICE pDevice,
                            SVGA3dUAViewId uaViewId,
                            uint32 structureCount)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_SET_STRUCTURE_COUNT,
                                             sizeof(SVGA3dCmdDXSetStructureCount), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXSetStructureCount *cmd = (SVGA3dCmdDXSetStructureCount *)pvCmd;
    SET_CMD_FIELD(uaViewId);
    SET_CMD_FIELD(structureCount);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10Dispatch(PVBOXDX_DEVICE pDevice,
                   uint32 threadGroupCountX,
                   uint32 threadGroupCountY,
                   uint32 threadGroupCountZ)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DISPATCH,
                                             sizeof(SVGA3dCmdDXDispatch), 0);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDispatch *cmd = (SVGA3dCmdDXDispatch *)pvCmd;
    SET_CMD_FIELD(threadGroupCountX);
    SET_CMD_FIELD(threadGroupCountY);
    SET_CMD_FIELD(threadGroupCountZ);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DispatchIndirect(PVBOXDX_DEVICE pDevice,
                           D3DKMT_HANDLE hAllocation,
                           uint32 byteOffsetForArgs)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DISPATCH_INDIRECT,
                                             sizeof(SVGA3dCmdDXDispatchIndirect), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDispatchIndirect *cmd = (SVGA3dCmdDXDispatchIndirect *)pvCmd;
    cmd->argsBufferSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(byteOffsetForArgs);

    vboxDXStorePatchLocation(pDevice, &cmd->argsBufferSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, false);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DrawIndexedInstancedIndirect(PVBOXDX_DEVICE pDevice,
                                       D3DKMT_HANDLE hAllocation,
                                       uint32 byteOffsetForArgs)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DRAW_INDEXED_INSTANCED_INDIRECT,
                                             sizeof(SVGA3dCmdDXDrawIndexedInstancedIndirect), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDrawIndexedInstancedIndirect *cmd = (SVGA3dCmdDXDrawIndexedInstancedIndirect *)pvCmd;
    cmd->argsBufferSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(byteOffsetForArgs);

    vboxDXStorePatchLocation(pDevice, &cmd->argsBufferSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, false);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10DrawInstancedIndirect(PVBOXDX_DEVICE pDevice,
                                D3DKMT_HANDLE hAllocation,
                                uint32 byteOffsetForArgs)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_DRAW_INSTANCED_INDIRECT,
                                             sizeof(SVGA3dCmdDXDrawInstancedIndirect), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXDrawInstancedIndirect *cmd = (SVGA3dCmdDXDrawInstancedIndirect *)pvCmd;
    cmd->argsBufferSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(byteOffsetForArgs);

    vboxDXStorePatchLocation(pDevice, &cmd->argsBufferSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hAllocation, 0, false);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10CopyStructureCount(PVBOXDX_DEVICE pDevice,
                             SVGA3dUAViewId srcUAViewId,
                             D3DKMT_HANDLE hDstBuffer,
                             uint32 destByteOffset)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_COPY_STRUCTURE_COUNT,
                                             sizeof(SVGA3dCmdDXCopyStructureCount), 1);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXCopyStructureCount *cmd = (SVGA3dCmdDXCopyStructureCount *)pvCmd;
    SET_CMD_FIELD(srcUAViewId);
    cmd->destSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(destByteOffset);

    vboxDXStorePatchLocation(pDevice, &cmd->destSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hDstBuffer, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}


int vgpu10PresentBlt(PVBOXDX_DEVICE pDevice,
                     D3DKMT_HANDLE hSrcAllocation,
                     uint32 srcSubResource,
                     D3DKMT_HANDLE hDstAllocation,
                     uint32 destSubResource,
                     SVGA3dBox const &boxSrc,
                     SVGA3dBox const &boxDest,
                     SVGA3dDXPresentBltMode mode)
{
    void *pvCmd = vboxDXCommandBufferReserve(pDevice, SVGA_3D_CMD_DX_PRESENTBLT,
                                             sizeof(SVGA3dCmdDXPresentBlt), 2);
    if (!pvCmd)
        return VERR_NO_MEMORY;

    SVGA3dCmdDXPresentBlt *cmd = (SVGA3dCmdDXPresentBlt *)pvCmd;
    cmd->srcSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(srcSubResource);
    cmd->dstSid = SVGA3D_INVALID_ID;
    SET_CMD_FIELD(destSubResource);
    SET_CMD_FIELD(boxSrc);
    SET_CMD_FIELD(boxDest);
    SET_CMD_FIELD(mode);

    vboxDXStorePatchLocation(pDevice, &cmd->srcSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hSrcAllocation, 0, false);
    vboxDXStorePatchLocation(pDevice, &cmd->dstSid, VBOXDXALLOCATIONTYPE_SURFACE,
                             hDstAllocation, 0, true);

    vboxDXCommandBufferCommit(pDevice);
    return VINF_SUCCESS;
}
