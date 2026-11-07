#include <inttypes.h>
#include "libcoopnet.h"
#include "coopnet.h"
#include "coopnet_id.h"
#include "pc/network/network.h"
#include "pc/network/version.h"
#include "pc/djui/djui_language.h"
#include "pc/djui/djui_popup.h"
#include "pc/mods/mods.h"
#include "pc/utils/misc.h"
#include "pc/debuglog.h"
#if defined(__XSDK__)
#include <unistd.h>
#include "pc/configfile.h"
#endif
#ifdef DISCORD_SDK
#include "pc/discord/discord.h"
#endif

#ifdef COOPNET

#define MAX_COOPNET_DESCRIPTION_LENGTH 1024

uint64_t gCoopNetDesiredLobby = 0;
char gCoopNetPassword[64] = "";
char sCoopNetDescription[MAX_COOPNET_DESCRIPTION_LENGTH] = "";

static uint64_t sLocalLobbyId = 0;
static uint64_t sLocalLobbyOwnerId = 0;
static enum NetworkType sNetworkType;
static bool sReconnecting = false;

static CoopNetRc coopnet_initialize(bool hosting);
#if defined(__XSDK__)
// The server the connection is open to: the official one, or the console's own when xbox_server_ip names one.
static char sConnectedIp[MAX_CONFIG_STRING] = "";
#endif

static const char* coopnet_game_name(const char* password) {
#if defined(__XSDK__)
    // Public official-service lobbies use the Android namespace; private servers
    // and password-protected lobbies retain the PC namespace used by this port.
    const char* ip = sConnectedIp[0] ? sConnectedIp : configCoopNetIp;
    if (password[0] == '\0' && strncmp(ip, "192.168.", 8) && strncmp(ip, "10.", 3) && strncmp(ip, "127.", 4)) { return "sm64coop-android"; }
#else
    (void)password;
#endif
    return GAME_NAME;
}

bool ns_coopnet_query(QueryCallbackPtr callback, QueryFinishCallbackPtr finishCallback, const char* password) {
    gCoopNetCallbacks.OnLobbyListGot = callback;
    gCoopNetCallbacks.OnLobbyListFinish = finishCallback;
    if (coopnet_initialize(false) != COOPNET_OK) { return false; }
    if (coopnet_lobby_list_get(coopnet_game_name(password), password) != COOPNET_OK) { return false; }
    return true;
}

static void coopnet_on_connected(uint64_t userId) {
    coopnet_set_local_user_id(userId);
}

static void coopnet_on_disconnected(bool intentional) {
    LOG_INFO("Coopnet shutdown!");
    if (!intentional) {
        djui_popup_create(DLANG(NOTIF, COOPNET_DISCONNECTED), 2);
    }
    coopnet_shutdown();
    gCoopNetCallbacks.OnLobbyListGot = NULL;
    gCoopNetCallbacks.OnLobbyListFinish = NULL;
}

static void coopnet_on_peer_disconnected(uint64_t peerId) {
    u8 localIndex = coopnet_user_id_to_local_index(peerId);
    if (localIndex != UNKNOWN_LOCAL_INDEX && gNetworkPlayers[localIndex].connected) {
        network_player_disconnected(gNetworkPlayers[localIndex].globalIndex);
    }
}

static void coopnet_on_load_balance(const char* host, uint32_t port) {
    if (host && strlen(host) > 0) {
        snprintf(configCoopNetIp, MAX_CONFIG_STRING, "%s", host);
    }
    configCoopNetPort = port;
    configfile_save(configfile_name());
}

static void coopnet_on_receive(uint64_t userId, const uint8_t* data, uint64_t dataLength) {
    coopnet_set_user_id(0, userId);
    u8 localIndex = coopnet_user_id_to_local_index(userId);
    network_receive(localIndex, &userId, (u8*)data, dataLength);
}

