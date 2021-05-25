#include "private_channel.h"
#include "Command.h"

extern std::string g_api_key;
extern std::string g_passphrase;
extern std::string g_secret;

void PrivateChannel::onConnected() {
    auto cmd = Command::makeLoginReq(g_api_key, g_passphrase, g_secret);


}
