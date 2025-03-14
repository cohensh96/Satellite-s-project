#include "TcpCommChannel.h"
#include "Utilities.h"
#include "EventLogger.h"

#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <cstring>  // memset, memcpy
#ifdef __APPLE__
    #include <sys/filio.h>  // macOS / BSD systems
#elif defined(__linux__)
    #include <sys/ioctl.h>  // Linux (Raspberry Pi)
#endif


//=====================================================
// Cross-platform settings and tools to simulate Windows calls on Linux/macOS as well
//=====================================================
#ifdef WIN32
    #define CLOSE_SOCKET(s)     closesocket(s)
    #define GET_LAST_ERROR()    WSAGetLastError()
    #define ioctlsocket_Wrap(fd, cmd, argp) ioctlsocket((fd), (cmd), (argp))
#else
// -- POSIX systems (Linux, macOS, Raspberry Pi) --
    #include <errno.h>
    #include <fcntl.h>

// Close the socket with close
    #define CLOSE_SOCKET(s) close(s)
    // נשתמש ב-errno במקום ב-WSAGetLastError
    #define GET_LAST_ERROR() errno

    // FIONBIO => we will become non-blocking using fcntl
    inline int ioctlsocket_Wrap(int fd, long cmd, u_long* argp) {
        if (cmd == FIONBIO) {
            //// if *argp == 1 => set O_NONBLOCK
            int flags = fcntl(fd, F_GETFL, 0);
            if (flags < 0) return -1;
            if (*argp == 1) {
                return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) ? -1 : 0;
            } else {
                // Turn off non-block mode
                return (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) == -1) ? -1 : 0;
            }
        }
        return 0; // No other commands are handled here.
    }

// We can set FIONBIO if not defined
    #ifndef FIONBIO
      #define FIONBIO 0x8004667e //Arbitrary value
    #endif
#endif

//-----------------------------------------------------
// Set message size
//-----------------------------------------------------
#ifndef SO_MAX_MSG_SIZE
#define SO_MAX_MSG_SIZE 65000
#endif

// (according to MAX_MESSAGE_SIZE definition in ICommChannel.h)
//**************************************************************
//     TCPServer - Cross-platform
//**************************************************************
TCPServer::TCPServer()
    : listenSocket(INVALID_SOCKET)
    , clientSocket(INVALID_SOCKET)
{
#ifdef WIN32
    // Initialize WinSock on Windows
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
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
    // 1) Create a socket
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
#ifdef WIN32
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        std::cout << "Error at socket(): " << errno << std::endl;
#endif
        return false;
    }

    // 2) Address/Port Setting
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

#ifdef WIN32
    serverAddress.sin_addr.s_addr = inet_addr(serverIp.c_str());
    if (serverAddress.sin_addr.s_addr == INADDR_NONE) {
        std::cout << "Invalid IP address: " << serverIp << std::endl;
        CLOSE_SOCKET(listenSocket);
        listenSocket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }
#else
    // On POSIX systems inet_pton is recommended
    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddress.sin_addr) <= 0) {
        std::cout << "Invalid IP address: " << serverIp << std::endl;
        CLOSE_SOCKET(listenSocket);
        listenSocket = INVALID_SOCKET;
        return false;
    }
#endif

    // 3) bind
    if (
#ifdef WIN32
        bind(listenSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR
#else
        bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0
#endif
    ) {
        std::cout << "Bind failed with error: " << GET_LAST_ERROR() << std::endl;
        CLOSE_SOCKET(listenSocket);
#ifdef WIN32
        WSACleanup();
#endif
        return false;
    }

    // 4) listen
    if (listen(listenSocket, SOMAXCONN)
#ifdef WIN32
        == SOCKET_ERROR
#else
        < 0
#endif
    ) {
        std::cout << "Listen failed with error: " << GET_LAST_ERROR() << std::endl;
        CLOSE_SOCKET(listenSocket);
#ifdef WIN32
        WSACleanup();
#endif
        return false;
    }

    // 5) Setting non-blocking (on Windows: ioctlsocket, on Linux/Mac: fcntl)
    {
        u_long mode = 1;  // 1 = enable non-blocking mode
        if (ioctlsocket_Wrap(listenSocket, FIONBIO, &mode) != 0) {
            std::cout << "Error setting non-blocking mode: " << GET_LAST_ERROR() << std::endl;
            CLOSE_SOCKET(listenSocket);
#ifdef WIN32
            WSACleanup();
#endif
            return false;
        }
    }

    // 6) Running a Thread that handles receipts/disconnections
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
    bool messageReceived = false;
    if (!m_receivedMessages.isEmpty()) {
        ReceivedMessage receivedMessage;
        if (m_receivedMessages.dequeue(receivedMessage)) {
            *size = getMin(getMin(receivedMessage.size, SO_MAX_MSG_SIZE), maxSize);
            std::memcpy(buffer, receivedMessage.buffer, *size);
            messageReceived = true;
        }
    }
    return messageReceived;
}

void TCPServer::reset() {
    // You can reset buffers/queues etc. if necessary
}

bool TCPServer::checkConnection() {
    // Assume we are 'connected' if clientSocket != INVALID_SOCKET
    return (clientSocket != INVALID_SOCKET);
}

