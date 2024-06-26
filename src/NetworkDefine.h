#pragma once
#include <stdint.h>
#include <chrono>

#define MAX_NAME 0x14
#define NAME_MIN 2

const std::chrono::milliseconds TICKRATE = std::chrono::milliseconds(60);
#define MAX_CLIENTS 0x80
const std::chrono::milliseconds VIRTUAL_PLAYER_TIMEOUT = std::chrono::milliseconds(20000);

#define PACKET_DATA 0x1000

#define CHAT_OFF_Y 32
#define CHAT_LINE_HEIGHT 7

enum CONNECT_CODES
{
	CONNECT_NORMAL,
};

enum DISCONNECT_CODES
{
	DISCONNECT_FORCE,
};

enum PACKET_CODES
{
	PACKETCODE_DEFINE_PLAYER = 0,
	PACKETCODE_CHAT_MESSAGE = 1,
	PACKETCODE_REPLICATE_PLAYER = 2,
	PACKETCODE_RECEIVE_DEATH = 3, // death link
	PACKETCODE_RECEIVE_CUSTOM_DATA = 4, // custom data for use with lua
};

struct VIRTUAL_PLAYER
{
	int cond;
	int unit;
	int flag;
	char name[MAX_NAME];
	std::chrono::high_resolution_clock::time_point timeout;

	//Position
	int x;
	int y;
	int xm;
	int ym;
	int lerpX;
	int lerpY;
	std::chrono::high_resolution_clock::duration lerpTick;

	//Visual
	int up;
	int down;
	int arms;
	int equip;
	int ani_no;
	RECT hit;
	int direct;
	int shock;
	int shockT;
	int bubble;
	short life;
	short max_life;
	int stage;
	unsigned int mim;
	bool hide_vp_on_map;
	int player_num;
	bool shooting;
	int soft_rensha;
};

struct PACKET
{
	uint32_t netVersion;
	uint32_t code;
	uint8_t data[PACKET_DATA];
};

struct CLIENT
{
	void* peer;
	char name[MAX_NAME];
	uint8_t* skinData;
	uint32_t skinSize;
	int player_num;
};
