/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: root
 *
 * Created on February 13, 2017, 10:55 AM
 */

#include <cstdlib>
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <iostream>
#include <unistd.h> //sleep(sec)

using namespace std;

const uint32_t MSG_MAX = 2005;

std::string GetFirstWord(const std::string& str)
{
    int nPos = 0;
    if (str.find_first_of(' ') == std::string::npos)
        nPos = str.length();
    else
        nPos = str.find_first_of(' ');
    
    return str.substr(0, nPos);
}

std::string GetLastWord(const std::string& str)
{
    int nPos = 0;
    if (str.find_last_of(' ') != std::string::npos)
        nPos = str.find_last_of(' ') + 1;
    
    return str.substr(nPos, str.length());
}

std::string StringHandler(int& type, const std::string& strOrigin) 
{
    if (type == 1) 
        return GetFirstWord(strOrigin);
    
    if (type == 2)
        return GetLastWord(strOrigin);
    
    return "err";
}

std::string MessageProcess(const std::string& strBuff)
{    
    int uType = atoi(strBuff.substr(0, 1).c_str());
    std::string strData = strBuff.substr(1, strBuff.length() - 1);
    
    return StringHandler(uType, strData) + " is processed.";
}

std::string RecvMessageHandler(const int& skClient)
{
    int32_t nReadSize = 0;
    uint32_t nMessageLength = 0;
    int8_t uClientMessage[MSG_MAX] = {0};
    
//    memset(uClientMessage, 0, strlen((char*)uClientMessage));
     // read 4 bytes (length data)
    nReadSize = recv(skClient , uClientMessage , 4, 0);
    if(nReadSize <= 0)
        return "";

    memcpy(&nMessageLength, uClientMessage, 4);
    nMessageLength = ntohl(nMessageLength);

    if((nMessageLength >= MSG_MAX) || !nMessageLength)
        return "";

    // Read data
    memset(uClientMessage, 0, 4); // Reset buffer
    nReadSize = recv(skClient , uClientMessage , nMessageLength, 0);
    if((nReadSize <= 0) || (nReadSize != nMessageLength))
    {
        perror("recv failed:");
        return "";
    }
    
    return (char*) uClientMessage;
}

/*
 * Return:
 *      true - success
 *      false - failed
 */
bool SendMessageHandler(const std::string& str, int& skClient)
{
    int32_t nSendSize = 0;
    uint32_t nMessageLength = 0;
    int8_t uResponseMessage[MSG_MAX+50] = {0};
    
    // Init response data
    nMessageLength = htonl(str.length());
    memcpy(uResponseMessage, &nMessageLength, sizeof(nMessageLength)); // Set header
    memcpy(uResponseMessage+4, str.c_str(), str.length()); // Set data

    // Send data
    nSendSize = send(skClient , uResponseMessage, str.length()+4, 0);
    
    // Reset buffer
//    memset(uResponseMessage, 0, str.length()+4);
        
    if ((nSendSize < 0) || (nSendSize != (str.length()+4)))
    {
        perror("send failed:");
        return false;
    }
    
    return true;
}

/*
 * This will handle connection for each client
 * */
void *ConnectionHandler(void *skDesc)
{
    if (!skDesc) 
        return NULL;
    
    //Get the socket descriptor
    int skClient = reinterpret_cast<std::uintptr_t>(skDesc);
         
    struct timeval tvTimeout;      
    tvTimeout.tv_sec = 600;
    tvTimeout.tv_usec = 0;
    
    // Set timeout
    if (setsockopt(skClient, SOL_SOCKET, SO_RCVTIMEO, (char *)&tvTimeout, sizeof(tvTimeout)) < 0)
    {
        close(skClient); // always close and free before return
        return NULL;
    }
    
    //Receive a message from client
    while (true)
    {
        // recv msg
        std::string str = RecvMessageHandler(skClient);
        if(str.empty())
            break;
        
        str = MessageProcess(str);
        
        // send response data
        if(!SendMessageHandler(str, skClient))
            break;
        
    }

    close(skClient);
    
    return NULL;
}


int main(int argc, char** argv) {
    int skListen , skClient , nAddrLen;
    struct sockaddr_in saServer , saClient;
    
    //Create socket
    skListen = socket(AF_INET , SOCK_STREAM , 0); //create server socket if error is returned -1
    if (skListen == -1)
    {
        std::cout << "Could not create socket" << endl;
        return 0;
    }
    std::cout << "Socket created" << endl;
     
    //Prepare the sockaddr_in structure
    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = INADDR_ANY;
    saServer.sin_port = htons(8888);
     
    //Bind: server require port for socket if < 0 return error
    if (bind(skListen, (struct sockaddr *)&saServer , sizeof(saServer)) < 0) 
    {
        //print the error message
        perror("bind failed. Error");
        close(skListen);
        return 0;
    }
    
    //Listen
    if (listen(skListen , 3) < 0)
    {
        close(skListen);
        return 0;
    }
    
    //Accept and incoming connection
    std::cout << "Waiting for incoming connections..." << endl;
    nAddrLen = sizeof(struct sockaddr_in);   
    while ((skClient = accept(skListen, (struct sockaddr *)&saClient, (socklen_t*)&nAddrLen)) >= 0)
    {

        std::cout << "New connection accepted" << endl;
         
        pthread_t pthHandler;

        // pointer cung chi la mot bien co gia tri
        // return 0 for success
        if( pthread_create( &pthHandler, NULL, ConnectionHandler, (void*) skClient) != 0)
        {
            perror("could not create thread");
            break;
        }
    }
     
    if(skClient < 0)
    {
        perror("accept failed: ");
    }
    
    close(skListen);
    
    return 0;
}

