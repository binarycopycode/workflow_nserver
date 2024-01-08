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
#include "workflow/Workflow.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include "wfrest/HttpServer.h"
#include "wfrest/json.hpp"
#include "hiredis/hiredis.h"

using Json = nlohmann::json;
using namespace protocol;

#define REDIRECT_MAX    5
#define RETRY_MAX       2

namespace http_client
{
    //default redisConnect("127.0.0.1", 6379)
    redisContext* get_redis_context();

    int is_data_node_online_in_redis(int node_index);
    int set_data_node_state_in_redis(int node_index ,int node_state);
    int add_date_to_redis(const std::string &ip_addr, const Json &all_date_list_json);
    int del_ip_in_redis(const std::string &ip_addr);

    int can_access_to_ip(const std::string &ip_addr);
    int query_data_node(const std::string &ip_addr, int node_index);
}