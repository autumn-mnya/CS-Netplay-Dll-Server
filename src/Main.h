#pragma once

#include <enet/enet.h>
#include "NetworkDefine.h"

#define BOOLFALSE 0
#define BOOLTRUE 1

extern char gModulePath[MAX_PATH];
extern char gScriptsPath[MAX_PATH];

extern ENetHost* host;
extern ENetAddress hostAddress;
extern CLIENT clients[MAX_CLIENTS];

extern int gNetVersion;
extern int packetcode;
extern int specialPacketCode;
extern char* specialData;

int HandleServerSynchronous(void *ptr);
bool StartServer(const char* ip, const char *port);
void KillServer();
bool IsHosting();
void BroadcastChatMessage(const char *text);
