#include "Lua.h"
#include "Lua_ENet.h"
#include <lua.hpp>
#include "../Main.h"
#include <enet/enet.h>
#include "../ByteStream.h"

static int SendPacketID(lua_State* L)
{
    int packetCode = (int)luaL_checknumber(L, 1);
    bool excludeSender = lua_toboolean(L, 2);


    uint8_t packet[8];
    ByteStream packetData(packet, 8);

    packetData.WriteLE32(gNetVersion);
    packetData.WriteLE32(packetCode);

    // Determine whether to broadcast the packet or send it to a specific peer
    bool toPeer = false; // Default value

    // Check if the optional third argument is provided
    if (lua_gettop(L) >= 3)
    {
        // Read the optional parameter
        toPeer = lua_toboolean(L, 3);
    }

    auto definePacket = enet_packet_create(packet, 8, ENET_PACKET_FLAG_RELIABLE);
    if (toPeer)
    {
        enet_peer_send(lua_peer, 0, definePacket);
    }
    else
    {
        if (excludeSender)
        {
            for (int v = 0; v < MAX_CLIENTS; v++)
            {
                if (v != lua_ClientNum && clients[v].peer)
                {
                    //Send packet
                    enet_peer_send((ENetPeer*)clients[v].peer, 0, definePacket);
                }
            }
        }
        else
            enet_host_broadcast(host, 0, definePacket);
    }

    return 0;
}

// Function to send a packet with variable-length data
static int SendPacketCustom(lua_State* L)
{
    int mainCode = 4;
    int packetCode = (int)luaL_checknumber(L, 1);

    // Create a dynamic buffer to hold the packet data
    size_t dataSize = 0;
    const char* packetData = luaL_checklstring(L, 2, &dataSize);

    // Calculate the total size of the packet (packet code + size + data)
    size_t packetSize = sizeof(gNetVersion) + sizeof(mainCode) + sizeof(packetCode) + sizeof(uint32_t) + dataSize;

    // Allocate memory for the packet
    uint8_t* packet = new uint8_t[packetSize];
    ByteStream packetDataStream(packet, packetSize);

    // Write packet version, packet code, and data size
    packetDataStream.WriteLE32(gNetVersion);
    packetDataStream.WriteLE32(mainCode);
    packetDataStream.WriteLE32(packetCode);
    packetDataStream.WriteLE32(static_cast<uint32_t>(dataSize));

    // Write the actual data
    packetDataStream.Write(packetData, 1, dataSize);

    // "Exclude Sender" argument.
    bool excludeSender = lua_toboolean(L, 3);

    // Determine whether to broadcast the packet or send it to a specific peer
    bool toPeer = false; // Default value

    // Check if the optional fourthc argument is provided
    if (lua_gettop(L) >= 4)
    {
        // Read the optional parameter
        toPeer = lua_toboolean(L, 4);
    }

    // Send packet
    auto definePacket = enet_packet_create(packet, packetSize, ENET_PACKET_FLAG_RELIABLE);
    if (toPeer)
    {
        enet_peer_send(lua_peer, 0, definePacket);
    }
    else
    {
        if (excludeSender)
        {
            for (int i = 0; i < MAX_CLIENTS; ++i)
            {
                if (i != lua_ClientNum) // Exclude the sender
                {
                    enet_peer_send((ENetPeer*)clients[i].peer, 0, definePacket); // send to every client that wasnt the original sender
                }
            }
        }
        else
            enet_host_broadcast(host, 0, definePacket);
    }

    return 0;
}

// Function table for ENet functions
FUNCTION_TABLE EnetFunctionTable[FUNCTION_TABLE_ENET_SIZE] = {
    {"sendPacketID", SendPacketID},
    {"sendSpecialPacket", SendPacketCustom}
};