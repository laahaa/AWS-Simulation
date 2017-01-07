#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>

#define SERVER_PORT_A 21143   
#define SERVER_PORT_B 22143   
#define SERVER_PORT_C 23143   

#define HOST "localhost"
int resultA;
int resultB;
int resultC;
int final_result;
char reduction_type[3];

#define MYPORT_TCP 25143    
#define MYPORT_UDP 24143    
#define BACKLOG 1     // how many pending connections queue will hold

int sockfd_AWS_TCP, childfd_TCP;  // listen on sock_fd, new connection on childfd_TCP
struct sockaddr_in aws_addr, aws_addr_UDP;    // my address information
struct sockaddr_in client_addr_TCP; // connector's address information
int sin_size;
struct sigaction sa;
int yes=1;
int TCP_buffer_LENGTH = 10000;
int buffer_TCP[10000];
int num_of_numbers;
int buff_to_serverA[10000];
int buff_to_serverB[10000];
int buff_to_serverC[10000];
int function;

int sockfd_AWS_UDP;
struct sockaddr_in serverA_UDP, serverB_UDP, serverC_UDP; // connector's address information
int serverA_len_UDP = sizeof(serverA_UDP);
int serverB_len_UDP = sizeof(serverB_UDP);
int serverC_len_UDP = sizeof(serverC_UDP);

struct hostent *serverA, *serverB, *serverC;
int numbytes;
int msg[] = {1, 3, 5, 7, 2, 4};

/**********************************************************************************************
Most of functions are self-explained by its name. 
Some more specific information will be added above function declarations*/
/**********************************************************************************************/

/*create a TCP socket for AWS, and set its option*/
void create_and_set_TCP_socket()
{
    sockfd_AWS_TCP = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_AWS_TCP < 0) {
        perror("AWS can't create a socket");
        exit(1);
    }
    
    if (setsockopt(sockfd_AWS_TCP,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) < 0) {
        perror("AWS setsockopt failed");
        exit(1);
    }
    
    printf("The AWS is up and running.\n");
}

/*set address info for struct sockaddr_in of TCP*/
void pre_set_TCP()
{
    aws_addr.sin_family = AF_INET;         // host byte order
    aws_addr.sin_port = htons(MYPORT_TCP);     // short, network byte order
    aws_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(aws_addr.sin_zero), '\0', 8); // zero the rest of the struct
}

/*bind for TCP socket*/
void bind_TCP_socket()
{
    if (bind(sockfd_AWS_TCP, (struct sockaddr *)&aws_addr, sizeof(struct sockaddr))
        < 0) {
        perror("AWS bind failed");
        exit(1);
    }
}

/*set address info for struct sockaddr_in of UDP*/
void pre_set_UDP()
{
    aws_addr_UDP.sin_family = AF_INET;         // host byte order
    aws_addr_UDP.sin_port = htons(MYPORT_UDP);     // short, network byte order
    aws_addr_UDP.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(aws_addr_UDP.sin_zero), '\0', 8); // zero the rest of the struct
}

/*bind for UDP socket*/
void bind_UDP_socket()
{
    if(bind(sockfd_AWS_UDP, (struct sockaddr *)&aws_addr_UDP, sizeof(struct sockaddr)) < 0){
        perror("AWS bind UDP failed");
        exit(1);
    }
}

/*listen for TCP client*/
void listen_TCP()
{
    if (listen(sockfd_AWS_TCP, BACKLOG) < 0) {
        perror("AWS listen failed");
        exit(1);
    }
}

