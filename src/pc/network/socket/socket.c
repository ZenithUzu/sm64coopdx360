#include "socket.h"
#include <stdio.h>
#include "pc/configfile.h"
#include "pc/debuglog.h"
#include "pc/djui/djui.h"

#if defined(__XSDK__)
typedef struct sockaddr_in CoopSocketAddress;
#define COOP_SOCKET_AF AF_INET
#define COOP_SOCKET_IP sin_addr
#define COOP_SOCKET_FAMILY sin_family
#define COOP_SOCKET_PORT sin_port
#define COOP_SOCKET_TEXT_LENGTH INET_ADDRSTRLEN
#define COOP_SOCKET_ANY ((struct in_addr){ .s_addr = INADDR_ANY })
#else
typedef struct sockaddr_in6 CoopSocketAddress;
#define COOP_SOCKET_AF AF_INET6
#define COOP_SOCKET_IP sin6_addr
#define COOP_SOCKET_FAMILY sin6_family
#define COOP_SOCKET_PORT sin6_port
#define COOP_SOCKET_TEXT_LENGTH INET6_ADDRSTRLEN
#define COOP_SOCKET_ANY in6addr_any
#endif
static SOCKET sCurSocket = INVALID_SOCKET;
static CoopSocketAddress sAddr[MAX_PLAYERS] = { 0 };
struct addrinfo hints;
struct addrinfo *result, *i;

char gGetHostName[MAX_CONFIG_STRING] = "";

// Resolves a hostname to an IP address. Current limitation: It still only gets the first address it sees and returns.
// getaddrinfo() is smart enough to prioritize IPv4 if the user is not in an IPv6 enabled network, so this shouldn't be a problem for now.
// TODO: Store all found addresses somewhere and make the game try to connect to each of them if one fails.
#if defined(__XSDK__)
static bool resolve_domain(CoopSocketAddress *addr) {
    struct addrinfo hint = {0}, *addresses = NULL;
    hint.ai_family = AF_INET; hint.ai_socktype = SOCK_DGRAM;
    int error = getaddrinfo(configJoinIp, NULL, &hint, &addresses);
    if (error || !addresses) { LOG_ERROR("IPv4 DNS lookup failed: %d", error); return false; }
    addr->sin_addr = ((struct sockaddr_in *)addresses->ai_addr)->sin_addr;
    freeaddrinfo(addresses);
    return true;
}
#else
void resolve_domain(CoopSocketAddress *addr) {
    // non zero value if getaddrinfo throws an error.
    int error;

    // set hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;

    // sanity check: remove square brackets from configJoinIp. getaddrinfo doesn't like those, at least on Linux.
    if (configJoinIp[0] == '[') {
        LOG_INFO("sanity check: found opening square bracket on configJoinIp, removing it.");
        for (int i = 0; i < MAX_CONFIG_STRING; i++) {
            if (configJoinIp[i] == '\0') { break; }
            if (configJoinIp[i] == ']') {
                configJoinIp[i] = '\0';
                memcpy(&configJoinIp, &configJoinIp[1], MAX_CONFIG_STRING-1);
                break;
            }
        }
    }

    // Get host addresses
    error = getaddrinfo(configJoinIp, NULL, &hints, &result);

    // If it was successful:
    if (error == 0) {
        // Iterate through results
        for (i = result; i != NULL; i = i->ai_next) {
            // buffer for IPv6 address
            char str[COOP_SOCKET_TEXT_LENGTH];
            // IPv6 address:
            if (i->ai_addr->sa_family == COOP_SOCKET_AF) {
                CoopSocketAddress *p = (CoopSocketAddress *)i->ai_addr;
                // copy address to sockaddr_in6 struct
                memcpy(&addr->COOP_SOCKET_IP, &p->COOP_SOCKET_IP, sizeof(struct in6_addr));
                // set new join IP for config file
                snprintf(configJoinIp, MAX_CONFIG_STRING, "%s", inet_ntop(COOP_SOCKET_AF, &p->COOP_SOCKET_IP, str, sizeof(str)));
                // Free results from memory and return
                freeaddrinfo(result);
                return;

            } else if (i->ai_addr->sa_family == AF_INET) { // IPv4 address. Convert it to an IPv6-mapped IPv4 address so it's compatible with the IPv6 socket.
                struct sockaddr_in *p = (struct sockaddr_in *)i->ai_addr;
                struct in6_addr ipv6_mapped_addr;
                // clear out IPv6-mapped IPv4 address buffer
                memset(&ipv6_mapped_addr, 0, sizeof(struct in6_addr));
                // ::ffff: Prefix
                ipv6_mapped_addr.s6_addr[10] = 0xff;
                ipv6_mapped_addr.s6_addr[11] = 0xff;
                // then copy the IPv4 address to the end of the IPv6 address. The address is now properly formed.
                memcpy(&ipv6_mapped_addr.s6_addr[12], &p->sin_addr, sizeof(p->sin_addr));
                // copy address to sockaddr_in6 struct
                memcpy(&addr->COOP_SOCKET_IP, &ipv6_mapped_addr, sizeof(struct in6_addr));

                // set new join IP for config file
                snprintf(configJoinIp, MAX_CONFIG_STRING, "%s", inet_ntop(COOP_SOCKET_AF, &ipv6_mapped_addr, str, sizeof(str)));
                // Free results from memory and return
                freeaddrinfo(result);
                return;
            }
        }
    } else {
        LOG_ERROR("getaddrinfo() failed with error code %i: %s", error, gai_strerror(error));
    }
}

