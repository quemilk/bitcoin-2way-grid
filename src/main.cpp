#include "logger.h"
#include "global.h"
#include "user_data.h"
#include "command.h"
#include "public_channel.h"
#include "private_channel.h"
#include "json.h"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>
#include <fstream>
#include <boost/dll.hpp>

std::string g_api_key;
std::string g_passphrase;
std::string g_secret;
std::string g_ticket;

bool g_show_trades = false;

int main(int argc, char** argv) {
    init_logger();

    std::string enviorment;

    std::string socks_proxy;

    std::string setting_filename = "setting.conf";
    setting_filename = (boost::dll::program_location().parent_path() / setting_filename).string();

    try {
        std::ifstream in(setting_filename);
        if (!in.is_open()) {
            LOG(error) << "open " << setting_filename << " failed!";
            return -1;
        }

        std::string json_content((std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());

        rapidjson::Document doc;
        doc.Parse<0>(json_content.data(), json_content.size());

        enviorment = doc["enviorment"].GetString();
        g_api_key = doc["api_key"].GetString();
        g_passphrase = doc["passphrase"].GetString();
        g_secret = doc["secret"].GetString();
        g_ticket = doc["ticket"].GetString();

        if (enviorment.empty() || g_api_key.empty() || g_secret.empty()) {
            LOG(error) << "missing api_key or secret!";
            return -1;
        }

        if (g_ticket.empty()) {
            LOG(error) << "missing ticket!";
            return -1;
        }

        if (doc.HasMember("socks_proxy"))
            socks_proxy = doc["socks_proxy"].GetString();
    } catch (const std::exception& e) {
        LOG(error) << "pase " << setting_filename << " failed!" << e.what();
        return -1;
    }

    std::string host, port, private_path, public_path;
    if (enviorment == "simu") {
        host = SIMU_WSS_HOST;
        port = SIMU_WSS_PORT;
        private_path = SIMU_WSS_PRIVATE_CHANNEL;
        public_path = SIMU_WSS_PUBLIC_CHANNEL;
    } else if (enviorment == "product") {
        host = WSS_HOST;
        port = WSS_PORT;
        private_path = WSS_PRIVATE_CHANNEL;
        public_path = WSS_PUBLIC_CHANNEL;
    } else if (enviorment == "aws") {
        host = AWS_WSS_HOST;
        port = AWS_WSS_PORT;
        private_path = AWS_WSS_PRIVATE_CHANNEL;
        public_path = AWS_WSS_PUBLIC_CHANNEL;
    } else {
        LOG(error) << "invalid enviorenment setting! " << enviorment << ". (simu, product, aws)";
        return -1;
    }

    net::io_context ioc;
    net::io_context::work worker(ioc);
    std::thread t([&ioc]() { ioc.run(); });

    auto public_channel = std::make_shared<PublicChannel>(ioc, host, port, public_path, socks_proxy);
    auto private_channel = std::make_shared<PrivateChannel>(ioc, host, port, private_path, socks_proxy);

    private_channel->waitLogined();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (;;) {
        std::cout << "> ";

        std::string op;
        std::getline(std::cin, op);

        if (op == "help") {
            std::cout << "commands:" << std::endl;
            std::cout << "\tshow position" << std::endl;
            std::cout << "\tshow balance" << std::endl;
            std::cout << "\tshow trades" << std::endl;
            std::cout << "\thide trades" << std::endl;
        } else if (op == "show trades") {
            g_show_trades = true;
        } else if (op == "hide trades") {
            g_show_trades = false;
        } else if (op == "show position") {
            g_user_data.lock();
            make_scope_exit([] { g_user_data.unlock(); });
            std::cout << g_user_data.position_;
        } else if (op == "show balance") {
            g_user_data.lock();
            make_scope_exit([] { g_user_data.unlock(); });
            std::cout << g_user_data.balance_;
        }
    }

    t.join();
    return EXIT_SUCCESS;
}
