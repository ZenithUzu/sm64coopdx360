#include <cstdint>
#include <vector>
#include "xsdk/gpu.h"
#include "xsdk/kernel.h"
#include "xsdk/log.h"
#include "xsdk/present.h"

static_assert(sizeof(void*) == 4);
static_assert(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__);
int main() {
    xsdk_log_write("SM64CoopDX xSDK C++23 hardware test; A=green B=red Back=exit");
    const std::vector<std::uint32_t> colours{0xFF193778, 0xFF14A03C, 0xFFBE2323};
    xsdk_log_write("%s: C++ vector", colours.size() == 3 ? "PASS" : "FAIL");
    NTSTATUS status = xsdk_gpu_open();
    if (!NT_SUCCESS(status)) { xsdk_log_write("FAIL: gpu_open 0x%08X", (unsigned)status); return 1; }
    std::uint32_t width = 0, height = 0;
    status = xsdk_present_open(XSDK_DISPLAY_720P, 2, 1, &width, &height);
    if (!NT_SUCCESS(status)) { xsdk_log_write("FAIL: present_open 0x%08X", (unsigned)status); return 1; }
    xsdk_log_write("PASS: display opened %ux%u", (unsigned)width, (unsigned)height);
    unsigned last_buttons = ~0u;
    bool input_ok = false, frame_ok = false, saw_a = false, saw_b = false;
    int result = 0;
    for (;;) {
        INPUT_STATE input{};
        const auto input_status = XamInputGetState(0, 0, &input);
        if (input_status == ERROR_SUCCESS && !input_ok) { input_ok = true; xsdk_log_write("PASS: controller read"); }
        const unsigned buttons = input_status == ERROR_SUCCESS ? input.gamepad.buttons : 0;
        if (buttons != last_buttons) { xsdk_log_write("buttons: 0x%04X input_status: 0x%08X", buttons, (unsigned)input_status); last_buttons = buttons; }
        if ((buttons & INPUT_GAMEPAD_A) && !saw_a) { saw_a = true; xsdk_log_write("PASS: A detected"); }
        if ((buttons & INPUT_GAMEPAD_B) && !saw_b) { saw_b = true; xsdk_log_write("PASS: B detected"); }
        if (buttons & INPUT_GAMEPAD_BACK) { xsdk_log_write("exit-stage: Back requested"); break; }
        std::uint32_t address = 0;
        status = xsdk_present_begin(&address, 5000);
        if (!NT_SUCCESS(status)) { xsdk_log_write("FAIL: present_begin 0x%08X", (unsigned)status); result = 1; break; }
        const auto colour = colours[(buttons & INPUT_GAMEPAD_B) ? 2 : (buttons & INPUT_GAMEPAD_A) ? 1 : 0];
        if (!frame_ok) { // Resolve clears AFTER copying: discard the first copy before presenting.
            xsdk_gpu_resolve(address, width, height, colour);
        }
        xsdk_gpu_resolve(address, width, height, colour);
        xsdk_present_end();
        if (!frame_ok) { frame_ok = true; xsdk_log_write("PASS: first frame submitted (visual verification required)"); }
    }
    xsdk_log_write("exit-stage: before present_close");
    xsdk_present_close();
    xsdk_log_write("exit-stage: after present_close, before gpu_close");
    status = xsdk_gpu_close();
    if (!NT_SUCCESS(status)) { xsdk_log_write("FAIL: gpu_close 0x%08X", (unsigned)status); result = 1; }
    xsdk_log_write("exit-stage: after gpu_close 0x%08X", (unsigned)status);
    xsdk_log_write("observed: controller=%u frame=%u A=%u B=%u", input_ok, frame_ok, saw_a, saw_b);
    return result;
}

