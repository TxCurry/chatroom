#include "utility.h"
int main(int argc,char* argv[]){
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    //创建套接字socket
    int sock = socket(PF_INET,SOCK_STREAM,0);
    if(sock < 0){
        perror("sock error");
        exit(-1);
    }
    //连接服务端
    if(connect(sock,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0){
        perror("connect failed");
        exit(-1);
    } 
    //创建管道，其中pipe_fd[0]用于父进程读，pipe_fd[1]用于子进程写
    int pipe_fd[2];
    if(pipe(pipe_fd) < 0){
        perror("pipe error");
        exit(-1);  
    }
    //创建epoll
    int epfd = epoll_create(EPOLL_SIZE);
    if(epfd < 0){
        perror("epoll error");
        exit(-1);
    }
    static struct epoll_event events[2];
    //将sock和管道读端描述符都添加到内核事件表中
    addfd(epfd, sock, true);
    addfd(epfd,pipe_fd[0],true);
    //表示客户端是否正常工作
    bool isClientwork = true;
    //聊天内容缓冲区
    char message[BUF_SIZE];
    
    //fork
    int pid = fork();
    if (pid < 0){
        perror("fork error");
        exit(-1);
    }
    //子进程空间
    else if(pid ==0){
        //子进程负责写入管道，因此先关闭读端
        close(pipe_fd[0]);
        printf("please input 'exit' to exit the chat room\n");
        while(isClientwork){
            bzero(message,BUF_SIZE);
            fgets(message,BUF_SIZE,stdin);
            
            //客户输入exit退出
            
            if(strncasecmp(message,"exit",strlen("exit"))==0){
                isClientwork = false;    
            }
            //子进程将输入写入管道
            else{
                if(write(pipe_fd[1],message,strlen(message)-1)< 0){
                    perror("write error");
                    exit(-1);      
                }
            }
        }
    } 
    //父进程空间
    else if(pid >0){
        //父进程负责读管道数据,先关闭写端
        close(pipe_fd[1]);
        //主循环(epoll_wait)
        while(isClientwork){
            int epoll_events_count = epoll_wait(epfd, events, 2,-1);
            //处理就绪事件
            for(int i = 0;i < epoll_events_count;i++){
                bzero(message,BUF_SIZE);
                
                //服务端发来消息
                if(events[i].data.fd == sock){
                    int ret = recv(sock, message, BUF_SIZE, 0);
                    //ret为0表示服务端关闭
                    if(ret == 0){
                        printf("server closed connection:%d\n",sock);
                        close(sock);
                        isClientwork = 0;
                    }
                    else{
                        printf("%s\n",message);
                    }
                }
                //子进程写入事件发生，父进程处理发送给服务端 
                else{
                    //父进程从管道中读数据
                    int ret = read(events[i].data.fd, message, BUF_SIZE);
                    if(ret ==0){
                        isClientwork = 0;       
                    }
                    //将消息发送给服务端
                    else{
                        send(sock,  message, BUF_SIZE, 0);   
                    }
                }      
            }                    
        }     
    }
    if(pid){
        //关闭父进程和sock
        close(pipe_fd[0]);
        close(sock);
    }
    else{
        //关闭子进程
        close(pipe_fd[1]);
    }
    return 0;
}
