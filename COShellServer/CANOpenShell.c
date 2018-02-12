/*
Program: CANOpenShell Server
Author: Sami Metoui
Description: This program is server deamon that accept TCP/IP connections
and receive CANOpenShell commands from a remote host. These commands are relayed to the CAN bus.
Then the Server reply by a predifined messege strings to the remote host when an answer
to the request is provided on the CAN bus.
*/


/*
This file is part of CanFestival, a library implementing CanOpen Stack.

Copyright (C): Edouard TISSERANT and Francis DUPIN

See COPYING file for copyrights details.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#if defined(WIN32) && !defined(__CYGWIN__)  //For win32 preprocessor
#include <windows.h>
#include <stdio.h>
#define CLEARSCREEN "cls"
#define SLEEP(time) Sleep(time * 1000)
#elif defined(__CYGWIN__)                   //For cygwin preprocessor
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CLEARSCREEN "cls"
#define SLEEP(time) sleep(time)
#else                                       //For unix preprocessor
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CLEARSCREEN "clear"
#define SLEEP(time) sleep(time)
#endif

//****************************************************************************
// INCLUDES
#include "../../../netSocket/netSocket.h"			//TCP socket header

//#if 0 //------- REMOVE TAGS WHEN TO TEST WITHOUT CAN INTERFACE
#include "canfestival.h"
#include "CANOpenShell.h"
#include "CANOpenShellMasterOD.h"
#include "CANOpenShellSlaveOD.h"
//#endif //------- REMOVE TAGS WHEN TO TEST WITHOUT CAN INTERFACE

//****************************************************************************
// DEFINES
#define MAXBUF 200
#define NPORT 5000
#define BKLOG 10
#define MAXMSG 128

#define MAX_NODES 127
#define cst_str4(c1, c2, c3, c4) ((((unsigned int)0 | \
                                    (char)c4 << 8) | \
                                   (char)c3) << 8 | \
                                  (char)c2) << 8 | \
                                 (char)c1

#define INIT_ERR 2
#define QUIT 1

//#if 0 //-------REMOVE TAGS WHEN TO TEST WITHOUT CAN INTERFACE

//****************************************************************************
// GLOBALS
char BoardBusName[31];
char BoardBaudRate[5];
s_BOARD Board = {BoardBusName, BoardBaudRate};
CO_Data* CANOpenShellOD_Data;
char LibraryPath[512];

static int gstaticSocketCanGateway;

/*
This function Sleep for n seconds
input: number of seconds
*/

void SleepFunction(int second)
{
    SLEEP(second);
}


/*
This function ask a slave node to go in operational mode
input: node identifier
*/

void StartNode(UNS8 nodeid)
{
    char retbuf[100];

    if(masterSendNMTstateChange(CANOpenShellOD_Data, nodeid, NMT_Start_Node)==1)
    {
        strcpy(retbuf,"404");
        sprintf(retbuf,"%s Unable to start node %d ",retbuf,nodeid);
        sendData(gstaticSocketCanGateway, retbuf);
    }

	strcpy(retbuf,"000");
    sprintf(retbuf,"%s Node %d started ok",retbuf,nodeid);
    sendData(gstaticSocketCanGateway, retbuf);
}


/*
This function ask a slave node to go in pre-operational mode
input: node identifier
*/

void StopNode(UNS8 nodeid)
{
	char retbuf[100];

    if(masterSendNMTstateChange(CANOpenShellOD_Data, nodeid, NMT_Stop_Node)==1)
    {
        strcpy(retbuf,"404");
        sprintf(retbuf,"%s Unable to stop node %d",retbuf,nodeid);
        sendData(gstaticSocketCanGateway, retbuf);
    }

	strcpy(retbuf,"000");
    sprintf(retbuf,"%s Node %d stopped ok",retbuf,nodeid);
    sendData(gstaticSocketCanGateway, retbuf);
}


/*
This function ask a slave node to reset
input: node identifier
*/

void ResetNode(UNS8 nodeid)
{
    char retbuf[100];

	if(masterSendNMTstateChange(CANOpenShellOD_Data, nodeid, NMT_Reset_Node)==1)
	{
	    strcpy(retbuf,"404");
        sprintf(retbuf,"%s Unable to reset node %d",retbuf,nodeid);
        sendData(gstaticSocketCanGateway, retbuf);
	}

	strcpy(retbuf,"000");
    sprintf(retbuf,"%s Node %d reseted ok",retbuf,nodeid);
    sendData(gstaticSocketCanGateway, retbuf);
}


/*
This function reset all nodes on the network and print message when boot-up
*/

