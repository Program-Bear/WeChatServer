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
#include "Client.h"
#include "Protocal.h"
#define SOCK_PORT 9890
#define BUFFER_LENGTH 1024*1024*100
#define MAX_CONN_LIMIT 512
#define MAX_CLIENT 200
#define IP "127.0.0.1"

using namespace std;

std:: list<int> current_socket;
std:: list<Client*> current_client;

Client* get_client(int socket_id){
    std:: list<Client*>:: iterator it;
    for(it = current_client.begin(); it != current_client.end(); it++){
        Client* now_client = *it;
        if(socket_id == now_client ->get_id()){
            return now_client;
        }
    }
    return NULL;
}

int get_socket(char* user_name){
    std::list <Client*>:: iterator it;
    for(it = current_client.begin(); it != current_client.end(); it++){
        Client* now_client = *it;
        if (strcmp(user_name, now_client ->get_name()) == 0){
            return now_client ->get_id();
        }
    }
    return -1;
}

Client* get_client(char* user_name){
    std::list <Client*>::iterator it;
    for(it = current_client.begin(); it != current_client.end();it++){
        Client* now_client = *it;
        if(strcmp(user_name, now_client ->get_name()) == 0){
            return now_client;
        }
    }
    return NULL;
}


bool check_name(char* temp_name){
    bool answer = false;
    std::list <Client*>:: iterator it;
    for(it = current_client.begin(); it != current_client.end(); it++){
        Client* now_client = *it;
        if (strcmp(now_client ->get_name(), temp_name) == 0){
            answer = true;
            break;
        }
    }
    return answer;
}

