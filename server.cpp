
#include <winsock.h>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <process.h>
#include "Thread.h"
#include "server.h"
using namespace std;
#include <iostream>
#include <fstream>
#include <string>


TcpServer::TcpServer()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0)
		TcpThread::err_sys((char*)"Starting WSAStartup() error\n");

	//Display name of local host
	if (gethostname(servername, HOSTNAME_LENGTH) != 0) //get the hostname
		TcpThread::err_sys((char*)"Get the host name error,exit");

	printf("Server: %s waiting to be contacted for PUT/GET request...\n", servername); // Display put/get meesage and wait for command


																					   //Create the server socket
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		TcpThread::err_sys((char*)"Create socket error,exit");

	//Fill-in Server Port and Address info.
	ServerPort = REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */

													 //Bind the server socket
	if (bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
		TcpThread::err_sys((char*)"Bind socket error,exit");

	//Successfull bind, now listen for Server requests.
	if (listen(serverSock, MAXPENDING) < 0)
		TcpThread::err_sys((char*)"Listen socket error,exit");


}

TcpServer::~TcpServer()
{
	WSACleanup();
}


void TcpServer::start()
{
	for (;;) /* Run forever */
	{
		/* Set the size of the result-value parameter */
		clientLen = sizeof(ClientAddr);

		/* Wait for a Server to connect */
		if ((clientSock = accept(serverSock, (struct sockaddr *) &ClientAddr,
			&clientLen)) < 0)
			TcpThread::err_sys((char*)"Accept Failed ,exit");

		/* Create a Thread for this new connection and run*/
		TcpThread * pt = new TcpThread(clientSock);
		pt->start();
	}
}

//////////////////////////////TcpThread Class //////////////////////////////////////////
void TcpThread::err_sys(char * fmt, ...)
{
	perror(NULL);
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	system("pause");
	exit(1);
}
int TcpThread::send_arry(string filename, infos list, string &send_arry,int &result)
{
	int location = -1;
	for (int i = 0; i < max_nu_peers; i++)
	{
		if (filename == list.files[i][0])
			location = i;
	}
	if (location < 0)
	{
		result = 0;
	}
	else
	{
		int count = 0;
		for (int i = 2; i < max_nu_peers; i++)
		{
			for (int j = 0; j < max_nu_peers; j++)
			{
				if (list.files[location][i] == list.status[j][0])
				{
					if (list.status[j][1] == "true")
					{
						//send_arry[count] = list.status[j][0]; //unkown error
						send_arry[count] = list.status[j][0]; //unkown error
						count++;
						break;
					}
				}
				if (list.files[location][i] == "NULL")
					break;
			}
		}
		if (count == 0)
		result = 1;
		else
			result = 2;
	}
	return count;
}

void TcpThread::send_peers(int count, string send_arry, Req * reqp, Resp * respp)
{
	Msg data;
	respp = (Resp *)data.buffer;
	for (int i = 0; i < count;i++) {
			sprintf(respp->response, "%s", send_arry[i]);
			//data.length = size; may need to edit the length function later
			if (msg_send(cs, &data) != data.length)
				err_sys((char*)"send Respose failed,exit");
	}
	printf(" List has been sent out for %s \n\n\n", reqp->hostname);
}

void TcpThread::update_lists(string hostname, infos list,infos files,int size)
{
	bool check = false;
	for (int i = 0; i <actual_peers; i++) { //checks if the peer is registored and change status
		if (hostname == list.status[i][0])
		{
			list.status[i][1] = true;
			break;
			 check = true;
		}
	}
	if(check==false) // add peer to the list if new
	{
		list.status[actual_peers][0] = hostname;
		list.status[actual_peers][1] = true;
		actual_peers++;
	}

	int count = NULL;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < actual_files; j++)
		{
			if (files.files[i][0]== list.files[j][0])
				count = 1;
			bool found = false;
			for (int m = 2; m < max_nu_peers; m++)
			{
				if (list.files[j][m] == hostname)
					break;
				else if (list.files[j][m] == "null") //fix later
					list.files[j][m] = hostname;
			}
		}
		if (count == NULL)
		{
			list.files[actual_files][0] = files.files[i][0];
			list.files[actual_files][1] = files.files[i][1];
			list.files[actual_files][2] = hostname;
			actual_files++;
		}
	}
	//save the data in file;
}
unsigned long TcpThread::ResolveName(char name[])
{
	struct hostent *host;            /* Structure containing host information */

	if ((host = gethostbyname(name)) == NULL)
		err_sys((char*)"gethostbyname() failed");

	/* Return the binary, network byte ordered address */
	return *((unsigned long *)host->h_addr_list[0]);
}

