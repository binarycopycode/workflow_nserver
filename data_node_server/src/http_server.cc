#include "http_server.h"

void http_server::sig_handler(int signo)
{
    http_server::wait_group.done();
}

int http_server::read_data(std::vector<std::string> &datetime_list_, std::vector<Json> &json_list_,
                           std::vector<std::string> &npcbuf_data_list_, Json &meta_json)
{
    namespace fs = std::filesystem;

    // save all the datetime and sort them in string order
    std::string dir_path = "npcbuf/s5m/index";
    if (fs::is_directory(dir_path))
    {
        for (const auto &entry : fs::directory_iterator(dir_path))
        {
            if (entry.is_regular_file())
            {
                std::string filename = entry.path().filename().string();
                if (fs::path(filename).extension() == ".npcbuf")
                {
                    size_t dot_index = filename.rfind(".");
                    if (dot_index != std::string::npos)
                    {
                        std::string datetime = filename.substr(0, dot_index);
                        datetime_list_.emplace_back(datetime);
                    }
                }
            }
        }
    }
    else
    {
        std::cerr << "dir_path is not a directory" << std::endl;
        return -1;
    }

    std::sort(datetime_list_.begin(), datetime_list_.end());
    // vector pre reserve save the expand move time,
    npcbuf_data_list_.reserve(datetime_list_.size());

    for (const std::string &datetime : datetime_list_)
    {
        // read npcbuf file
        std::string npcbuf_file_path = dir_path + "/" + datetime + ".npcbuf";
        std::ifstream npcbuf_file(npcbuf_file_path, std::ios::binary);
        if (!npcbuf_file)
        {
            std::cerr << "can not open npcbuf file path:" << npcbuf_file_path << std::endl;
            return -1;
        }
        std::string npcbuf_content((std::istreambuf_iterator<char>(npcbuf_file)), std::istreambuf_iterator<char>());
        npcbuf_data_list_.emplace_back(npcbuf_content);
        npcbuf_file.close();

        // read json file
        std::string json_file_path = dir_path + "/" + datetime + ".json";
        std::ifstream json_file(json_file_path);
        if (!json_file)
        {
            std::cerr << "can not open json file path:" << json_file_path << std::endl;
            return -1;
        }
        Json json_content;
        json_file >> json_content;
        json_list_.emplace_back(json_content);
        json_file.close();
    }

    //read meta json 
    std::string meta_json_file_path=dir_path+"/meta.json";
    std::ifstream meta_json_file(meta_json_file_path);
    if(!meta_json_file)
    {
        std::cerr << "can not open meta json file path:" << meta_json_file_path << std::endl;
        return -1;
    }
    meta_json_file >> meta_json;
    
    return 0;
}

int http_server::run(unsigned short port)
{
    std::vector<std::string> datetime_list_;
    std::vector<Json> json_list_;
    std::vector<std::string> npcbuf_data_list_;
    Json meta_json;

    if (http_server::read_data(datetime_list_, json_list_, npcbuf_data_list_, meta_json) < 0)
    {
        std::cerr << "server read data failed" << std::endl;
        return -1;
    }
    //process ctrl+c in terminal
    signal(SIGINT, sig_handler);
    wfrest::HttpServer svr;

    svr.GET("/json/{datetime}", [&](const wfrest::HttpReq *req, wfrest::HttpResp *resp)
    {
        const std::string& datetime = req->param("datetime");
        // resp->set_status(HttpStatusOK); // automatically
        int file_index=std::lower_bound(datetime_list_.begin(),datetime_list_.end(),datetime)-datetime_list_.begin();
        resp->Json(json_list_[file_index]);
    });

    svr.GET("/npcbuf/{datetime}", [&](const wfrest::HttpReq *req, wfrest::HttpResp *resp) 
    {
        const std::string& datetime = req->param("datetime");
        // resp->set_status(HttpStatusOK); // automatically
        int file_index=std::lower_bound(datetime_list_.begin(),datetime_list_.end(),datetime)-datetime_list_.begin();
        resp->String(npcbuf_data_list_[file_index]); 
    });

    svr.GET("/metajson", [&](const wfrest::HttpReq *req, wfrest::HttpResp *resp) 
    {
        // resp->set_status(HttpStatusOK); // automatically
        resp->Json(meta_json); 
    });

    svr.GET("/alldate",[&](const wfrest::HttpReq *req, wfrest::HttpResp *resp) 
    {
        // resp->set_status(HttpStatusOK); // automatically
        Json all_date(datetime_list_);
        resp->Json(all_date);
        std::cout << "return all date succ" << std::endl; 
    });

    svr.GET("/heartbeat",[&](const wfrest::HttpReq *req, wfrest::HttpResp *resp) 
    {
        // resp->set_status(HttpStatusOK); // automatically
        resp->String("success"); 
        std::cout << "heart beat succ" << std::endl;
    });


    if (svr.start(port) == 0)
    {
        //wait done() notify
        wait_group.wait();
        svr.stop();
    }
    else
    {
        std::cerr << "server start listen port failed" << std::endl;
        return -1;
    }
    return 0;
}
