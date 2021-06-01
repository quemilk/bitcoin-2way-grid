#pragma once

#include "channel.h"


class PrivateChannel : public Channel {
public:
    using Channel::Channel;

    void waitLogined();

protected:
    void onConnected() override;

    void onLogined();

private:
    std::mutex cond_mutex_;
    bool logined_ = false;
    std::condition_variable conn_condition_;

};

extern std::shared_ptr<PrivateChannel> g_private_channel;