void TCPServer::runServerService() {
    fd_set readfds;
    std::vector<SOCKET> clientSockets;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(listenSocket, &readfds);
        SOCKET max_sd = listenSocket;

        // Add the client sockets
        for (auto& sock : clientSockets) {
            FD_SET(sock, &readfds);
            if (sock > max_sd) {
                max_sd = sock;
            }
        }

        // select
        int activity =
#ifdef WIN32
            select((int)max_sd + 1, &readfds, NULL, NULL, NULL);
#else
            select((int)max_sd + 1, &readfds, NULL, NULL, NULL);
#endif

        if (activity < 0) {
#ifndef WIN32
            if (errno == EINTR) {
                continue;
            }
#endif
            std::cerr << "Select error" << std::endl;
        }

        // Check if there is a new connection on the listenSocket
        if (FD_ISSET(listenSocket, &readfds)) {
            SOCKET newSocket =
#ifdef WIN32
                accept(listenSocket, NULL, NULL);
#else
                accept(listenSocket, nullptr, nullptr);
#endif
            if (newSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed with error: " << GET_LAST_ERROR() << std::endl;
            } else {
                clientSockets.push_back(newSocket);
                clientSocket = newSocket;
                std::cout << "New connection, socket fd is " << newSocket << std::endl;
            }
        } else {
            // Otherwise it's recv on an existing socket
            for (auto it = clientSockets.begin(); it != clientSockets.end(); ) {
                SOCKET sock = *it;
                if (FD_ISSET(sock, &readfds)) {
                    ReceivedMessage receivedMessage;
                    int recvSize = recv(sock,
                                        reinterpret_cast<char*>(receivedMessage.buffer),
                                        MAX_MESSAGE_SIZE,
                                        0);
                    if (recvSize == 0) {
                        // Someone disconnected
                        std::cout << "Host disconnected, socket fd: " << sock << std::endl;
                        CLOSE_SOCKET(sock);
                        if (sock == clientSocket) {
                            clientSocket = INVALID_SOCKET;
                        }
                        it = clientSockets.erase(it);
                    } else if (recvSize > 0) {
                        receivedMessage.size = recvSize;
                        if ((unsigned)recvSize <= MAX_MESSAGE_SIZE) {
                            m_receivedMessages.enqueue(receivedMessage);
                        }
                        ++it;
                    } else {
                        // error in recv
                        std::cerr << "recv error: " << GET_LAST_ERROR()
                                  << ", socket fd: " << sock << std::endl;
                        CLOSE_SOCKET(sock);
                        if (sock == clientSocket) {
                            clientSocket = INVALID_SOCKET;
                        }
                        it = clientSockets.erase(it);
                    }
                } else {
                    ++it;
                }
            }
        }
    }
}

//**************************************************************
//               TCPClient - Cross-platform
//**************************************************************
#ifdef WIN32
TCPClient::TCPClient()
    : m_socket(INVALID_SOCKET)
    , m_serverPort(0)
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
}
#else
TCPClient::TCPClient()
    : m_socket(-1)
    , m_serverPort(0)
{
    // On Linux/Mac no special initialization is required
}
#endif

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
    m_serverIp   = serverIp;
    m_serverPort = serverPort;
    return connectToServer(serverIp, serverPort);
}

bool TCPClient::sendMessage(unsigned char* buffer, unsigned int size) {
    bool sent = false;
    if (
#ifdef WIN32
        m_socket != INVALID_SOCKET
#else
        m_socket != -1
#endif
    ) {
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
        connectToServer(m_serverIp, m_serverPort);
    }
    return sent;
}

bool TCPClient::getNextMessage(unsigned char* buffer, unsigned int maxSize, unsigned int* size) {
    bool breceived = false;
    if (
#ifdef WIN32
        m_socket != INVALID_SOCKET
#else
        m_socket != -1
#endif
    ) {
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
    // Clean/reset as needed
}

bool TCPClient::checkConnection() {
#ifdef WIN32
    return (m_socket != INVALID_SOCKET);
#else
    return (m_socket != -1);
#endif
}

#ifdef WIN32
bool TCPClient::connectToServer(const std::string& serverIp, int serverPort) {
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        std::string logString = "ConnectToServer: Error at socket(): " + std::to_string(WSAGetLastError());
        EventLogger::getInstance().log(logString, "TCPClient");
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIp.c_str());
    serverAddress.sin_port = htons(serverPort);

    if (connect(m_socket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
        std::string logString = "ConnectToServer: Connect failed with error: " + std::to_string(WSAGetLastError());
        EventLogger::getInstance().log(logString, "TCPClient");
        closesocket(m_socket);
        WSACleanup();
        m_socket = INVALID_SOCKET;
        return false;
    }
    return true;
}
#else
bool TCPClient::connectToServer(const std::string& serverIp, int serverPort)
{
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) {
        std::string logString = "ConnectToServer: Error at socket() => " + std::to_string(errno);
        EventLogger::getInstance().log(logString, "TCPClient");
        std::cerr << "Error at socket(): " << errno << std::endl;
        m_socket = -1;
        return false;
    }
    std::cerr << "socket created" << std::endl;
    EventLogger::getInstance().log("ConnectToServer: socket created", "TCPClient");

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(serverPort);
    server.sin_addr.s_addr = inet_addr(serverIp.c_str());

    if (connect(m_socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Failed to connect" << std::endl;
        std::string logString = "ConnectToServer: Failed to connect => " + std::to_string(errno);
        EventLogger::getInstance().log(logString, "TCPClient");
        close(m_socket);
        m_socket = -1;
        return false;
    }
    std::cout << "Connected to server" << std::endl;
    EventLogger::getInstance().log("ConnectToServer: Connected to server", "TCPClient");
    return true;
}
#endif

