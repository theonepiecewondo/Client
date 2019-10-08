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
/*
	Compiled with gcc client.c client.h clientCommands.c -lncurses -lpthread
	Known bugs / issues
	1.Screen resizing as you know it isn't a thing (in this client anyways...)
	  This is painfully apparent on the right screen for a small terminal window.
	2.Message when characters are being created are repeated twice. Is this the server sending the type twice for some reason?
	3.Creating a character and dying doesn't allow you to create a new character
	  My idea was to prevent, you from creating a character while alive but the flag values aren't passing after death to my variables
	4.Most likely more.
*/
unsigned short type;
struct game_description desc;
struct character c;
struct error e;
struct message m;
struct room r;
struct accept a;
struct connection cn;
void exit_graceful(int signal){
	endwin();
	exit(0);
}

void* receive_print(void* arg){
	struct receive_info* prfi = (struct receive_info*)arg;
	char readstring[1024*1024];
	ssize_t readsize;
	for(;;){
		read(prfi->sockfd, &type, 1);
		if(type == 13){
			wrefresh(prfi->top);
			wprintw(prfi->top, "\n");
			Connection(&cn, prfi);
			wrefresh(prfi->top);
		}		
		
		else if(type == 11){
			wrefresh(prfi->top);
			Game(&desc, prfi);
			wrefresh(prfi->top);
		}
		else if (type == 10){
			CleanC(&c);
                        wrefresh(prfi->right);
                        Character(&c, prfi);
			wprintw(prfi->right,"\n");
                        wrefresh(prfi->right);
                }
		else if (type == 9){
			wrefresh(prfi->top);
			wprintw(prfi->top, "\n");
                        Room(&r, prfi);
			wrefresh(prfi->top);
		}
		else if(type == 8){
			wrefresh(prfi->top);
                        Accept(&a, prfi);
			wrefresh(prfi->top);
                }
		else if (type == 7){
			wrefresh(prfi->top);
			wprintw(prfi->top, "\n");
			Error(&e, prfi);
			wrefresh(prfi->top);
		}
		else if (type == 2){
			
		}
		else if (type == 1){
			wrefresh(prfi->top);
			wprintw(prfi->top, "\n");
                        Message(&m, prfi);
			wrefresh(prfi->top);
		}
		//wprintw(prfi->top, "Character = %s\n",c.name);
		/*readsize = read(prfi->sockfd, &readstring, 1024*1024);
		if(!readsize){
			wattron(prfi->top, COLOR_PAIR(2));
			wprintw(prfi->top, "Connection Terminated\n");
			wattroff(prfi->top, COLOR_PAIR(2));
			wmove(prfi->bottom, LINES/4 - 2, 0);
			wrefresh(prfi->top);
			wrefresh(prfi->bottom);
			break;
		}
		readstring[readsize] = 0;
		wprintw(prfi->top, readstring); 
		*/
		wrefresh(prfi->top);
		wmove(prfi->bottom, LINES/4 - 1, 0);
		wrefresh(prfi->top);
		wrefresh(prfi->bottom);
	}
	return 0; 
}

