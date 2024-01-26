/* $Id: VBoxDXCmd.h $ */
/** @file
 * VBoxVideo Display D3D User mode dll
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

#ifndef GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_wddm_dx_VBoxDXCmd_h
#define GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_wddm_dx_VBoxDXCmd_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

int vgpu10DefineBlendState(PVBOXDX_DEVICE pDevice,
                           SVGA3dBlendStateId blendId,
                           uint8 alphaToCoverageEnable,
                           uint8 independentBlendEnable,
                           const SVGA3dDXBlendStatePerRT *perRT);
int vgpu10DestroyBlendState(PVBOXDX_DEVICE pDevice,
                            SVGA3dBlendStateId blendId);
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
                                  SVGA3dComparisonFunc backStencilFunc);
int vgpu10DestroyDepthStencilState(PVBOXDX_DEVICE pDevice,
                                   SVGA3dDepthStencilStateId depthStencilId);
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
                                uint16 lineStipplePattern);
int vgpu10DestroyRasterizerState(PVBOXDX_DEVICE pDevice,
                                 SVGA3dRasterizerStateId rasterizerId);
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
                             float maxLOD);
int vgpu10DestroySamplerState(PVBOXDX_DEVICE pDevice,
                              SVGA3dSamplerId samplerId);
int vgpu10DefineElementLayout(PVBOXDX_DEVICE pDevice,
                              SVGA3dElementLayoutId elementLayoutId,
                              uint32_t cElements,
                              SVGA3dInputElementDesc *paDesc);
int vgpu10DestroyElementLayout(PVBOXDX_DEVICE pDevice,
                               SVGA3dElementLayoutId elementLayoutId);
int vgpu10SetInputLayout(PVBOXDX_DEVICE pDevice,
                         SVGA3dElementLayoutId elementLayoutId);
int vgpu10SetBlendState(PVBOXDX_DEVICE pDevice,
                        SVGA3dBlendStateId blendId,
                        const float blendFactor[4],
                        uint32 sampleMask);
int vgpu10SetDepthStencilState(PVBOXDX_DEVICE pDevice,
                               SVGA3dDepthStencilStateId depthStencilId,
                               uint32 stencilRef);
int vgpu10SetRasterizerState(PVBOXDX_DEVICE pDevice,
                             SVGA3dRasterizerStateId rasterizerId);
int vgpu10SetSamplers(PVBOXDX_DEVICE pDevice,
                      uint32 startSampler,
                      SVGA3dShaderType type,
                      uint32_t numSamplers,
                      const SVGA3dSamplerId *paSamplerIds);
int vgpu10SetTopology(PVBOXDX_DEVICE pDevice,
                      SVGA3dPrimitiveType topology);
int vgpu10Draw(PVBOXDX_DEVICE pDevice,
               uint32 vertexCount,
               uint32 startVertexLocation);
int vgpu10DrawIndexed(PVBOXDX_DEVICE pDevice,
                      uint32 indexCount,
                      uint32 startIndexLocation,
                      int32 baseVertexLocation);
int vgpu10DrawInstanced(PVBOXDX_DEVICE pDevice,
                        uint32 vertexCountPerInstance,
                        uint32 instanceCount,
                        uint32 startVertexLocation,
                        uint32 startInstanceLocation);
int vgpu10DrawIndexedInstanced(PVBOXDX_DEVICE pDevice,
                               uint32 indexCountPerInstance,
                               uint32 instanceCount,
                               uint32 startIndexLocation,
                               int32 baseVertexLocation,
                               uint32 startInstanceLocation);
int vgpu10DrawAuto(PVBOXDX_DEVICE pDevice);
int vgpu10SetViewports(PVBOXDX_DEVICE pDevice,
                       uint32_t cViewports,
                       const D3D10_DDI_VIEWPORT *paViewports);
int vgpu10SetScissorRects(PVBOXDX_DEVICE pDevice,
                          uint32_t cRects,
                          const D3D10_DDI_RECT *paRects);
int vgpu10DefineShader(PVBOXDX_DEVICE pDevice,
                       SVGA3dShaderId shaderId,
                       SVGA3dShaderType type,
                       uint32_t sizeInBytes);
int vgpu10DefineStreamOutputWithMob(PVBOXDX_DEVICE pDevice,
                                    SVGA3dStreamOutputId soid,
                                    uint32 numOutputStreamEntries,
                                    uint32 numOutputStreamStrides,
                                    const uint32 *streamOutputStrideInBytes,
                                    uint32 rasterizedStream);
int vgpu10BindStreamOutput(PVBOXDX_DEVICE pDevice,
                           SVGA3dStreamOutputId soid,
                           D3DKMT_HANDLE hAllocation,
                           uint32 offsetInBytes,
                           uint32 sizeInBytes);
int vgpu10SetStreamOutput(PVBOXDX_DEVICE pDevice,
                          SVGA3dStreamOutputId soid);
int vgpu10DestroyShader(PVBOXDX_DEVICE pDevice,
                        SVGA3dShaderId shaderId);
int vgpu10BindShader(PVBOXDX_DEVICE pDevice,
                     uint32_t shid,
                     D3DKMT_HANDLE hAllocation,
                     uint32_t offsetInBytes);
int vgpu10SetShader(PVBOXDX_DEVICE pDevice,
                    SVGA3dShaderId shaderId,
                    SVGA3dShaderType type);
int vgpu10SetVertexBuffers(PVBOXDX_DEVICE pDevice,
                           uint32_t startBuffer,
                           uint32_t numBuffers,
                           D3DKMT_HANDLE *paAllocations,
                           const UINT *paStrides,
                           const UINT *paOffsets);
int vgpu10SetIndexBuffer(PVBOXDX_DEVICE pDevice,
                         D3DKMT_HANDLE hAllocation,
                         SVGA3dSurfaceFormat format,
                         uint32_t offset);
int vgpu10SoSetTargets(PVBOXDX_DEVICE pDevice,
                       uint32_t numTargets,
                       D3DKMT_HANDLE *paAllocations,
                       uint32_t *paOffsets,
                       uint32_t *paSizes);
int vgpu10DefineShaderResourceView(PVBOXDX_DEVICE pDevice,
                                   SVGA3dShaderResourceViewId shaderResourceViewId,
                                   D3DKMT_HANDLE hAllocation,
                                   SVGA3dSurfaceFormat format,
                                   SVGA3dResourceType resourceDimension,
                                   SVGA3dShaderResourceViewDesc const *pDesc);
int vgpu10GenMips(PVBOXDX_DEVICE pDevice,
                  SVGA3dShaderResourceViewId shaderResourceViewId);
int vgpu10DestroyShaderResourceView(PVBOXDX_DEVICE pDevice,
                                    SVGA3dShaderResourceViewId shaderResourceViewId);
int vgpu10DefineRenderTargetView(PVBOXDX_DEVICE pDevice,
                                 SVGA3dRenderTargetViewId renderTargetViewId,
                                 D3DKMT_HANDLE hAllocation,
                                 SVGA3dSurfaceFormat format,
                                 SVGA3dResourceType resourceDimension,
                                 SVGA3dRenderTargetViewDesc const *pDesc);
int vgpu10ClearRenderTargetView(PVBOXDX_DEVICE pDevice,
                                SVGA3dRenderTargetViewId renderTargetViewId,
                                const float rgba[4]);
int vgpu10DestroyRenderTargetView(PVBOXDX_DEVICE pDevice,
                                  SVGA3dRenderTargetViewId renderTargetViewId);
int vgpu10DefineDepthStencilView(PVBOXDX_DEVICE pDevice,
                                 SVGA3dDepthStencilViewId depthStencilViewId,
                                 D3DKMT_HANDLE hAllocation,
                                 SVGA3dSurfaceFormat format,
                                 SVGA3dResourceType resourceDimension,
                                 uint32 mipSlice,
                                 uint32 firstArraySlice,
                                 uint32 arraySize,
                                 SVGA3DCreateDSViewFlags flags);
int vgpu10ClearDepthStencilView(PVBOXDX_DEVICE pDevice,
                                uint16 flags,
                                uint16 stencil,
                                SVGA3dDepthStencilViewId depthStencilViewId,
                                float depth);
int vgpu10DestroyDepthStencilView(PVBOXDX_DEVICE pDevice,
                                  SVGA3dDepthStencilViewId depthStencilViewId);
int vgpu10SetRenderTargets(PVBOXDX_DEVICE pDevice,
                           SVGA3dDepthStencilViewId depthStencilViewId,
                           uint32_t numRTVs,
                           uint32_t numClearSlots,
                           uint32_t *paRenderTargetViewIds);
int vgpu10SetShaderResources(PVBOXDX_DEVICE pDevice,
                             SVGA3dShaderType type,
                             uint32 startView,
                             uint32_t numViews,
                             uint32_t *paViewIds);
int vgpu10SetSingleConstantBuffer(PVBOXDX_DEVICE pDevice,
                                  uint32 slot,
                                  SVGA3dShaderType type,
                                  D3DKMT_HANDLE hAllocation,
                                  uint32 offsetInBytes,
                                  uint32 sizeInBytes);
int vgpu10UpdateSubResource(PVBOXDX_DEVICE pDevice,
                            D3DKMT_HANDLE hAllocation,
                            uint32 subResource,
                            const SVGA3dBox *pBox);
int vgpu10ReadbackSubResource(PVBOXDX_DEVICE pDevice,
                            D3DKMT_HANDLE hAllocation,
                            uint32 subResource);
int vgpu10TransferFromBuffer(PVBOXDX_DEVICE pDevice,
                             D3DKMT_HANDLE hSrcAllocation,
                             uint32 srcOffset,
                             uint32 srcPitch,
                             uint32 srcSlicePitch,
                             D3DKMT_HANDLE hDstAllocation,
                             uint32 destSubResource,
                             SVGA3dBox const &destBox);
int vgpu10ResourceCopyRegion(PVBOXDX_DEVICE pDevice,
                             D3DKMT_HANDLE hDstAllocation,
                             uint32 dstSubResource,
                             uint32 dstX,
                             uint32 dstY,
                             uint32 dstZ,
                             D3DKMT_HANDLE hSrcAllocation,
                             uint32 srcSubResource,
                             SVGA3dBox const &srcBox);
int vgpu10ResourceCopy(PVBOXDX_DEVICE pDevice,
                       D3DKMT_HANDLE hDstAllocation,
                       D3DKMT_HANDLE hSrcAllocation);
int vgpu10MobFence64(PVBOXDX_DEVICE pDevice,
                     uint64 value,
                     D3DKMT_HANDLE hAllocation,
                     uint32 mobOffset);
int vgpu10DefineQuery(PVBOXDX_DEVICE pDevice,
                      SVGA3dQueryId queryId,
                      SVGA3dQueryType type,
                      SVGA3dDXQueryFlags flags);
int vgpu10DestroyQuery(PVBOXDX_DEVICE pDevice,
                       SVGA3dQueryId queryId);
int vgpu10BindQuery(PVBOXDX_DEVICE pDevice,
                    SVGA3dQueryId queryId,
                    D3DKMT_HANDLE hAllocation);
int vgpu10SetQueryOffset(PVBOXDX_DEVICE pDevice,
                         SVGA3dQueryId queryId,
                         uint32 mobOffset);
int vgpu10BeginQuery(PVBOXDX_DEVICE pDevice,
                     SVGA3dQueryId queryId);
int vgpu10EndQuery(PVBOXDX_DEVICE pDevice,
                   SVGA3dQueryId queryId);
int vgpu10ReadbackQuery(PVBOXDX_DEVICE pDevice,
                        SVGA3dQueryId queryId);
int vgpu10SetPredication(PVBOXDX_DEVICE pDevice,
                         SVGA3dQueryId queryId,
                         uint32 predicateValue);
int vgpu10DefineUAView(PVBOXDX_DEVICE pDevice,
                       SVGA3dUAViewId uaViewId,
                       D3DKMT_HANDLE hAllocation,
                       SVGA3dSurfaceFormat format,
                       SVGA3dResourceType resourceDimension,
                       const SVGA3dUAViewDesc &desc);
int vgpu10DestroyUAView(PVBOXDX_DEVICE pDevice,
                        SVGA3dUAViewId uaViewId);
int vgpu10ClearUAViewUint(PVBOXDX_DEVICE pDevice,
                          SVGA3dUAViewId uaViewId,
                          const uint32 value[4]);
int vgpu10ClearUAViewFloat(PVBOXDX_DEVICE pDevice,
                           SVGA3dUAViewId uaViewId,
                           const float value[4]);
int vgpu10SetCSUAViews(PVBOXDX_DEVICE pDevice,
                       uint32 startIndex,
                       uint32 numViews,
                       const SVGA3dUAViewId *paViewIds);
int vgpu10SetUAViews(PVBOXDX_DEVICE pDevice,
                     uint32 uavSpliceIndex,
                     uint32 numViews,
                     const SVGA3dUAViewId *paViewIds);
int vgpu10SetStructureCount(PVBOXDX_DEVICE pDevice,
                            SVGA3dUAViewId uaViewId,
                            uint32 structureCount);
int vgpu10Dispatch(PVBOXDX_DEVICE pDevice,
                   uint32 threadGroupCountX,
                   uint32 threadGroupCountY,
                   uint32 threadGroupCountZ);
int vgpu10DispatchIndirect(PVBOXDX_DEVICE pDevice,
                           D3DKMT_HANDLE hAllocation,
                           uint32 byteOffsetForArgs);
int vgpu10DrawIndexedInstancedIndirect(PVBOXDX_DEVICE pDevice,
                                       D3DKMT_HANDLE hAllocation,
                                       uint32 byteOffsetForArgs);
int vgpu10DrawInstancedIndirect(PVBOXDX_DEVICE pDevice,
                                D3DKMT_HANDLE hAllocation,
                                uint32 byteOffsetForArgs);
int vgpu10CopyStructureCount(PVBOXDX_DEVICE pDevice,
                             SVGA3dUAViewId srcUAViewId,
                             D3DKMT_HANDLE hDstBuffer,
                             uint32 destByteOffset);
int vgpu10ClearRenderTargetViewRegion(PVBOXDX_DEVICE pDevice,
                                      SVGA3dRenderTargetViewId viewId,
                                      const float color[4],
                                      const D3D10_DDI_RECT *paRects,
                                      uint32_t cRects);
int vgpu10PresentBlt(PVBOXDX_DEVICE pDevice,
                     D3DKMT_HANDLE hSrcAllocation,
                     uint32 srcSubResource,
                     D3DKMT_HANDLE hDstAllocation,
                     uint32 destSubResource,
                     SVGA3dBox const &boxSrc,
                     SVGA3dBox const &boxDest,
                     SVGA3dDXPresentBltMode mode);

#endif /* !GA_INCLUDED_SRC_WINNT_Graphics_Video_disp_wddm_dx_VBoxDXCmd_h */