void DiscoverNodes()
{
    char retbuf[100];

    printf("Wait for Slave nodes bootup...\n\n");
    //strcpy(retbuf,"000");											//not implemented because of the number of node is unknown by the remote host
    //sprintf(retbuf,"%s Wait for Slave nodes bootup...",retbuf);	//and because this host still wait for a receive from the remote host in a
	//sendData(gstaticSocketCanGateway, retbuf);					//non multi-thread environnement
    ResetNode(0x00);	//tags this line when the CAN bus contain more than a master node and 1 slave node
}

int get_info_step = 0;


/*
This function callback function that check the read SDO demand
input: CO_Data structure, node id

*/
void CheckReadInfoSDO(CO_Data* d, UNS8 nodeid)
{
    UNS32 abortCode;
    UNS32 data=0;
    UNS32 size=64;
    char retbuf[100];

    if(getReadResultNetworkDict(CANOpenShellOD_Data, nodeid, &data, &size, &abortCode) != SDO_FINISHED)
        {
            printf("Master : Failed in getting information for slave %2.2x, AbortCode :%4.4x \n", nodeid, abortCode);
            //strcpy(retbuf,"404");
            //sprintf(retbuf,"%s Master : Failed in getting information for slave %2.2x, AbortCode :%4.4x ",retbuf, nodeid, abortCode);
            //sendData(gstaticSocketCanGateway, retbuf);
        }
    else
    {
        /* Display data received */
        switch(get_info_step)
        {
        case 1:
            printf("Device type     : %x\n", data);
            break;
        case 2:
            printf("Vendor ID       : %x\n", data);
            break;
        case 3:
            printf("Product Code    : %x\n", data);
            break;
        case 4:
            printf("Revision Number : %x\n", data);
            break;
        }
    }
    /* Finalize last SDO transfer with this node */
    closeSDOtransfer(CANOpenShellOD_Data, nodeid, SDO_CLIENT);

    GetSlaveNodeInfo(nodeid);
}

/* Retrieve node informations located at index 0x1000 (Device Type) and 0x1018 (Identity) */
void GetSlaveNodeInfo(UNS8 nodeid)
{
    switch(++get_info_step)
    {
    case 1: /* Get device type */
        printf("##################################\n");
        printf("#### Informations for node %x ####\n", nodeid);
        printf("##################################\n");
        readNetworkDictCallback(CANOpenShellOD_Data, nodeid, 0x1000, 0x00, 0, CheckReadInfoSDO);
        break;

    case 2: /* Get Vendor ID */
        readNetworkDictCallback(CANOpenShellOD_Data, nodeid, 0x1018, 0x01, 0, CheckReadInfoSDO);
        break;

    case 3: /* Get Product Code */
        readNetworkDictCallback(CANOpenShellOD_Data, nodeid, 0x1018, 0x02, 0, CheckReadInfoSDO);
        break;

    case 4: /* Get Revision Number */
        readNetworkDictCallback(CANOpenShellOD_Data, nodeid, 0x1018, 0x03, 0, CheckReadInfoSDO);
        break;

    case 5: /* Print node info */
        get_info_step = 0;
    }
}

/* Callback function that check the read SDO demand */
void CheckReadSDO(CO_Data* d, UNS8 nodeid)
{
    UNS32 abortCode;
    UNS32 data=0;
    UNS32 size=64;
    char retbuf[100]; //RSDO

    if(getReadResultNetworkDict(CANOpenShellOD_Data, nodeid, &data, &size, &abortCode) != SDO_FINISHED)
    {
        printf("\nResult : Failed in getting information for slave %2.2x, AbortCode :%4.4x \n", nodeid, abortCode);
        strcpy(retbuf,"404"); //RSDO
        sprintf(retbuf,"%s Error ssdo node %d with abort code: %x",retbuf,nodeid,abortCode); //RSDO
        sendData(gstaticSocketCanGateway, retbuf); //RSDO
    }

    else
    {
        printf("\nResult : %x\n", data);
        strcpy(retbuf,"000"); //RSDO
        sprintf(retbuf,"%s ssdo node %d ok with result: %x ",retbuf,nodeid,data); //RSDO
        sendData(gstaticSocketCanGateway, retbuf); //RSDO

    }

    /* Finalize last SDO transfer with this node */
    closeSDOtransfer(CANOpenShellOD_Data, nodeid, SDO_CLIENT);
}

