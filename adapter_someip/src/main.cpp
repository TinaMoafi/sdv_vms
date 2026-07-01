#include <vsomeip/vsomeip.hpp>

#include <atomic>
#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr vsomeip::service_t SERVICE_ID = 0x1234;
constexpr vsomeip::instance_t INSTANCE_ID = 0x5678;
constexpr vsomeip::event_t EVENT_ID = 0x8778;
constexpr vsomeip::eventgroup_t EVENTGROUP_ID = 0x4465;

constexpr const char *APP_NAME = "lidar_adapter";
constexpr const char *EMULATOR_BIND_IP = "127.0.0.1";
constexpr uint16_t EMULATOR_BIND_PORT = 25000;

std::shared_ptr<vsomeip::application> app;
std::atomic_bool running{true};

void handle_signal(int) {
    running = false;
    if (app) {
        app->stop_offer_event(SERVICE_ID, INSTANCE_ID, EVENT_ID);
        app->stop_offer_service(SERVICE_ID, INSTANCE_ID);
        app->stop();
    }
}

std::shared_ptr<vsomeip::payload> make_payload(const std::string &frame) {
    std::vector<vsomeip::byte_t> bytes(frame.begin(), frame.end());

    auto payload = vsomeip::runtime::get()->create_payload();
    payload->set_data(bytes);
    return payload;
}

void publish_frame(const std::string &frame) {
    if (!app || frame.empty()) {
        return;
    }

    auto payload = make_payload(frame);
    app->notify(SERVICE_ID, INSTANCE_ID, EVENT_ID, payload);

    std::cout << "[adapter] Published SOME/IP event 0x"
              << std::hex << EVENT_ID
              << std::dec << ", payload bytes=" << frame.size()
              << std::endl;
}

void udp_receive_loop() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        std::cerr << "[adapter] Failed to create UDP socket" << std::endl;
        return;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(EMULATOR_BIND_PORT);

    if (inet_pton(AF_INET, EMULATOR_BIND_IP, &addr.sin_addr) != 1) {
        std::cerr << "[adapter] Invalid bind IP" << std::endl;
        close(fd);
        return;
    }

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[adapter] Failed to bind UDP "
                  << EMULATOR_BIND_IP << ":" << EMULATOR_BIND_PORT
                  << " error=" << std::strerror(errno)
                  << std::endl;
        close(fd);
        return;
    }

    std::cout << "[adapter] Listening for emulator UDP frames on "
              << EMULATOR_BIND_IP << ":" << EMULATOR_BIND_PORT
              << std::endl;

    std::vector<char> buffer(65535);

    while (running) {
        ssize_t received = recv(fd, buffer.data(), buffer.size(), 0);

        if (received < 0) {
            if (running) {
                std::cerr << "[adapter] UDP receive failed: "
                          << std::strerror(errno)
                          << std::endl;
            }
            continue;
        }

        std::string frame(buffer.data(), static_cast<size_t>(received));
        std::cout << "[adapter] Received emulator frame, bytes="
                  << frame.size()
                  << std::endl;

        publish_frame(frame);
    }

    close(fd);
}

void on_state(vsomeip::state_type_e state) {
    if (state != vsomeip::state_type_e::ST_REGISTERED) {
        return;
    }

    std::set<vsomeip::eventgroup_t> eventgroups;
    eventgroups.insert(EVENTGROUP_ID);

    app->offer_service(SERVICE_ID, INSTANCE_ID);

    app->offer_event(
        SERVICE_ID,
        INSTANCE_ID,
        EVENT_ID,
        eventgroups,
        vsomeip::event_type_e::ET_EVENT,
        std::chrono::milliseconds::zero(),
        false,
        true
    );

    std::cout << "[adapter] Offered SOME/IP service 0x"
              << std::hex << SERVICE_ID
              << ", instance 0x" << INSTANCE_ID
              << ", event 0x" << EVENT_ID
              << ", eventgroup 0x" << EVENTGROUP_ID
              << std::dec << std::endl;
}

} // namespace

int main() {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    app = vsomeip::runtime::get()->create_application(APP_NAME);

    if (!app->init()) {
        std::cerr << "[adapter] Failed to initialize vSomeIP application" << std::endl;
        return 1;
    }

    app->register_state_handler(on_state);

    std::thread udp_thread(udp_receive_loop);

    std::cout << "[adapter] Starting vSomeIP app: " << APP_NAME << std::endl;
    app->start();

    running = false;

    if (udp_thread.joinable()) {
        udp_thread.join();
    }

    return 0;
}