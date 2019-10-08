#ifndef CLIENT_H
#define CLIENT_H
#include <ncurses.h>
int monster,alive,started,ready,join;
struct receive_info {
        int sockfd;
        WINDOW* top;
        WINDOW* bottom;
	WINDOW* right;
};

struct message{
	unsigned short length;
	char recipient[32]; 
	char sender[32];
	char message[1024*1024]; 
};

struct error{
	unsigned short code;
	unsigned short length;
	char message[1024*1024];

};

struct room{
	unsigned short number;
	char name[32];
	unsigned short length;
	char description[1024*1024];
};

struct character{
	char name[32];
	unsigned char flag;
	unsigned short attack;
	unsigned short defense;
	unsigned short regen;
	signed short health;
	unsigned short gold;
	unsigned short room_num;
	unsigned short length;
	char player_d[1024*1024];
};

struct game_description{
	unsigned short initial;
	unsigned short limit;
	unsigned short length;
	char description[1024*1024];
};
struct accept{
	unsigned short accept;
};

struct connection{
	unsigned short number;
	char name[32];
	unsigned short length;
	char description[1024*1024];
};
void Game(struct game_description *game, struct receive_info *socket);
void Error(struct error *e, struct receive_info *socket);
void Character(struct character *c, struct receive_info *socket);
void Message(struct message *m, struct receive_info *socket);
void Write_Message(struct message *m, struct receive_info *socket, char *input);
void Room(struct room *r, struct receive_info *socket);
void Create_Character(struct character *c, struct receive_info *socket, char *input);
void Accept(struct accept *a, struct receive_info *socket);
void Connection(struct connection *cn, struct receive_info *socket);
void CleanC(struct character *c);
void CleanM(struct message *m);
#endif

