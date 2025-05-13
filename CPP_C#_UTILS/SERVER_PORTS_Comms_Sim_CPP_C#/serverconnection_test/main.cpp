/*
 * Server Monitor & Port Tester
 * Author: Alushi
 * Description:
 *   - Continuously monitors a given IP by attempting TCP connections.
 *   - If the host is unreachable for a configured threshold, sets serverConnection = false.
 *   - Also supports on-demand port testing from user input.
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib") // Link Winsock library
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

 // Shared atomic flag used for monitoring thread communication
std::atomic<bool> serverConnection{ true };

// ---------- TCP-BASED SERVER CHECK ----------
// Attempts to connect to a host on a given port with a short timeout
bool isHostReachable(const std::string& ip, int port = 80, int timeoutMs = 500) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return false;

#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode); // Set non-blocking mode
#else
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    connect(sockfd, (sockaddr*)&addr, sizeof(addr)); // Non-blocking connect

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sockfd, &writeSet);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeoutMs * 1000;

    int res = select(sockfd + 1, nullptr, &writeSet, nullptr, &tv);

    bool connected = false;
    if (res > 0) {
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
        connected = (so_error == 0);
    }

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif

    return connected;
}

// ---------- SERVER MONITOR ----------
// Periodically checks if the server is reachable; updates global status flag
void monitorServer(const std::string& ip) {
    int failureCount = 0;
    const int intervalMs = 5000;           // 5 seconds between checks
    const int failureThreshold = 3;        // Fail after 3 consecutive failures (~15s)

    while (true) {
        bool reachable = isHostReachable(ip, 80);

        if (reachable) {
            failureCount = 0;
        }
        else {
            failureCount++;
        }

        if (failureCount >= failureThreshold) {
            serverConnection.store(false);
        }
        else {
            serverConnection.store(true);
        }

        // Print current status for debug purposes
        std::cerr << "[DEBUG] Check IP: " << ip
            << " | Reachable: " << reachable
            << " | Failures: " << failureCount
            << " | Status: " << (serverConnection ? "Online" : "Offline") << "\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

// ---------- PORT TEST ----------
// Tests a specific port using non-blocking TCP connection
bool testPortFast(const std::string& ip, int port, int timeoutMs = 200) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return false;

#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode);
#else
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    connect(sockfd, (sockaddr*)&addr, sizeof(addr));

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sockfd, &writeSet);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeoutMs * 1000;

    int res = select(sockfd + 1, nullptr, &writeSet, nullptr, &tv);

    bool connected = false;
    if (res > 0) {
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
        connected = (so_error == 0);
    }

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif

    return connected;
}

// ---------- MAIN ----------
int main() {
    std::string ip = "127.0.0.1"; // Change to target IP for monitoring
    std::thread monitorThread(monitorServer, ip); // Launch background monitor

    std::cout << "=== Server Monitor & Port Tester ===\n";
    std::cout << "Commands:\n";
    std::cout << "  status       - Show server status\n";
    std::cout << "  test <port>  - Test specific port\n";
    std::cout << "  exit         - Quit\n";

    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "exit") break;

        if (input == "status") {
            std::cout << "[Server status] " << (serverConnection ? "Online" : "Offline") << "\n";
        }
        else if (input.rfind("test ", 0) == 0) {
            try {
                int port = std::stoi(input.substr(5));
                bool ok = testPortFast(ip, port);
                std::cout << "[Port " << port << "] " << (ok ? "Open" : "Closed") << "\n";
            }
            catch (...) {
                std::cout << "[!] Invalid port number.\n";
            }
        }
        else {
            std::cout << "[!] Unknown command.\n";
        }
    }

    monitorThread.detach(); // Clean up on exit (or replace with join or stop flag)
    return 0;
}
