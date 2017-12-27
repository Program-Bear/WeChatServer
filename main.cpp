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
#define SOCK_PORT 9886
#define BUFFER_LENGTH 1024*1024*100
#define MAX_CONN_LIMIT 512
#define MAX_CLIENT 200
#define CHUNK_SIZE 1024
#define BUFFER_SIZE 2048
#define IP "127.0.0.1"
#include <sys/stat.h>

unsigned long get_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size;
    }
    return filesize;
}
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

Client* get_client(const char* user_name){
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
void getData(){
    struct timeval tv;
    tv.tv_sec = 0.02;
    tv.tv_usec = 0;
    bool is_file = false;

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
            char* buf = new char[BUFFER_SIZE];
            bzero(buf, BUFFER_SIZE);

            int len = recv(*it, buf, BUFFER_SIZE, 0);

            if (len == 0){
                shutdown(*it, SHUT_RDWR);
            }else {
                printf("get message from %d\n", *it);

                //cout << "After send size: " << strlen(buf + 24) << endl;

                int* command_place = new int();
                memcpy(command_place, buf, 4);
                int now_command = *command_place;

                int * total_len_place = new int();
                memcpy(total_len_place,buf + 220, 4);
                int now_len = *total_len_place;
                if(now_len == 0){
                    cout << "empty packet" << endl;
                    continue;
                }

                if(now_command > 200 || now_command < 0) {
                   // cout << buf << endl;
                    cout << "--------------ERROR----------------------" << endl;
                    continue;
                }

                Protocal *now_protocal = new Protocal(buf);
                COMMAND_DEF command = now_protocal ->get_command();
                cout << "command is: " << command << endl;
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

                    case FILE_START:
                    {
                        if (is_file) {
                            break;
                        }
                        is_file = true;
                        int source_id = now_protocal ->get_source();

                        string file_name = now_protocal ->get_file_name();
                        string user_name = now_protocal ->get_user_name();

                        cout << "file_name: " << file_name << " user_name: " << user_name << endl;
                        Client* target_client = get_client(user_name.c_str());
                        Client* source_client = get_client(now_protocal ->get_source());

                        string now_path = "./temp/" + string(file_name);

                        cout << "file_path: " << now_path << endl;

                        if(target_client == NULL){
                            cout << "Exception" << endl;
                            break;
                        }
                        FILE* fp = fopen(now_path.c_str(),"wb+");

                        char* buffer = new char[BUFFER_SIZE];
                        bzero(buffer,BUFFER_SIZE);
                        int length = recv(*it, buffer, BUFFER_SIZE,0);

                        do{
                            cout << "get file packet" << endl;
                            cout << length << endl;
                            fwrite(buffer, sizeof(char), length, fp);
                            //if(length < BUFFER_SIZE) break;
                            bzero(buffer, BUFFER_SIZE);
                            if(length != 1024 && length != 2048) break;

                            length = recv(*it, buffer, BUFFER_SIZE,0);
                            cout << "next judge: " << length << endl;
                        }while(length > 0);
                        cout << "File end" << endl;

                        target_client->set_file_buffer(now_path,source_client ->get_name(),file_name.c_str());

                        Protocal* file_end = new Protocal(FILE_END_ACK,SUCCESS,-1,-1,NULL,0);
                        cout << "Send message to: " << source_client->get_id() << endl;
                        send(source_client ->get_id() ,file_end ->send_data(), file_end->get_length(), 0);

                        if(target_client ->log_in()){
                            Protocal*new_file = new Protocal(NEW_FILE,SUCCESS,-1,-1,NULL,0);
                            send(target_client->get_id(),new_file ->send_data(),new_file->get_length(),0);
                        }

                        fclose(fp);
                        cout << "close file" << endl;
                        is_file = false;
                        break;

                    }
                    case TESTFILE:{
                        int source_id = now_protocal ->get_source();
                        int ack = FAIL;
                        Client* source_client = get_client(source_id);
                        //string now_path = source_client ->get_file_path();
                        string now_path = "./temp/Archive.zip";
                        string file_name = source_client ->get_file_name();
                        //string file_name = "Archive.zip";
                        string user_name = source_client ->get_fileuser_name();
                        //string user_name = "weijy2";
                        cout << "path: " << now_path << " file_name: " << file_name << " user_name: " << user_name << endl;
                        //source_client ->set_file_buffer("./temp/Archive.zip", "Archive.zip", "weijy2");

                        if(access(now_path.c_str(),F_OK) == 0){
                            cout << "here" << endl;
                            int size = get_file_size(now_path.c_str());
                            Protocal* refile_start = new Protocal(REFILE_ACK,SUCCESS,-1,size,"",file_name,user_name,0);
                            send(source_id, refile_start ->send_data(), refile_start ->get_length(), 0);
                            cout << "Refile Transfer Start" << endl;

                            char* chunk = new char[CHUNK_SIZE];
                            int length = 0;
                            bzero(chunk,CHUNK_SIZE);


                            FILE* fp = fopen(now_path.c_str(),"rb");
                            //int temp = 0;
                            while((length = fread(chunk,sizeof(char),CHUNK_SIZE,fp)) > 0){
                                cout << "Send Packet of length: " << length << endl;
                                if(send(source_id,chunk,length,0) == -1){
                                    cout << "Socket Error" << endl;
                                }
                                //temp += length;
                                bzero(chunk,length);
                            }
                            //cout << "temp: " << temp << endl;
                            cout << "Send finish" << endl;
                            fclose(fp);
                        }else{
                            cout << "Invalid file path!" << endl;
                            Protocal* re_file = new Protocal(REFILE_ACK,FAIL,-1,-1,NULL,0);
                            send(source_id,re_file ->send_data(),re_file->get_length(),0);
                        }

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
                    case REFILE:
                    {
                        int source_id = now_protocal ->get_source();
                        int ack = FAIL;
                        Client* source_client = get_client(source_id);
                        //source_client ->set_file_buffer("./temp/Archive.zip", "weijy2", "Archive.zip");
                        string now_path = source_client ->get_file_path();
                        //string now_path = "./temp/Archive.zip";
                        string file_name = source_client ->get_file_name();
                        //string file_name = "Archive.zip";
                        string user_name = source_client ->get_fileuser_name();
                        //string user_name = "weijy2";
                        cout << "path: " << now_path << " file_name: " << file_name << " user_name: " << user_name << endl;
                        //source_client ->set_file_buffer("./temp/Archive.zip", "Archive.zip", "weijy2");

                        if(access(now_path.c_str(),F_OK) == 0){
                            cout << "here" << endl;
                            int size = get_file_size(now_path.c_str());
                            Protocal* refile_start = new Protocal(REFILE_ACK,SUCCESS,-1,size,"",file_name,user_name,0);
                            send(source_id, refile_start ->send_data(), refile_start ->get_length(), 0);
                            cout << "Refile Transfer Start" << endl;

                            char* chunk = new char[CHUNK_SIZE];
                            int length = 0;
                            bzero(chunk,CHUNK_SIZE);


                            FILE* fp = fopen(now_path.c_str(),"rb");
                            int temp = 0;
                            while((length = fread(chunk,sizeof(char),CHUNK_SIZE,fp)) > 0){
                                //cout << "Send Packet of length: " << length << endl;
                                if(send(source_id,chunk,length,0) == -1){
                                    cout << "Socket Error" << endl;
                                }
                                temp += length;
                                bzero(chunk,length);
                            }
                            cout << "temp: " << temp << endl;
                            cout << "Send finish" << endl;
                            fclose(fp);
                            source_client ->flush_file_buffer();
                        }else{
                            cout << "Invalid file path!" << endl;
                            Protocal* re_file = new Protocal(REFILE_ACK,FAIL,-1,-1,NULL,0);
                            send(source_id,re_file ->send_data(),re_file->get_length(),0);
                        }

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