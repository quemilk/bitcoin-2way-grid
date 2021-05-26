﻿#include "private_channel.h"
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
                LOG(debug) << "<< login ok.";
                this->onLogined();
            } else
                throw std::runtime_error("<< login failed! " + resp.msg);
        }
    );
}

void PrivateChannel::onLogined() {
    auto req = Command::makeSubscriBebalanceAndPositionsChannel();

    LOG(debug) << ">> subscribe. " << req.data;

    this->sendCmd(std::move(req),
        [this](Command::Response& resp) {
            if (resp.code == 0)
                LOG(debug) << "<< subscribe ok. " << resp.data;
            else
                throw std::runtime_error("<< subscribe failed! " + resp.msg);
        }
    );

    req = Command::makeSubscribeOrdersChannel();
    LOG(debug) << ">> subscribe. " << req.data;

    this->sendCmd(std::move(req),
        [this](Command::Response& resp) {
            if (resp.code == 0)
                LOG(debug) << "<< subscribe ok. " << resp.data;
            else
                throw std::runtime_error("<< subscribe failed! " + resp.msg);
        }
    );

    std::unique_lock lock(cond_mutex_);
    logined_ = true;
    conn_condition_.notify_one();
}

void PrivateChannel::waitLogined() {
    std::unique_lock lock(cond_mutex_);
    if (logined_)
        return;
    conn_condition_.wait(lock);
}
