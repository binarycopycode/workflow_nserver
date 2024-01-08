#pragma once

#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <thread>
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include "wfrest/HttpServer.h"
#include "wfrest/json.hpp"
#include "hiredis/hiredis.h"

using Json = nlohmann::json;

namespace main_server
{
    const static std::vector<std::string> data_node_ip_list = 
    {
        "127.0.0.1:2333",
    };

    static std::atomic<bool> stopRequested(false);
    void query_all_data_node_per_hour();
    int query_all_data_node();

    static WFFacilities::WaitGroup wait_group(1);
    void sig_handler(int signo);

    int run(unsigned short port);
}