#endif

static int socket_bind(SOCKET socket, unsigned int port) {
    CoopSocketAddress rxAddr;
    // Clean struct to prevent rare cases of binding errors due to garbage data in that memory location. This just happened to me randomly on Windows and left me very confused.
    memset(&rxAddr, 0, sizeof(CoopSocketAddress));
    rxAddr.COOP_SOCKET_FAMILY = COOP_SOCKET_AF;
    rxAddr.COOP_SOCKET_PORT = htons(port);
    rxAddr.COOP_SOCKET_IP = COOP_SOCKET_ANY;

    int rc = bind(socket, (SOCKADDR *)&rxAddr, sizeof(rxAddr));

    if (rc != 0) {
        LOG_ERROR("bind failed with error %d", SOCKET_LAST_ERROR);
    }

    return rc;
}

static int socket_send(SOCKET socket, CoopSocketAddress* addr, u8* buffer, u16 bufferLength) {
    int addrSize = sizeof(CoopSocketAddress);
    int rc = sendto(socket, (char*)buffer, bufferLength, 0, (struct sockaddr*)addr, addrSize);
    if (rc != SOCKET_ERROR) { return NO_ERROR; }

    int error = SOCKET_LAST_ERROR;
    if (error == SOCKET_EWOULDBLOCK) { return NO_ERROR; }

    LOG_ERROR("sendto failed with error: %d", error);
    return rc;
}

static int socket_receive(SOCKET socket, CoopSocketAddress* rxAddr, u8* buffer, u16 bufferLength, u16* receiveLength, u8* localIndex) {
    *receiveLength = 0;

    RX_ADDR_SIZE_TYPE rxAddrSize = sizeof(CoopSocketAddress);
    int rc = recvfrom(socket, (char*)buffer, bufferLength, 0, (struct sockaddr*)rxAddr, &rxAddrSize);

    for (int i = 1; i < MAX_PLAYERS; i++) {
        if (memcmp(rxAddr, &sAddr[i], sizeof(CoopSocketAddress)) == 0) {
            *localIndex = i;
            break;
        }
    }

    if (rc == SOCKET_ERROR) {
        int error = SOCKET_LAST_ERROR;
        if (error != SOCKET_EWOULDBLOCK && error != SOCKET_ECONNRESET) {
            LOG_ERROR("recvfrom failed with error %d", SOCKET_LAST_ERROR);
        }
        return SOCKET_ERROR;
    }

    *receiveLength = rc;
    return NO_ERROR;
}