/*
msg_recv returns the length of bytes in the msg_ptr->buffer,which have been recevied successfully.
*/
int TcpThread::msg_recv(int sock, Msg * msg_ptr)
{
	int rbytes, n;

	for (rbytes = 0; rbytes<MSGHDRSIZE; rbytes += n)
		if ((n = recv(sock, (char *)msg_ptr + rbytes, MSGHDRSIZE - rbytes, 0)) <= 0)
			err_sys((char*)"Recv MSGHDR Error");

	for (rbytes = 0; rbytes<msg_ptr->length; rbytes += n)
		if ((n = recv(sock, (char *)msg_ptr->buffer + rbytes, msg_ptr->length - rbytes, 0)) <= 0)
			err_sys((char*)"Recevier Buffer Error");

	return msg_ptr->length;
}
void TcpThread::data_recv(CHAR *filename, int size, int sock, Msg rmsg, Req * reqp, Resp * respp) {
	ofstream file_d;
	file_d.open(filename, std::ofstream::binary); //create new file with the filename that is recieved
	while (size > 0)
	{
		if (size <= BUFFER_LENGTH)
		{
			if (msg_recv(sock, &rmsg) != rmsg.length) //recieve data
				err_sys((char*)"recv response error,exit");
			respp = (Resp *)rmsg.buffer;
			file_d.write(respp->response, size); // write buffer to file
			size = 0; //exit loop
		}
		else
		{ //if "size" > 1024B

			if (msg_recv(sock, &rmsg) != rmsg.length) //recieve data
				err_sys((char*)"recv response error,exit");
			respp = (Resp *)rmsg.buffer;
			file_d.write(respp->response, BUFFER_LENGTH); // write buffer to file
			size -= BUFFER_LENGTH; //subtract 1024B from "size" and loop

		}
	}
	file_d.close();
	printf("file: %s has been recieved\n", reqp->filename);
}


/* msg_send returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*/
int TcpThread::msg_send(int sock, Msg * msg_ptr)
{
	int n;
	if ((n = send(sock, (char *)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0)) != (MSGHDRSIZE + msg_ptr->length))
		err_sys((char*)"Send MSGHDRSIZE+length Error");
	return (n - MSGHDRSIZE);

}
/*/ send binary files*/
void TcpThread::data_send(Req * reqp, Resp * respp) {
	Msg data;
	ifstream file(reqp->filename, ios::in | ios::binary | ios::ate);     //open file in binary mode, get pointer at the end of the file (ios::ate)
	int size = file.tellg(); //Find the size of file
	file.seekg(0, file.beg);
	respp = (Resp *)data.buffer;
	while (size > 0) {
		if (size <= BUFFER_LENGTH) { //if size less than 1024B
			file.read(data.buffer, size); // read the file with the number of bytes determined by "size"
			sprintf(respp->response, "%s", data.buffer); // store read data
			data.length = size;
			if (msg_send(cs, &data) != data.length) //send data
				err_sys((char*)"send Respose failed,exit");
			size = 0; //exit loop
		}
		else {
			file.read(data.buffer, BUFFER_LENGTH); //if size > 1024B, read only 1024B.
			sprintf(respp->response, "%s", data.buffer);// store read data
			data.length = BUFFER_LENGTH;
			if (msg_send(cs, &data) != data.length)//send data
				err_sys((char*)"send Respose failed,exit");
			memset(respp->response, 0, sizeof(Resp));//Clear response
			size = size - BUFFER_LENGTH; //subtract 1024B from "size" and loop
		}
	}
	file.close();  //close file
	printf(reqp->filename);
	printf(" file has been sent out for %s \n\n\n", reqp->hostname);
}