/* Read a slave node object dictionary entry */
void ReadDeviceEntry(char* sdo)
{
    int ret=0;
    int nodeid;
    int index;
    int subindex;
    int datatype = 0;
    char retbuf[100];

    ret = sscanf(sdo, "rsdo#%2x,%4x,%2x", &nodeid, &index, &subindex);
    if (ret == 3)
    {

        printf("##################################\n");
        printf("#### Read SDO                 ####\n");
        printf("##################################\n");
        printf("NodeId   : %2.2x\n", nodeid);
        printf("Index    : %4.4x\n", index);
        printf("SubIndex : %2.2x\n", subindex);

        readNetworkDictCallback(CANOpenShellOD_Data, (UNS8)nodeid, (UNS16)index, (UNS8)subindex, (UNS8)datatype, CheckReadSDO);
    }
    else
        {
            printf("Wrong command  : %s\n", sdo);
            strcpy(retbuf,"404");
            sprintf(retbuf,"%s wrong command sent",retbuf);
        }
}

/* Callback function that check the write SDO demand */
void CheckWriteSDO(CO_Data* d, UNS8 nodeid)
{
    UNS32 abortCode;
    char retbuf[100];

    if(getWriteResultNetworkDict(CANOpenShellOD_Data, nodeid, &abortCode) != SDO_FINISHED)
    {
        printf("\nResult : Failed in getting information for slave %2.2x, AbortCode :%4.4x \n", nodeid, abortCode);
        strcpy(retbuf,"404");
        sprintf(retbuf,"%s Error wsdo node %d with abort code %x",retbuf,nodeid,abortCode);
        sendData(gstaticSocketCanGateway, retbuf);
    }
    else
    {
        printf("\nSend data OK\n");
        strcpy(retbuf,"000");
        sprintf(retbuf,"%s wsdo node %d ok",retbuf,nodeid);
        sendData(gstaticSocketCanGateway, retbuf);
    }


    /* Finalize last SDO transfer with this node */
    closeSDOtransfer(CANOpenShellOD_Data, nodeid, SDO_CLIENT);
}

/* Write a slave node object dictionnary entry */
void WriteDeviceEntry(char* sdo)
{
    int ret=0;
    int nodeid;
    int index;
    int subindex;
    int size;
    int data;
    char retbuf[100];

    ret = sscanf(sdo, "wsdo#%2x,%4x,%2x,%2x,%x", &nodeid , &index, &subindex, &size, &data);
    if (ret == 5)
    {
        printf("##################################\n");
        printf("#### Write SDO                ####\n");
        printf("##################################\n");
        printf("NodeId   : %2.2x\n", nodeid);
        printf("Index    : %4.4x\n", index);
        printf("SubIndex : %2.2x\n", subindex);
        printf("Size     : %2.2x\n", size);
        printf("Data     : %x\n", data);

        writeNetworkDictCallBack(CANOpenShellOD_Data, nodeid, index, subindex, size, 0, &data, CheckWriteSDO);
    }
    else
    {
        printf("Wrong command  : %s\n", sdo);
        strcpy(retbuf,"404");
        sprintf(retbuf,"%s wrong command sent",retbuf);
        sendData(gstaticSocketCanGateway, retbuf);

    }
}

void CANOpenShellOD_post_SlaveBootup(CO_Data* d, UNS8 nodeid)
{
    printf("Slave %x boot up\n", nodeid);
}

/***************************  CALLBACK FUNCTIONS  *****************************************/
void CANOpenShellOD_initialisation(CO_Data* d)
{
    printf("Node_initialisation\n");
}

void CANOpenShellOD_preOperational(CO_Data* d)
{
    printf("Node_preOperational\n");
}

void CANOpenShellOD_operational(CO_Data* d)
{
    printf("Node_operational\n");
}

void CANOpenShellOD_stopped(CO_Data* d)
{
    printf("Node_stopped\n");
}

void CANOpenShellOD_post_sync(CO_Data* d)
{
    //printf("Master_post_sync\n");
}

void CANOpenShellOD_post_TPDO(CO_Data* d)
{
    //printf("Master_post_TPDO\n");
}

/***************************  INITIALISATION  **********************************/
void Init(CO_Data* d, UNS32 id)
{
    if(Board.baudrate)
    {
        /* Init node state*/
        setState(CANOpenShellOD_Data, Initialisation);
    }
}

/***************************  CLEANUP  *****************************************/
void Exit(CO_Data* d, UNS32 nodeid)
{
    if(strcmp(Board.baudrate, "none"))
    {
        /* Reset all nodes on the network */
        masterSendNMTstateChange(CANOpenShellOD_Data, 0 , NMT_Reset_Node);

        /* Stop master */
        setState(CANOpenShellOD_Data, Stopped);
    }
}


