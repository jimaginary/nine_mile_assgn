#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>

static const char* SOCKET_PATH = "/tmp/nine_mile_assgn_socket";
static const int BUFFER_SIZE = 128;

struct Event {
    int time_millis;
    std::string symbol;
    std::string exchange;
    int bid_quantity;
    double bid_price;
    int offer_quantity;
    double offer_price;
    double last_price;
    double close_price;
};

Event parse_string(std::string event_string) {
    Event event;
    std::stringstream event_stream(event_string);
    std::string field;

    std::getline(event_stream, field, ',');
    event.time_millis = std::stoi(field);
    
    std::getline(event_stream, event.symbol, ',');

    std::getline(event_stream, event.exchange, ',');

    std::getline(event_stream, field, ',');
    event.bid_quantity = std::stoi(field);

    std::getline(event_stream, field, ',');
    event.bid_price = std::stod(field);

    std::getline(event_stream, field, ',');
    event.offer_price = std::stod(field);

    std::getline(event_stream, field, ',');
    event.offer_quantity = std::stoi(field);

    std::getline(event_stream, field, ',');
    event.last_price = std::stod(field);

    std::getline(event_stream, field);
    event.close_price = std::stod(field);

    return event;
}

double qwap(Event event) {
    return (event.offer_price * event.bid_quantity + 
            event.bid_price * event.offer_quantity) / 
            (event.bid_quantity + event.offer_quantity);
}

double spread_percentage(Event event) {
    double midpoint = (event.bid_price + event.offer_price) / 2.0;
    return 100.0 * (event.offer_price - event.bid_price) / midpoint;
}

int connect_client() {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("socket");
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        throw std::runtime_error("connect");
    }

    return server_fd;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        throw std::runtime_error("expected two arguments: imbalance threshhold (percentage), QWAP output file");
    }

    double spread_threshhold = std::stod(argv[1]);

    std::ofstream file(argv[2], std::ios::app);
    if (!file.is_open()) {
        throw std::invalid_argument("failed to open output file");
        return 1;
    }

    int server_fd = connect_client();

    std::cout << "Connected\n";
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(server_fd, buffer, BUFFER_SIZE - 1)) != -1) {
        if (bytes_read == 0) break;
        buffer[bytes_read] = '\0';

        Event event = parse_string(buffer);

        double spread = spread_percentage(event);
        if (spread > spread_threshhold) {
            std::cout << "Imbalance of " + std::to_string(spread) +
                         " in excess of threshhold " + std::to_string(spread_threshhold) +
                         " at event:\n";
            std::cout << buffer;
        }
        file << std::to_string(qwap(event)) + '\n';
    }

    std::cout << "Connection closed.\n";
    file.close();
    close(server_fd);
    return 0;
}