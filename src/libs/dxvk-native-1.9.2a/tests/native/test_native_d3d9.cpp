#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <cstring>

#include <d3d9.h>

#include <wsi/native_wsi.h>

#include "../test_utils.h"

using namespace dxvk;

/*
  struct VS_INPUT {
    float3 Position : POSITION;
  };

  struct VS_OUTPUT {
    float4 Position : POSITION;
  };

  VS_OUTPUT main( VS_INPUT IN ) {
    VS_OUTPUT OUT;
    OUT.Position = float4(IN.Position, 0.6f);

    return OUT;
  }
 */

const std::array<uint8_t, 148> g_vertexShaderCode = {{
  0x00, 0x02, 0xfe, 0xff, 0xfe, 0xff, 0x14, 0x00, 0x43, 0x54, 0x41, 0x42,
  0x1c, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x02, 0xfe, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x1c, 0x00, 0x00, 0x00, 0x76, 0x73, 0x5f, 0x32, 0x5f, 0x30, 0x00, 0x4d,
  0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29,
  0x20, 0x48, 0x4c, 0x53, 0x4c, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72,
  0x20, 0x43, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65, 0x72, 0x20, 0x31, 0x30,
  0x2e, 0x31, 0x00, 0xab, 0x51, 0x00, 0x00, 0x05, 0x00, 0x00, 0x0f, 0xa0,
  0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x9a, 0x99, 0x19, 0x3f,
  0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80,
  0x00, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x0f, 0xc0,
  0x00, 0x00, 0x24, 0x90, 0x00, 0x00, 0x40, 0xa0, 0x00, 0x00, 0x95, 0xa0,
  0xff, 0xff, 0x00, 0x00
}};

/*
  struct VS_OUTPUT {
    float4 Position : POSITION;
  };

  struct PS_OUTPUT {
    float4 Colour   : COLOR;
  };

  sampler g_texDepth : register( s0 );

  PS_OUTPUT main( VS_OUTPUT IN ) {
    PS_OUTPUT OUT;

    OUT.Colour = tex2D(g_texDepth, float2(0, 0));
    OUT.Colour = 1.0;

    return OUT;
  }
 */

const std::array<uint8_t, 140> g_pixelShaderCode = {{
  0x00, 0x02, 0xff, 0xff, 0xfe, 0xff, 0x14, 0x00, 0x43, 0x54, 0x41, 0x42,
  0x1c, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x1c, 0x00, 0x00, 0x00, 0x70, 0x73, 0x5f, 0x32, 0x5f, 0x30, 0x00, 0x4d,
  0x69, 0x63, 0x72, 0x6f, 0x73, 0x6f, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29,
  0x20, 0x48, 0x4c, 0x53, 0x4c, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72,
  0x20, 0x43, 0x6f, 0x6d, 0x70, 0x69, 0x6c, 0x65, 0x72, 0x20, 0x31, 0x30,
  0x2e, 0x31, 0x00, 0xab, 0x51, 0x00, 0x00, 0x05, 0x00, 0x00, 0x0f, 0xa0,
  0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x0f, 0x80,
  0x00, 0x00, 0x00, 0xa0, 0x01, 0x00, 0x00, 0x02, 0x00, 0x08, 0x0f, 0x80,
  0x00, 0x00, 0xe4, 0x80, 0xff, 0xff, 0x00, 0x00
}};

Logger Logger::s_instance("triangle.log");

class TriangleApp {
  
public:
  
