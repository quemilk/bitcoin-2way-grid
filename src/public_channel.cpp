#include "public_channel.h"
#include "Command.h"
#include "logger.h"

extern std::string g_ticket;

void PublicChannel::onConnected() {
    auto req = Command::makeSubscribeInstrumentsChannel("SWAP");

    LOG(debug) << ">> subscribe. " << req.data;

    this->sendCmd(std::move(req),
        [this](Command::Response& resp) {
            if (resp.code == 0)
                LOG(debug) << "<< subscribe ok. " << resp.data;
            else
                throw std::runtime_error("<< subscribe failed! " + resp.msg);
        }
    );

  /*  req = Command::makeSubscribeTickersChannel(g_ticket);

    LOG(debug) << ">> subscribe. " << req.data;

    this->sendCmd(std::move(req),
        [this](Command::Response& resp) {
            if (resp.code == 0)
                LOG(debug) << "<< subscribe ok. " << resp.data;
            else
                throw std::runtime_error("<< subscribe failed! " + resp.msg);
        }
    );*/
}
