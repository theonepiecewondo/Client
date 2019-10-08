#include<ncurses.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include "client.h"
char character_sender[32];
int first_join = 0;
void Game(struct game_description *game, struct receive_info *socket ){
	memset(game->description, 0, sizeof(game->description));
	read(socket->sockfd, &game->initial,2);
	read(socket->sockfd, &game->limit,2);
	read(socket->sockfd, &game->length,2);
	read(socket->sockfd, &game->description,game->length);
	if(game->length > 1){
        	wprintw(socket->top, "initial = %d\n", game->initial);
		wprintw(socket->top, "Stat limit = %d\n", game->limit);
		wprintw(socket->top, "length = %d\n", game->length);
		wprintw(socket->top, "description = %s\n", game->description);
	}
	memset(game->description, 0, sizeof(game->description));
}
void Error(struct error *e, struct receive_info *socket){
	read(socket->sockfd, &e->code, 1);
	read(socket->sockfd, &e->length, 2);
	read(socket->sockfd, &e->message, e->length);
	if(e->length > 1){
		wprintw(socket->top, "Error Code = %d\n", e->code);
		wprintw(socket->top, "Error message = %s\n", e->message);
	}
	memset(e->message, 0, sizeof(e->message));
}
void Message(struct message *m, struct receive_info *socket){
	CleanM(m);
	read(socket->sockfd, &m->length, 2);
        read(socket->sockfd, &m->recipient, 32);
        read(socket->sockfd, &m->sender, 32);
	read(socket->sockfd, &m->message, m->length);
	wprintw(socket->top, "Incoming message!!!\n");
	//wprintw(socket->top, "To: %s\n",m->recipient); You don't need to know that your the recipient of the message.... but it's there if you want to test it
	wprintw(socket->top, "From: %s\n",m->sender);
	wprintw(socket->top, "Message: %s\n",m->message);
	CleanM(m);
}
void Room(struct room *r, struct receive_info *socket){
	read(socket->sockfd, &r->number, 2);
	read(socket->sockfd, &r->name, 32);
	read(socket->sockfd, &r->length, 2);
	read(socket->sockfd, &r->description, r->length);
	if(r->length > 1){
		wprintw(socket->top, "Room Number = %d\n", r->number);
		wprintw(socket->top, "Room Name = %s\n", r->name);
		wprintw(socket->top, "Room Description = %s\n", r->description);
	}
	memset(r->name, 0, sizeof(r->name));
	memset(r->description, 0, sizeof(r->description));
}
void Character(struct character *c, struct receive_info *socket){
	char flags[32];
	read(socket->sockfd, &c->name,32);
	read(socket->sockfd, &c->flag,1);
	read(socket->sockfd, &c->attack,2);
	read(socket->sockfd, &c->defense,2);
	read(socket->sockfd, &c->regen,2);
	read(socket->sockfd, &c->health,2);
	read(socket->sockfd, &c->gold,2);
	read(socket->sockfd, &c->room_num,2);
	read(socket->sockfd, &c->length,2);
	read(socket->sockfd, &c->player_d,c->length);

	alive = 128 & c->flag;
	join = 64 & c->flag;
	monster = 32 & c->flag;
	started = 16 & c->flag;
	ready = 8 & c->flag;
	
	if(monster && alive){
		strcpy(flags,"(Monster Alive)");
	
	}
	else if(monster && !alive){
		strcpy(flags,"(Monster Dead)");
		c->health = 0;
	
	}
	else if (!monster && alive){
		strcpy(flags,"(Player Alive)");
	}
	else{
		strcpy(flags, "(Player Dead)");
		c->health = 0;
	}
	
	
	if(started){
		wprintw(socket->right, "Name: %s %s\n",c->name, flags);
		wprintw(socket->right, "Flag: %d \n",c->flag);
		wprintw(socket->right, "Attack: %d | ",c->attack);
		wprintw(socket->right, "Defense: %d | ",c->defense);
		wprintw(socket->right, "Regen: %d\n",c->regen);
		wprintw(socket->right, "Health: %d | ",c->health);
		wprintw(socket->right, "Gold: %d\n",c->gold);
		wprintw(socket->right, "%s\n",c->player_d);
	}
	CleanC(c);
}

void Accept(struct accept *a, struct receive_info *socket){
	read(socket->sockfd, &a->accept,2);
}
void Connection(struct connection *cn, struct receive_info *socket){
	read(socket->sockfd, &cn->number, 2);
	read(socket->sockfd, &cn->name, 32);
	read(socket->sockfd, &cn->length, 2);
	read(socket->sockfd, &cn->description, cn->length);
	
	if(cn->length > 1){
		wprintw(socket->top, "Room Number = %d (CONNECTION)\n", cn->number);
		wprintw(socket->top, "Room Name = %s\n", cn->name);
		wprintw(socket->top, "Room Description = %s\n", cn->description);
	}
	memset(cn->name, 0, sizeof(cn->name));
	memset(cn->description, 0, sizeof(cn->description));
}
void Write_Message(struct message *m, struct receive_info *socket, char *input){	
	wprintw(socket->bottom,"Enter a player to receive a message: ");
	wgetstr(socket->bottom,input);
	strcpy(m->recipient,input);
	wprintw(socket->bottom,"Enter a message: ");
	wgetstr(socket->bottom,input);
	strcpy(m->message,input);
	strcpy(m->sender,character_sender);
	m->length = strlen(m->message);
	write(socket->sockfd,&m->length,2);
	write(socket->sockfd,&m->recipient,32);
	write(socket->sockfd,&m->sender,32);
	write(socket->sockfd,&m->message,m->length);
}
void Create_Character(struct character *c, struct receive_info *socket, char *input){
		CleanC(c);
		wprintw(socket->bottom, "Enter a name for your character: ");
		wgetstr(socket->bottom,input);
		strcpy(c->name,input);

		wprintw(socket->bottom,"Enter your attack: ");
		wgetstr(socket->bottom,input);
		c->attack = atoi(input);

		wprintw(socket->bottom,"Enter your defense: ");
		wgetstr(socket->bottom,input);
		c->defense = atoi(input);

		wprintw(socket->bottom,"Enter your regen: ");
		wgetstr(socket->bottom,input);
		c->regen = atoi(input);

		wprintw(socket->bottom,"Enter a player description\n");
		wgetstr(socket->bottom,input);
		strcpy(c->player_d,input);
		c->length = strlen(c->player_d);

		strcpy(character_sender,c->name);

		write(socket->sockfd, &c->name, 32);
		write(socket->sockfd, &c->flag,1);
		write(socket->sockfd,&c->attack,2);
		write(socket->sockfd, &c->defense,2);
		write(socket->sockfd, &c->regen,2);
		write(socket->sockfd, &c->health,2);
		write(socket->sockfd, &c->gold,2);
		write(socket->sockfd, &c->room_num,2);
		write(socket->sockfd, &c->length,2);
		write(socket->sockfd, &c->player_d, c->length);
		first_join = 1;
}
void CleanC(struct character *c){
	memset(c->name, 0, sizeof(c->name));
	memset(c->player_d, 0, sizeof(c->player_d));
}
void CleanM(struct message *m){
	memset(m->recipient, 0, sizeof(m->recipient));
	memset(m->sender, 0, sizeof(m->sender));
	memset(m->message, 0, sizeof(m->message));
}