/*
This fuction initialise the node
*/

int NodeInit(int NodeID, int NodeType)
{
    char retbuf[100];

    if(NodeType)
        CANOpenShellOD_Data = &CANOpenShellMasterOD_Data;
    else
        CANOpenShellOD_Data = &CANOpenShellSlaveOD_Data;

    /* Load can library */
    LoadCanDriver(LibraryPath);

    /* Define callback functions */
    CANOpenShellOD_Data->initialisation = CANOpenShellOD_initialisation;
    CANOpenShellOD_Data->preOperational = CANOpenShellOD_preOperational;
    CANOpenShellOD_Data->operational = CANOpenShellOD_operational;
    CANOpenShellOD_Data->stopped = CANOpenShellOD_stopped;
    CANOpenShellOD_Data->post_sync = CANOpenShellOD_post_sync;
    CANOpenShellOD_Data->post_TPDO = CANOpenShellOD_post_TPDO;
    CANOpenShellOD_Data->post_SlaveBootup=CANOpenShellOD_post_SlaveBootup;

    /* Open the Peak CANOpen device */
    if(!canOpen(&Board,CANOpenShellOD_Data))
    {
        strcpy(retbuf,"404");
        sprintf(retbuf,"%s Error creating node %d ",retbuf,NodeID);
        sendData(gstaticSocketCanGateway, retbuf);
        return INIT_ERR;
    }

    /* Defining the node Id */
    setNodeId(CANOpenShellOD_Data, NodeID);

    /* Start Timer thread */
    StartTimerLoop(&Init);

    strcpy(retbuf,"000");
    sprintf(retbuf,"%s Node %d creation ok\n",retbuf,NodeID);
    printf("sent msg %s",retbuf);
    sendData(gstaticSocketCanGateway, retbuf);

    return 0;
}


/*
This fuction display command line help
*/

void help_menu(void)
{
    printf("   MANDATORY COMMAND (must be the first command):\n");
    printf("     load#CanLibraryPath,channel,baudrate,nodeid,type (0:slave, 1:master)\n");
    printf("\n");
    printf("   NETWORK: (if nodeid=0x00 : broadcast)\n");
    printf("     ssta#nodeid : Start a node\n");
    printf("     ssto#nodeid : Stop a node\n");
    printf("     srst#nodeid : Reset a node\n");
    printf("     scan : Reset all nodes and print message when bootup\n");
    printf("     wait#seconds : Sleep for n seconds\n");
    printf("\n");
    printf("   SDO: (size in bytes)\n");
    printf("     info#nodeid\n");
    printf("     rsdo#nodeid,index,subindex : read sdo\n");
    printf("        ex : rsdo#42,1018,01\n");
    printf("     wsdo#nodeid,index,subindex,size,data : write sdo\n");
    printf("        ex : wsdo#42,6200,01,01,FF\n");
    printf("\n");
    printf("   Note: All numbers are hex\n");
    printf("\n");
    printf("     help : Display this menu\n");
    printf("     quit : Quit application\n");
    printf("\n");
    printf("\n");
}


/*
This function extract node identifier from string
input: command string
output: node identifier
*/

int ExtractNodeId(char *command)
{
    int nodeid;
    sscanf(command, "%2x", &nodeid);
    return nodeid;
}


/*
This function compare the 4 first characters of command string and call the correponding sub-function
input: command strig pointer
output: 0 or node identifier if a new node is created
*/

