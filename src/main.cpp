#include "ws_session.h"
#include "logger.h"
#include "global.h"
#include "command.h"
#include "json.h"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>
#include <fstream>

int main(int argc, char** argv) {
    init_logger();

    std::string api_key;
    std::string passphrase;
    std::string secret;
    std::string enviorment;

    std::string socks5_proxy = "socks5://127.0.0.1:9980";

    try {
        std::ifstream in("setting.json");
        if (!in.is_open()) {
            LOG(error) << "open setting.json failed!";
            return -1;
        }

        std::string json_content((std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());

        rapidjson::Document doc;
        doc.Parse<0>(json_content.data(), json_content.size());

        enviorment = doc["enviorment"].GetString();
        api_key = doc["api_key"].GetString();
        passphrase = doc["passphrase"].GetString();
        secret = doc["secret"].GetString();

        if (enviorment.empty() || api_key.empty() || secret.empty()) {
            LOG(error) << "missing api_key or secret!";
            return -1;
        }

        if (doc.HasMember("socks_proxy"))
            socks5_proxy = doc["socks_proxy"].GetString();
    } catch (std::exception& e) {
        LOG(error) << "pase setting.json failed!" << e.what();
        return -1;
    }

    auto const host = SIMU_WSS_HOST;
    auto const port = SIMU_WSS_PORT;
    auto const path = SIMU_WSS_PRIVATE_CHANNEL;

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12_client };

    // Launch the asynchronous operation
    auto ws_session = std::make_shared<WSSession>(ioc, ctx);
    ws_session->setSocksProxy(socks5_proxy.c_str());
    ws_session->run(host, port, path);

    std::thread thread([&] { ioc.run(); });

    ws_session->waitUtilConnected(std::chrono::seconds(10));

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
