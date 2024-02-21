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

class FileClient {
public:
    FileClient(const std::string& server_ip, unsigned short server_port, const std::string& file_location)
        : server_ip(server_ip), server_port(server_port), file_location(file_location) {
        connect_to_server();
    }

private:
    std::string server_ip;
    unsigned short server_port;
    std::string file_location;

    void connect_to_server() {
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            std::cerr << "ERROR: Unable to create client socket." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
        server_addr.sin_port = htons(server_port);

        if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            std::cerr << "ERROR: Unable to connect to the server." << std::endl;
            close(client_socket);
            std::exit(EXIT_FAILURE);
        }

        send_file(client_socket);
        close(client_socket);
    }

    void send_file(int client_socket) {
        std::ifstream file_stream(file_location, std::ios::binary);
        if (!file_stream) {
            std::cerr << "ERROR: Unable to open file: " << file_location << std::endl;
            std::exit(EXIT_FAILURE);
        }

        char buffer[MAX_BUFFER_SIZE];
        while (file_stream.good()) {
            file_stream.read(buffer, sizeof(buffer));
            ssize_t bytes_read = file_stream.gcount();
            if (bytes_read > 0) {
                if (send(client_socket, buffer, bytes_read, 0) == -1) {
                    std::cerr << "ERROR: Failed to send data to server." << std::endl;
                    std::exit(EXIT_FAILURE);
                }
            }
        }

        file_stream.close();
        std::cout << "File sent successfully." << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <file_location>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string server_ip = argv[1];
    unsigned short server_port = std::atoi(argv[2]);
    std::string file_location = argv[3];

    FileClient client(server_ip, server_port, file_location);

    return 0;
}
