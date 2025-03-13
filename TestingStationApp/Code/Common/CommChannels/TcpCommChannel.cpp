#include "TcpCommChannel.h"
#include "Utilities.h"
#include "EventLogger.h"

#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <cstring> // memset, memcpy

//************************************
// Cross-platform settings/macros
//************************************
#ifdef WIN32
#include <errno.h>
#define CLOSE_SOCKET(s) closesocket(s)
#define GET_LAST_ERROR() WSAGetLastError()

inline int SetNonBlocking(SOCKET s) {
    u_long mode = 1;  // enable non-blocking
    return ioctlsocket(s, FIONBIO, &mode);
}

#else // Linux / macOS / Raspberry Pi
#include <errno.h>
#include <fcntl.h>
#define CLOSE_SOCKET(s) close(s)
#define GET_LAST_ERROR() errno

inline int SetNonBlocking(SOCKET s) {
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}
#endif

static const unsigned int MY_SO_MAX_MSG_SIZE = MAX_MESSAGE_SIZE;

//**************************************************************
//                TCPServer (Cross-platform)
//**************************************************************
TCPServer::TCPServer()
    : listenSocket(INVALID_SOCKET)
    , clientSocket(INVALID_SOCKET)
{
#ifdef WIN32
    // אתחול WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "WSAStartup failed: " << result << std::endl;
    }
#endif
    std::memset(&serverAddress, 0, sizeof(serverAddress));
}

TCPServer::~TCPServer() {
    CLOSE_SOCKET(listenSocket);
    CLOSE_SOCKET(clientSocket);
#ifdef WIN32
    WSACleanup();
#endif
}

bool TCPServer::Init(std::string serverIp, int serverPort) {
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "[TCPServer] Error at socket(): " << GET_LAST_ERROR() << std::endl;
#ifdef WIN32
        WSACleanup();
#endif
        return false;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

#ifdef WIN32
    serverAddress.sin_addr.s_addr = inet_addr(serverIp.c_str());
    if (serverAddress.sin_addr.s_addr == INADDR_NONE) {
        std::cout << "[TCPServer] Invalid IP address: " << serverIp << std::endl;
        CLOSE_SOCKET(listenSocket);
        listenSocket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
#else
    //Linux/Mac recommended inet_pton
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddress.sin_addr) <= 0) {
        std::cout << "[TCPServer] Invalid IP address: " << serverIp << std::endl;
        CLOSE_SOCKET(listenSocket);
        listenSocket = INVALID_SOCKET;
        return false;
    }
#endif

    // bind + listen
#ifdef WIN32
    if (bind(listenSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cout << "[TCPServer] Bind failed: " << GET_LAST_ERROR() << std::endl;
        CLOSE_SOCKET(listenSocket);
        WSACleanup();
        return false;
    }
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "[TCPServer] Listen failed: " << GET_LAST_ERROR() << std::endl;
        CLOSE_SOCKET(listenSocket);
        WSACleanup();
        return false;
    }
#else
    if (::bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cout << "[TCPServer] Bind failed: " << errno << std::endl;
        CLOSE_SOCKET(listenSocket);
        listenSocket = INVALID_SOCKET;
        return false;
    }
    if (listen(listenSocket, SOMAXCONN) < 0) {
        std::cout << "[TCPServer] Listen failed: " << errno << std::endl;
        CLOSE_SOCKET(listenSocket);
        listenSocket = INVALID_SOCKET;
        return false;
    }
#endif

    // Setting non-blocking
    if (SetNonBlocking(listenSocket) != 0) {
        std::cout << "[TCPServer] Error setting non-blocking: " << GET_LAST_ERROR() << std::endl;
        CLOSE_SOCKET(listenSocket);
#ifdef WIN32
        WSACleanup();
#endif
        return false;
    }

    // server (thread)
    m_thisThread = std::thread(&TCPServer::runServerService, this);

    return true;
}

bool TCPServer::sendMessage(unsigned char* buffer, unsigned int size) {
    if (clientSocket == INVALID_SOCKET) {
        return false;
    }
    int iSendResult = send(clientSocket, (const char*)buffer, size, 0);
#ifdef WIN32
    if (iSendResult == SOCKET_ERROR) {
        return false;
    }
#else
    if (iSendResult < 0) {
        return false;
    }
#endif
    return true;
}

bool TCPServer::getNextMessage(unsigned char* buffer, unsigned int maxSize, unsigned int* size) {
    if (!m_receivedMessages.isEmpty()) {
        ReceivedMessage receivedMessage;
        if (m_receivedMessages.dequeue(receivedMessage)) {
            *size = getMin(getMin(receivedMessage.size, MY_SO_MAX_MSG_SIZE), maxSize);
            std::memcpy(buffer, receivedMessage.buffer, *size);
            return true;
        }
    }
    return false;
}

void TCPServer::reset() {
    // איפוס אם צריך
}

bool TCPServer::checkConnection() {
    return (clientSocket != INVALID_SOCKET);
}

void TCPServer::runServerService() {
    fd_set readfds;
    std::vector<SOCKET> clientSockets;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);
        SOCKET max_sd = listenSocket;

        for (auto& sock : clientSockets) {
            FD_SET(sock, &readfds);
            if (sock > max_sd) max_sd = sock;
        }

        int activity =
#ifdef WIN32
            select((int)max_sd + 1, &readfds, NULL, NULL, NULL);
#else
            select(max_sd + 1, &readfds, nullptr, nullptr, nullptr);
