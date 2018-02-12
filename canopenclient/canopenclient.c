/*
Program: canopenclient.c
Author: Sami Metoui
Description: Can client program that send OpenShell command string to the CanOpenShell server
and display received can message from the OpenShell Server. A status machine mode allow
to send predifined command to a stepper-engine by modifying it velocity and position.
*/

#include "..\netSocket\netSocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <strings.h>
#include <conio.h>
#include <unistd.h>

#define USAGE "Usage: %s server_name [init_file_name]"
#define NPORT 5000
#define MAXMSG 1024
#define cst_str4(c1, c2, c3, c4) ((((unsigned int)0 | \
                                    (char)c4 << 8) | \
                                   (char)c3) << 8 | \
                                  (char)c2) << 8 | \
                                 (char)c1

void help_menu(void);
void enterStatusMachine(int);
int processInitFile(char*, int);

int main (int argc, char*argv[])
{
    int n, sfd,ret=0;
    char bufs[MAXMSG];              //source buffer
    char bufc[MAXMSG];              //target buffet
    char command[MAXMSG];

    initNet();

    if(argc>3)
    {
        fprintf(stderr,USAGE,argv[0]);
    }
    else
    {
        if(argc < 2)
        {
            fprintf(stderr,USAGE,argv[0]);
        }
    }

    if((sfd=connectClient(argv[1],NPORT))<0)
    {
        exit(EX_OSERR);
    }

    if(argc > 2) processInitFile(argv[2],sfd);

    while(ret!=-1)
    {
        fflush(stdin);
        printf("\n$> ");
        fgets(command,MAXMSG,stdin);
        switch(cst_str4(command[0], command[1], command[2], command[3]))
        {
        case cst_str4('s', 's', 't', 'a') : /* Slave Start*/
        case cst_str4('s', 's', 't', 'o') : /* Slave Stop */
        case cst_str4('s', 'r', 's', 't') : /* Slave Reset */
        case cst_str4('i', 'n', 'f', 'o') : /* Retrieve node informations */
        case cst_str4('r', 's', 'd', 'o') : /* Read device entry */
        case cst_str4('w', 's', 'd', 'o') : /* Write device entry */
        case cst_str4('s', 'c', 'a', 'n') : /* Display master node state */
        case cst_str4('w', 'a', 'i', 't') : /* Display master node state */
        case cst_str4('l', 'o', 'a', 'd') : /* Library Interface*/
            sscanf(command,"%s", bufs);    //
            bufs[strlen(bufs)]='\0';
            if (sendData(sfd,bufs)<0) exit(EX_OSERR);
            if ((n=receiveData(sfd,bufc,MAXMSG)) < 0) exit(EX_OSERR);
            printf("\nReceived : %s",bufc);
            break;

        case cst_str4('s', 'e', 'n', 'd') : /* Send a command string to the server*/
            bufs[0]='\0';
            sscanf(command,"%*s %s", bufs);
            if(!strlen(bufs)) printf("Error: send command require argument");
            else
            {
                bufs[strlen(bufs)]='\0';
                if (sendData(sfd,bufs)<0) exit(EX_OSERR);
                if ((n=receiveData(sfd,bufc,MAXMSG)) < 0) exit(EX_OSERR);
                printf("\nReceived : %s",bufc);
            }
            break;

        case cst_str4('h', 'e', 'l', 'p') : /* Display Help*/
            help_menu();
            break;

        case cst_str4('q', 'u', 'i', 't') : /* Quit the program*/
            ret=-1;
            break;

        case cst_str4('s', 't', 'a', 't') :
            enterStatusMachine(sfd);
            break;

        default :
            sscanf(command,"%s", bufs);
            printf("Error : Unkown command %s, type \'help\' for details",bufs);
        }
    }
    disconnect(sfd);
    closeNet();
    return 0;
}


/*
This fuction process commands from init file.
'#' prefixed lines are not processed and inserted space before the command are ignored
input: file name, socket
*/

int processInitFile(char* fileName,int pSockFd)
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
                if (sendData(pSockFd,psrcbuf)<0) exit(EX_OSERR);
                if ((n=receiveData(pSockFd,ptarbuf,MAXMSG)) < 0) exit(EX_OSERR);
                printf("\nReceived : %s",ptarbuf);
                usleep(999999);
            }

        }
        fclose(pPFile);
    }
    return 0;
}


