#include "private_channel.h"
#include "Command.h"
#include "logger.h"

extern std::string g_api_key;
extern std::string g_passphrase;
extern std::string g_secret;

void PrivateChannel::onConnected() {
    auto req = Command::makeLoginReq(g_api_key, g_passphrase, g_secret);

    LOG(debug) << ">> login. api_key=" << g_api_key;

    this->sendCmd(std::move(req),
        [this](Command::Response& resp) {
            if (resp.code == 0) {
                LOG(debug) << "<< login succeeded.";
                this->onLogined();
            } else
                throw std::runtime_error("<< login failed! " + resp.msg);
        }
    );
}

void PrivateChannel::onLogined() {
    auto req = Command::makeSubscribeAccountChannel();

    LOG(debug) << ">> subscribe. " << req.data;

    this->sendCmd(std::move(req),
        [this](Command::Response& resp) {
            if (resp.code == 0)
                LOG(debug) << "<< subscribe succeeded. " << resp.data;
            else
                LOG(debug) << "<< subscribe failed. " << resp.data;
        }
    );
}
