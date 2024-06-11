/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: mimi
 *
 * Created on 23 maja 2024, 15:27
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <strings.h>
#include <sys/time.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <unistd.h>
#include "BrokerService.h"


static int broker_udp_socket;
static struct sockaddr_in broker_addr;


static
ssize_t broker_send_request(uint8_t request_type)
{
    struct ServiceRequestInfo reqest;
    reqest.RequestType = request_type;
    reqest.Modul = 0;
    return sendto(broker_udp_socket, &reqest, sizeof(reqest), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr));
}

static
ssize_t broker_recv_response( void *respbuf, size_t bufsize  )
{
    ssize_t n = recvfrom(broker_udp_socket, respbuf, bufsize, 0,NULL,NULL); 
    if (n<5) return 0;
    if(strstr(respbuf,"HELLO"))  // service odpowiada HELLO na nieznany request type
    {
        puts("Request error");
        return 0;
    }
    return n;
}




static
const char * get_state_msg( uint8_t errcode )
{
    const char *normal ="OK  ";
    const char *wait   ="Wait";
    const char *griddt ="Grid Detect";
    const char *connerr="Conn error";
    const char *fault  ="Grid Fault";
    const char *error  ="Error";
    const char *init   ="Init";
    const char *statoff="Off ";

    if(errcode==InverterStateNormal) return normal;
    if(errcode==InverterStateInit)   return init;
    if(errcode==InverterStateOff)	return statoff;
    if(errcode==InverterStateWait)   return wait;
    if(errcode==InverterStateGridDetect) return griddt;
    if(errcode==InverterStateConnectionErr) return connerr;
    if(errcode==InverterStateGridFault)  return fault;
    return error;
}


static
void broker_send_request_inverter_info( )
{ 
    struct ServiceResponseInverterInfo  rInverterInfo;
    
    // clear response struct
    bzero(&rInverterInfo,sizeof(rInverterInfo));
    
    broker_send_request(RequestTypeInverterInfo);
    
    int n = broker_recv_response(&rInverterInfo, sizeof(rInverterInfo) );
    if(n<5) return;
    
    rInverterInfo.InverterState.activepower = ntohs(rInverterInfo.InverterState.activepower);
    rInverterInfo.InverterState.averagepower = ntohs(rInverterInfo.InverterState.averagepower);
    rInverterInfo.Grid.Rvoltage = ntohs(rInverterInfo.Grid.Rvoltage);
    rInverterInfo.Grid.Svoltage = ntohs(rInverterInfo.Grid.Svoltage);
    rInverterInfo.Grid.Tvoltage = ntohs(rInverterInfo.Grid.Tvoltage);
    rInverterInfo.Grid.Rcurrent = ntohs(rInverterInfo.Grid.Rcurrent);
    rInverterInfo.Grid.Scurrent = ntohs(rInverterInfo.Grid.Scurrent);
    rInverterInfo.Grid.Tcurrent = ntohs(rInverterInfo.Grid.Tcurrent);
    rInverterInfo.Grid.voltage = ntohs(rInverterInfo.Grid.voltage);
    
    printf(" %.2i:%.2i:%.2i", rInverterInfo.InverterState.tmhour, rInverterInfo.InverterState.tmmin, rInverterInfo.InverterState.tmsec);
    printf("; %i:%s; Power: %i0  average: %i0; \n" , 
            rInverterInfo.InverterState.state, get_state_msg(rInverterInfo.InverterState.state), 
            rInverterInfo.InverterState.activepower, rInverterInfo.InverterState.averagepower);
    printf("Grid U: %i  %i  %i; I: %i  %i  %i; " , 
            rInverterInfo.Grid.Rvoltage, rInverterInfo.Grid.Svoltage, rInverterInfo.Grid.Tvoltage,
            rInverterInfo.Grid.Rcurrent, rInverterInfo.Grid.Scurrent, rInverterInfo.Grid.Tcurrent);
    printf("; MAX %i %i; OverV: %i; " , rInverterInfo.Grid.maxphase, rInverterInfo.Grid.voltage, rInverterInfo.Grid.phaseOverV);
    puts("");
}


