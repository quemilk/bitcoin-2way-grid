#pragma once

//实盘API交易地址如下：

#define REST_API_HOST               "https://www.okex.com"
#define WSS_HOST                    "ws.okex.com"
#define WSS_PORT                    "8443"
#define WSS_PUBLIC_CHANNEL          "/ws/v5/public"
#define WSS_PRIVATE_CHANNEL         "/ws/v5/private"

// AWS 地址如下：
#define AWS_REST_API_HOST           "https://aws.okex.com"
#define AWS_WSS_HOST                "wsaws.okex.com"
#define AWS_WSS_PORT                "8443"
#define AWS_WSS_PUBLIC_CHANNEL      "/ws/v5/public"
#define AWS_WSS_PRIVATE_CHANNEL     "/ws/v5/private"

// 模拟盘API交易地址如下：
#define SIMU_REST_API_HOST          "https://www.okex.com"
#define SIMU_WSS_HOST               "wspap.okex.com"
#define SIMU_WSS_PORT               "8443"
#define SIMU_WSS_PUBLIC_CHANNEL     "/ws/v5/public?brokerId=9999"
#define SIMU_WSS_PRIVATE_CHANNEL    "/ws/v5/private?brokerId=9999"