  TriangleApp(SDL_Window* window)
  : m_window(window) {
    if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &m_d3d)))
      throw DxvkError("Failed to create D3D9 interface");

    D3DPRESENT_PARAMETERS params;
    getPresentParams(params);

    if (FAILED(m_d3d->CreateDeviceEx(
                 D3DADAPTER_DEFAULT,
                 D3DDEVTYPE_HAL,
                 wsi::toHwnd(m_window),
                 D3DCREATE_HARDWARE_VERTEXPROCESSING,
                 &params,
                 nullptr,
                 &m_device)))
      throw DxvkError("Failed to create D3D9 device");

    if (FAILED(m_device->CreateVertexShader(reinterpret_cast<const DWORD*>(g_vertexShaderCode.data()), &m_vs)))
      throw DxvkError("Failed to create vertex shader");
    m_device->SetVertexShader(m_vs.ptr());

    if (FAILED(m_device->CreatePixelShader(reinterpret_cast<const DWORD*>(g_pixelShaderCode.data()), &m_ps)))
      throw DxvkError("Failed to create pixel shader");
    m_device->SetPixelShader(m_ps.ptr());

    std::array<float, 9> vertices = {
       0.0f,  0.5f, 0.0f,
       0.5f, -0.5f, 0.0f,
      -0.5f, -0.5f, 0.0f,
    };

    const size_t vbSize = vertices.size() * sizeof(float);

    if (FAILED(m_device->CreateVertexBuffer(vbSize, 0, 0, D3DPOOL_DEFAULT, &m_vb, nullptr)))
      throw DxvkError("Failed to create vertex buffer");

    void* data = nullptr;
    if (FAILED(m_vb->Lock(0, 0, &data, 0)))
      throw DxvkError("Failed to lock vertex buffer");

    std::memcpy(data, vertices.data(), vbSize);

    if (FAILED(m_vb->Unlock()))
      throw DxvkError("Failed to unlock vertex buffer");

    m_device->SetStreamSource(0, m_vb.ptr(), 0, 3 * sizeof(float));

    std::array<D3DVERTEXELEMENT9, 2> elements;

    elements[0].Method     = 0;
    elements[0].Offset     = 0;
    elements[0].Stream     = 0;
    elements[0].Type       = D3DDECLTYPE_FLOAT3;
    elements[0].Usage      = D3DDECLUSAGE_POSITION;
    elements[0].UsageIndex = 0;

    elements[1] = D3DDECL_END();

    if (FAILED(m_device->CreateVertexDeclaration(elements.data(), &m_decl)))
      throw DxvkError("Failed to create vertex decl");

    m_device->SetVertexDeclaration(m_decl.ptr());
  }
  
  void run() {
    this->adjustBackBuffer();

    m_device->BeginScene();

    m_device->Clear(
      0, nullptr,
      D3DCLEAR_TARGET,
      D3DCOLOR_RGBA(44, 62, 80, 0),
      0, 0);

    m_device->Clear(
      0, nullptr,
      D3DCLEAR_ZBUFFER,
      0, 0.5f, 0);

    m_device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

    m_device->EndScene();

    m_device->PresentEx(
      nullptr,nullptr,
      nullptr, nullptr,
      0);
  }
  
  void adjustBackBuffer() {
    int32_t w, h;
    SDL_GetWindowSize(m_window, &w, &h);

    uint32_t newWindowSizeW = uint32_t(w);
    uint32_t newWindowSizeH = uint32_t(h);

    if (m_windowSizeW != newWindowSizeW
     || m_windowSizeH != newWindowSizeH) {
      m_windowSizeW = newWindowSizeW;
      m_windowSizeH = newWindowSizeH;

      D3DPRESENT_PARAMETERS params;
      getPresentParams(params);
      HRESULT status = m_device->ResetEx(&params, nullptr);

      if (FAILED(status))
        throw DxvkError("Device reset failed");
    }
  }
  
  void getPresentParams(D3DPRESENT_PARAMETERS& params) {
    params.AutoDepthStencilFormat     = D3DFMT_UNKNOWN;
    params.BackBufferCount            = 1;
    params.BackBufferFormat           = D3DFMT_X8R8G8B8;
    params.BackBufferWidth            = m_windowSizeW;
    params.BackBufferHeight           = m_windowSizeH;
    params.EnableAutoDepthStencil     = 0;
    params.Flags                      = 0;
    params.FullScreen_RefreshRateInHz = 0;
    params.hDeviceWindow              = m_window;
    params.MultiSampleQuality         = 0;
    params.MultiSampleType            = D3DMULTISAMPLE_NONE;
    params.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;
    params.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
    params.Windowed                   = TRUE;
  }
    
private:
  
  SDL_Window*                      m_window;
  uint32_t                         m_windowSizeW = 1024;
  uint32_t                         m_windowSizeH = 600;
  
  Com<IDirect3D9Ex>                m_d3d;
  Com<IDirect3DDevice9Ex>          m_device;

  Com<IDirect3DVertexShader9>      m_vs;
  Com<IDirect3DPixelShader9>       m_ps;
  Com<IDirect3DVertexBuffer9>      m_vb;
  Com<IDirect3DVertexDeclaration9> m_decl;
  
};

int main(int argc, char** argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    std::cerr << "Failed to init SDL" << std::endl;
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
    "DXVK Native Triangle! - D3D9",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    1024, 600,
    SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::cerr << "Failed to create SDL window" << std::endl;
    return 1;
  }

  TriangleApp app(window);

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        default:
          break;
      }
    }

    app.run();
  }

  return 0;
}