static
void broker_send_request_inverter_state()
{
    struct  Inverter  InverterState;
    
    // clear response struct
    bzero(&InverterState,sizeof(InverterState));
    
    broker_send_request(RequestTypeInverterState);
    
    int n = broker_recv_response(&InverterState, sizeof(InverterState) );
    if(n<5) return;
    
    InverterState.activepower = ntohs(InverterState.activepower);
    InverterState.averagepower = ntohs(InverterState.averagepower);
    
    printf(" %.2i:%.2i:%.2i", InverterState.tmhour, InverterState.tmmin, InverterState.tmsec);
    printf("; %i:%s; Power: %i0  average: %i0; \n" , 
            InverterState.state, get_state_msg(InverterState.state), 
            InverterState.activepower, InverterState.averagepower);
    puts("");
}

static
void broker_send_request_grid_state()
{
    struct  Grid  GridState;
    
    // clear response struct
    bzero(&GridState,sizeof(GridState));
    
    broker_send_request(RequestTypeGridState);
    
    int n = broker_recv_response(&GridState, sizeof(GridState) );
    if(n<5) return;
    
    GridState.Rvoltage = ntohs(GridState.Rvoltage);
    GridState.Svoltage = ntohs(GridState.Svoltage);
    GridState.Tvoltage = ntohs(GridState.Tvoltage);
    GridState.Rcurrent = ntohs(GridState.Rcurrent);
    GridState.Scurrent = ntohs(GridState.Scurrent);
    GridState.Tcurrent = ntohs(GridState.Tcurrent);
    GridState.voltage = ntohs(GridState.voltage);
    
    printf("Grid U: %i  %i  %i; I: %i  %i  %i; " , 
            GridState.Rvoltage, GridState.Svoltage, GridState.Tvoltage,
            GridState.Rcurrent, GridState.Scurrent, GridState.Tcurrent);
    printf("; MAX %i %i; OverV: %i; " , GridState.maxphase, GridState.voltage, GridState.phaseOverV);
    puts("");
}


static
void broker_send_request_json_info()
{
    #define JSONINFOSIZE 256
    char jsoninfo[JSONINFOSIZE];
    
    // clear response struct
    bzero(jsoninfo, JSONINFOSIZE);
    broker_send_request(RequestTypeInverterInfoJson); 
    int n=broker_recv_response(jsoninfo, JSONINFOSIZE);
    if(n<=0) return n;
    puts(jsoninfo);
    return n;
}


static
void broker_send(int stateOpt)
{
    
    switch(stateOpt)
    {
      case 0:
        broker_send_request_inverter_info();
        break;
      case 1:
        broker_send_request_inverter_state();
        break;
      case 2:
        broker_send_request_grid_state();
        break;
      case 3:
        broker_send_request_json_info();
        break;
    }
}


void print_usage()
{
    puts(" brokercmd <broker_addr> <broker_port> --<cmd> ");
    puts("  <cmd>:");
    puts("  --info     inverter info ");
    puts("  --state    inverter state ");
    puts("  --grid     grid state " );
    puts("  --json     get data in json format " );
    /* w przypadku localhost oraz portu domyslnego, 
     * podajemy odrazu komendę  --cmd
     */
}

int parseCommand(int argc, char *arg3)
{
    if(strstr(arg3,"--info"))   return 0;
    if(strstr(arg3,"--state"))   return 1;
    if(strstr(arg3,"--grid"))    return 2; 
    if(strstr(arg3,"--json"))    return 3;
    return 0;
}

/*
 * 
 */
int main(int argc, char** argv) 
{
    int stateOpt = 0;
    char *ipadd;
    int port;

    printf("inverter broker test ");

    if (argc < 2)
    {
        print_usage();  
        exit(1);
    }
        
    /* w przypadku localhost oraz portu domyslnego, 
     * podajemy odrazu komendę  --cccc
     */
    if(strstr(argv[1], "--")) 
    {
        ipadd = "127.0.0.1";
        port = 8890;
        if (argc>1)  stateOpt = parseCommand(argc, argv[1]); 
    }
    else 
    {
        ipadd = argv[1];
        port = atoi(argv[2]);
        if (argc>3)  stateOpt = parseCommand(argc, argv[3]);
    }
    
    printf("Connect to %s:%i\n", ipadd, port);
    
    broker_udp_socket=socket(AF_INET,SOCK_DGRAM,0);

    struct timeval tv;
    tv.tv_sec = 10; // receive timeout_in_seconds;
    tv.tv_usec = 0;
    setsockopt(broker_udp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    bzero(&broker_addr,sizeof(broker_addr));
    broker_addr.sin_family = AF_INET;
    broker_addr.sin_addr.s_addr=inet_addr(ipadd);
    broker_addr.sin_port=htons(port);
    
    broker_send(stateOpt);
    
    close(broker_udp_socket);
    puts("");

    return (EXIT_SUCCESS);
}
