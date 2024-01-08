#include "http_client.h"

//------------------------redis part ---------------------------------------------

redisContext *http_client::get_redis_context()
{
    redisContext *redis_ctx_ptr = redisConnect("127.0.0.1", 6379);
    return redis_ctx_ptr;
}

int http_client::is_data_node_online_in_redis(int node_index)
{
    const char *key = "data_node_status";
    redisContext *redis_ctx_ptr = get_redis_context();
    if (redis_ctx_ptr == nullptr || redis_ctx_ptr->err)
    {
        if (redis_ctx_ptr)
        {
            std::cerr << "redis connection error" << redis_ctx_ptr->err << std::endl;
            redisFree(redis_ctx_ptr);
        }
        else
        {
            std::cerr << "Connection error: can't allocate redis context" << std::endl;
        }
        return -1;
    }
    redisReply *reply = (redisReply *)redisCommand(redis_ctx_ptr, "GETBIT %s %d", key, node_index);
    int redis_status = reply->integer;
    freeReplyObject(reply);
    redisFree(redis_ctx_ptr);
    return redis_status;
}

int http_client::set_data_node_state_in_redis(int node_index, int node_state)
{
    const char *key = "data_node_status";
    redisContext *redis_ctx_ptr = get_redis_context();
    if (redis_ctx_ptr == nullptr || redis_ctx_ptr->err)
    {
        if (redis_ctx_ptr)
        {
            std::cerr << "redis connection error" << redis_ctx_ptr->err << std::endl;
            redisFree(redis_ctx_ptr);
        }
        else
        {
            std::cerr << "Connection error: can't allocate redis context" << std::endl;
        }
        return -1;
    }

    redisReply *reply = (redisReply *)redisCommand(redis_ctx_ptr, "SETBIT %s %d %d", key, node_index, node_state);
    freeReplyObject(reply);
    redisFree(redis_ctx_ptr);
    return 0;
}

int http_client::add_date_to_redis(const std::string &ip_addr, const Json &all_date_list_json)
{
    redisContext *redis_ctx_ptr = get_redis_context();
    if (redis_ctx_ptr == nullptr || redis_ctx_ptr->err)
    {
        if (redis_ctx_ptr)
        {
            std::cerr << "redis connection error" << redis_ctx_ptr->err << std::endl;
            redisFree(redis_ctx_ptr);
        }
        else
        {
            std::cerr << "Connection error: can't allocate redis context" << std::endl;
        }
        return -1;
    }

    redisReply *reply;
    for (auto &json_item : all_date_list_json)
    {
        std::string date(json_item);
        reply = (redisReply *)redisCommand(redis_ctx_ptr, "SADD %s %s", ip_addr.c_str(), date.c_str());
        freeReplyObject(reply);
        reply = (redisReply *)redisCommand(redis_ctx_ptr, "SADD %s %s", date.c_str(), ip_addr.c_str());
        freeReplyObject(reply);
    }
    redisFree(redis_ctx_ptr);
    std::cout << "success add all date from ip:" << ip_addr << " in redis" << std::endl;
    return 0;
}

int http_client::del_ip_in_redis(const std::string &ip_addr)
{
    redisContext *redis_ctx_ptr = get_redis_context();
    if (redis_ctx_ptr == nullptr || redis_ctx_ptr->err)
    {
        if (redis_ctx_ptr)
        {
            std::cerr << "redis connection error" << redis_ctx_ptr->err << std::endl;
            redisFree(redis_ctx_ptr);
        }
        else
        {
            std::cerr << "Connection error: can't allocate redis context" << std::endl;
        }
        return -1;
    }

    std::vector<std::string> ip2date_list;

    redisReply *reply = (redisReply *)redisCommand(redis_ctx_ptr, "SMEMBERS %s", ip_addr.c_str());
    if (reply != nullptr)
    {
        for (size_t i = 0; i < reply->elements; ++i)
        {
            ip2date_list.emplace_back(reply->element[i]->str);
        }
    }
    else
    {
        std::cerr << "can not find ip2date set: " << ip_addr << "in redis" << std::endl;
    }
    freeReplyObject(reply);

    for(auto &date : ip2date_list)
    {
        reply = (redisReply *)redisCommand(redis_ctx_ptr, "SREM %s %s", date.c_str(), ip_addr.c_str());
        freeReplyObject(reply);
    }

    reply = (redisReply *)redisCommand(redis_ctx_ptr, "DEL %s", ip_addr.c_str());
    freeReplyObject(reply);
    
    redisFree(redis_ctx_ptr);
    std::cout << "success del ip:" << ip_addr << " in redis" << std::endl;
    return 0;
}

