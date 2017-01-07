#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>

#define SERVER_PORT 25143
#define HOST "localhost"

int clientfd_TCP, i;
struct sockaddr_in client_addr_TCP, aws_addr_TCP;
struct hostent *h;
int len, bytes_sent;
char buff_for_nums[10000];
char buff_for_receive[10000];
char *buffer;
int num_of_nums = 0;
long lSize;
int count = 0;
int iArr[10000];

/**********************************************************************************************
Most of functions are self-explained by its name. 
Some more specific information will be added above function declarations*/
/**********************************************************************************************/


/*represent function by int, min ->0, max -> 1, sum -> 2, sos -> 3*/
int function;   

int final_result;
int *pfinal_result = &final_result;
char reduction_type[3];

int final_result_len = sizeof (final_result);

/*set address info for struct sockaddr_in of TCP, for connecting AWS*/
void prep_set()
{
    h = gethostbyname(HOST);
    if(h==NULL) {
        printf("%s: unknown host '%s'\n", HOST);
        exit(1);
    }

    aws_addr_TCP.sin_family = h->h_addrtype;
    memcpy((char *) &aws_addr_TCP.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    aws_addr_TCP.sin_port = htons(SERVER_PORT);  
}

/*create a TCP socket*/
void create_socket_client_TCP()
{
    clientfd_TCP = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd_TCP<0) {
        perror("cannot open socket ");
        exit(1);
    }
}

/*connect with AWS*/
void connect_AWS()
{
    if(connect(clientfd_TCP, (struct sockaddr *) &aws_addr_TCP, sizeof(aws_addr_TCP)) < 0) {
        perror("cannot connect ");
        exit(1);
    }  
    printf("The client is up and running.\n");
}    


/*read numbers from file. Here, store every number in an int array, from the third
position. The first position is for the total number of this source file, the
second position in array is for informing function
This function is modified from http://stackoverflow.com/questions/3463426/in-c-how-should-i-read-a-text-file-and-print-all-strings,
which tells how to read file from data*/
void read_nums()
{
    FILE *fp;    
    FILE* file = fopen ("nums.csv", "r");
    while (!feof (file))
    {  
        fscanf (file, "%d", &iArr[count + 2]);
        count++;
    }
    
    iArr[0] = count;
    iArr[1] = function;
    fclose (file);
    
}

/*send file to AWS. */
void send_message()
{
    len = sizeof(iArr);
    int send_Val = send(clientfd_TCP, iArr, (count + 2) * sizeof(int), 0);
    if(send_Val < 0){
        perror("Client send message failed");
        exit(1);
    }
    printf("The client has sent the reduction type %s to AWS.\n", reduction_type);
    printf("The client has sent %d numbers to AWS\n", iArr[0]);
}

/*receive final result*/
void receive_final_result()
{
    int recv_Val = recv(clientfd_TCP, pfinal_result, sizeof(final_result), 0);
    if(recv_Val < 0){
        perror("client receive final_result failed");
        exit(0);
    }
    printf("The client has received reduction %s: %d\n", reduction_type, final_result);
}


/********************main***********************/
int main (int argc, char *argv[]) {
    prep_set();
    create_socket_client_TCP();
    connect_AWS();
    
    /*Observe that max, min, sum, sos have 
    different character in the middle position,
    so we just need to distinguish them by the
    second charactar in each argument*/
    if(argv[1][1] == 'i'){
        function = 0;
        //reduction_type = "MIN";
        strcpy(reduction_type, "MIN");
    }else if(argv[1][1] == 'a'){
        function = 1;
        strcpy(reduction_type, "MAX");
    }else if(argv[1][1] == 'u'){
        function = 2;
        strcpy(reduction_type, "SUM");
    }else{
        function = 3;
        strcpy(reduction_type, "SOS");
    }
    read_nums();
    send_message();
    receive_final_result();
    close(clientfd_TCP);
    return 0;
}
