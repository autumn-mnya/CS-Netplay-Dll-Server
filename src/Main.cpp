#include <enet/enet.h>
#include <thread>
#include <string>
#include <iostream>
#include "Main.h"
#include "NetworkDefine.h"
#include "ByteStream.h"
#include <shlwapi.h>

#include <lua.hpp>
#include "lua/Lua.h"

char gModulePath[MAX_PATH];
char gScriptsPath[MAX_PATH];

ENetHost* host;
ENetAddress hostAddress;
CLIENT clients[MAX_CLIENTS];

int gNetVersion = 11;
int packetcode = 0;
int specialPacketCode = 0; // used for sending a custom string amount of data
char* specialData = "null";

void GetPaths()
{
	// Get executable's path
	GetModuleFileNameA(NULL, gModulePath, MAX_PATH);
	PathRemoveFileSpecA(gModulePath);

	// Get path of the data folder
	strcpy(gScriptsPath, gModulePath);
	strcat(gScriptsPath, "\\Scripts");
}

uint8_t* SendDeath()
{
	int actual_size = 8;

	uint8_t* l = new uint8_t[actual_size]();

	ByteStream bs(l, actual_size);

	bs.WriteLE32(gNetVersion);
	bs.WriteLE32(PACKETCODE_RECEIVE_DEATH);

	return l;
}

//Functions needed for getting ENetAddress from IP and port
static int ConvertIpToAddress(ENetAddress* address, const char* name)
{
	enet_uint8 vals[4] = { 0, 0, 0, 0 };
	for (int i = 0; i < 4; i++)
	{
		const char* next = name + 1;
		if (*name != '0')
		{
			long val = strtol(name, (char**)&next, 10);
			if (val < 0 || val > 0xFF || next == name || next - name > 3)
				return -1;
			vals[i] = (enet_uint8)val;
		}

		if (*next != (i < 3 ? '.' : '\0'))
			return -1;
		name = next + 1;
	}

	memcpy(&address->host, vals, sizeof(enet_uint32));
	address->host = address->host; //There was an endian swap where needed here
	return 0;
}

static bool VerifyPort(const char* port)
{
	for (int i = 0; port[i]; i++)
	{
		if (port[i] < '0' || port[i] > '9')
			return false;
	}

	return true;
}

//Start hosting a server
std::thread* ServerThread;
bool toEndThread = false;

// Runs whenever a packetcode is received that isnt in the switch statement, allowing for some amount of lua trickery

int ServerActNetScript()
{
	lua_getglobal(gL, "CaveNet");
	lua_getfield(gL, -1, "ActServer");

	if (lua_isnil(gL, -1))
	{
		lua_settop(gL, 0); // Clear stack
		return BOOLTRUE;
	}

	if (lua_pcall(gL, 0, 0, 0) != LUA_OK)
	{
		const char* error = lua_tostring(gL, -1);

		// ErrorLog(error, 0);
		printf("ERROR: %s\n", error);
		// MessageBoxA(ghWnd, "Couldn't execute game act function", "ModScript Error", MB_OK);
		return BOOLFALSE;
	}

	lua_settop(gL, 0); // Clear stack

	return BOOLTRUE;
}