static bool ns_socket_initialize(enum NetworkType networkType, UNUSED bool reconnecting) {
    // sanity check port
    unsigned int port = (networkType == NT_CLIENT) ? configJoinPort : configHostPort;
    if (port == 0) { port = DEFAULT_PORT; }

    // create a receiver socket to receive datagrams
    sCurSocket = socket_initialize();
    if (sCurSocket == INVALID_SOCKET) { return false; }

    // connect
    if (networkType == NT_SERVER) {
        int reuse = 1;
        if (setsockopt(sCurSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
            LOG_ERROR("setsockopt(SO_REUSEADDR) failed");
        }

#ifdef SO_REUSEPORT
        if (setsockopt(sCurSocket, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) {
            LOG_ERROR("setsockopt(SO_REUSEPORT) failed");
        }
#endif
        // bind the socket to any address and the specified port.
        int rc = socket_bind(sCurSocket, port);
        if (rc != NO_ERROR) {
            LOG_ERROR("bind returned an error.");
            return false;
        }
        LOG_INFO("bound to port %u", port);
    } else if (networkType == NT_CLIENT) {
        CoopSocketAddress addr;
        // set and clean struct to prevent garbage data
        memset(&addr, 0, sizeof(CoopSocketAddress));
        // save the port to send to
        sAddr[0].COOP_SOCKET_FAMILY = COOP_SOCKET_AF;
        sAddr[0].COOP_SOCKET_PORT = htons(port);
        // resolve and get address list to connect
#if defined(__XSDK__)
        if (!resolve_domain(&addr)) { socket_shutdown(sCurSocket); sCurSocket = INVALID_SOCKET; return false; }
#else
        resolve_domain(&addr);
#endif
        sAddr[0].COOP_SOCKET_IP = addr.COOP_SOCKET_IP;
        LOG_INFO("connecting to %s, port %u", configJoinIp, port);
        // copy hostname to be saved to config file
        snprintf(configJoinIp, MAX_CONFIG_STRING, "%s", gGetHostName);

        // kick off first packet
        char joinText[128] = { 0 };
        snprintf(joinText, 63, "%s %d", configJoinIp, configJoinPort);
        djui_connect_menu_open();

        gNetworkType = NT_CLIENT;
    }

    LOG_INFO("initialized");

    if (networkType == NT_CLIENT) {
        network_send_mod_list_request();
    }

    // success
    return true;
}

static s64 ns_socket_get_id(UNUSED u8 localId) {
    return 0;
}

static char* ns_socket_get_id_str(u8 localId) {
    if (localId == UNKNOWN_LOCAL_INDEX) { localId = 0; }
    static char id_str[COOP_SOCKET_TEXT_LENGTH] = { 0 };
    inet_ntop(COOP_SOCKET_AF, &sAddr[localId].COOP_SOCKET_IP, id_str, sizeof(id_str));
    return id_str;
}

static void ns_socket_save_id(u8 localId, UNUSED s64 networkId) {
    SOFT_ASSERT(localId > 0);
    SOFT_ASSERT(localId < MAX_PLAYERS);
    sAddr[localId] = sAddr[0];
    LOG_INFO("saved addr for id %d", localId);
}

static void ns_socket_clear_id(u8 localId) {
    if (localId == 0) { return; }
    SOFT_ASSERT(localId < MAX_PLAYERS);
    memset(&sAddr[localId], 0, sizeof(CoopSocketAddress));
    LOG_INFO("cleared addr for id %d", localId);
}

static void* ns_socket_dup_addr(u8 localIndex) {
    void* address = malloc(sizeof(CoopSocketAddress));
    memcpy(address, &sAddr[localIndex], sizeof(CoopSocketAddress));
    return address;
}

static bool ns_socket_match_addr(void* addr1, void* addr2) {
    return !memcmp(addr1, addr2, sizeof(CoopSocketAddress));
}

static void ns_socket_update(void) {
    if (gNetworkType == NT_NONE) { return; }
    do {
        // receive packet
        u8 data[PACKET_LENGTH + 1];
        u16 dataLength = 0;
        u8 localIndex = UNKNOWN_LOCAL_INDEX;
        int rc = socket_receive(sCurSocket, &sAddr[0], data, PACKET_LENGTH + 1, &dataLength, &localIndex);
        SOFT_ASSERT(dataLength < PACKET_LENGTH);
        if (rc != NO_ERROR) { break; }
        network_receive(localIndex, &sAddr[0], data, dataLength);
    } while (true);
}

static int ns_socket_send(u8 localIndex, void* address, u8* data, u16 dataLength) {
    if (localIndex != 0) {
        if (gNetworkType == NT_SERVER && gNetworkPlayers[localIndex].type != NPT_CLIENT) { return SOCKET_ERROR; }
        if (gNetworkType == NT_CLIENT && gNetworkPlayers[localIndex].type != NPT_SERVER) { return SOCKET_ERROR; }
    }

    CoopSocketAddress* userAddr = &sAddr[localIndex];
    if (localIndex == 0 && address != NULL) { userAddr = (CoopSocketAddress*)address; }

    int rc = socket_send(sCurSocket, userAddr, data, dataLength);
    if (rc) {
        LOG_ERROR("    localIndex: %d, packetType: %d, dataLength: %d", localIndex, data[0], dataLength);
    }
    return rc;
}

static void ns_socket_get_lobby_id(char* destination, u32 destLength) {
    snprintf(destination, destLength, "%s", ""); // TODO: we can probably hook this up
}

static void ns_socket_get_lobby_secret(char* destination, u32 destLength) {
    snprintf(destination, destLength, "%s", ""); // TODO: we can probably hook this up
}

static void ns_socket_shutdown(UNUSED bool reconnecting) {
    socket_shutdown(sCurSocket);
    sCurSocket = INVALID_SOCKET;
    for (u16 i = 0; i < MAX_PLAYERS; i++) {
        memset(&sAddr[i], 0, sizeof(CoopSocketAddress));
    }
    LOG_INFO("shutdown");
}

struct NetworkSystem gNetworkSystemSocket = {
    .initialize       = ns_socket_initialize,
    .get_id           = ns_socket_get_id,
    .get_id_str       = ns_socket_get_id_str,
    .save_id          = ns_socket_save_id,
    .clear_id         = ns_socket_clear_id,
    .dup_addr         = ns_socket_dup_addr,
    .match_addr       = ns_socket_match_addr,
    .update           = ns_socket_update,
    .send             = ns_socket_send,
    .get_lobby_id     = ns_socket_get_lobby_id,
    .get_lobby_secret = ns_socket_get_lobby_secret,
    .shutdown         = ns_socket_shutdown,
    .requireServerBroadcast = true,
    .name             = "Socket",
};