int ProcessCommand(char* command)
{
    int ret = 0;
    int sec = 0;
    int NodeID;
    int NodeType;
    char retbuf[30];

    EnterMutex();
    switch(cst_str4(command[0], command[1], command[2], command[3]))
    {
    case cst_str4('h', 'e', 'l', 'p') : /* Display Help*/
        help_menu();
        break;
    case cst_str4('s', 's', 't', 'a') : /* Slave Start*/
        StartNode(ExtractNodeId(command + 5));
        break;
    case cst_str4('s', 's', 't', 'o') : /* Slave Stop */
        StopNode(ExtractNodeId(command + 5));
        break;
    case cst_str4('s', 'r', 's', 't') : /* Slave Reset */
        ResetNode(ExtractNodeId(command + 5));
        break;
    case cst_str4('i', 'n', 'f', 'o') : /* Retrieve node informations */
        GetSlaveNodeInfo(ExtractNodeId(command + 5));
        break;
    case cst_str4('r', 's', 'd', 'o') : /* Read device entry */
        ReadDeviceEntry(command);
        break;
    case cst_str4('w', 's', 'd', 'o') : /* Write device entry */
        WriteDeviceEntry(command);
        break;
    case cst_str4('s', 'c', 'a', 'n') : /* Display master node state */
        DiscoverNodes();
        break;
    case cst_str4('w', 'a', 'i', 't') : /* Display master node state */
        ret = sscanf(command, "wait#%d", &sec);
        if(ret == 1)
        {
            LeaveMutex();
            strcpy(retbuf,command);
            sendData(gstaticSocketCanGateway, retbuf);
            SleepFunction(sec);
            return 0;
        }
        break;
    case cst_str4('q', 'u', 'i', 't') : /* Quit application */
        LeaveMutex();
        return QUIT;
    case cst_str4('l', 'o', 'a', 'd') : /* Library Interface*/
        ret = sscanf(command, "load#%100[^,],%30[^,],%4[^,],%d,%d",LibraryPath,BoardBusName,BoardBaudRate,
                     &NodeID,&NodeType);

        if(ret == 5)
        {
            LeaveMutex();
            ret = NodeInit(NodeID, NodeType);
            return ret;
        }
        else
        {
            printf("Invalid load parameters\n");
        }
        break;
    default :
        help_menu();
        LeaveMutex();
        return -1;
    }
    LeaveMutex();
    return 0;
}

//#endif //-------REMOVE TAGS IF CAN INTERFACE IS PRESENT


/*
This fuction process commands from init file
input: file name, socket
*/

int processServerInitFile(char* fileName)
{
    int i,n;
    char psrcbuf[128];
    char ptarbuf[128];
    char ptmpobuf[128];
    FILE* pPFile;

    if((pPFile=fopen(fileName,"r"))==NULL)
    {
        printf("\nError while opening file %s",fileName);
        return -1;
    }
    else
    {
        printf("\nProcessing init file %s",fileName);
        while (fgets(ptmpobuf,MAXMSG,pPFile)!=NULL)
        {

            for(i=0; ptmpobuf[i]==' '; i++) {}
            strcpy(psrcbuf,ptmpobuf+i);
            if (strlen(psrcbuf)>=2 && psrcbuf[0]!='#')
            {
                ProcessCommand(psrcbuf);
                usleep(999999);
            }

        }
        fclose(pPFile);
    }
    return 0;
}

/****************************************************************************/
/***************************  MAIN  *****************************************/
/****************************************************************************/

int main(int argc, char** argv)
{
    extern char *optarg;
    char command[200];
    char* res;
    int ret=0;
    int sysret=0;
    int i=0;

    //*********** TCP Server declarations

    int sfd,rlen;
    char cl[MAXBUF];
    char tbuf[MAXBUF];
    FILE* pf;


		/* Init stack timer */
    TimerInit();			        //-------REMOVE TAGS IF CAN INTERFACE IS PRESENT

    //goto init_fail; INIT_ERR		//------- USE THIS LINE INSTRUCTION FOR EMERGENCY EXIT

		/*load winsock dll for windows environment*/
    initNet();

		/*create the socket*/
	if ((sfd=socketServ(NPORT))<0) return 0;

		/*Process init file if required param token*/
	if (argc>1)
    {
        processServerInitFile(argv[1]);
    }

    while (1)
    {

			/*establish connection with a host*/
        gstaticSocketCanGateway=acceptServ(sfd, cl);

        if (gstaticSocketCanGateway>0)
        {
            printf("\nConnection with the host established");
            ret=0;
            while(ret==0)
            {

					/*receive data command line from the host*/
                if ((rlen=receiveData(gstaticSocketCanGateway, tbuf, MAXBUF))<0) break;
                else {
                    tbuf[rlen]='\0';
                    printf("\nReceived command: %s\n",tbuf);

					/* Enter in a loop to read stdin command until "quit" is called */
                    ret = ProcessCommand(tbuf);						//-------REMOVE COMMENT TAGS IF CAN INTERFACE IS PRESENT
                }
            }
            disconnect(gstaticSocketCanGateway);
            printf("\nDisconnected from the host");
        }
    }
    disconnect(sfd);
    closeNet();

    printf("\nFinishing.");

    // Stop timer thread
    StopTimerLoop(&Exit);					//-------REMOVE COMMENT TAGS IF CAN INTERFACE IS PRESENT

    /* Close CAN board */
    canClose(CANOpenShellOD_Data);			//-------REMOVE COMMENT TAGS IF CAN INTERFACE IS PRESENT

init_fail:

    TimerCleanup();							//-------REMOVE COMMENT TAGS IF CAN INTERFACE IS PRESENT
    return 0;
}