void HandleServerEvent(ENetEvent event)
{
	switch (event.type)
	{
	case ENET_EVENT_TYPE_CONNECT:
		//Add peer to client-list
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].peer == NULL)
			{
				//Remove old data
				free(clients[i].skinData);
				memset(&clients[i], 0, sizeof(CLIENT));

				//Set peer
				clients[i].peer = (void*)event.peer;
				printf("User successfully connected,\nHost: %d\n", event.peer->address.host);
				break;
			}
			else if (i == MAX_CLIENTS - 1)
			{
				//Failed to connect
				printf("User attempted to connect,\nbut the server is full...\n(Host: %d)\n", event.peer->address.host);
				enet_peer_disconnect_now(event.peer, DISCONNECT_FORCE);
				break;
			}
		}

		break;

	case ENET_EVENT_TYPE_DISCONNECT:
		//Remove peer from client-list
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].peer == event.peer)
			{
				//Quit message if has a name
				if (strlen(clients[i].name) >= NAME_MIN)
				{
					char quitMsg[PACKET_DATA];
					sprintf(quitMsg, "%s has left", clients[i].name);
					BroadcastChatMessage(quitMsg);
				}

				//Remove data
				free(clients[i].skinData);
				memset(&clients[i], 0, sizeof(CLIENT));
				break;
			}
		}

		//Disconnected
		printf("User disconnected,\nHost: %d\n", event.peer->address.host);
		break;

	case ENET_EVENT_TYPE_RECEIVE:
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].peer == event.peer)
			{
				//Handle packet data
				ByteStream* packetData = new ByteStream(event.packet->data, event.packet->dataLength);
				ByteStream* repPacketData;

				char* wounceMsg = new char[event.packet->dataLength - 8];
				int netver;

				if ((netver = packetData->ReadLE32()) == gNetVersion)
				{
					packetcode = packetData->ReadLE32();
					switch (packetcode)
					{
						default:
						{
							// This runs lua code if its not one of the predefined packetcodes.
							// We must figure out a way, in here, to allow both normal ID sending, and custom amounts of data sending.
							// Thanks!
							ServerActNetScript(); 
							break;
						}
						case PACKETCODE_DEFINE_PLAYER:
							//Load name
							packetData->Read(clients[i].name, 1, MAX_NAME);
							clients[i].player_num = i;

							//Broadcast join message
							char joinMsg[PACKET_DATA];
							sprintf(joinMsg, "%s has joined", clients[i].name);
							BroadcastChatMessage(joinMsg);
							break;
						case PACKETCODE_CHAT_MESSAGE:
							packetData->Read(wounceMsg, 1, event.packet->dataLength - 8);
							BroadcastChatMessage(wounceMsg);
							break;
						case PACKETCODE_REPLICATE_PLAYER:
							//Bounce to other clients
							uint8_t packet[0x100];
							memset(packet, 0, 0x100);

							repPacketData = new ByteStream(packet, 0x100);

							repPacketData->WriteLE32(gNetVersion);
							repPacketData->WriteLE32(PACKETCODE_REPLICATE_PLAYER);

							//Set attributes
							repPacketData->WriteLE32(i);
							repPacketData->WriteLE32(packetData->ReadLE32());		//cond
							repPacketData->WriteLE32(packetData->ReadLE32());		//unit
							repPacketData->WriteLE32(packetData->ReadLE32());		//flag
							//TODO
							//if(HIDE_AND_SEEK)
							//   do something to overwrite the client name with 0's?
							repPacketData->Write(clients[i].name, 1, MAX_NAME);	//name
							repPacketData->WriteLE32(clients[i].player_num);       //client's player num
							repPacketData->WriteLE32(packetData->ReadLE32());		//x
							repPacketData->WriteLE32(packetData->ReadLE32());		//y
							repPacketData->WriteLE32(packetData->ReadLE32());		//xm
							repPacketData->WriteLE32(packetData->ReadLE32());		//ym
							repPacketData->WriteLE32(packetData->ReadLE32());		//up
							repPacketData->WriteLE32(packetData->ReadLE32());		//down
							repPacketData->WriteLE32(packetData->ReadLE32());		//arms
							repPacketData->WriteLE32(packetData->ReadLE32());		//equip
							repPacketData->WriteLE32(packetData->ReadLE32());		//ani_no
							repPacketData->WriteLE32(packetData->ReadLE32());		//hit.front
							repPacketData->WriteLE32(packetData->ReadLE32());		//hit.top
							repPacketData->WriteLE32(packetData->ReadLE32());		//hit.back
							repPacketData->WriteLE32(packetData->ReadLE32());		//hit.bottom
							repPacketData->WriteLE32(packetData->ReadLE32());		//direct
							repPacketData->WriteLE32(packetData->ReadLE32());		//shock
							repPacketData->WriteLE32(packetData->ReadLE32());		//life
							repPacketData->WriteLE32(packetData->ReadLE32());		//max life
							repPacketData->WriteLE32(packetData->ReadLE32());		//stage
							repPacketData->WriteLE32(packetData->ReadLE32());		//mim
							repPacketData->WriteLE32(packetData->ReadLE32());		//hide_vp_on_map / hide_me_on_map (bool)
							delete repPacketData;

							for (int v = 0; v < MAX_CLIENTS; v++)
							{
								if (v != i && clients[v].peer)
								{
									//Send packet
									ENetPacket* definePacket = enet_packet_create(packet, 0x100, 0);
									enet_peer_send((ENetPeer*)clients[v].peer, 0, definePacket);
								}
							}
							break;

						case PACKETCODE_RECEIVE_DEATH:
						{
							// Death-Link stuff
							printf("Received deathlink packet from %d\n", event.peer->address.host);
							uint8_t* packet2 = SendDeath();

							ENetPacket* definePacket = enet_packet_create(packet2, 8, ENET_PACKET_FLAG_RELIABLE);
							enet_host_broadcast(host, 0, definePacket);

							delete[] packet2;
							break;
						}

						case PACKETCODE_RECEIVE_CUSTOM_DATA: // Adjust this to match your packet code
						{
							// Read data size
							specialPacketCode = packetData->ReadLE32();
							uint32_t dataSize = packetData->ReadLE32();

							// Read variable-length data
							specialData = new char[dataSize];
							packetData->Read(specialData, 1, dataSize);

							printf("Received custom data from client %d: %s\n", i, specialData);

							// Run lua code here for the user to do stuff
							ServerActNetScript();

							// Clean up
							specialData = "null";
							break;
						}
						}
				}
				else
				{
					printf("User disconnected,\nReason: Invalid gNetVersion (%d)\nHost: %d\n", netver, event.peer->address.host);
					enet_peer_disconnect_now(event.peer, DISCONNECT_FORCE);
				}

				// We have to delete it, since we used new -Brayconn
				delete[] wounceMsg;
				delete packetData;
				break;
			}
		}

		//Finished with the packet
		enet_packet_destroy(event.packet);
		break;
	}
}

