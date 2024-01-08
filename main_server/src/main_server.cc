#include "http_client.h"
#include "main_server.h"

void main_server::sig_handler(int signo)
{
    wait_group.done();
}

int main_server::query_all_data_node()
{
    std::cout << "start query all data node" << std::endl;
    int data_node_num = main_server::data_node_ip_list.size();
    for (int i = 0; i < data_node_num; i++)
    {
        if(http_client::query_data_node(data_node_ip_list[i], i) < 0)
        {
            std::cerr << "redis query node i:" << i << "  failed" << std::endl;
            return -1;
        }
    }
    std::cout << "query all data node finished" << std::endl;
    return 0;
}

void main_server::query_all_data_node_per_hour()
{
    std::this_thread::sleep_for(std::chrono::seconds(3600));
    while (!stopRequested.load())
    {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        char buffer[80];
        std::strftime(buffer, 80, "%Y-%m-%d %H:%M", std::localtime(&currentTime));
        std::cerr << std::endl;
        std::cerr << "query all data node at time: " << buffer << std::endl;
        if (query_all_data_node() < 0)
        {
            std::cerr << "query all data node per hour failed" << std::endl;
        }
        else
        {
            std::cerr << "query all data node per hour success" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(3600));
    }
}

int main_server::run(unsigned short port)
{
    if (main_server::query_all_data_node() < 0)
    {
        std::cerr << "initiallly query all data node failed" << std::endl;
        return -1;
    }
    std::thread query_per_hour_thread(main_server::query_all_data_node_per_hour);

    signal(SIGINT, sig_handler);
    wfrest::HttpServer svr;

    svr.GET("/{datetime}", [&](const wfrest::HttpReq *req, wfrest::HttpResp *resp)
    {
        const std::string& datetime = req->param("datetime");
        redisContext *redis_ctx_ptr = http_client::get_redis_context();
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
            resp->String("redis failed");
            return;
        }

        std::vector<std::string> date2ip_list;

        redisReply *reply = (redisReply *)redisCommand(redis_ctx_ptr, "SMEMBERS %s", datetime.c_str());
        if (reply != nullptr)
        {
            for (size_t i = 0; i < reply->elements; ++i)
            {
                date2ip_list.emplace_back(reply->element[i]->str);
            }
        }
        else
        {
            std::cerr << "can not find date2ip set: " << datetime << "in redis" << std::endl;
        }
        freeReplyObject(reply);
        redisFree(redis_ctx_ptr);
        
        std::string return_str = "find ip failed";
        for(auto &ip_addr: date2ip_list)
        {
            if(http_client::can_access_to_ip(ip_addr))
            {
                return_str = ip_addr;
                break;
            }
        }
        std::cout << "main_server : get req date:" << datetime << "  return resp:" << return_str << std::endl; 
        resp->String(return_str); 
    });

    if (svr.start(port) == 0)
    {
        wait_group.wait();
        svr.stop();
    }
    else
    {
        std::cerr << "main_server start listen port failed" << std::endl;
        stopRequested.store(true);
        query_per_hour_thread.join();
        return -1;
    }
    stopRequested.store(true);
    query_per_hour_thread.join();
    return 0;
}