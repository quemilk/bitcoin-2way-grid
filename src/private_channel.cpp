#include "private_channel.h"
#include "Command.h"
#include "logger.h"

extern std::string g_api_key;
extern std::string g_passphrase;
extern std::string g_secret;

void PrivateChannel::onConnected() {
    auto cmd = Command::makeLoginReq(g_api_key, g_passphrase, g_secret);
    ws_session_->send(cmd);
    LOG(debug) << "send login. api_key=" << g_api_key;
}
