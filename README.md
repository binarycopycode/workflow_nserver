# Dependencies

+ [workflow](https://github.com/sogou/workflow) 
+ [wfrest](https://github.com/wfrest/wfrest)
+ [hiredis](https://github.com/redis/hiredis)
+ gcc , g++  >=10

# Npcbuf Data Preparation

For testing, download the data in issue and extract the data to the directory `/data_node_server/npcbuf`.

Or you can change the `std::string dir_path` in the source code file `/data_node_server/src/http_server.cc:line14`.

# Install and run on pure Ubuntu18.04

Install git

```shell
sudo apt install git
```

Install wfrest [Requirement](https://github.com/wfrest/wfrest/blob/main/docs/requirement.md#requirement)

```shell
sudo apt-get install build-essential cmake zlib1g-dev libssl-dev libgtest-dev -y
```

Install gcc-10 g++-10

```shell
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt upgrade
sudo apt install gcc-10
sudo apt install g++-10
```

Install wfrest

```shell
git clone --recursive https://github.com/wfrest/wfrest
cd wfrest
make
sudo make install
```

Install workflow, `\wfrest\workflow`

```shell
cd workflow
make
sudo make install
```

Install redis

```shell
sudo apt install redis-server
```

Install hiredis

```shell
git clone https://github.com/redis/hiredis.git
cd hiredis
make
sudo make install
```

install pip3 numpy pandas

```shell
sudo apt install python3-pip
sudo pip3 install numpy
sudo pip3 install pandas
```



clone this project

```shell
git clone https://github.com/qinwf/np_rdma.git
cd np_rdma
```

compile data_node_server  (use g++-10)

```shell
cd data_node_server
mkdir build
cd build
export CXX=/usr/bin/g++-10
cmake ..
make
cd ..
cd ..

```

compile main_server

```shell
cd main_server
mkdir build
cd build
export CXX=/usr/bin/g++-10
cmake ..
make
cd ..
cd ..

```

run the data_node_server listening to the port 2333

```shell
cd data_node_server
./data_node_server 2333
```

open another terminal window and cd  `/np_rdma`

run the main_server listening to the port 8888

```
cd main_server
./main_server 8888
```

open another terminal window and cd  `/np_rdma`

run the python client

```
cd python_client
python3 main.py
```

if you get `<class 'numpy.ndarray'>` 

it means python client get the data successfully

# install on CentOS 

follow as ubuntu version.

Tips1: When you can not load shared libraries, add `/usr/local/lib` to the `/etc/ld.so.conf` and run `ldconfig` . 

Tips2: `scl enable gcc-toolset-10 bash` before cmake
