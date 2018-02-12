#ifndef NETSOCKET_H_INCLUDED
#define NETSOCKET_H_INCLUDED

/*
This funcion load winsock.dll for windows and create socket
return: nothing or -1 if an error occure
*/
int initNet();

/*
This function create socket and establish the connection with the server
input: char array with the name or the ip address separate by dots, port number
return: the socket number or -1 if the error occure when the socket is created
and -2 if the error occure when the connecion is established.
*/
int connectClient(char*, int);

/*
This fuction create the server socket
input: port number
return: socket number
*/
int socketServ(int);

/*
This fuction accept connection to the host
input: socket number, buffer which will receive the host ip address string
*/
int acceptServ(int, char*);

/*
This function send buffer content to the host
input: socket number, buffer, buffer length
return: number of sent characters
*/
int sendData(int, char*);

/*
This function receive data from host a store it in a buffer
input: socket number, buffer, buffer length
return: number of received characters
*/
int receiveData(int, char*, int);

/*
This function close a socket
input: socket number
*/
void disconnect(int);

/*
This function unload winsock.dll
*/
void closeNet();

#endif // NETSOCKET_H_INCLUDED
