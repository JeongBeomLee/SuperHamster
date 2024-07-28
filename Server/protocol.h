#include "stdafx.h"

constexpr int PORT = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

constexpr int MAX_USER = 2;

// Packet ID
constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;
constexpr char CS_LOGOUT = 2;
constexpr char CS_SHOOT = 3;

constexpr char SC_LOGIN_INFO = 2;
constexpr char SC_ADD_PLAYER = 3;
constexpr char SC_REMOVE_PLAYER = 4;
constexpr char SC_MOVE_PLAYER = 5;
constexpr char SC_ADD_BULLET = 6;
constexpr char SC_MOVE_BULLET = 7;
constexpr char SC_REMOVE_BULLET = 8;

#pragma pack (push, 1)
struct CS_LOGIN_PACKET {
	unsigned char size;
	char	type;
};

struct CS_MOVE_PACKET {
	unsigned char size;
	char	type;
	char	direction;  // 0 : UP, 1 : DOWN, 2 : LEFT, 3 : RIGHT, 4 : SPACE, 5 : Q, 6 : W, 7 : E, 8 : A, 9 : S
	char	prevDirection;
};

struct CS_LOGOUT_PACKET {
	unsigned char size;
	char	type;
};

struct CS_SHOOT_PACKET
{
	unsigned char size;
	char type;
};

struct SC_LOGIN_INFO_PACKET {
	unsigned char size;
	char	type;
	short	id;
	Vec3 	pos, dir, scale;
};

struct SC_ADD_PLAYER_PACKET {
	unsigned char size;
	char	type;
	short	id;
	Vec3 	pos, dir, scale;
	//char	name[NAME_SIZE];
};

struct SC_REMOVE_PLAYER_PACKET {
	unsigned char size;
	char	type;
	short	id;
};

struct SC_MOVE_PLAYER_PACKET {
	unsigned char size;
	char	type;
	short	id;
	Vec3 	pos, dir, scale;
	float	velocity;
	char	state;
	//unsigned int move_time;
};

struct SC_ADD_BULLET_PACKET
{
	unsigned char size;
	char type;
	int bulletId;
	Vec3 position;
	Vec3 direction;
};

struct SC_MOVE_BULLET_PACKET
{
	unsigned char size;
	char type;
	int bulletId;
	Vec3 position;
};

struct SC_REMOVE_BULLET_PACKET
{
	unsigned char size;
	char type;
	int bulletId;
};

#pragma pack (pop)