﻿#include "logger.h"
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
std::shared_ptr<PublicChannel> g_public_channel;
std::shared_ptr<PrivateChannel> g_private_channel;

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

    LOG(info) << g_ticket;

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

    g_public_channel = std::make_shared<PublicChannel>(ioc, host, port, public_path, socks_proxy);
    g_private_channel = std::make_shared<PrivateChannel>(ioc, host, port, private_path, socks_proxy);

    g_private_channel->waitLogined();

    while (!g_user_data.balance_.inited || g_user_data.public_product_info_.data.empty() || g_user_data.public_trades_info_.trades_data.empty())
        std::this_thread::sleep_for(std::chrono::seconds(1));
   
    {
        g_user_data.lock();
        auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

        LOG(info) << g_user_data.balance_;
        LOG(info) << g_user_data.position_;
    }

    for (;;) {
        std::cout << "> ";

        std::string op;
        std::getline(std::cin, op);
        trimString(op);

        if (op == "show trades") {
            g_show_trades = true;
        } else if (op == "hide trades") {
            g_show_trades = false;
        } else if (op == "show instruments") {
            g_user_data.lock();
            auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });
            std::cout << g_user_data.public_product_info_;
        } else if (op == "show position") {
            g_user_data.lock();
            auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });
            std::cout << g_user_data.position_;
        } else if (op == "show balance") {
            g_user_data.lock();
            auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });
            std::cout << g_user_data.balance_;
        } else if (op == "start grid" || op == "test grid") {
            std::cout << "inject cash: ";
            std::string inject_cash;
            std::getline(std::cin, inject_cash);
            trimString(inject_cash);

            std::cout << "grid level (15): ";
            std::string grid_level;
            std::getline(std::cin, grid_level);
            trimString(grid_level);
            if (grid_level.empty())
                grid_level = "15";

            std::cout << "grid step (0.005): ";
            std::string grid_step;
            std::getline(std::cin, grid_step);
            trimString(grid_step);
            if (grid_step.empty())
                grid_step = "0.005";

            UserData::GridStrategy::Option option;
            option.injected_cash = strtof(inject_cash.c_str(), nullptr);
            option.grid_count = strtol(grid_level.c_str(), nullptr, 0);
            option.step_ratio = strtof(grid_step.c_str(), nullptr);

            if (op == "start grid") {
                std::cout << "run? (Y): ";
                std::string grid_run;
                std::getline(std::cin, grid_run);
                trimString(grid_run);
                if (grid_run.empty())
                    grid_run = "y";
                if (grid_run == "Y" || grid_run == "y")                
                    g_user_data.startGrid(option, false);
            } else
                g_user_data.startGrid(option, false, true);
        } else if (op == "stop grid") {
            g_user_data.clearGrid();
        } else if (op == "show grid") {
            g_user_data.lock();
            auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });
            std::cout << g_user_data.grid_strategy_;
        } else {
            std::cout << "commands:" << std::endl;
            std::cout << "\tshow position" << std::endl;
            std::cout << "\tshow balance" << std::endl;
            std::cout << "\tshow instruments" << std::endl;
            std::cout << "\tshow trades" << std::endl;
            std::cout << "\thide trades" << std::endl;
            std::cout << "\tshow grid" << std::endl;
            std::cout << "\tstart grid" << std::endl;
            std::cout << "\tstop grid" << std::endl;
            std::cout << "\ttest grid" << std::endl;
        }
    }

    t.join();
    return EXIT_SUCCESS;
}
