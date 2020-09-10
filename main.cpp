
#include <unistd.h> //uni std
#include <arpa/inet.h>
#include <string.h>
#include <sys/epoll.h>

#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#include <stdio.h>
#include <thread>
#include <vector>
#include <algorithm>

std::vector<SOCKET> g_clients;
bool g_run = true;
void cmdThread() {
    while (true) {
        char cmdMsg[32];
        scanf("%s", cmdMsg);
        if (0 == strcmp(cmdMsg, "exit")) {
            printf("线程退出 \n");
            g_run = false;
            //client->Close();
            break;
        }
    }
}

int cell_epoll_ctl(int epfd, int op, int sockfd, uint32_t events){
    epoll_event ev;
    ev.events = events;
    ev.data.fd = sockfd;
    epoll_ctl(epfd, op, sockfd, &ev);
}

int recvData(SOCKET _cSock){
    char szRecv[4096] = {};
    int nLen = (int)recv(_cSock, szRecv, 4096, 0);
    if(nLen<=0){
        printf("client exit socket = %d\n", _cSock);
        return -1;
    } 

    return nLen;
}

int clientLeave(SOCKET cSock){
    close(cSock);
    auto itr = std::find(g_clients.begin(), g_clients.end(), cSock);
    g_clients.erase(itr);
}

int main(){
    
    std::thread cmd(cmdThread);
    cmd.detach(); //和主线程分离

    //-socket
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //ipv4, 流数据, tcp
    if (INVALID_SOCKET == _sock) {
        printf("socket error \n");
    } else {
        printf("socket success \n");
    }
    //-bind-
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567); //主机数据转换到网络数据 host to net unsigned short
    _sin.sin_addr.s_addr = inet_addr("192.168.1.181"); //INADDR_ANY; // ip

    if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in))) {
        printf("bind error \n");
    } else {
        printf("bind success \n");
    }
    //-listen-
    if (SOCKET_ERROR == listen(_sock, 5)) {//监听人数 
        printf("listen error \n");
    } else {
        printf("listen success \n");
    }
    //linux 2.6.8(参数无意义)
    //cat /proc/sys/fs/file-max
    int epfd = epoll_create(256);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = _sock;
    //向epoll添加
    epoll_ctl(epfd, EPOLL_CTL_ADD, _sock, &ev);

    epoll_event events[256] = {};
    while (g_run
     ) {
        //-1 有事件才返回， 0立即返回
        int n = epoll_wait(epfd, events, 256, 1);
        if(n<0){
            printf("error\n");
            break;
        }
        for(int i = 0; i < n;i++){
            if(events[i].data.fd == _sock){
                 sockaddr_in clientAddr = {};
                int nAddrLen = sizeof(sockaddr_in);
                SOCKET _cSock = INVALID_SOCKET; //无效socket
                _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
                if (INVALID_SOCKET == _cSock) {
                    printf("error, client invalid socket \n");
                }else{
                    g_clients.push_back(_cSock);
                    cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _cSock, EPOLLIN);
                    printf("add new client :socket = %d, ip = %s\n", (int)_sock, inet_ntoa(clientAddr.sin_addr));
                }
            }else if(events[i].events & EPOLLIN){
                auto cSockfd = events[i].data.fd;
                int ret = recvData(cSockfd);
                if(ret <= 0){
                    clientLeave(cSockfd);
                    printf("client :socket = %d exit\n", (int)cSockfd);
                }else{
                    printf("client :socket = %d len = %d\n", (int)cSockfd, ret);
                }
                
            }
        }
        
    }
    
    for(auto client: g_clients){
        close(client);
    }

    close(epfd);
    close(_sock);
    printf("server exist out\n");
    getchar();
    return 0;
}