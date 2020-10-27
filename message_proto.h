#pragma once
#include <cstring>
#include "error_handling.h"

class message_proto{
    char _buff[32 + 255];
    public:
    message_proto(const char* name, const char* msg)
    {
        memset(_buff, 0, sizeof _buff);
        strcpy(_buff + 0, name);
        strcpy(_buff + 32, msg);
    }
    message_proto() {
        memset(_buff, 0, sizeof _buff);
    }
    std::string get_name() const{
        return std::string(_buff, 32);
    }   
    std::string get_msg() const{
        return std::string(_buff + 32);
    }
};

// sends message_proto obj
void msgp_send(int sock, message_proto msgp){
    ASSERT(send(sock, &msgp, sizeof msgp, NULL));
}

// receive message_proto obj, store in msg_container
void msgp_recv(int sock, message_proto& msg_container){
    int bytes_recv = recv(sock, &msg_container, sizeof msg_container, NULL);
    ASSERT(bytes_recv);
}