int main(int argc, char ** argv){
	// Usage Information
	if(argc < 3){
		printf("Usage:  %s hostname port\n", argv[0]);
		return 1;
	}

	// Handle Signals
	struct sigaction sa;
	sa.sa_handler = exit_graceful;
	sigaction(SIGINT, &sa, 0);

	// Prepare the network connection, but don't call connect yet
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
		goto err;
	short port = atoi(argv[2]);
	struct sockaddr_in connect_addr;
	connect_addr.sin_port = htons(port);
	connect_addr.sin_family = AF_INET;
	struct hostent* entry = gethostbyname(argv[1]);
	if(!entry)
		goto err;
	struct in_addr **addr_list = (struct in_addr**)entry->h_addr_list;
	struct in_addr* c_addr = addr_list[0];
	char* ip_string = inet_ntoa(*c_addr);
	connect_addr.sin_addr = *c_addr;
	
	// Set up curses.  This is probably easier in Python.
	initscr();
	start_color();
	use_default_colors();
	init_pair(1, COLOR_GREEN, -1);
	init_pair(2, COLOR_RED, -1);
	refresh();
	//WINDOW* top = newwin(LINES*3/4, COLS, 0, 0);
	//WINDOW* bottom = newwin(LINES/4 - 1, COLS, LINES*3/4 + 1, 0);
	WINDOW* top = newwin(LINES*3/4, COLS*13/16 - 6, 0, 0);
        WINDOW* bottom = newwin(LINES/4 - 1, COLS*13/16 - 6, LINES*3/4 + 1, 0);
	WINDOW* right = newwin(LINES, COLS/5 - 2, 0,COLS*13/16 - 5);
	refresh();
	wmove(stdscr, LINES*3/4, 0);
	whline(stdscr, ACS_HLINE , COLS*13/16 - 5);
	wmove(stdscr, 0, COLS*13/16 - 6);
	wvline(stdscr, ACS_VLINE , LINES*10);
	wmove(right, COLS*13/16 + 1,0);
	wmove(bottom, LINES/4 - 2, 0);
	scrollok(bottom, 1);
	scrollok(top, 1);
	scrollok(right,1);
	wrefresh(top);
	wrefresh(bottom);
	wrefresh(right);
	refresh();

	// The UI is up, let's reassure the user that whatever name they typed resolved to something
	wattron(top, COLOR_PAIR(1));
	wprintw(top, "Connecting to host %s (%s)\n", entry->h_name, ip_string);
	wrefresh(top);

	// Actually connect.  It might connect right away, or sit here and hang - depends on how the
	// host is feeling today
	if(connect(sockfd, (struct sockaddr*)&connect_addr, sizeof(struct sockaddr_in)))
		goto err;
	
	// Let the user know we're connected, so they can start doing whatever they do.
	wprintw(top, "Connected\n");
	wrefresh(top);
	wattroff(top, COLOR_PAIR(1));

	// Start the receive thread
	struct receive_info rfi;
	rfi.sockfd = sockfd;
	rfi.top = top;
	rfi.bottom = bottom;
	rfi.right = right;
	pthread_t t;
	pthread_create(&t, 0, receive_print, &rfi);;
	// Get user input.  Ctrl + C is the way out now.
	char input[1024*1024];
	wmove(bottom, LINES/4 - 1, 0);
	//wgetnstr(bottom, input, 1024*1024-1);
	size_t length;
	wprintw(bottom,"Use the /help command to display commands\n");
	for(;;){
		wscrl(bottom, 0);
		wrefresh(bottom);
		wgetnstr(bottom, input, 1024*1024-1);
		if(!strcmp(input,"/create")){
			if(!alive && !started){
				type = 10;
				write(sockfd,&type,1);
				Create_Character(&c, &rfi, input);
			}
			else{
				wprintw(bottom, "You've already started a character...\n");
			}
                }
		else if(!strcmp(input,"/start")){
			wrefresh(top);
                        wrefresh(right);
			type = 6;
                        write(sockfd,&type,1);
			wrefresh(top);
			wrefresh(right);
		}
		else if(!strcmp(input,"/pvp")){
			char pvp_name[32];
			type = 4;
			write(sockfd,&type,1);
			wprintw(bottom, "Initiate PVP with: ");
			wgetstr(bottom,input);
			strcpy(pvp_name,input);
			write(sockfd,&pvp_name,32);

		}
		else if (!strcmp(input,"/loot")){
			char l_name[32];
			type = 5;
			write(sockfd,&type,1);
			wprintw(bottom, "Loot: ");
			wgetstr(bottom,input);
			strcpy(l_name,input);
			write(sockfd,&l_name,32);
		}
		else if(!strcmp(input,"/fight")){
			type = 3;
			write(sockfd,&type,1);
		}
		else if(!strcmp(input,"/goto")){
			unsigned short room_change;
			type = 2;
                        write(sockfd,&type,1);
			wprintw(bottom,"Choose a room number to move to: ");
			wgetstr(bottom,input);
			werase(top);
			wrefresh(top);
			room_change = atoi(input);
			write(sockfd,&room_change,2);
			werase(right);
			refresh();
		}
		else if(!strcmp(input,"/message")){
			type = 1;
			write(sockfd,&type,1);
			Write_Message(&m,&rfi,input);
		}
		else if(!strcmp(input,"/help")){
			wprintw(bottom,"Use the /create command to create a character\n");
			wprintw(bottom,"Use the /start command to start the game\n");
			wprintw(bottom,"Use the /fight command to fight monsters\n");
			wprintw(bottom,"Use the /goto command to move rooms. Must use room numbers to move\n");
			wprintw(bottom,"Use the /message command to send a message to another player\n");
			wprintw(bottom,"Use the /loot command to loot another dead player\n");
			wprintw(bottom,"Use the /pvp command to commence pvp (if the server supports it)\n");
			wprintw(bottom,"Use the /leave or /quit command to leave the server\n");
		}
		else if(!strcmp(input,"/leave") || !strcmp(input,"/quit")){
			type = 12;
			write(sockfd,&type,1);
			endwin();
			exit(0);
		}
		wrefresh(top);
		wrefresh(bottom);
	}
err:
	endwin();
	perror(argv[0]);
	return 1;
}
