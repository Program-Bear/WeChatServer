//
// Created by victor on 23/12/2017.
//

#ifndef WECHAT_CLIENT_H
#define WECHAT_CLIENT_H
#include <list>
using namespace std;

class Client{
    char* user_name;
    char* password;
    char* message_buffer;
    char* name_buffer;

    char* file_name_buffer;
    char* file_user_name_buffer;
    string file_path;

    int socket_id;
    bool in_log;
    bool in_chat;
    int chat_target;
    char* chat_name;

    list<Client *> *friends;
public:
    Client(){
    }

    Client(char* _user_name, char* _pass_word, int _id){
        user_name = new char[100];
        password = new char[100];
        memcpy(user_name,_user_name,strlen(_user_name));
        memcpy(password,_pass_word,strlen(_pass_word));

        message_buffer = new char[1024];
        file_path = "";
        file_name_buffer = new char[100];
        file_user_name_buffer = new char[100];
        name_buffer = new char[100];
        chat_name = new char[100];

        socket_id = _id;
        in_log = false;
        in_chat = false;
        chat_target = -1;
        friends = new list<Client*> ();

    }

    ~Client(){
        delete user_name;
        delete password;
        delete message_buffer;
        delete file_name_buffer;
        delete file_user_name_buffer;
        delete chat_name;
        delete name_buffer;
        std:: list<Client*>:: iterator it;
        for(it = friends ->begin(); it != friends ->end(); it++){
            delete *it;
        }
        delete friends;
    }

    char* get_message_buffer(){
        char* buffer = new char[1024];
        memcpy(buffer, message_buffer, strlen(message_buffer));
        return buffer;
    }

    string get_file_path(){
        return file_path;
    }
    char* get_fileuser_name(){
        char* buffer = new char[100];
        memcpy(buffer, file_user_name_buffer, strlen(file_user_name_buffer));
        return buffer;
    }
    char* get_file_name(){
        char* buffer = new char[100];
        memcpy(buffer, file_name_buffer, strlen(file_name_buffer));
        return buffer;
    }

    char* get_name_buffer(){
        char* buffer = new char[1024];
        memcpy(buffer, name_buffer, strlen(name_buffer));
        return buffer;
    }

    void set_message_buffer(char* buffer){
        memcpy(message_buffer, buffer, strlen(buffer));
    }
    void set_name_buffer(char* buffer){
        memcpy(name_buffer, buffer, strlen(buffer));
    }
    void set_file_buffer(string _path, const char* user_name, const char* file_name){
        memcpy(file_user_name_buffer, user_name, strlen(user_name));
        memcpy(file_name_buffer, file_name, strlen(file_name));
        cout << "set username finish " << "name is "  << user_name << endl;
        file_path = _path;
        cout << "set filename finish " << "name is " << file_name << endl;
    }
    void flush_file_buffer(){
        file_path = "";
        bzero(file_user_name_buffer,100);
        bzero(file_name_buffer,100);
    }
    int get_id(){
        return socket_id;
    }
    char* get_name(){
        char* buffer = new char[100];
        memcpy(buffer, user_name, strlen(user_name));
        return buffer;
    }
    char* get_password(){
        char* buffer = new char[100];
        memcpy(buffer, password, strlen(password));
        return buffer;
    }

    bool check_password(char* target){
        return (strcmp(target, password) == 0);
    }

    void set_log(bool _log){
        in_log = _log;
    }

    void set_chat(bool _chat){
        in_chat = _chat;
    }
    void set_id(int _id){
        socket_id = _id;
    }
    void set_target(int _id){
        chat_target = _id;
    }

    void set_chat_name(char* _name){
        memcpy(chat_name,_name,strlen(_name));
    }

    char* get_target_name(){
        char* buffer = new char[100];
        memcpy(buffer, chat_name, strlen(chat_name));
        return buffer;
    }

    int get_target(){
        return chat_target;
    }

    bool log_in(){
        return in_log;
    }

    bool chat_in(){
        return in_chat;
    }
    bool add_friend(Client* now_friend){
        friends ->push_back(now_friend);
        return true;
    }


    list<Client*> * get_frineds(){
        return friends;
    }
    bool is_friend(Client* temp){
        std::list<Client*>::iterator it;
        for(it = friends->begin(); it != friends->end(); it++){
            Client* now_client = *it;
            if (strcmp(temp ->get_name(),now_client ->get_name()) == 0) return true;
        }
        return false;
    }


};

#endif //WECHAT_CLIENT_H