/*segment file into three parts*/
void segment_nums()
{
    int i;
    for(i = 0; i < ((num_of_numbers) / 3); i++){
        buff_to_serverA[i + 2] = buffer_TCP[i + 2];
    }
    buff_to_serverA[0] = (num_of_numbers) / 3;
    buff_to_serverA[1] = function;
    
    
    for(i = 0; i < ((num_of_numbers) / 3 + 2); i++){
        buff_to_serverB[i + 2] = buffer_TCP[i + (num_of_numbers)/ 3 + 2];
    }
    buff_to_serverB[0] = (num_of_numbers) / 3;
    buff_to_serverB[1] = function;
    
    
    for(i = 0; i < ((num_of_numbers) / 3); i++){
        buff_to_serverC[i + 2] = buffer_TCP[i + (num_of_numbers)/3 * 2 + 2];
    }
    buff_to_serverC[0] = (num_of_numbers) / 3;
    buff_to_serverC[1] = function;
    
}

void aws_send_UDP();
void receive_from_servers();
void get_final_result();
void send_final_result();

/*accept and receive TCP connection, entering a loop for providing contiguous service,
which contains segmentating file, sending file to three backend servers. receiving from
three backend servers, doing some computation in AWS, sending back to client*/
void main_process()
{
    int count = 1;
    
    while(1) {  // main accept() loop
        sin_size = sizeof(struct sockaddr_in);
        childfd_TCP = accept(sockfd_AWS_TCP,
                             (struct sockaddr *)&client_addr_TCP, &sin_size);
        if(childfd_TCP < 0){
            perror("AWS accept failed");
        }
        
        int recv_Val = recv(childfd_TCP, buffer_TCP, 10000, 0);
        if(recv_Val < 0){
            perror("AWS receive failed");
            exit(0);
        }

        //in case of binding repeatedly
        if(count == 1){
            pre_set_UDP();
            bind_UDP_socket();    
            count++;
        }
        
        num_of_numbers = buffer_TCP[0];
        function = buffer_TCP[1];
        
        printf("The AWS has received %d numbers from the client using TCP over port %d\n", num_of_numbers, MYPORT_TCP);
        
        segment_nums();
        aws_send_UDP();
        receive_from_servers();
        get_final_result();
        
        send_final_result();

        /*clean buffer*/
        final_result = 0;
        resultA = 0;
        resultB = 0;
        resultC = 0;
        close(childfd_TCP);
    }
}

/*******************UDP*************************/

void create_UDP_socket_and_get_serverinfo()
{
    if ((serverA = gethostbyname(HOST)) == NULL) {  // get the host info
        perror("AWS gethostbyname A failed");
        exit(1);
    }
    
    if ((serverB = gethostbyname(HOST)) == NULL) {  // get the host info
        perror("AWS gethostbyname B failed");
        exit(1);
    }
    
    if ((serverC = gethostbyname(HOST)) == NULL) {  // get the host info
        perror("AWS gethostbyname C failed");
        exit(1);
    }
    
    sockfd_AWS_UDP = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (sockfd_AWS_UDP == -1){
        perror("AWS UDP created socket failed");
        exit(1);
    }
}

/*In UDP transmission, AWS is a client. This function is to set address information
for its servers, which are three backend servers*/
void prepset()
{
    serverA_UDP.sin_family = AF_INET;     // host byte order
    serverA_UDP.sin_port = htons(SERVER_PORT_A); // short, network byte order
    serverA_UDP.sin_addr = *((struct in_addr *)serverA->h_addr);
    memset(&(serverA_UDP.sin_zero), '\0', 8); // zero the rest of the struct
    
    serverB_UDP.sin_family = AF_INET;     // host byte order
    serverB_UDP.sin_port = htons(SERVER_PORT_B); // short, network byte order
    serverB_UDP.sin_addr = *((struct in_addr *)serverB->h_addr);
    memset(&(serverB_UDP.sin_zero), '\0', 8); // zero the rest of the struct
    
    serverC_UDP.sin_family = AF_INET;     // host byte order
    serverC_UDP.sin_port = htons(SERVER_PORT_C); // short, network byte order
    serverC_UDP.sin_addr = *((struct in_addr *)serverC->h_addr);
    memset(&(serverC_UDP.sin_zero), '\0', 8); // zero the rest of the struct
}

