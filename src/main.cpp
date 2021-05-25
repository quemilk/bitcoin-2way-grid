#include "logger.h"
#include "global.h"
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

int main(int argc, char** argv) {
    init_logger();

    std::string api_key;
    std::string passphrase;
    std::string secret;
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
        api_key = doc["api_key"].GetString();
        passphrase = doc["passphrase"].GetString();
        secret = doc["secret"].GetString();

        if (enviorment.empty() || api_key.empty() || secret.empty()) {
            LOG(error) << "missing api_key or secret!";
            return -1;
        }

        if (doc.HasMember("socks_proxy"))
            socks_proxy = doc["socks_proxy"].GetString();
    } catch (std::exception& e) {
        LOG(error) << "pase " << setting_filename << " failed!" << e.what();
        return -1;
    }

    std::string host, port, path;
    if (enviorment == "simu") {
        host = SIMU_WSS_HOST;
        port = SIMU_WSS_PORT;
        path = SIMU_WSS_PRIVATE_CHANNEL;
    } else if (enviorment == "product") {
        host = WSS_HOST;
        port = WSS_PORT;
        path = WSS_PRIVATE_CHANNEL;
    } else if (enviorment == "aws") {
        host = AWS_WSS_HOST;
        port = AWS_WSS_PORT;
        path = AWS_WSS_PRIVATE_CHANNEL;
    } else {
        LOG(error) << "invalid enviorenment setting! " << enviorment << ". (simu, product, aws)";
        return -1;
    }

    auto public_channel = std::make_shared<PublicChannel>(host, port, path, socks_proxy);


    /*
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
    */

    getchar();
    return EXIT_SUCCESS;
}
