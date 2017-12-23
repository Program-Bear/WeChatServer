#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <thread>
#include <list>
#define SOCK_PORT 9900
#define BUFFER_LENGTH 1024
#define MAX_CONN_LIMIT 512
#define IP "127.0.0.1"

using namespace std;

std:: list<int> current_client;
sockaddr_in s_addr_in;
socklen_t len;


void sendMess(int target, char* content){
    char buf[1024];
    memset(buf,0,sizeof(buf));
    //fgets(buf,sizeof(buf),stdin);
    strcpy(buf, content);
    std::list<int>:: iterator it;
    cout << "begin to send" << buf << endl;
    for (it=current_client.begin(); it!=current_client.end(); it++){
        if (target == *it){
            send(*it, buf, sizeof(buf), 0);
        }
    }
    memset(buf,0,sizeof(buf));
}
void getData(){
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    std::list<int>:: iterator it;
    for (it = current_client.begin(); it != current_client.end(); it++){
        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = 0;
        int retval = 0;
        FD_SET(*it, &rfds);
        if (maxfd < *it){
            maxfd = *it;
        }
        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);

        if (retval == -1){
            cout << "Select Error!" << endl;
        }else if(retval == 0){
            cout << "No Message get!" << endl;
        }
        else{
            char buf[1024];
            memset(buf,0,sizeof(buf));
            int len = recv(*it, buf, sizeof(buf), 0);
            if (len == 0){
                printf("disconnect of %d", *it);
            }else {
                printf("get message from %d, %s", *it, buf);
                sendMess(5, buf);
            }
        }
    }
}

int main(void){
    int fd_temp;


    int sockfd_server = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd_server == -1){
        cout << "socket error!" << endl;
        exit(1);
        //cout << "socket error!" << endl;
    }

    memset(&s_addr_in, 0, sizeof(s_addr_in));
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = INADDR_ANY;
    s_addr_in.sin_port = htons(SOCK_PORT);

    len = sizeof(s_addr_in);

    fd_temp = bind(sockfd_server, (const sockaddr *)&s_addr_in, len);

    if (fd_temp == -1){
        cout << "bind error!" << endl;
        exit(1);
    }

    fd_temp = bind(sockfd_server, (const sockaddr *)&s_addr_in, len);

    fd_temp = listen(sockfd_server, MAX_CONN_LIMIT);
    if (fd_temp == -1)
    {
        fprintf(stderr, "listen error!\n");
        exit(1);
    }

    cout << "Server established waiting for client!" << endl;

    while(1){
        fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        int maxfd = 0;

        FD_SET(sockfd_server, &rfds);
        if (maxfd < sockfd_server) maxfd = sockfd_server;

        int retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1){
            cout << "select fail" << endl;
            exit(1);
        }else if (retval == 0){
            getData();
        }else{
            struct sockaddr_in client_addr;
            memset(&client_addr ,0,sizeof(client_addr));
            socklen_t  client_addrlen = sizeof(client_addr);
            int conn = accept(sockfd_server, (struct sockaddr*)&s_addr_in, &client_addrlen);

            if (conn != -1) {
                cout << "new client connet with code: " << conn << endl;
                current_client.push_back(conn);
            }
        }
    }

}