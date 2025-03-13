#ifndef SHIELD_TcpCommChannel_H
#define SHIELD_TcpCommChannel_H

#include "ICommChannel.h"

#include <string>
#include <thread>
#include <iostream>
#include <vector>
#include "SafeQueue.h"

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for close function

//To prevent code cracking
typedef int SOCKET;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR    (-1)
#endif
#ifndef SOMAXCONN
#define SOMAXCONN 128
#endif
#endif //WIN32

//----------------------------------------------------
// TCP Server
//----------------------------------------------------
class TCPServer : public ICommChannel
{
public:
    TCPServer();
    ~TCPServer();

    
    bool Init(std::string serverIp, int serverPort);

    // Functions available in the ICommChannel interface
    bool sendMessage(unsigned char* buffer, unsigned int size) override;
    bool getNextMessage(unsigned char* buffer, unsigned int maxSize, unsigned int* size) override;
    void reset() override;
    bool checkConnection() override;

private:
    void runServerService();
    std::thread m_thisThread;

    SOCKET listenSocket;
    SOCKET clientSocket;

    sockaddr_in serverAddress;

    SafeQueue<ReceivedMessage> m_receivedMessages;
};

//----------------------------------------------------
// TCP Client
//----------------------------------------------------
class TCPClient : public ICommChannel
{
public:
    TCPClient();
    ~TCPClient();

    
    bool Init(std::string serverIp, int serverPort);

    // Functions available in the ICommChannel interface
    bool sendMessage(unsigned char* buffer, unsigned int size) override;
    bool getNextMessage(unsigned char* buffer, unsigned int maxSize, unsigned int* size) override;
    void reset() override;
    bool checkConnection() override;

private:
    bool connectToServer(const std::string& serverIp, int serverPort);

    SOCKET m_socket;
    std::string m_serverIp;
    int m_serverPort;
};

#endif // SHIELD_TcpCommChannel_H