#endif
        if (activity < 0) {
#ifndef WIN32
            if (errno == EINTR) {
                continue;
            }
#endif
            std::cerr << "[TCPServer] select error" << std::endl;
        }

        // New Connection?
        if (FD_ISSET(listenSocket, &readfds)) {
            SOCKET newSocket = accept(listenSocket, NULL, NULL);
            if (newSocket == INVALID_SOCKET) {
                std::cerr << "[TCPServer] accept failed: " << GET_LAST_ERROR() << std::endl;
            }
            else {
                clientSockets.push_back(newSocket);
                clientSocket = newSocket;
                std::cout << "New connection, socket fd is " << newSocket << std::endl;
            }
        }
        else {
            // recv on existing sockets
            for (auto it = clientSockets.begin(); it != clientSockets.end();) {
                SOCKET sock = *it;
                if (FD_ISSET(sock, &readfds)) {
                    ReceivedMessage receivedMessage;
                    int recvSize = recv(sock,
                        reinterpret_cast<char*>(receivedMessage.buffer),
                        MAX_MESSAGE_SIZE,
                        0);
                    if (recvSize == 0) {
                        // ניתוק
                        std::cout << "Host disconnected, socket fd: " << sock << std::endl;
                        CLOSE_SOCKET(sock);
                        if (sock == clientSocket) {
                            clientSocket = INVALID_SOCKET;
                        }
                        it = clientSockets.erase(it);
                    }
                    else if (recvSize > 0) {
                        receivedMessage.size = recvSize;
                        if ((unsigned)recvSize <= MAX_MESSAGE_SIZE) {
                            m_receivedMessages.enqueue(receivedMessage);
                        }
                        ++it;
                    }
                    else {
                        // Error
                        std::cerr << "recv error, socket fd: " << sock
                            << " error=" << GET_LAST_ERROR() << std::endl;
                        CLOSE_SOCKET(sock);
                        if (sock == clientSocket) {
                            clientSocket = INVALID_SOCKET;
                        }
                        it = clientSockets.erase(it);
                    }
                }
                else {
                    ++it;
                }
            }
        }
    }
}

//**************************************************************
//                TCPClient (Cross-platform)
//**************************************************************
TCPClient::TCPClient()
    : m_socket(INVALID_SOCKET)
    , m_serverPort(0)
{
#ifdef WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "[TCPClient] WSAStartup failed: " << result << std::endl;
    }
#endif
}

TCPClient::~TCPClient() {
#ifdef WIN32
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
    }
    WSACleanup();
#else
    if (m_socket != -1) {
        close(m_socket);
    }
#endif
    reset();
}

bool TCPClient::Init(std::string serverIp, int serverPort) {
    m_serverIp = serverIp;
    m_serverPort = serverPort;
    return connectToServer(serverIp, serverPort);
}

bool TCPClient::sendMessage(unsigned char* buffer, unsigned int size) {
    bool sent = false;
#ifdef WIN32
    if (m_socket != INVALID_SOCKET)
#else
    if (m_socket != -1)
#endif
    {
        int iSendResult = send(m_socket, (const char*)buffer, size, 0);
#ifdef WIN32
        if (iSendResult != SOCKET_ERROR) {
            sent = true;
        }
#else
        sent = (iSendResult == (int)size);
#endif
    }
    else {
        // Try connecting again?
        connectToServer(m_serverIp, m_serverPort);
    }
    return sent;
}

bool TCPClient::getNextMessage(unsigned char* buffer, unsigned int maxSize, unsigned int* size) {
    bool breceived = false;
#ifdef WIN32
    if (m_socket != INVALID_SOCKET)
#else
    if (m_socket != -1)
#endif
    {
        int iResult = recv(m_socket, (char*)buffer, maxSize, 0);
        if (iResult > 0) {
            *size = iResult;
            breceived = true;
        }
    }
    else {
        connectToServer(m_serverIp, m_serverPort);
    }
    return breceived;
}

void TCPClient::reset() {
    
}

bool TCPClient::checkConnection() {
#ifdef WIN32
    return (m_socket != INVALID_SOCKET);
#else
    return (m_socket != -1);
#endif
}

bool TCPClient::connectToServer(const std::string& serverIp, int serverPort) {
#ifdef WIN32
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        std::cerr << "[TCPClient] Error at socket(): " << WSAGetLastError() << std::endl;
        EventLogger::getInstance().log("ConnectToServer: Error at socket()", "TCPClient");
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = inet_addr(serverIp.c_str());

    if (connect(m_socket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "[TCPClient] Connect failed: " << WSAGetLastError() << std::endl;
        std::string logString = "ConnectToServer: Connect failed with error: " + std::to_string(WSAGetLastError());
        EventLogger::getInstance().log(logString, "TCPClient");
        closesocket(m_socket);
        WSACleanup();
        m_socket = INVALID_SOCKET;
        return false;
    }
    return true;
#else
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        std::cerr << "[TCPClient] Error at socket() => " << errno << std::endl;
        EventLogger::getInstance().log("ConnectToServer: Error at socket()", "TCPClient");
        m_socket = -1;
        return false;
    }

    std::cerr << "[TCPClient] socket created" << std::endl;

    sockaddr_in server;
    std::memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPort);
    server.sin_addr.s_addr = inet_addr(serverIp.c_str());

    if (connect(m_socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "[TCPClient] Failed to connect" << std::endl;
        EventLogger::getInstance().log("ConnectToServer: Failed to connect", "TCPClient");
        close(m_socket);
        m_socket = -1;
        return false;
    }
    std::cout << "[TCPClient] Connected to server" << std::endl;
    EventLogger::getInstance().log("ConnectToServer: Connected to server", "TCPClient");
    return true;
#endif
}

