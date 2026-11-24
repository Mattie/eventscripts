#ifndef ES_RCON_H
#define ES_RCON_H

// We will probably need to include a few files here..
#ifndef WIN32
ConVar	rcon_address		( "rcon_address", "", 0, "Address of remote server if sending unconnected rcon commands" );
ConVar  rcon_port			( "rcon_port"   , "27015", 0, "Port of remote server if sending unconnected rcon commands" );

/*
# This is a simple linux command line utility to execute rcon commands
# Just change the YOUR_PASSWORD_HERE to your rcon password (unless
# you want to enter it every time) and possibly change the default
# IP address from 127.0.0.1 (localhost)
#
# once downloaded on your linux system, compile it with:
#
#   gcc -o rcon rcon.c
#
# note, it should work on non-linux too, but may require changing the 
# socket stuff (i.e. windows will definitely need to add the winsock
# initialization line)
#
# written by [ASY]Zyrain (http://www.asyserver.com/~cstrike/rcon.c)
#
*/
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#define DEBUG 0

#define SERVERDATA_EXECCOMMAND 2
#define SERVERDATA_AUTH 3
#define SERVERDATA_RESPONSE_VALUE 0
#define SERVERDATA_AUTH_RESPONSE 2

int send_rcon(int sock, int id, int command, char *string1, char *string2) {
	int size, ret;
	size = 10+strlen(string1)+strlen(string2);

	ret = send(sock,&size,sizeof(int),0);
	if(ret == -1) {
		DxMsg(0, __FUNCTION__, __LINE__,"send() failed:\n");
		return -1;
	}
	ret = send(sock,&id,sizeof(int),0);
	if(ret == -1) {
		DxMsg(0, __FUNCTION__, __LINE__,"send() failed:\n");
		return -1;
	}
	ret = send(sock,&command,sizeof(int),0);
	if(ret == -1) {
		DxMsg(0, __FUNCTION__, __LINE__,"send() failed:\n");
		return -1;
	}
	ret = send(sock,string1,strlen(string1)+1,0);
	if(ret == -1) {
		DxMsg(0, __FUNCTION__, __LINE__,"send() failed:\n");
		return -1;
	}
	ret = send(sock,string2,strlen(string2)+1,0);
	if(ret == -1) {
		DxMsg(0, __FUNCTION__, __LINE__,"send() failed:\n");
		return -1;
	}
	DxMsg(1, __FUNCTION__, __LINE__,"Sent %d bytes\n",size+4);
	return 0;
}

int recv_rcon(int sock, int timeout, int *id, int *command, char *string1,
			  char *string2) {
				  struct timeval tv;
				  fd_set readfds;
				  int size;
				  char *ptr;
				  int ret;
				  char buf[8192];

				  size=0xDEADBEEF;
				  *id=0xDEADBEEF;
				  *command=0xDEADBEEF;
				  string1[0]=0;
				  string2[0]=0;

				  tv.tv_sec = timeout;
				  tv.tv_usec = 0;

				  FD_ZERO(&readfds);
				  FD_SET(sock, &readfds);

				  /* don't care about writefds and exceptfds: */
				  select(sock+1, &readfds, NULL, NULL, &tv);

				  if (!FD_ISSET(sock, &readfds)) {
					  DxMsg(1, __FUNCTION__, __LINE__,"recv timeout\n");
					  return -1; // timeout
				  }
				  DxMsg(1, __FUNCTION__, __LINE__,"Got a response\n");
				  ret = recv(sock, &size, sizeof(int), 0);
				  if(ret == -1) {
					  DxMsg(0, __FUNCTION__, __LINE__,"recv() failed:\n");
					  return -1;
				  }
				  if((size<10) || (size>8192)) {
					  DxMsg(0, __FUNCTION__, __LINE__,"Illegal size %d\n",size);
					  return (-1);
				  }
				  ret = recv(sock, id, sizeof(int),0);
				  if(ret == -1) {
					  DxMsg(0, __FUNCTION__, __LINE__,"recv() failed:\n");
					  return -1;
				  }
				  size-=ret;
				  ret = recv(sock, command, sizeof(int),0);
				  if(ret == -1) {
					  DxMsg(0, __FUNCTION__, __LINE__,"recv() failed:\n");
					  return -1;
				  }
				  size-=ret;

				  ptr = buf;
				  while(size) {
					  ret = recv(sock, ptr, size, 0);
					  if(ret == -1) {
						  DxMsg(0, __FUNCTION__, __LINE__,"recv() failed:\n");
						  return -1;
					  }
					  size -= ret; 
					  ptr += ret;
				  }
				  buf[8190] = 0;
				  buf[8191] = 0;

				  strncpy(string1, buf, 4095);
				  string1[4095] = 0;
				  strncpy(string2, buf+strlen(string1)+1, 4095);

				  return 0;
}