void TcpThread::run() //cs: Server socket
{
	Resp * respp;//a pointer to response
	Req * reqp; //a pointer to the Request Packet
	Msg smsg, rmsg; //send_message receive_message
	struct _stat stat_buf;
	int result;

	if (msg_recv(cs, &rmsg) != rmsg.length)
		err_sys((char*)"Receive Req error,exit");

	//cast it to the request packet structure		
	reqp = (Req *)rmsg.buffer;
	printf("Receive a request from client:%s\n", reqp->hostname); //contruct the response and send it out
	//smsg.type = RESP;
	//smsg.length = sizeof(Resp);
	//respp = (Resp *)smsg.buffer;
	//memset(respp->response, 0, sizeof(Resp));

	//-----------------------------------------------------------------------------------
	//update the list
	//the client will send two massages the first is to update the list and the second is depending on the type of the massage
	//-----------------------------------------------------------------------------------
	//large loop to cover all types and to be used multiple times.

	//  {
	//-----------------------------------------------------------------------------------
	if (rmsg.type == REQ) {// // Check recieved cmd

	string peers[10];
	int result=0;
	int count = send_arry(reqp->filename, list, peers[], result);
	smsg.type = RESP; //need to fix it later
	smsg.length = sizeof(Resp);
	respp = (Resp *)smsg.buffer;
	memset(respp->response, 0, sizeof(Resp));

	switch (result) {
	case 0: //file not found
		char sends = result;
		sprintf(respp->response, "%s", sends);
		if (msg_send(cs, &smsg) != smsg.length)
			err_sys((char*)"send Respose failed,exit");
		printf("File requested does not exist\n");
		break;
	case 1: //file found but no available peer at the moment 
		char sends = result;
		sprintf(respp->response, "%s", sends);
		if (msg_send(cs, &smsg) != smsg.length)
			err_sys((char*)"send Respose failed,exit");
		printf("No avilable peers\n");
	case 2: //send array of availabe peers
		char sends = result;
		smsg.type = SP; //need to fix it later
		smsg.length = sizeof(Resp);
		sprintf(respp->response, "%s", count);
		if (msg_send(cs, &smsg) != smsg.length)
			err_sys((char*)"send Respose failed,exit");
		printf("Sending peers list\n");



//send_peers(count,send_arry);
	}
	//-----------------------------------------------------------------------
	//if (rmsg.type == fail)
	//change the status of the failed peers. assuming that they are offline 
	//----------------------------------------------------------------------
	//if (rmsg.type == off)
	//close the socket and connection and change status to offline
	//----------------------------------------------------------------------
	//if (rmsg.type == act)
	//connetion will stay and will wait for any later update
	//----------------------------------------------------------------------
	//}



	//if (rmsg.type == REQ_PUT) {// // Check recieved cmd
	//						   //receive the response
	//	if (msg_recv(cs, &rmsg) != rmsg.length)
	//		err_sys((char*)"recv response error,exit");
	//	//cast it to the response structure
	//	respp = (Resp *)rmsg.buffer;
	//	printf("Response:%s\n\n\n", respp->response);
	//	string temp = respp->response;
	//	if (temp == "Error On client side") { //check if no error on client side
	//		closesocket(cs);
	//		return;
	//	}
	//	else {
	//		int size = stoi(respp->response); // cast char to int for size of file
	//		data_recv(reqp->filename, size, cs, rmsg, reqp, respp); // function to recieve file
	//	}
	//}
	//else
	//{
		//if (rmsg.type == REQ_GET) // Check recieved cmd
		//{ //Request GET

		//	if ((result = _stat(reqp->filename, &stat_buf)) != 0)  //check if file exists
		//	{
		//		sprintf(respp->response, "No such a file");
		//		if (msg_send(cs, &smsg) != smsg.length)
		//			err_sys((char*)"send Respose failed,exit");
		//		printf("File requested does not exist\n");
		//	}
		//	else
		//	{
		//		sprintf(respp->response, "%ld", stat_buf.st_size); //store size in buffer
		//		if (msg_send(cs, &smsg) != smsg.length)// send size of file
		//			err_sys((char*)"send Respose failed,exit");
		//		data_send(reqp, respp); //function to send data
		//	}
		//}
	//}
	closesocket(cs);
}
////////////////////////////////////////////////////////////////////////////////////////

int main(void)
{
	// add adding the list or extracting the list 
	//____________________________________________
	//____________________________________________
	//____________________________________________
	TcpServer ts;
	ts.start();

	return 0;
}