#ifndef DYNOS_CMD_ORDER_H
#define DYNOS_CMD_ORDER_H

#include <stddef.h>
#include <stdint.h>
#include "pc/network/packets/packet_wire.h"

// command_macros_base.h packs level and geo commands differently per byte order, and binaries come from little-endian
// builds. Each command word converts by its kind:
//   B = CMD_BBH, Q = CMD_BBBB, H = CMD_HH; P and W (CMD_PTR, CMD_W) are plain values.
static const char *const sDynosLevelCmdKinds[0x45] = {
    "BPPP", "BPPP", "B", "B", "B", "BP", "BP", "B", "B", "B", "B", "QW", "QWP", "QWP", "QW", "B",
    "B", "BP", "BP", "B", "B", "B", "BPPP", "BPP", "BPP", "B", "BPP", "B", "B", "B", "B", "QP",
    "B", "BP", "BP", NULL, "QHHHWP", "QWP", "QQ", "QQ", "QHH", "Q", "Q", "QHH", "B", "B", "BP", "BP",
    "Q", "B", "B", "QQ", "Q", "Q", "BH", "B", "B", "BP", "BHH", "QHH", "Q", "B", "B", "QHHHWP",
    "QHHHWPP", "BP", "QWP", "BPPPPPPPPPP", "QPP",
};

// The layer byte's flags choose a geo command's length: see geo_commands.h.
static inline const char *dynos_geo_cmd_kinds(uint8_t op, uint8_t param) {
    const int dl = (param & 0x80) != 0;
    switch (op) {
        case 0x00: return "BP";
        case 0x02: return "BP";
        case 0x08: return "BHH";
        case 0x0A: return param ? "BHP" : "BH";
        case 0x0D: return "BH";
        case 0x0E: return "BP";
        case 0x0F: return "BHHHP";
        case 0x10:
            switch (param & 0x70) {
                case 0x00: return dl ? "BHHHP" : "BHHH";
                case 0x10:
                case 0x20: return dl ? "BHP" : "BH";
                default:   return dl ? "BP" : "B";
            }
        case 0x11:
        case 0x12:
        case 0x14: return dl ? "BHP" : "BH";
        case 0x13: return "BHP";
        case 0x15: return "BP";
        case 0x16: return "BH";
        case 0x18:
        case 0x19: return "BP";
        case 0x1A: return "BH";
        case 0x1C: return "BHP";
        case 0x1D:
            if (param & 0x40) { return dl ? "BWWWP" : "BWWW"; }
            return dl ? "BWP" : "BW";
        case 0x1E: return "BH";
        case 0x1F: return "BHHH";
        case 0x21: return "BPP";
        case 0x22:
        case 0x23: return "BW";
        case 0x24: return dl ? "BHHHWWWP" : "BHHHP";
        default:   return "B";
    }
}

struct DynosCmdOrder {
    const char *next;
};

// Takes a command word as a little-endian build holds it and gives it as this build's macros would pack it. Call it for
// every word in order, pointer codes included, as they keep the place within the command.
static inline uint32_t dynos_cmd_order_word(struct DynosCmdOrder *order, uint32_t value, int level) {
    if (!order->next || !*order->next) {
        const uint8_t op = (uint8_t)value;
        const uint8_t param = (uint8_t)(value >> 8);
        order->next = level ? (op < 0x45 && sDynosLevelCmdKinds[op] ? sDynosLevelCmdKinds[op] : "B")
                            : dynos_geo_cmd_kinds(op, param);
    }
    const char kind = *order->next++;
    if (!PACKET_WIRE_SWAP) { return value; }
    switch (kind) {
        case 'B': return (value & 0xFF) << 24 | (value & 0xFF00) << 8 | value >> 16;
        case 'Q': return (value & 0xFF) << 24 | (value & 0xFF00) << 8 | (value & 0xFF0000) >> 8 | value >> 24;
        case 'H': return value << 16 | value >> 16;
        default:  return value;
    }
}

#endif
