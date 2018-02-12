/********************************************************************************************/
/* Library name: netsocket.c                                                                */
/* Author: Sami Metoui                                                                      */
/* This library contain a set of fuctions that may be used by server and client application */
/* These fonctions are used to initialize and create sockets, to listen on a specific port  */
/* to accept connexions from hosts, connect to remote host, to disconnect and close sockets */
/********************************************************************************************/

#ifdef WIN32
#include <winsock.h>

#elif defined __unix__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include "netsocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysexits.h"

/***************************************************************/
/* This funcion load winsock.dll for windows and create socket */
/* return: nothing or -1 if an error occure                    */
/***************************************************************/
int initNet()
{
#ifdef WIN32
    int err;
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(1,1);
    err=WSAStartup(wVersionRequested, &wsaData);
    if(err!=0)
    {
        return(-1);
    }
#endif
    return(0);
}

/***********************************************************************************/
/* This function create socket and establish the connection with the server        */
/* input: char array with the name or the ip address separate by dots, port number */
/* return: the socket number or -1 if the error occure when the socket is created  */
/* and -2 if the error occure when the connecion is established.                   */
/***********************************************************************************/
int connectClient(char* nom,int port)
{
    int sfd;
    struct sockaddr_in saddr;
    struct hostent* phostEnt;

        //create the socket
    if ((sfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
	{
        perror("socket");
        return(-1);
    }

        //recover the address structure from port and the machine name
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    phostEnt = gethostbyname (nom);

    saddr.sin_addr.s_addr = ((struct in_addr*)(phostEnt->h_addr))->s_addr;

    if (connect(sfd,(struct sockaddr*)&saddr,sizeof saddr ) < 0)
    {
        perror("connect");
        return(-2);
    }

    return(sfd);
}

/********************************************************************************/
/* This fuction create the server socket and begin listening on a specific port */
/* input: port number                                                           */
/* return: socket number                                                        */
/********************************************************************************/
int socketServ(int port)
{
    int sfd;
    struct sockaddr_in saddr;

            // insert code here
    if ((sfd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
        {
            perror("socket");
            return(-1);
        }

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sfd,(struct sockaddr*)(&saddr),sizeof(saddr)) < 0)
        {
            perror("bind");
            return(-2);
        }

    if (listen(sfd,0) < 0)
    {
        perror("listen");
        return(-3);
    }

    return(sfd);
}

/******************************************************************************/
/* This fuction accept connection from remote host                            */
/* input: socket number, buffer which will receive the host ip address string */
/******************************************************************************/
int acceptServ(int s, char* client)
{
    int sfd;
    struct sockaddr_in saddr;
    int saddrlen=sizeof saddr;

    if((sfd=accept(s,(struct sockaddr*)&saddr,&saddrlen)) < 0)
    {
        perror("accept");
        return(-1);
    }
    strcpy(client,inet_ntoa(saddr.sin_addr));
	return sfd;
}

/*************************************************/
/* This function send buffer content to the host */
/* input: socket number, buffer, buffer length   */
/* return: number of sent characters             */
/*************************************************/
int sendData(int s, char* buf)
{
    int n;

    n=strlen(buf);
    n=send(s,buf,n,0);

    return n;
}

/***************************************************************/
/* This function receive data from host a store it in a buffer */
/* input: socket number, buffer, buffer length                 */
/* return: number of received characters                       */
/***************************************************************/
int receiveData(int s, char* buf, int len)
{
    int rlen;

    rlen=recv(s,buf,len,0);
    if (rlen>=0) buf[rlen]='\0';
    return rlen;
}

/********************************/
/* This function close a socket */
/* input: socket number         */
/********************************/
void disconnect(int s)
{
#ifdef WIN32
    shutdown(s,2);
#else
    close(s);
#endif
}

/************************************/
/* This function unload winsock.dll */
/************************************/
void closeNet()
{
#ifdef WIN32
    WSACleanup();
#endif
}

