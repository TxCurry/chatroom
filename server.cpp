#include "utility.h"
int main(int argc,char* argv[]){
    //服务器IP+端口
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    //创建监听socket
    int listener = socket(PF_INET,SOCK_STREAM,0);
    if(listener < 0){
        perror("listener");
        exit(-1); 
    }
    //将服务器地址与监听socket绑定
    if(bind(listener, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0){
        perror("bind error");
        exit(-1);
    }
    //监听连接listen
    int ret = listen(listener, 5);
    if(ret < 0){
        perror("listen error");
        exit(-1);
    }
    printf("start to listen:%s\n",SERVER_IP);
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0){
        perror("epfd error");
        exit(-1);  
    }
    printf("epoll created,epollfd=%d\n",epfd);
    static struct epoll_event events[EPOLL_SIZE];
    //往内核事件表里添加事件
    addfd(epfd,listener,true);
    while(1){
        //epoll_events_count表示就绪时间的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0){
            perror("epoll failure");
            break;
        }
        printf("epoll_events_count=%d\n",epoll_events_count);
        //处理就绪事件
        for(int i = 0;i < epoll_events_count;i++){
            int sockfd = events[i].data.fd;
            //有新用户连接
            if(sockfd == listener){
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept(listener,(struct sockaddr *)&client_address, &client_addrLength);
                printf("client connection from %s:%d(IP:PORT),clientfd=%d\n",inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port),clientfd);
                addfd(epfd, clientfd, true);
                
                //服务端用list保存用户连接
                clients_list.push_back(clientfd);
                printf("add new clientfd=%d to epoll\n",clientfd);
                printf("now there are %d clients int the chat room\n",(int)clients_list.size());
                
                //服务端发送欢迎消息
                printf("welcome message\n");
                char message[BUF_SIZE];
                bzero(message,BUF_SIZE);
                sprintf(message,SERVER_WELCOME,clientfd);
                int ret = send(clientfd, message, BUF_SIZE,0);
                if(ret < 0){
                    perror("send error");
                    exit(-1);  
                }    
            } 
            //处理用户发来的消息，并广播，使其他用户收到消息
            else{
                int ret = sendBroadcastmessage(sockfd);
                if(ret<0){
                    perror("broadcast error");
                    exit(-1);
                }
            }
        }
    }
    close(epfd);
    close(listener);
    return 0;
}
