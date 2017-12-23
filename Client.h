//
// Created by victor on 23/12/2017.
//

#ifndef WECHAT_CLIENT_H
#define WECHAT_CLIENT_H
#include <list>
using namespace std;

class Client{
    char* user_name[1024];
    char* password[1024];
    bool in_log;
    bool in_chat;
    int chat_target;
    list<Client*> friends;
public:
    Client(){
    }

    Client(char* user_name, char* pass_word, int id);
    ~Client();

    int sock_id;

    char* get_name();
    char* get_password();

    bool check_password(char* target);

    bool log_in();
    bool log_out();
    bool add_friend(int target);
    void start_chat(int tartget);

    char* sync(); //显示好友信息
    char* profile(); //显示个人账户信息
    char* get_frineds_name(); //获取好友姓名



};

#endif //WECHAT_CLIENT_H