/*send files to three servers separately*/
void aws_send_UDP()
{
    /***************************send to A*************************************/
    if ((numbytes = sendto(sockfd_AWS_UDP, &buff_to_serverA, sizeof(buff_to_serverA), 0,
                           (struct sockaddr *)&serverA_UDP, sizeof(struct sockaddr))) == -1) {
        perror("AWS UDP sendto failed");
        exit(1);
    }
    
    printf("The AWS has sent %d numbers to Backend-Server A\n", num_of_numbers / 3);
    
    /***************************send to B*************************************/
    if ((numbytes = sendto(sockfd_AWS_UDP, &buff_to_serverB, sizeof(buff_to_serverB), 0,
                           (struct sockaddr *)&serverB_UDP, sizeof(struct sockaddr))) == -1) {
        perror("AWS UDP sendto failed");
        exit(1);
    }
    
    printf("The AWS has sent %d numbers to Backend-Server B\n", num_of_numbers / 3);
    
    /***************************send to C*************************************/
    if ((numbytes = sendto(sockfd_AWS_UDP, &buff_to_serverC, sizeof(buff_to_serverC), 0,
                           (struct sockaddr *)&serverC_UDP, sizeof(struct sockaddr))) == -1) {
        perror("AWS UDP sendto failed");
        exit(1);
    }
    
    printf("The AWS has sent %d numbers to Backend-Server C\n", num_of_numbers / 3);
}

/*receive from three backend servers in UDP connection*/
void receive_from_servers()
{
    /*******************receive from serverA***********************/
    if(recvfrom(sockfd_AWS_UDP, &resultA, sizeof(resultA), 0,
                (struct sockaddr *)&serverA_UDP, &serverA_len_UDP) < 0){
        perror("AWS UDP recvfrom serverA failed");
        exit(1);
    }
        
    /*******************receive from serverB***********************/
    if(recvfrom(sockfd_AWS_UDP, &resultB, sizeof(resultB), 0,
                (struct sockaddr *)&serverB_UDP, &serverB_len_UDP) < 0){
        perror("AWS UDP recvfrom serverB failed");
        exit(1);
    }
    
    /*******************receive from serverc***********************/
    if(recvfrom(sockfd_AWS_UDP, &resultC, sizeof(resultC), 0,
                (struct sockaddr *)&serverC_UDP, &serverC_len_UDP) < 0){
        perror("AWS UDP recvfrom serverC failed");
        exit(1);
    }
}

/*do computation for getting final result*/
void get_final_result()
{
    if(function == 0){
        strcpy(reduction_type, "MIN");
        final_result = (resultA < resultB)? resultA : resultB;
        final_result = (final_result < resultC)? final_result : resultC;
    }else if(function == 1){
        strcpy(reduction_type, "MAX");
        final_result = (resultA > resultB)? resultA : resultB;
        final_result = (final_result > resultC)? final_result : resultC;
    }else {
        if(function == 2){
            strcpy(reduction_type, "SUM");
        }else{
            strcpy(reduction_type, "SOS");
        }
        final_result = (resultA + resultB + resultC);
    }
    printf("The AWS received reduction result of %s from Backend-Server A using UDP over port 24143 and it is %d\n", reduction_type, resultA);
    printf("The AWS received reduction result of %s from Backend-Server B using UDP over port 24143 and it is %d\n", reduction_type, resultB);
    printf("The AWS received reduction result of %s from Backend-Server C using UDP over port 24143 and it is %d\n", reduction_type, resultC);
    printf("The AWS has successfully finished the reduction %s: %d\n", reduction_type, final_result);
}

/*send final result to client over TCP connection*/
void send_final_result()
{
    if(send(childfd_TCP, &final_result, sizeof(int), 0) < 0){
        perror("AWS send final result failed");
        exit(0);
    }
    printf("The AWS has successfully finished sending the reduction value to client.\n");
}

/*******************main*************************/

int main(void)
{
    create_and_set_TCP_socket();
    pre_set_TCP();
    bind_TCP_socket();
    
    create_UDP_socket_and_get_serverinfo();
    prepset();

    listen_TCP();
    main_process();
    
    return 0;
}
