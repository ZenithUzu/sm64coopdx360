#include <xtl.h>
#include <d3d9.h>

// Standalone hardware smoke test. No game code or ROM assets are linked.
void __cdecl main() {
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return;
    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 1280;
    pp.BackBufferHeight = 720;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    IDirect3DDevice9* device = NULL;
    HRESULT hr = d3d->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device);
    if (FAILED(hr)) { d3d->Release(); return; }
    for (;;) {
        XINPUT_STATE state = {};
        DWORD color = D3DCOLOR_XRGB(25, 55, 120);
        if (XInputGetState(0, &state) == ERROR_SUCCESS) {
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
                color = D3DCOLOR_XRGB(20, 160, 60);
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
                color = D3DCOLOR_XRGB(190, 35, 35);
        }
        device->Clear(0, NULL, D3DCLEAR_TARGET, color, 1.0f, 0);
        if (FAILED(device->Present(NULL, NULL, NULL, NULL))) break;
    }
    device->Release();
    d3d->Release();
}