//-------------------------------------- ---------------------------------------------

struct query_data_node_context
{
    std::string ip_addr;
    int node_index;
    std::string heart_beat_url;
    std::string all_date_url;
    bool redis_error;
};

void all_date_callback(WFHttpTask *all_date_task)
{
    HttpResponse *resp = all_date_task->get_resp();
    int state = all_date_task->get_state();

    if (state != WFT_STATE_SUCCESS)
    {
        std::cerr << "after heart beat succ but get all date failed" << std::endl;
        return;
    }

    SeriesWork *series = series_of(all_date_task); /* get the series of this task */
    query_data_node_context *context =
        (query_data_node_context *)series->get_context();

    const void *body;
    size_t body_len;

    resp->get_parsed_body(&body, &body_len);
    std::string all_date_str((char *)body, body_len);
    Json all_date_list_json = Json::parse(all_date_str);

    if(http_client::add_date_to_redis(context->ip_addr, all_date_list_json) == -1 )
    {
        context->redis_error = true;
        return;
    }
}

void heart_beat_callback(WFHttpTask *heart_beat_task)
{
    int state = heart_beat_task->get_state();

    int http_state = 0;
    if (state == WFT_STATE_SUCCESS)
        http_state = 1;

    SeriesWork *series = series_of(heart_beat_task); /* get the series of this task */
    query_data_node_context *context =
        (query_data_node_context *)series->get_context();

    int redis_state = http_client::is_data_node_online_in_redis(context->node_index);
    if (redis_state == -1)
    {
        context->redis_error = true;
        return;
    }

    if (redis_state == 0 && http_state == 1)
    {
        WFHttpTask *all_date_task = WFTaskFactory::create_http_task(context->all_date_url,
                                                                    REDIRECT_MAX, RETRY_MAX,
                                                                    all_date_callback);

        if (http_client::set_data_node_state_in_redis(context->node_index, 1) == -1)
        {
            context->redis_error = true;
            return;
        }

        series->push_back(all_date_task);
    }
    else if (redis_state == 1 && http_state == 0)
    {
        if (http_client::set_data_node_state_in_redis(context->node_index, 0) == -1)
        {
            context->redis_error = true;
            return;
        }
        
        if (http_client::del_ip_in_redis(context->ip_addr) == -1)
        {
            context->redis_error = true;
            return;
        }
    }
}

int http_client::can_access_to_ip(const std::string &ip_addr)
{
    WFHttpTask *can_access_task;
    WFFacilities::WaitGroup can_access_wait_group(1);
    int access_state = 0;
    auto can_access_callback = [&can_access_wait_group, &access_state](WFHttpTask *can_access_task)
    {
        HttpResponse *resp = can_access_task->get_resp();
        int state = can_access_task->get_state();
        if (state == WFT_STATE_SUCCESS)
            access_state = 1;

        can_access_wait_group.done();
    };
    
    std::string url = "http://" + ip_addr + "/heartbeat";
    can_access_task = WFTaskFactory::create_http_task(url,
                                                      REDIRECT_MAX, RETRY_MAX,
                                                      can_access_callback);
    HttpRequest *req = can_access_task->get_req();
    req->add_header_pair("Accept", "*/*");
    req->add_header_pair("Connection", "close");

    can_access_task->start();
    can_access_wait_group.wait();
    
    return access_state;
}

int http_client::query_data_node(const std::string &ip_addr, int node_index)
{
    WFHttpTask *heart_beat_task;
    struct query_data_node_context context;
    context.ip_addr = ip_addr;
    context.node_index = node_index;
    context.heart_beat_url = "http://" + ip_addr + "/heartbeat";
    context.all_date_url = "http://" + ip_addr + "/alldate";
    context.redis_error = false;

    heart_beat_task = WFTaskFactory::create_http_task(context.heart_beat_url,
                                                      REDIRECT_MAX, RETRY_MAX,
                                                      heart_beat_callback);
    HttpRequest *req = heart_beat_task->get_req();
    req->add_header_pair("Accept", "*/*");
    req->add_header_pair("Connection", "close");

    WFFacilities::WaitGroup wait_group(1);
    auto series_callback = [&wait_group](const SeriesWork *series)
    {
        query_data_node_context *context = (query_data_node_context *)
                                               series->get_context();
        if (context->redis_error)
            std::cerr << "redis connection error" << std::endl;

        wait_group.done();
    };

    /* Create a series */
    SeriesWork *series = Workflow::create_series_work(heart_beat_task,
                                                      series_callback);
    series->set_context(&context);
    series->start();

    wait_group.wait();
    return 0;
}
