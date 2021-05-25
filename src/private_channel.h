#pragma once

#include "channel.h"


class PrivateChannel : public Channel {
public:
    using Channel::Channel;

protected:
    void onConnected() override;








};