/* This is set to 1 when we've been authorized */
int auth = 0;
char string1[4096];
char string2[4096];

int process_response(int sock) {
	int ret;
	int id;
	int command;

	ret=recv_rcon(sock, 1, &id, &command, string1, string2);
	DxMsg(1, __FUNCTION__, __LINE__,"Received = %d : id=%d, command=%d, s1=%s, s2=%s\n",
		ret, id, command, string1, string2);
	if(ret==-1) {
		return -1;
	}

	switch(command) {
  case SERVERDATA_AUTH_RESPONSE:
	  switch(id) {
  case 20: 
	  auth = 1;
	  break;
  case -1:
	  DxMsg(0, __FUNCTION__, __LINE__,"Password Refused\n");
	  return -1;
  default:
	  DxMsg(0, __FUNCTION__, __LINE__,"Bad Auth Response ID = %d\n",id);
	  return -1;
	  };
	  break;
  case SERVERDATA_RESPONSE_VALUE:
	  DxMsg(0, __FUNCTION__, __LINE__,"%s\n",string1);
	  break;
  default:
	  DxMsg(0, __FUNCTION__, __LINE__,"Unexpected command: %d\n",command);
	  break;
	};
}

CON_COMMAND( rcon, "Issue an rcon command." )
{
	static struct sockaddr_in a;
	static int sock;
	int ret, i;
	ConVar* cpassword = FindVar("rcon_password");
	char password[512]="YOUR_PASSWORD_HERE";
	short port = 27015;
	static char address[512] = "127.0.0.1";
	int arg;

	if (auth>0 && FStrEq(address, rcon_address.GetString()))
	{
		auth = 0;
		DxMsg(1, __FUNCTION__, __LINE__,"Sending Command: \"%s\"\n", ARGS());

		ret=send_rcon(sock, 20, SERVERDATA_EXECCOMMAND, ARGS(), "");

		if(ret == -1) {
			DxMsg(1, __FUNCTION__, __LINE__,"cmd send failure -- try to reauthenticate\n");
			auth = 0;
			close(sock);
		}

	}
	else
	{
		auth = 0;
		close(sock);
	}
	if (auth<1)
	{
		close(sock);
		strncpy(address, rcon_address.GetString(), 512);
		port = rcon_port.GetInt();
		strncpy(password, cpassword->GetString(), 512);
		address[511] = '\0';
		password[511] = '\0';

		a.sin_family = AF_INET;
		a.sin_addr.s_addr = inet_addr(address);
		a.sin_port = htons(port);

		sock = socket(AF_INET, SOCK_STREAM,0); // TCP socket

		ret = 0;
		ret = connect(sock,(struct sockaddr *)&a,sizeof(a));

		if(ret == -1) {
			DxMsg(0, __FUNCTION__, __LINE__,"connect() failed.\n");
			return -1;
		} else {
			DxMsg(1, __FUNCTION__, __LINE__,"Connected to Server\n");
		}

		DxMsg(1, __FUNCTION__, __LINE__,"Sending RCON Password\n");
		ret=send_rcon(sock, 20, SERVERDATA_AUTH, password, "");

		if(ret == -1) {
			DxMsg(1, __FUNCTION__, __LINE__,"Couldn't send password\n");
			return -1;
		};

		while(auth==0) {
			if(process_response(sock)==-1) {
				DxMsg(0, __FUNCTION__, __LINE__,"Couldn't Authenticate\n");
				return -1;
			}
		}


		DxMsg(1, __FUNCTION__, __LINE__,"Password Accepted\n");

		DxMsg(1, __FUNCTION__, __LINE__,"Sending Command: \"%s\"\n", ARGS());

		ret=send_rcon(sock, 20, SERVERDATA_EXECCOMMAND, ARGS(), "");

		if(ret == -1) {
			DxMsg(0, __FUNCTION__, __LINE__,"cmd send failure\n");
			auth = 0;
			close(sock);
			return -1;
		}
	}
	// process responses until a timeout
	//while(process_response(sock) != -1);

	return 0;
}

#endif


#endif // ES_RCON_H

