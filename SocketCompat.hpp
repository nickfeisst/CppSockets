/*
 * Cross-platform compatibility superclass for sockets
 *
 * Copyright (C) 2019 Simon D. Levy
 *
 * MIT License
 */

#pragma once

// Windows
#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#undef TEXT
#include <winsock2.h>
#include <ws2tcpip.h>

// Linux
#else
#define sprintf_s sprintf
typedef int SOCKET;
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
static const int INVALID_SOCKET = -1;
static const int SOCKET_ERROR   = -1;
#endif

#include <stdio.h>

#ifndef _WIN32
static void closesocket(int socket) { close(socket); }
#endif

class Socket {

    protected:

        SOCKET _sock;

        char _message[200];

        bool initWinsock(void)
        {
#ifdef _WIN32
            WSADATA wsaData;
            int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (iResult != 0) {
                sprintf_s(_message, "WSAStartup() failed with error: %d\n", iResult);
                return false;
            }
#endif
            return true;
        }

        void cleanup(void)
        {
#ifdef _WIN32
            WSACleanup();
#endif
        }

        void inetPton(const char * host, struct sockaddr_in & saddr_in)
        {
#ifdef _WIN32
            WCHAR wsz[64];
            swprintf_s(wsz, L"%S", host);
            InetPton(AF_INET, wsz,   &(saddr_in.sin_addr.s_addr));
#else
            inet_pton(AF_INET, host, &(saddr_in.sin_addr));
#endif
        }

        void setUdpTimeout(uint32_t msec)
        {
#ifdef _WIN32
            setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &msec, sizeof(msec));

#else
            struct timeval timeout;
            timeout.tv_sec = msec / 1000;
            timeout.tv_usec = (msec * 1000) % 1000000;
            setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif
        }

    public:

        bool setUdpBlocking(bool is_blocking)
        {
            bool ret = true;

#ifdef _WIN32
            /// @note windows sockets are created in blocking mode by default
            // currently on windows, there is no easy way to obtain the socket's current blocking mode since WSAIsBlocking was deprecated
            u_long flags = is_blocking ? 0 : 1;
            ret = NO_ERROR == ioctlsocket(_sock, FIONBIO, &flags);
#else
            const int flags = fcntl(_sock, F_GETFL, 0);
            if ((flags & O_NONBLOCK) && !is_blocking) {
                return ret; // already in non-blocking mode
            }
            if (!(flags & O_NONBLOCK) && is_blocking) {
                return ret; // already in blocking mode
            }
            ret = 0 == fcntl(_sock, F_SETFL, is_blocking ? flags ^ O_NONBLOCK : flags | O_NONBLOCK);
#endif
            return ret;
        }

        void closeConnection(void)
        {
#ifdef _WIN32
            closesocket(_sock);
#else
            close(_sock);
#endif
        }

        char * getMessage(void)
        {
            return _message;
        }
};
