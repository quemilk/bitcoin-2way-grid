#include "ws_session.h"
#include "logger.h"
#include "global.h"
#include <cstdlib>
#include <functional>
#include <iostream>

int main(int argc, char** argv) {
    init_logger();

    auto const host = SIMU_WSS_HOST;
    auto const port = SIMU_WSS_PORT;
    auto const path = SIMU_WSS_PUBLIC_CHANNEL;
    auto const socks5 = "socks5://127.0.0.1:9980";

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12_client };

    // Launch the asynchronous operation
    auto ws_session = std::make_shared<WSSession>(ioc, ctx);
    ws_session->setSocksProxy(socks5);
    ws_session->run(host, port, path);

    // Run the I/O service. The call will return when
    // the socket is closed.
    ioc.run();

    return EXIT_SUCCESS;
}