int HandleServerSynchronous(void* ptr)
{
	ENetEvent event;

	while (IsHosting() && !toEndThread)
	{
		int ret = enet_host_service(host, &event, 2000);

		if (ret < 0)
		{
			KillServer();
			return -1;
		}

		if (ret == 0)
			continue;

		HandleServerEvent(event);
	}

	return 0;
}

bool StartServer(const char* ip, const char* port)
{
	//Set IP address
	if (enet_address_set_host(&hostAddress, ip) < 0)
		return false;

	//Set port
	if (!VerifyPort(port))
		return false;
	hostAddress.port = std::stoi(std::string(port), nullptr, 10);

	//Create host
	if ((host = enet_host_create(&hostAddress, MAX_CLIENTS, 1, 0, 0)) == NULL)
		return false;

	//Start thread
	toEndThread = false;
	ServerThread = new std::thread(HandleServerSynchronous, nullptr);
	return true;
}

//Kill server
void KillServer()
{
	//End thread
	toEndThread = true;
	ServerThread->join();

	//Disconnect all clients
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients[i].peer != NULL)
		{
			enet_peer_disconnect_now((ENetPeer*)clients[i].peer, DISCONNECT_FORCE);
			clients[i].peer = NULL;
		}
	}
	//Kill host
	if (host)
		enet_host_destroy(host);
	host = NULL;
}

//Check if host still exists
bool IsHosting()
{
	return (host != NULL);
}

//Chat
void BroadcastChatMessage(const char* text)
{
	//Write packet data
	auto packetSize = 8 + (strlen(text) + 1);
	uint8_t* packet = new uint8_t[packetSize];

	ByteStream* packetData = new ByteStream(packet, packetSize);
	packetData->WriteLE32(gNetVersion);
	packetData->WriteLE32(PACKETCODE_CHAT_MESSAGE);
	packetData->Write(text, 1, strlen(text) + 1);
	delete packetData;

	//Send packet
	ENetPacket* definePacket = enet_packet_create(packet, packetSize, ENET_PACKET_FLAG_RELIABLE);
	enet_host_broadcast(host, 0, definePacket);
	delete[] packet;
}

#undef main

int main(int argc, char *argv[])
{
	// Get the path of the exe + scripts folder
	GetPaths();

	if (argc <= 2)
	{
		std::cout << "Freeware Online Command-line server\nHow to use:\nserver 'ip' 'port'\n";
		return -1;
	}
	else
	{
		//Init enet
		if (enet_initialize() < 0)
		{
			std::cout << "Failed to initialize ENet\n";
			return -1;
		}

		InitNetScript();

		// port "
		//Start server
		std::cout << "Trying to run server at " << argv[1] << ":" << argv[2] << std::endl;

		if (!StartServer(argv[1], argv[2]))
		{
			std::cout << "Failed to start server\n";
			return -1;
		}

		std::cout << "Success, type 'quit' to shutdown...\n";

		//Wait for q to be pressed
		while (true)
		{
			std::string command;
			std::cin >> command;

			if (command == "quit")
				break;
			else if(command == "send_tsc")
			{
				packetcode = 22;
				ServerActNetScript();
			}
		}

		KillServer();
		std::cout << "Shutdown server!\n";
	}

	enet_deinitialize();
	return 0;
}