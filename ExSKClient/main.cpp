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
const uint8_t HEADER_SIZE = sizeof(uint32_t);

int g_skClient; // Client socket


bool SendMessageHandler(const int8_t* uMessage)
{
    if (!uMessage)
        return false;
    
    uint32_t uSendSize = 0;
    uint32_t uLenData = 0;
    uint32_t uLenSend = 0;
    uint8_t uSendBuffer[IO_MSG_MAX + 5] = {0};
    
    uLenData = htonl(strlen((char*)uMessage));
    memcpy(uSendBuffer, &uLenData, HEADER_SIZE); // Set 4 bytes data length header
    memcpy(uSendBuffer + HEADER_SIZE, uMessage, strlen((char*)uMessage)); // Set data
    uLenSend = HEADER_SIZE  + strlen((char*)uMessage);
    
//    cout << strlen((char*)uSendMessage) << endl; output uncorrect because of 4 bytes header
    uSendSize = send(g_skClient, uSendBuffer, uLenSend, 0);    
    if (uSendSize != uLenSend)
    {
        perror("send failed:");
        return false;
    }
    
    return true;
}

std::string RecvMessageHandler()
{
    uint32_t uRecvSize;
    uint32_t uLenData = 0;
    uint8_t uRecvBuffer[6] = {0};
    uint8_t uBytes = 0;
    std::string strRecv;
    
    // Receive 4 bytes header
    uRecvSize = recv(g_skClient, &uLenData, HEADER_SIZE, 0);
    if (uRecvSize != HEADER_SIZE) {
        perror("recv header failed: ");
        return "";
    }
    uLenData = ntohl(uLenData);
    if (!uLenData || (uLenData >= RECV_MSG_MAX))
        return "";

    //Receive a reply from the server
    while(uLenData > 0)
    {
        uBytes = std::min(uLenData, (uint32_t)(sizeof(uRecvBuffer) - 1));
        
        memset(uRecvBuffer, 0, sizeof(uRecvBuffer));
        uRecvSize = recv(g_skClient, uRecvBuffer, uBytes, 0);
        if (uRecvSize != uBytes) {
            perror("recv data failed: ");
            return "";
        }
        
        strRecv += std::string((char*)uRecvBuffer);
        uLenData -= uBytes;
    }

    return strRecv;
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
        
        // Reset buffer
        memset(uClientMessage, 0, strlen((char*)uClientMessage));
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
    }

    close(g_skClient);
    return 0;
}

