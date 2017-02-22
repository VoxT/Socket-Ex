/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: root
 *
 * Created on February 13, 2017, 10:57 AM
 */

#include <cstdlib>
#include <unistd.h>
#include <stdio.h> //printf
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <iostream>

using namespace std;

const uint32_t IO_MSG_MAX = 2000;
const uint32_t RECV_MSG_MAX = 2055;

int g_skClient; // Client socket


bool SendMessageHandler(const int8_t* uMessage)
{
    if (!uMessage)
        return false;
    
    int32_t nSendSize = 0;
    int8_t uSendMessage[IO_MSG_MAX+5] = {0};
    
    uint32_t nStringLength = strlen((char*)uMessage);
    uint32_t nDataLength = htonl(nStringLength);

    memcpy(uSendMessage, &nDataLength, sizeof(int32_t)); // Set 4 bytes data length header
    memcpy(uSendMessage+4, uMessage, nStringLength); // Set data
    
//    cout << strlen((char*)uSendMessage) << endl; output uncorrect because of 4 bytes header
    nSendSize = send(g_skClient, uSendMessage, (nStringLength + 4), 0);
    
    // Reset buffer
//    memset(uSendMessage, 0, sizeof(uSendMessage));
    
    if ((nSendSize < 0) || (nSendSize != (nStringLength + 4)) )
    {
        perror("send failed:");
        return false;
    }
    
    return true;
}

std::string RecvMessageHandler()
{
    int32_t nRecvSize;
    uint32_t nMessageLength = 0;
    int8_t uServerReply[RECV_MSG_MAX] = {0};
    
//    memset(uServerReply, 0, strlen((char*)uServerReply));
    
    // Receive 4 bytes header
    nRecvSize = recv(g_skClient, uServerReply, 4, 0);
    if (nRecvSize < 0) {
        perror("recv failed: ");
        return "";
    }
    memcpy(&nMessageLength, uServerReply, 4);
    nMessageLength = ntohl(nMessageLength);
    if (!nMessageLength || (nMessageLength >= RECV_MSG_MAX))
        return "";

    //Receive a reply from the server
    memset(uServerReply, 0, 4);
    nRecvSize = recv(g_skClient, uServerReply, nMessageLength, 0);
    if ((nRecvSize < 0) || (nRecvSize != nMessageLength)) {
        perror("recv failed: ");
        return "";
    }

    if (nRecvSize == 0)
    {
        std::cout << "server timeout!" << endl;
        return "";
    }

    return (char*)uServerReply;
}

/*  
 * 
 */
int main(int argc, char** argv) {
    
    struct sockaddr_in saServer;
    int8_t uClientMessage[IO_MSG_MAX] = {0};

    //Create socket
    g_skClient = socket(AF_INET, SOCK_STREAM, 0);
    if (g_skClient == -1) {
        std::cout << "Could not create socket" << endl;
        return 0;
    }
    std::cout << "Socket created" << endl;

    saServer.sin_addr.s_addr = inet_addr("127.0.0.1");
    saServer.sin_family = AF_INET;
    saServer.sin_port = htons(8888);
    
    //Connect to remote server
    if(connect(g_skClient, (struct sockaddr *) &saServer, sizeof (saServer)) < 0) {
        perror("connect failed. Error");
        close(g_skClient);
        return 0;
    }
    std::cout << "Connected" << endl;
        
    struct timeval tvTimeout;
    tvTimeout.tv_sec = 600;
    tvTimeout.tv_usec = 0;
    
    // set timeout
    if (setsockopt(g_skClient, SOL_SOCKET, SO_RCVTIMEO, (char *)&tvTimeout,sizeof(struct timeval)) < 0)
    {
        perror("set socket option failed!");
        close(g_skClient);
        return 0;
    }

    //keep communicating with server
    while(true) {
        std::cout << "Enter message: ";
        std::cin.getline((char*)uClientMessage, IO_MSG_MAX);
        
        if (!strcmp((char*)uClientMessage, "stop"))
            break;

        // Send message
        if (!SendMessageHandler(uClientMessage))
            break;
        
        // Receive Msg
        std::string strResponseMsg = RecvMessageHandler();
        if(strResponseMsg.empty())
            break;
        
        std::cout << "Server reply: " << strResponseMsg << endl;
        // Reset buffer
        memset(uClientMessage, 0, strlen((char*)uClientMessage));
    }

    close(g_skClient);
    return 0;
}

