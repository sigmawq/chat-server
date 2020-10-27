#include <string.h>
#include <iostream>
#include <queue>
#include <vector>
#include <list>
#include <unordered_map>
//
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

// Barebone variables
int       server_socket;
addrinfo  *server_info;
addrinfo  hints;
const int yes = 1;
const int no = 0;
fd_set    main_socket_set_read;
fd_set    main_socket_set_read_tmp;
int       max_socket = server_socket;
char      iobuffer[1024];


std::queue<message_proto> awating_messages;
std::queue<int> awaiting_messages_senders;
std::unordered_map<int, std::string> socket_to_user_binding;

// Initialize server
static void Init(std::string port){
    // Create hints to use in address request
    memset(&hints, 0, sizeof(hints));
    hints.ai_addr = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int code = getaddrinfo(NULL, port.c_str(), &hints, &server_info);
    if (code != 0){
        ErrorOut_Custom(__LINE__, __FILE__, code);
        std::terminate();
    }
    ASSERT(server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol));
    ASSERT(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (const void*)&yes, sizeof yes));
    ASSERT(bind(server_socket, server_info->ai_addr, server_info->ai_addrlen));
    ASSERT(listen(server_socket, 10));

    // Get socket sets ready
    FD_ZERO(&main_socket_set_read);
    FD_ZERO(&main_socket_set_read_tmp);
    FD_SET(server_socket, &main_socket_set_read);
    max_socket = server_socket;

    freeaddrinfo(server_info);
    std::cout << "Server intialized successfully\n";
}

static std::string GetDottedIP(sockaddr* address_info){
    char ip_buff[INET6_ADDRSTRLEN];
    if (address_info->sa_family == AF_INET){
        auto sinaddr = ((sockaddr_in*)address_info)->sin_addr;
        inet_ntop(address_info->sa_family, &sinaddr, ip_buff, INET_ADDRSTRLEN);
    }
    else{
        auto sinaddr = &((sockaddr_in6*)address_info)->sin6_addr;
        inet_ntop(address_info->sa_family, sinaddr, ip_buff, INET6_ADDRSTRLEN);
    }
    return std::string(ip_buff);

}

void AddNewSocket(int new_socket){
    FD_SET(new_socket, &main_socket_set_read);
    if (max_socket < new_socket){
        max_socket = new_socket;
    }
}

void RemoveSocket(int socket){
    FD_CLR(socket, &main_socket_set_read);
    std::cout << "Lost connection to socket " << socket << std::endl;
}

static void HandleServerSocket(){
    sockaddr_storage new_client_info;
    socklen_t length = sizeof new_client_info;
    int new_socket = accept(server_socket, (sockaddr*)&new_client_info, &length);
    ASSERT(new_socket);
    AddNewSocket(new_socket);
    std::cout << "Got new connection from: " << GetDottedIP((sockaddr*)&new_client_info) << std::endl;
    std::cout << "New connection assigned to socket: " << new_socket << std::endl;
}

void ProcessInputFromSocket(int socket){
    message_proto new_msg;
    int bytes_read = 0;
    bytes_read = read(socket, &new_msg, sizeof new_msg);
    ASSERT(bytes_read);

    // Zero bytes read => client disconnected, remove assigned socket.
    if (bytes_read == 0){
        RemoveSocket(socket);
        return; 
    }
    std::cout << "Socket " << socket << " sent a message (length: " << bytes_read
    << ")" << ": \nName:";
    Out(new_msg.get_name().c_str()); Out("\n");
    Out(new_msg.get_msg().c_str()); Out("\n");
    awaiting_messages_senders.push(socket); 
    awating_messages.push(new_msg);
}

static void PerformMainCycle(){
    main_socket_set_read_tmp = main_socket_set_read;
    ASSERT(select(max_socket + 1, &main_socket_set_read_tmp, NULL, NULL, NULL));
    for (int current_socket = 0; current_socket <= max_socket; current_socket++){
        if (FD_ISSET(current_socket, &main_socket_set_read_tmp)){
            if (current_socket == server_socket){
                HandleServerSocket();
                return;
            }
            ProcessInputFromSocket(current_socket);
        }
    }
}

void SendMsgToAll(std::string& msg){
    main_socket_set_read_tmp = main_socket_set_read;
    timeval wait {0, 10000}; // Wait 0.01 second 
    ASSERT(select(max_socket + 1, NULL, &main_socket_set_read_tmp, NULL, &wait));
    for (int current_socket = 0; current_socket <= max_socket; current_socket++){
        if (FD_ISSET(current_socket, &main_socket_set_read_tmp)){
            send(current_socket, msg.c_str(), msg.size(), NULL);
        }
    }
}

void SendMsgToAll(message_proto& msg, int sender_socket){
    main_socket_set_read_tmp = main_socket_set_read;
    timeval wait {0, 10000}; // Wait 0.01 second 
    ASSERT(select(max_socket + 1, NULL, &main_socket_set_read_tmp, NULL, &wait));
    for (int current_socket = 0; current_socket <= max_socket; current_socket++){
        if (FD_ISSET(current_socket, &main_socket_set_read_tmp) &&
            current_socket != sender_socket){
            send(current_socket, &msg, sizeof msg, NULL);
        }
    }
}

void HandleMessageQueue(){
    while (awating_messages.size() > 0){
        message_proto msg = awating_messages.back();
        awating_messages.pop();
        int sender_socket = awaiting_messages_senders.back();
        awaiting_messages_senders.pop();
        SendMsgToAll(msg, sender_socket);
    }
}

int main(int argc, char* argv[]){
    message_proto a("Tom", "Hi");
    std::cout << "---------------------Chat Server---------------------" << std::endl;
    const char* server_port = "3940";
    if (argc == 2){
        server_port = argv[1];
    }
    std::cout << "Using server port: " << server_port << std::endl;

    Init(server_port);
    while (true){
        PerformMainCycle();
        HandleMessageQueue();
    }
    return 0;
}
