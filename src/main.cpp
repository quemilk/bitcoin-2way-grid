#include "ws_session.h"
#include "logger.h"
#include "global.h"
#include "command.h"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    init_logger();

    auto const host = SIMU_WSS_HOST;
    auto const port = SIMU_WSS_PORT;
    auto const path = SIMU_WSS_PRIVATE_CHANNEL;
    auto const socks5 = "socks5://127.0.0.1:9980";

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12_client };

    // Launch the asynchronous operation
    auto ws_session = std::make_shared<WSSession>(ioc, ctx);
    ws_session->setSocksProxy(socks5);
    ws_session->run(host, port, path);

    std::thread thread([&] { ioc.run(); });

    ws_session->waitUtilConnected(std::chrono::seconds(10));

    auto const api_key = "c8e7a07f-fbdd-4554-8511-5e379332f395";
    auto const passphrase = "123456";
    auto const secret = "485F4DB9CC2606A345E5609BB45914DC";
    auto cmd = Command::makeLoginReq(api_key, passphrase, secret);

    ws_session->send(cmd);
    LOG(debug) << "send login: " << cmd;

    std::string resp;
    ws_session->read(&resp);
    LOG(debug) << "resp: " << resp;

    cmd = Command::makeSubscribeAccountChannel();
    ws_session->send(cmd);
    LOG(debug) << "send subscribe: " << cmd;

    ws_session->read(&resp);
    LOG(debug) << "resp: " << resp;

    ws_session->read(&resp);
    LOG(debug) << "resp: " << resp;
    thread.join();


    return EXIT_SUCCESS;
}
