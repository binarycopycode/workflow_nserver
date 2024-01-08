#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <csignal>
#include <filesystem>
#include <iostream>

#include "workflow/WFFacilities.h"
#include "wfrest/HttpServer.h"
#include "wfrest/json.hpp"

using Json = nlohmann::json;

namespace http_server
{
    static WFFacilities::WaitGroup wait_group(1);
    void sig_handler(int signo);

    int read_data(std::vector<std::string> &datetime_list_, std::vector<Json> &json_list_,
                  std::vector<std::string> &npcbuf_data_list_, Json &meta_json);
    int run(unsigned short port);
}