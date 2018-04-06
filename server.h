#ifndef SER_TCP_H
#define SER_TCP_H

#define HOSTNAME_LENGTH 20
#define RESP_LENGTH 40
#define FILENAME_LENGTH 20
#define REQUEST_PORT 5001
#define BUFFER_LENGTH 1024 
#define MAXPENDING 10
#define MSGHDRSIZE 8 //Message Header Size
#define max_nu_peers 100
#define true_max_peers 98
#define max_nu_files 100

typedef enum {
	REQ = 1, fail, off, act,NF,NP,SP //Message type
} Type;

typedef struct
{
	char hostname[HOSTNAME_LENGTH];
	char filename[FILENAME_LENGTH];
} Req;  //request

typedef struct
{
	char response[RESP_LENGTH];
} Resp; //response


typedef struct
{
	Type type;
	int  length; //length of effective bytes in the buffer
	char buffer[BUFFER_LENGTH];
} Msg; //message format used for sending and receiving


struct infos {
	string files[max_nu_files][max_nu_peers];// number of peers is max_nu_peers-2, since the array [x][0,1] are detecated for file name and size.
	string status[max_nu_peers - 2][2];// first colume is the IP address and the second is the state.
	double speed;
};

class TcpServer
{
	int serverSock, clientSock;     /* Socket descriptor for server and client*/
	struct sockaddr_in ClientAddr; /* Client address */
	struct sockaddr_in ServerAddr; /* Server address */
	unsigned short ServerPort;     /* Server port */
	int clientLen;            /* Length of Server address data structure */
	char servername[HOSTNAME_LENGTH];

public:
	TcpServer();
	~TcpServer();
	void start();
};

class TcpThread :public Thread
{

	int cs;
	int actual_peers;
	int actual_files;
	infos list;
public:
	TcpThread(int clientsocket) :cs(clientsocket)
	{}
	virtual void run();
	int msg_recv(int, Msg *);
	void data_recv(CHAR *, int, int, Msg, Req *, Resp *);
	int msg_send(int, Msg *);
	void data_send(Req *, Resp *);
	unsigned long ResolveName(char name[]);
	static void err_sys(char * fmt, ...);
	void update_lists(string hostname, infos list, infos files, int size);
	int send_arry(string filename, infos list, string &send_arry, int &result);
	void send_peers(int count,string send_arry, Req * reqp, Resp * respp);
};

#endif
#pragma once