/*
This function enter in status machine mode.
This mode allow user to modify volocity, positon and stop the stepper motor
input: socket
*/
void enterStatusMachine(int sockFd)
{

    int state=0;
    int dsec,sec;
    int stl,i,n;
    int vitesse,position;
    char choice;
    char srcbuf[128];
    char tarbuf[128];
    char tpobuf[128];
    char stTab[8][30]=
    {
        "wsdo#6,6060,00,04,00000001",
        "wsdo#6,6081,00,04,",

        "wsdo#6,6040,00,04,0000007F",
        "wsdo#6,607A,00,04,",

        "wsdo#6,607A,00,04,0000017F",
        "info#6",
    };

    sec=dsec=0;
    while(state!=-1)
    {

        printf("\rElapsed time %02d:%2d ",sec,dsec);
        if (dsec==99)
            {
                dsec=0;
                sec++;
                if (sec==60) sec=0;
            }
        usleep(10000);
        dsec++;

        if (kbhit()!=0)
        {

            choice=getch();
            switch(choice)
            {
            case 'v':
                printf("\nSet velocity: ");
                scanf("%d",&vitesse);
                vitesse=vitesse*256;
                strcpy(srcbuf,stTab[1]);
                itoa(vitesse,tpobuf,16);
                stl=strlen(tpobuf);
                stl=8-stl;
                for(i=0; i<stl; i++)
                {
                    tpobuf[i]='0';
                }
                itoa(vitesse,tpobuf+stl,16);
                strcat(srcbuf,tpobuf);

                sendData(sockFd,srcbuf);
                usleep(999999);

                //Remove tag when used with echo server
                if ((n=receiveData(sockFd,tarbuf,MAXMSG)) < 0) exit(EX_OSERR);
                printf("\nReceived : %s",tarbuf);

                sendData(sockFd,stTab[0]);
                usleep(999999);

                //Remove tag when used with echo server
                if ((n=receiveData(sockFd,tarbuf,MAXMSG)) < 0) exit(EX_OSERR);
                printf("\nReceived : %s",tarbuf);



                printf("\n");
                break;

            case 'p':
                printf("\nSet position: ");
                scanf("%d",&position);
                position=position*64;
                strcpy(srcbuf,stTab[3]);
                itoa(position,tpobuf,16);
                stl=strlen(tpobuf);
                stl=8-stl;
                for(i=0; i<stl; i++)
                {
                    tpobuf[i]='0';
                }
                itoa(position,tpobuf+stl,16);
                strcat(srcbuf,tpobuf);

                sendData(sockFd,stTab[2]);
                usleep(2000000);

                //Remove tag when used with echo server
                if ((n=receiveData(sockFd,tarbuf,MAXMSG)) < 0) exit(EX_OSERR);
                printf("\nReceived : %s",tarbuf);

                sendData(sockFd,srcbuf);
                usleep(999999);

                //Remove tag when used with echo server
                if ((n=receiveData(sockFd,tarbuf,MAXMSG)) < 0) exit(EX_OSERR);
                printf("\nReceived : %s",tarbuf);

                printf("\n");
                break;

            case 's' :
                sendData(sockFd,stTab[4]);
                usleep(999999);
                if ((n=receiveData(sockFd,tarbuf,MAXMSG)) < 0) exit(EX_OSERR);
                printf("\nReceived : %s",tarbuf);

                printf("\n");
                break;

            case 'i' :
                sendData(sockFd,stTab[5]);
                usleep(999999);
                if ((n=receiveData(sockFd,tarbuf,MAXMSG)) < 0) exit(EX_OSERR);
                printf("\nReceived : %s",tarbuf);

                printf("\n");
                break;

            case 'h':
                printf("\n type v : to define velocity");
                printf("\n      p : to define new position");
                printf("\n      s : to halt the motor");
                printf("\n      i : to retrive information from the node");
                printf("\n      h : to print this help");
                printf("\n      q : to leave status machine mode\n");
                break;

            case 'q':
                state=-1;
            }
        }
    }
    return;
}

/*
This function display the help
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
