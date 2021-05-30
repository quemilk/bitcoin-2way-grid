#pragma once

class RestApi {
public:
    RestApi(const std::string& host);

    void setLeverage(int lever);

    void getLeverage();

private:
    std::string call(const std::string& verb, const std::string& req);

private:
    const std::string host_;
};
