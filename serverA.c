#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <limits.h>

#define BUFFER_UDP_LEN 100000  //Max length of buffer
#define PORT_UDP 21143   //The port on which to listen for incoming data
#define SERVER_NAME "A"

struct sockaddr_in aws_server_UDP, aws_client_UDP;
int sockfd_AWS_UDP, client_len_UDP = sizeof(aws_client_UDP) , recv_UDP_len;
int buff_for_UDP[BUFFER_UDP_LEN];
int min = INT_MAX;
int max = INT_MIN;
int sum = 0;
int sos = 0;
int num_of_nums = 0;
int function;
int result;
char reduction_type[3];

/**********************************************************************************************
Most of functions are self-explained by its name. 
Some more specific information will be added above some function declaration*/
/**********************************************************************************************/

/*create a UDP socket for server*/
void create_UDP_socket()
{
    if ((sockfd_AWS_UDP = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("AWS UDP created socket failed");
        exit(1);
    }

}

/*set address info for struct sockaddr_in of UDP*/
void prep_set_UDP()
{
    memset((char *) &aws_server_UDP, 0, sizeof(aws_server_UDP));
     
    aws_server_UDP.sin_family = AF_INET;
    aws_server_UDP.sin_port = htons(PORT_UDP);
    aws_server_UDP.sin_addr.s_addr = htonl(INADDR_ANY);
}
     
/*bind this socket with its address info*/
void bind_for_UDP()
{
    if( bind(sockfd_AWS_UDP , (struct sockaddr*)&aws_server_UDP, sizeof(aws_server_UDP) ) == -1)
    {
        perror("AWS UDP bind failed");
        exit(1);
    }
    printf("The Server %s is up and running using UDP on port %d\n", SERVER_NAME, PORT_UDP);
}

/*main process, receive data from AWS, processing data by given function in
array, sending outcome back to AWS*/
void main_process()
{
    while(1){
        fflush(stdout);
    
        recv_UDP_len = recvfrom(sockfd_AWS_UDP, buff_for_UDP, BUFFER_UDP_LEN, 0, 
            (struct sockaddr *) &aws_client_UDP, &client_len_UDP);  
        if (recv_UDP_len == -1)
        {
            perror("AWS UDP receive failed");
        }
        num_of_file();
        function_name();

        if(function == 0){
            result = process_MIN();
        }else if(function == 1){
            result = process_MAX();
        }else if(function == 2){
            result = process_SUM();
        }else{
            result = process_SOS();
        }
        printf("The Server %s has received %d numbers \n", SERVER_NAME, num_of_nums);
        printf("The Server %s has successfully finished the reduction %s: %d\n", SERVER_NAME, reduction_type, result);
        if ((sendto(sockfd_AWS_UDP, &result, sizeof(int), 0,
             (struct sockaddr *)&aws_client_UDP, client_len_UDP)) == -1){
            perror("serverA send back failed");
        }

        printf("The Server %s has successfully finished sending the reduction value to AWS server.\n", SERVER_NAME);
        min = INT_MAX;
        max = INT_MIN;
        sum = 0;
        sos = 0;

        result = 0;
    }
}

/*set number of file*/
void num_of_file()
{
    num_of_nums = buff_for_UDP[0];
}

/*set function name*/
void function_name()
{
    function = buff_for_UDP[1];
    if(function == 0){
        strcpy(reduction_type, "MIN");
    }else if(function == 1){
        strcpy(reduction_type, "MAX");
    }else if(function == 2){
        strcpy(reduction_type, "SUM");
    }else{
        strcpy(reduction_type, "SOS");
    }
}

/*compute max*/
int process_MAX()
{
    int i;
    for(i = 0; i < num_of_nums; i++){
        if(buff_for_UDP[i + 2] > max){
            max = buff_for_UDP[i + 2];
        }
    }
    return max;
}

/*compute min*/
int process_MIN()
{
    int i;
    for(i = 0; i < num_of_nums; i++){
        if(buff_for_UDP[i + 2] < min){
            min = buff_for_UDP[i + 2];
        }
    }
    return min;
}

/*compute sum*/
int process_SUM()
{
    int i;
    for(i = 0; i < num_of_nums; i++){
        sum = sum + buff_for_UDP[i + 2];
    }
    return sum;
}

/*compute sos*/
int process_SOS()
{
    int i;
    for(i = 0; i < num_of_nums; i++){
        sos = sos + buff_for_UDP[i + 2] * buff_for_UDP[i + 2];
    }
    return sos;
}


/************************main****************************/
int main(void)
{
    create_UDP_socket();
    prep_set_UDP();
    bind_for_UDP();
    main_process();
    
    return 0;
}
