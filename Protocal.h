//
// Created by victor on 23/12/2017.
//

#ifndef WECHAT_PROTOCAL_H
#define WECHAT_PROTOCAL_H
#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<string.h>
#include<fstream>
#include"command_def.h"
#include"ack_def.h"

using namespace std;


class Protocal{
    COMMAND_DEF command;
    ACK_DEF ack;
    int source;
    int target;
    int data_len;
    int total_len;
    char* data;
public:
    Protocal(char* raw_data){

        int* command_place = new int();
        memcpy(command_place, raw_data, 4);
        command = *command_place;
    //    delete command_place;

        int * ack_place = new int();
        memcpy(ack_place, raw_data+4, 4);
        ack = *ack_place;
    //    delete ack_place;

        int * src_place = new int();
        memcpy(src_place, raw_data+8, 4);
        source = *src_place;
    //    delete src_place;

        int * dst_place = new int();
        memcpy(dst_place, raw_data+12, 4);
        target = *dst_place;
    //    delete dst_place;

        int * data_len_place = new int();
        memcpy(data_len_place, raw_data+16, 4);
        data_len = *data_len_place;
    //    delete data_len_place;

        int * total_len_place = new int();
        memcpy(total_len_place, raw_data+20, 4);
        total_len = *total_len_place;
    //    delete total_len_place;

        cout << "create protocal with data length: " << data_len << endl;

        data = new char[data_len];

        memcpy(data, raw_data+24, data_len);
        cout << "Finish" << endl;
    }
    Protocal(COMMAND_DEF _command, ACK_DEF _ack, int sid, int tid, char* _data, int len){
        command = _command;
        ack = _ack;
        source = sid;
        target = tid;
        data_len = len;
        if(_data != NULL) {
            //cout << "here data is " << _data << endl;
            //char *buffer = new char[len];
            data = new char[len];
            memcpy(data, _data, len);
        }
        total_len = sizeof(_command) + sizeof(_ack) + sizeof(sid) + sizeof(tid) + sizeof(data_len) + sizeof(total_len) + 200 +len;
    }
    Protocal(COMMAND_DEF _command, ACK_DEF _ack, int sid, int tid, string _data, int len){
        command = _command;
        ack = _ack;
        source = sid;
        target = tid;
        data_len = len;
        if(_data.c_str() != NULL){
            data = new char[len];
            _data.copy(data, len, 0);
        }
        total_len = sizeof(int)*6 + len;
    }
    ~Protocal(){
        if(data != NULL) delete[] data;
    }
    COMMAND_DEF get_command(){
        return command;
    }
    ACK_DEF get_ack(){
        return ack;
    }
    int get_source(){
        return source;
    }
    int get_target(){
        return target;
    }
    int get_length(){
        return total_len;
    }
    int get_data_length(){
        return data_len;
    }
    char* get_data(){
        char* buffer = new char[data_len];
        //cout << "data: " << data << " len: " << data_len << endl;
        memcpy(buffer, data, data_len);
        return buffer;
    }

    char* send_data(){
        char* raw_buffer = new char[total_len];
        memset(raw_buffer, 0, total_len);

        int* temp_command = new int(command);
        memcpy(raw_buffer, temp_command, 4);
        int * temp_ack = new int(ack);
        memcpy(raw_buffer + 4, temp_ack, 4);
        int * temp_src = new int(source);
        memcpy(raw_buffer + 8, temp_src, 4);
        int * temp_dst = new int(target);
        memcpy(raw_buffer + 12, temp_dst, 4);

        int * temp_len = new int(data_len);
        memcpy(raw_buffer + 16, temp_len, 4);

        int * temp_total = new int(total_len);
        memcpy(raw_buffer + 20, temp_total, 4);

        memcpy(raw_buffer + 24, data, data_len);
        return raw_buffer;
    }
    static string read_file(string filename){
        ifstream f(filename, ios::binary);
        string ans((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        f.close();
        return ans;
    }
    static void write_file(string filename, char* data, int len){
        ofstream f(filename, ios::binary);
        f.write(data, len);
        f.close();
    }

};


#endif //WECHAT_PROTOCAL_H
