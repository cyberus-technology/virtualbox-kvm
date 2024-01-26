#pragma once

#include "windows_base.h"

DEFINE_GUID(IID_IUnknown, 0x00000000,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46); /* vbox: added ';' */
struct IUnknown {

public:

  virtual HRESULT QueryInterface(REFIID riid, void** ppvObject) = 0;

  virtual ULONG AddRef()  = 0;
  virtual ULONG Release() = 0;

};
DECLARE_UUIDOF_HELPER(IUnknown, 0x00000000,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46)

#define IID_PPV_ARGS(ppType) __uuidof(decltype(**(ppType))), [](auto** pp) { (void)static_cast<IUnknown*>(*pp); return reinterpret_cast<void**>(pp); }(ppType)
