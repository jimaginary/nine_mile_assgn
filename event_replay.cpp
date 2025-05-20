#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

static const char* SOCKET_PATH = "/tmp/nine_mile_assgn_socket";

struct ClientServerFd {
    int client_fd;
    int server_fd;
};

ClientServerFd connect_server() {
    // create socket
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("socket");;
    }

    // setup socket file
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCKET_PATH);

    // bind to socket
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        throw std::runtime_error("bind");
    }

    // listen for connections
    if (listen(server_fd, 1) == -1) {
        throw std::runtime_error("listen");;
    }

    std::cout << "Waiting for consumer...\n";
    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == -1) {
        throw std::runtime_error("accept");;
    }

    ClientServerFd client_server_fd;
    client_server_fd.client_fd = client_fd;
    client_server_fd.server_fd = server_fd;
    return client_server_fd;
}

int get_line_time(std::string line) {
    int end_of_time_field = line.find(',', 0);
    if (end_of_time_field == -1) {
        throw std::runtime_error("only one field in line");
        return -1;
    }
    
    std::string time_string = line.substr(0, end_of_time_field);
    return std::stoi(time_string);
}

int main(int argc, char* argv[]) {
    if (argc > 3) {
        throw std::invalid_argument("one argument expected, event csv file path (and possibly -p flag for printing).");
        return 1;
    }

    bool log = false;
    std::string event_file;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p") {
            log = true;
        } else {
            event_file = arg;
        }
    }

    // open csv file
    std::ifstream file(event_file);
    if (!file.is_open()) {
        throw std::invalid_argument("csv file does not exist");
        return 1;
    }

    ClientServerFd client_server_fd = connect_server();

    // produce events
    std::cout << "Consumer connected, producing events.\n";
    std::string line;
    // skip the header line
    if (!std::getline(file, line)) {
        throw std::runtime_error("empty file");
    }

    auto next_time = std::chrono::system_clock::now();
    int previous_market_time = 0;
    while (std::getline(file, line)) {
        int event_time = get_line_time(line);
        next_time += std::chrono::milliseconds(event_time - previous_market_time);
        previous_market_time = event_time;

        if (log) {
            std::cout << line + '\n';
        }

        std::this_thread::sleep_until(next_time);
        write(client_server_fd.client_fd, (line + '\n').c_str(), line.length() + 1);
    }

    return 0;
}