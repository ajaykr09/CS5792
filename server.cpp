#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

constexpr int MAX_BUFFER_SIZE = 1024;
constexpr int MAX_FILE_SIZE = 100 * 1024 * 1024; // 100 MiB

class FileServer {
public:
    FileServer(unsigned short server_port, const std::string& file_directory)
        : server_port(server_port), file_directory(file_directory), connection_count(0), timed_out(false) {
        signal(SIGQUIT, handle_signal);
        signal(SIGTERM, handle_signal);
        start_server();
    }

private:
    unsigned short server_port;
    std::string file_directory;
    unsigned int connection_count;
    bool timed_out;

    static void handle_signal(int signum) {
        if (signum == SIGQUIT || signum == SIGTERM) {
            std::exit(EXIT_SUCCESS);
        }
    }

    std::string get_server_ip() const {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == -1) {
            std::cerr << "ERROR: Unable to create a temporary socket for retrieving IP address." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr("8.8.8.8"); // Use a well-known address (Google's DNS server)
        server_addr.sin_port = htons(53);

        if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            std::cerr << "ERROR: Unable to connect to the temporary socket for retrieving IP address." << std::endl;
            close(sock);
            std::exit(EXIT_FAILURE);
        }

        sockaddr_in local_addr{};
        socklen_t addr_len = sizeof(local_addr);
        getsockname(sock, reinterpret_cast<sockaddr*>(&local_addr), &addr_len);

        close(sock);

        return inet_ntoa(local_addr.sin_addr);
    }

    void send_ack(int client_socket, const std::string& message) const {
        send(client_socket, message.c_str(), message.size(), 0);
    }

    void start_server() {
        std::string server_ip = get_server_ip();
        std::cout << "Server IP: " << server_ip << std::endl;

        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            std::cerr << "ERROR: Unable to create server socket." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(server_port);

        if (bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            std::cerr << "ERROR: Unable to bind to the specified port." << std::endl;
            close(server_socket);
            std::exit(EXIT_FAILURE);
        }

        listen(server_socket, 1);

        while (true) {
            int client_socket;
            timeval timeout{};
            timeout.tv_sec = 30;

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_socket, &readfds);

            int activity = select(server_socket + 1, &readfds, nullptr, nullptr, &timeout);

            if (activity == 0) {
                std::cout << "No connections for 30 seconds. Shutting down the server." << std::endl;
                break;
            }

            if (FD_ISSET(server_socket, &readfds)) {
                client_socket = accept(server_socket, nullptr, nullptr);
                if (client_socket == -1) {
                    std::cerr << "ERROR: Unable to accept connection." << std::endl;
                    continue;  // Continue to the next iteration to accept another connection
                }

                connection_count++;
                std::cout << "Connection " << connection_count << " accepted." << std::endl;
                send_ack(client_socket, "ACK Connection established");

                std::string file_name = file_directory + "/" + std::to_string(connection_count) + ".file";

                std::ofstream file_stream(file_name, std::ios::binary);

                timed_out = false;
                handle_connection(client_socket, file_stream);

                file_stream.close();
                send_ack(client_socket, "ACK File received successfully");

                close(client_socket);
            }
        }

        close(server_socket);
    }

    void handle_connection(int client_socket, std::ofstream& file_stream) {
        ssize_t bytes_read;
        char buffer[MAX_BUFFER_SIZE];

        while ((bytes_read = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
            file_stream.write(buffer, bytes_read);
        }

        if (bytes_read == -1) {
            std::cerr << "ERROR: Unable to receive data from the client." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        // Check if the received file exceeds the maximum allowed size
        if (file_stream.tellp() > MAX_FILE_SIZE) {
            std::cerr << "ERROR: File size exceeds the maximum allowed size." << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "ERROR: Invalid number of arguments." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    unsigned short server_port = std::atoi(argv[1]);
    std::string file_directory = argv[2];

    if (server_port == 0) {
        std::cerr << "ERROR: Invalid port number." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    FileServer server(server_port, file_directory);

    return 0;
}
