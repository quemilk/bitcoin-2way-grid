#include "private_channel.h"
#include "Command.h"
#include "logger.h"

extern std::string g_api_key;
extern std::string g_passphrase;
extern std::string g_secret;

void PrivateChannel::onConnected() {
    auto req = Command::makeLoginReq(g_api_key, g_passphrase, g_secret);
    this->sendCmd(std::move(req),
        [this](Command::Response& resp) {
            if (resp.code == 0) {
                LOG(debug) << "login succeeded.";
            }
        }
    );

    LOG(debug) << "send login. api_key=" << g_api_key;
}
