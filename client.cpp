#include <string.h>
#include <iostream>
#include <thread>
#include <pthread.h>

#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include "message_proto.h"
#include "error_handling.h"
#include "utility.h"

const int yes = 1;
std::string client_name;

int Init(const char* server_hostname, const char* server_port){
    addrinfo *server_info;
    addrinfo hints;
    int sock;

    memset(&hints, 0, sizeof hints);
    hints.ai_addr = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    ASSERT(getaddrinfo(server_hostname, server_port, &hints, &server_info));
    ASSERT((sock = socket(server_info->ai_family,
     server_info->ai_socktype, server_info->ai_protocol)));
    ASSERT(connect(sock, server_info->ai_addr, server_info->ai_addrlen));
    std::cout << "Connected" << std::endl;
    return sock;
}

// Send a string
void SendMsg(std::string msg, int socket){
    ASSERT(send(socket, msg.c_str(), msg.size(), NULL));
}

void RunInputRoutine(int sock){
    while(true){
        char iobuff[255];
        std::cin.getline(iobuff, 255, '\n');
        message_proto msg(client_name.c_str(), iobuff);
        msgp_send(sock, msg);
    }
}

void RunOutputRoutine(int sock){
    message_proto new_msg;
    while (true){
        int bytes_recv;
        bytes_recv = recv(sock, &new_msg, sizeof new_msg, NULL);
        ASSERT(bytes_recv);
        if (bytes_recv > 0){
            Out(new_msg.get_name().c_str());
            Out(": ");
            Out(new_msg.get_msg().c_str()); Out("\n");
        }
        else if (bytes_recv == 0){
            std::cout << "Server closed connection" << std::endl;
            close(sock);
            std::terminate();
            break;
        }
    }
}

int main(int argc, char* argv[]){
    char name[32];
    std::cout << "---------------------Chat Client---------------------" << std::endl;
    std::cout << "Enter your name: "; 
    std::cin.getline(name, 32, '\n');
    client_name = std::string(name);

    const char* server_port = "3940";
    const char* server_hostname = "sigmawq";
    if (argc == 3){
        server_hostname = argv[1];
        server_port = argv[2];
    }
    std::cout << "Using server host name: " << server_hostname << std::endl;
    std::cout << "Using server port: " << server_port << std::endl;

    int server_sock = Init(server_hostname, server_port);
    std::thread task_Input(&RunInputRoutine, server_sock);
    std::thread task_Output(&RunOutputRoutine, server_sock);
    task_Input.join(); task_Output.join();
    return 0;
}