/*
void sendMess(int target, char* content, int len){
    char *buf = new char[len];
    memset(buf,0,sizeof(buf));
    memcpy(buf, content, len);
    cout << "begin to send" << endl;
    send(target, buf, len, 0);
}
*/
char buf[BUFFER_LENGTH];
void getData(){
    struct timeval tv;
    tv.tv_sec = 0.05;
    tv.tv_usec = 0;
    std::list<int>:: iterator it;
    for (it = current_socket.begin(); it != current_socket.end(); it++){
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
            //cout << "No Message get!" << endl;
        }
        else{
            memset(buf, 0, sizeof(buf));
            int len = recv(*it, buf, sizeof(buf), 0);
            if (len == 0){
                //printf("disconnect of %d", *it);
                shutdown(*it, SHUT_RDWR);
            }else {
                printf("get message from %d\n", *it);
               // cout << "After send total length" << buf+20 << endl;
                cout << "After send size: " << strlen(buf + 24) << endl;
                Protocal* now_protocal = new Protocal(buf);

                COMMAND_DEF command = now_protocal ->get_command();
                switch(command){
                    case SIGN_UP:
                    {
                        int socket_id = now_protocal ->get_source();
                        int ack = SUCCESS;
                        char*buffer_data = now_protocal ->get_data();
                        char* temp = strtok(buffer_data, "&");
                        char* _name = temp;
                        if (check_name(_name)){
                             ack = SAME_NAME;
                        }else {
                            temp = strtok(NULL, "&");
                            char *_password = temp;
                            Client *new_client = new Client(_name, _password, -1);
                            if (current_client.size() < MAX_CLIENT) {
                                current_client.push_back(new_client);
                            } else {
                                ack = TOO_MANY_CLIENT;
                            }
                        }
                        Protocal* sign_ack = new Protocal(SIGN_UP_ACK,ack,-1,-1,NULL,0);
                        char* send_data = sign_ack ->send_data();
                        send(socket_id, send_data, sign_ack ->get_length(), 0);
                        //delete sign_ack;
                        break;
                    }

                    case LOG_IN:
                    {
                        int socket_id = now_protocal -> get_source();
                        char* buffer_data = now_protocal -> get_data();
                        char* temp = strtok(buffer_data, "&");
                        char* _name = temp;
                        cout << _name << endl;
                        temp = strtok(NULL, "&");
                        char* _password = temp;

                        list<Client*>:: iterator it;
                        int ack = BAD_USER;
                        for(it = current_client.begin(); it != current_client.end(); it++){
                            Client* now_client = *it;
                            //cout << now_client ->get_name() << endl;
                            if (strcmp(now_client ->get_name(), _name) == 0){
                                if(!now_client ->check_password(_password)){
                                    ack = BAD_PASSWORD;
                                }else if(now_client ->log_in()){
                                    ack = RE_LOG;
                                }else{
                                    ack = SUCCESS;
                                    now_client ->set_log(true);
                                    now_client ->set_id(socket_id);
                                }
                            }
                        }
                        Protocal* log_ack = new Protocal(LOG_IN_ACK, ack, -1,-1, NULL,0);
                        char* send_data = log_ack ->send_data();
                        send(socket_id, send_data, log_ack ->get_length(), 0);
                        //delete log_ack;
                        break;
                    }

                    case LOG_OUT:
                    {
                        int socket_id = now_protocal ->get_source();
                        list<Client*>:: iterator it;
                        int ack = BAD_USER;
                        for(it = current_client.begin(); it != current_client.end(); it++){
                            Client* now_client = *it;
                            if(now_client ->get_id() == socket_id){
                                if (!now_client ->log_in()){
                                    ack = BAD_LOGOUT;
                                }else{
                                    ack = SUCCESS;
                                    now_client ->set_log(false);
                                    now_client ->set_id(-1);
                                }
                            }
                        }
                        Protocal* logout_ack = new Protocal(LOG_OUT_ACK, ack, -1,-1,NULL,0);
                        char* send_data = logout_ack ->send_data();
                        send(socket_id, send_data, logout_ack ->get_length(), 0);
                        //delete logout_ack;
                        break;
                    }

                    case ADD:
                    {
                        int source_id = now_protocal -> get_source();
                        Client* source_client = get_client(source_id);
                        Client* target_client = NULL;
                        char* target_name = now_protocal ->get_data();
                        list<Client*>:: iterator it;
                        int ack = BAD_USER;
                        if (strcmp(target_name, source_client ->get_name()) == 0){
                            ack = ADD_SELF;
                        }else {

                            for (it = current_client.begin(); it != current_client.end(); it++) {
                                target_client = *it;
                                if (strcmp(target_client->get_name(), target_name) == 0) {
                                    ack = SUCCESS;
                                    target_client->add_friend(source_client);
                                    source_client->add_friend(target_client);
                                    int target_id = target_client->get_id();
                                    char *source_name = source_client->get_name();

                                    Protocal *target_ack = new Protocal(ADD_ACK, ack, -1, -1, source_name,
                                                                        strlen(source_name));
                                    char *target_data = target_ack->send_data();
                                    send(target_id, target_data, target_ack->get_length(), 0);
                                    // delete target_ack;
                                    break;
                                }
                            }

                        }
                        Protocal* source_ack = new Protocal(ADD_ACK, ack, -1,-1, target_name, strlen(target_name));
                        char* source_data = source_ack ->send_data();
                        send(source_id, source_data, source_ack ->get_length(),0);
                       // delete source_ack;
                        break;
                    }

                    case LS: {
                        int socket_id = now_protocal ->get_source();
                        Client* now_client = get_client(socket_id);
                        list<Client*>:: iterator it;
                        list<Client*> *friend_list = now_client ->get_frineds();
                        int ack = SUCCESS;
                        char* buffer = new char[MAX_CLIENT * 100];
                        int offset = 0;

                        for(it = friend_list ->begin(); it != friend_list ->end(); it++){
                            Client* now_client = *it;
                            char* now_name = now_client ->get_name();
                            cout << "get name of" << now_name << endl;
                            memcpy(buffer + offset, now_name, strlen(now_name));
                            offset += strlen(now_name);
                            memcpy(buffer + offset, "&", 1);
                            offset += 1;
                        }
                        cout << "buffer is " << buffer << endl;
                        Protocal* list_ack = new Protocal(LS_ACK, ack, -1,-1,buffer, strlen(buffer));
                        char* send_data = list_ack ->send_data();
                        send(socket_id, send_data, list_ack ->get_length(), 0);
                        break;
                    }

                    case SEARCH:
                    {
                        int socket_id = now_protocal ->get_source();
                        list<Client*>:: iterator it;
                        int ack = SUCCESS;
                        char* buffer = new char[MAX_CLIENT*100];
                        int offset = 0;
                        for(it = current_client.begin(); it != current_client.end(); it++){

                            Client* now_client = *it;
                            char* now_name = now_client ->get_name();
                            cout << "get name of: " << now_name << endl;
                            memcpy(buffer+offset, now_name, strlen(now_name));
                            offset += strlen(now_name);
                            memcpy(buffer+offset, "&", 1);
                            offset += 1;
                        }
                        cout << "buffer is: " << buffer << endl;
                        Protocal* search_ack = new Protocal(SEARCH_ACK, ack, -1,-1, buffer, strlen(buffer));
                        char* send_data = search_ack ->send_data();
                        send(socket_id, send_data, search_ack ->get_length(), 0);
                       // delete search_ack;
                        break;
                    }

                    case CHAT:
                    {
                        int source_id = now_protocal ->get_source();
                        list<Client*>:: iterator it;
                        char* target_name = now_protocal ->get_data();
                        int ack = SUCCESS;
                        int target_id = -1;
                        Client* target_client = get_client(target_name);
                        Client* source_client = get_client(source_id);
                        char* source_name = source_client ->get_name();

                        if(target_client == NULL){
                            ack = BAD_USER;
                        }else{
                            if(!source_client ->is_friend(target_client)){
                                ack = NOT_FRIEND;
                            }else{
                                if (!target_client ->log_in()) {
                                    ack = OFF_LINE;
                                    target_id = -1;
                                }else{
                                    target_id = target_client ->get_id();
                                    target_client ->set_chat(true);
                                    target_client ->set_target(source_id);
                                    source_client ->set_target(target_id);
                                    Protocal* chat_ack = new Protocal(CHAT_ACK, ack, -1, source_id, source_name,strlen(source_name));
                                    send(target_id, chat_ack->send_data(), chat_ack ->get_length(), 0);
                                }

                            }
                        }
                        Protocal* chat_ack = new Protocal(CHAT_ACK, ack, -1, target_id, target_name, strlen(target_name));
                        send(source_id, chat_ack ->send_data(), chat_ack ->get_length(), 0);
                        source_client ->set_chat(true);
                        source_client ->set_chat_name(target_name);
                        break;
                    }

                    case MESSAGE:
                    {
                        int source_id = now_protocal ->get_source();
                        int target_id = now_protocal ->get_target();
                        char* message = strtok(now_protocal ->get_data(), "&");

                        cout << "message is: " << message << endl;

                        Client* source_client = get_client(source_id);

                        Client* target_client = get_client(source_client ->get_target_name());
                        if(target_id != -1){
                            Protocal* mes = new Protocal(NEW_MESSAGE, SUCCESS, source_id, target_id, message, strlen(message));
                            send(target_id, mes ->send_data(), mes ->get_length(), 0);
                        }else if(target_client != NULL){
                            target_client ->set_message_buffer(message);
                            target_client ->set_name_buffer(source_client ->get_name());
                        }
                        break;
                    }

                    case EXIT:
                    {
                        int source_id = now_protocal ->get_source();
                        Client* source_client = get_client(source_id);
                        int ack = -1;
                        if(source_client ->chat_in()){
                            ack = SUCCESS;
                            source_client ->set_chat(false);
                            if(source_client ->get_target() != -1){
                                Protocal* _exit = new Protocal(EXIT_ACK, SUCCESS, source_id, -1, NULL, 0);
                                send(source_client ->get_target(), _exit ->send_data(), _exit ->get_length(), 0);
                            }
                        }else{
                            ack = FAIL;
                        }
                        Protocal* _exit = new Protocal(EXIT_ACK, ack, source_id, -1, NULL,0);
                        send(source_id, _exit ->send_data(), _exit ->get_length(), 0);
                        break;
                    }

                    case FILESEND:
                    {
                        cout << "After Send: " << now_protocal ->get_length() << endl;
                        int source_id = now_protocal ->get_source();
                        Client* source_client = get_client(source_id);

                        char* content = strtok(now_protocal ->get_data(),"&");
                        char* file_name = strtok(NULL,"&");
                        char* user_name = strtok(NULL, "&");

                        cout << "file_name: " << file_name << " user_name: " << user_name << endl;

                        Client* target_client = get_client(user_name);
                        if(target_client == NULL){
                            cout << "Exception" << endl;
                            break;
                        }

                        string file_path = "./temp/" + string(file_name);

                        cout << "file_path: " << file_path << endl;

                        Protocal::write_file(file_path, content, now_protocal ->get_data_length());

                        cout << "write finish!" << endl;
                        if(target_client ->log_in()){
                            Protocal* _newfile =new Protocal(NEW_FILE, SUCCESS, source_id, -1, file_name, strlen(file_name));
                            send(target_client ->get_id(),_newfile ->send_data(), _newfile ->get_length(), 0);
                        }else{
                            target_client ->set_file_buffer(file_path,user_name,file_name);
                        }

                        Protocal* file_ack = new Protocal(FILE_END_ACK, SUCCESS, source_id, -1,NULL, 0);
                        send(source_client ->get_id(),file_ack ->send_data(), file_ack ->get_length(), 0);
                        break;
                    }

                    case REMESSAGE:
                    {
                        int source_id = now_protocal ->get_source();
                        Client* source_client = get_client(source_id);
                        char* buffer = new char[1024];
                        int ack = -1;
                        if (source_client ->log_in()){
                            ack = SUCCESS;
                            char* msg = source_client ->get_message_buffer();
                            char* name = source_client ->get_name_buffer();
                            cout << "Message: " << msg << endl;
                            cout << "Name: " << name << endl;
                            int offset = 0;
                            memcpy(buffer+offset,msg,strlen(msg));
                            offset += strlen(msg);
                            memcpy(buffer+offset,"&",1);
                            offset += 1;
                            memcpy(buffer+offset,name,strlen(name));
                            cout << "Before send: " << buffer << endl;
                        }else{
                            ack = FAIL;
                        }
                        Protocal* reg_ = new Protocal(OLD_MESSAGE, ack, source_id, -1, buffer, strlen(buffer));
                        send(source_id, reg_ ->send_data(), reg_ ->get_length(), 0);
                        break;
                    }

                    case REFILE:
                    {

                    }

                    case PROFILE:
                    {
                        int source_id = now_protocal ->get_source();
                        Client* source_client = get_client(source_id);
                        char* buffer = new char[205];
                        int ack = -1;
                        if(source_client ->log_in()){
                            ack = SUCCESS;
                            char* name = source_client ->get_name();
                            char* password = source_client ->get_password();
                            int offset = 0;
                            memcpy(buffer+offset, name, strlen(name));
                            offset += strlen(name);
                            memcpy(buffer+offset, "&", 1);
                            offset += 1;
                            memcpy(buffer+offset, password, strlen(password));
                            cout << "Before send" << buffer << endl;
                        }else{
                            ack = FAIL;
                        }
                        Protocal* _profile = new Protocal(PROFILE_ACK, ack, source_id, -1, buffer, strlen(buffer));
                        send(source_id, _profile ->send_data(), _profile ->get_length(), 0);
                        break;
                    }


                }

           // delete now_protocal;

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
    sockaddr_in s_addr_in;
    socklen_t len;

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
                current_socket.push_back(conn);
                Protocal* connect_ack = new Protocal(CONNECT_ACK, SUCCESS, conn, -1, NULL, 0);
                char* send_content = connect_ack ->send_data();
                send(conn, send_content, connect_ack ->get_length(),0);
            }
        }
    }

}