static void coopnet_on_lobby_joined(uint64_t lobbyId, uint64_t userId, uint64_t ownerId, uint64_t destId) {
    LOG_INFO("coopnet_on_lobby_joined!");
    coopnet_set_user_id(0, ownerId);
    sLocalLobbyId = lobbyId;
    sLocalLobbyOwnerId = ownerId;

    if (userId == coopnet_get_local_user_id()) {
        coopnet_clear_dest_ids();
        snprintf(configDestId, MAX_CONFIG_STRING, "%" PRIu64 "", destId);
    }

    coopnet_save_dest_id(userId, destId);

    if (userId == coopnet_get_local_user_id() && gNetworkType == NT_CLIENT) {
        network_send_mod_list_request();
    }
#ifdef DISCORD_SDK
    if (gDiscordInitialized) {
        discord_activity_update();
    }
#endif
}

static void coopnet_on_lobby_left(uint64_t lobbyId, uint64_t userId) {
    LOG_INFO("coopnet_on_lobby_left!");
    coopnet_clear_dest_id(userId);
    if (lobbyId == sLocalLobbyId && userId == coopnet_get_local_user_id()) {
        network_shutdown(false, false, true, false);
    }
}

static void coopnet_on_error(enum MPacketErrorNumber error, uint64_t tag) {
    LOG_ERROR("CoopNet error %d, tag %" PRIu64 "", error, tag);
    switch (error) {
        case MERR_COOPNET_VERSION:
            djui_popup_create(DLANG(NOTIF, COOPNET_VERSION), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_PEER_FAILED:
            {
                char built[256] = { 0 };
                u8 localIndex = coopnet_user_id_to_local_index(tag);
                char* name = DLANG(NOTIF, UNKNOWN);
                if (localIndex == 0) {
                    name = DLANG(NOTIF, LOBBY_HOST);
                } else if (localIndex != UNKNOWN_LOCAL_INDEX && gNetworkPlayers[localIndex].connected) {
                    name = gNetworkPlayers[localIndex].name;
                }
                djui_language_replace(DLANG(NOTIF, PEER_FAILED), built, 256, '@', name);
                djui_popup_create(built, 2);
            }
            break;
        case MERR_LOBBY_NOT_FOUND:
            djui_popup_create(DLANG(NOTIF, LOBBY_NOT_FOUND), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_LOBBY_JOIN_FULL:
            djui_popup_create(DLANG(NOTIF, DISCONNECT_FULL), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_LOBBY_JOIN_FAILED:
            djui_popup_create(DLANG(NOTIF, LOBBY_JOIN_FAILED), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_LOBBY_PASSWORD_INCORRECT:
            djui_popup_create(DLANG(NOTIF, LOBBY_PASSWORD_INCORRECT), 2);
            network_shutdown(false, false, false, false);
            break;
        case MERR_NONE:
        case MERR_MAX:
            break;
    }
}

static bool ns_coopnet_initialize(enum NetworkType networkType, bool reconnecting) {
    sNetworkType = networkType;
    sReconnecting = reconnecting;
    if (reconnecting) { return true; }
#if defined(__XSDK__)
    // It may be connected to the other server: coopnet_initialize switches.
    return coopnet_initialize(networkType == NT_SERVER) == COOPNET_OK;
#else
    return coopnet_is_connected()
        ? true
        : (coopnet_initialize(networkType == NT_SERVER) == COOPNET_OK);
#endif
}

static char* ns_coopnet_get_id_str(u8 localIndex) {
    static char id_str[32] = { 0 };
    if (localIndex == UNKNOWN_LOCAL_INDEX) {
        snprintf(id_str, 32, "???");
    } else {
        uint64_t userId = ns_coopnet_get_id(localIndex);
        uint64_t destId = coopnet_get_dest_id(userId);
        snprintf(id_str, 32, "%" PRIu64 "", destId);
    }
    return id_str;
}

static bool ns_coopnet_match_addr(void* addr1, void* addr2) {
    return !memcmp(addr1, addr2, sizeof(u64));
}

bool ns_coopnet_is_connected(void) {
    return coopnet_is_connected();
}

static void coopnet_populate_description(void) {
    char* buffer = sCoopNetDescription;
    int bufferLength = MAX_COOPNET_DESCRIPTION_LENGTH;
    // get version
    const char* version = get_version();
    int versionLength = strlen(version);
    snprintf(buffer, bufferLength, "%s", version);
    buffer += versionLength;
    bufferLength -= versionLength;

    // get mod strings
    if (gActiveMods.entryCount <= 0) { return; }
    char* strings[gActiveMods.entryCount];
    for (int i = 0; i < gActiveMods.entryCount; i++) {
        struct Mod* mod = gActiveMods.entries[i];
        strings[i] = mod->name;
    }

    // add seperator
    char* sep = "\n\nMods:\n";
    snprintf(buffer, bufferLength, "%s", sep);
    buffer += strlen(sep);
    bufferLength -= strlen(sep);

    // concat mod strings
    str_seperator_concat(buffer, bufferLength, strings, gActiveMods.entryCount, "\\#dcdcdc\\\n");
}

void ns_coopnet_update(void) {
    if (!coopnet_is_connected()) { return; }

    coopnet_update();
    if (gNetworkType != NT_NONE && sNetworkType != NT_NONE) {
        if (sNetworkType == NT_SERVER) {
            char mode[64] = "";
            mods_get_main_mod_name(mode, 64);
            if (sReconnecting) {
                LOG_INFO("Update lobby");
                coopnet_populate_description();
                coopnet_lobby_update(sLocalLobbyId, coopnet_game_name(gCoopNetPassword), get_version(), configPlayerName, mode, sCoopNetDescription);
            } else {
                LOG_INFO("Create lobby");
#if defined(__XSDK__)
                // CoopNet lists a lobby without a password for everyone: the console's are private unless asked otherwise.
                if (!configXboxPublicLobby && configPassword[0] == '\0') {
                    static const char letters[] = "abcdefghjkmnpqrstuvwxyz23456789";
                    u8 random[10];
                    if (getentropy(random, sizeof(random)) != 0) {
                        LOG_ERROR("No randomness for a lobby password!");
                        network_shutdown(false, false, false, false);
                        return;
                    }
                    for (u32 i = 0; i < sizeof(random); i++) { configPassword[i] = letters[random[i] % (sizeof(letters) - 1)]; }
                    configPassword[sizeof(random)] = '\0';
                    configfile_save(configfile_name());
                    char message[96];
                    snprintf(message, sizeof(message), "Partida privada, clave: %s", configPassword);
                    djui_popup_create(message, 2);
                }
#endif
#if defined(__XSDK__)
                snprintf(gCoopNetPassword, 64, "%s", configXboxPublicLobby ? "" : configPassword);
#else
                snprintf(gCoopNetPassword, 64, "%s", configPassword);
#endif
                coopnet_populate_description();
                coopnet_lobby_create(coopnet_game_name(gCoopNetPassword), get_version(), configPlayerName, mode, (uint16_t)configAmountOfPlayers, gCoopNetPassword, sCoopNetDescription);
            }
        } else if (sNetworkType == NT_CLIENT) {
            LOG_INFO("Join lobby");
            coopnet_lobby_join(gCoopNetDesiredLobby, gCoopNetPassword);
        }
        sNetworkType = NT_NONE;
    }
}

static int ns_coopnet_network_send(u8 localIndex, void* address, u8* data, u16 dataLength) {
    if (!coopnet_is_connected()) { return 1; }
    //if (gCurLobbyId == 0) { return 2; }
    u64 userId = coopnet_raw_get_id(localIndex);
    if (localIndex == 0 && address != NULL) { userId = *(u64*)address; }
    coopnet_send_to(userId, data, dataLength);

    return 0;
}

static bool coopnet_allow_invite(void) {
    if (sLocalLobbyId == 0) { return false; }
    return (sLocalLobbyOwnerId == coopnet_get_local_user_id()) || (strlen(gCoopNetPassword) == 0);
}

static void ns_coopnet_get_lobby_id(UNUSED char* destination, UNUSED u32 destLength) {
    if (sLocalLobbyId == 0) {
        snprintf(destination, destLength, "%s", "");
    } else {
        snprintf(destination, destLength, "coopnet:%" PRIu64 "", sLocalLobbyId);
    }
}

static void ns_coopnet_get_lobby_secret(UNUSED char* destination, UNUSED u32 destLength) {
    if (sLocalLobbyId == 0 || !coopnet_allow_invite()) {
        snprintf(destination, destLength, "%s", "");
    } else {
        snprintf(destination, destLength, "coopnet:%" PRIu64":%s", sLocalLobbyId, gCoopNetPassword);
    }
}

static void ns_coopnet_shutdown(bool reconnecting) {
    if (reconnecting) { return; }
    LOG_INFO("Coopnet shutdown!");
    coopnet_shutdown();
    gCoopNetCallbacks.OnLobbyListGot = NULL;
    gCoopNetCallbacks.OnLobbyListFinish = NULL;

    gCoopNetCallbacks.OnConnected = NULL;
    gCoopNetCallbacks.OnDisconnected = NULL;
    gCoopNetCallbacks.OnReceive = NULL;
    gCoopNetCallbacks.OnLobbyJoined = NULL;
    gCoopNetCallbacks.OnLobbyLeft = NULL;
    gCoopNetCallbacks.OnError = NULL;
    gCoopNetCallbacks.OnPeerDisconnected = NULL;
    gCoopNetCallbacks.OnLoadBalance = NULL;

    sLocalLobbyId = 0;
    sLocalLobbyOwnerId = 0;
}

static CoopNetRc coopnet_initialize(bool hosting) {
    gCoopNetCallbacks.OnConnected = coopnet_on_connected;
    gCoopNetCallbacks.OnDisconnected = coopnet_on_disconnected;
    gCoopNetCallbacks.OnReceive = coopnet_on_receive;
    gCoopNetCallbacks.OnLobbyJoined = coopnet_on_lobby_joined;
    gCoopNetCallbacks.OnLobbyLeft = coopnet_on_lobby_left;
    gCoopNetCallbacks.OnError = coopnet_on_error;
    gCoopNetCallbacks.OnPeerDisconnected = coopnet_on_peer_disconnected;
    gCoopNetCallbacks.OnLoadBalance = coopnet_on_load_balance;

#if defined(__XSDK__)
    // Joining always goes to the console's server when there is one; hosting, when asked to.
    const bool xboxServer = configXboxServerIp[0] != '\0' && (!hosting || configXboxHostOnXboxServer);
    const char* host = xboxServer ? configXboxServerIp : configCoopNetIp;
    if (coopnet_is_connected() && strcmp(sConnectedIp, host)) { coopnet_shutdown(); }
    if (coopnet_is_connected()) { return COOPNET_OK; }
    snprintf(sConnectedIp, sizeof(sConnectedIp), "%s", host);
#else
    (void)hosting;
    if (coopnet_is_connected()) { return COOPNET_OK; }
    const char* host = configCoopNetIp;
#endif

    char* endptr = NULL;
    uint64_t destId = strtoull(configDestId, &endptr, 10);

    CoopNetRc rc = coopnet_begin(host, configCoopNetPort, configPlayerName, destId);
    if (rc == COOPNET_FAILED) {
        djui_popup_create(DLANG(NOTIF, COOPNET_CONNECTION_FAILED), 2);
    }
    return rc;
}

struct NetworkSystem gNetworkSystemCoopNet = {
    .initialize       = ns_coopnet_initialize,
    .get_id           = ns_coopnet_get_id,
    .get_id_str       = ns_coopnet_get_id_str,
    .save_id          = ns_coopnet_save_id,
    .clear_id         = ns_coopnet_clear_id,
    .dup_addr         = ns_coopnet_dup_addr,
    .match_addr       = ns_coopnet_match_addr,
    .update           = ns_coopnet_update,
    .send             = ns_coopnet_network_send,
    .get_lobby_id     = ns_coopnet_get_lobby_id,
    .get_lobby_secret = ns_coopnet_get_lobby_secret,
    .shutdown         = ns_coopnet_shutdown,
    .requireServerBroadcast = false,
    .name             = "CoopNet",
};

#endif
