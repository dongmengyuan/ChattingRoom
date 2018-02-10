/*************************************************************************
> File Name: my_server1.c
> Author: dongmengyuan
> Mail: 1322762504@qq.com
> Created Time: 2016年08月06日 星期六 09时09分33秒
************************************************************************/

//服务器端程序

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<pthread.h>
#include<time.h>


#define MAX 1024
#define SERV_PORT   8888            //服务器端口
#define LISTEN_NUM 12               //定义连接请求队列长度
static char recv_buf[1024];         //自定义缓冲区

int savefile_fd = -1;
struct data {
    int type;                             //消息类型．如：群聊，私聊，注册，登录
    char users[64];                       //存当前用户名
    char name[64];                        //存目标用户名
    char news[255];                       //表示多种信息，如：密码，消息
    char times[64];
    int size;
    char mess[MAX];
    struct data *next;
};

struct user_online{   //在线用户信息
    char name[64];
    int id;
    struct user_online *next;
};

struct user_online *onlhead;
int sock_fd;                        //套接字
int conn_fd;

//设置返回显示当地的当前时间
char *times ()   
{   
	struct tm *ptm; 
	long ts;
	static char timer[20];
    int y,m,d,h,n,s; 
	ts = time(NULL); 
	ptm  = localtime(&ts); 

	y = ptm -> tm_year + 1900;       //年 
	m = ptm -> tm_mon + 1;           //月 
	d = ptm -> tm_mday;              //日 
	h = ptm -> tm_hour;              //时 
	n = ptm -> tm_min;               //分 
	s = ptm -> tm_sec;               //秒
	sprintf(timer,"%02d-%02d-%02d-%02d:%02d:%02d",y,m,d,h,n,s);
	return timer;
} 

//日志信息文件
void log_file(char time[],char news[]){
	FILE *fp;
	fp = fopen("log.txt","a+");
	fprintf(fp,"%s %s\n",time,news);
	fclose(fp);
}

//自定义错误处理函数
void my_err(const char *err_string, int line)
{
    fprintf(stderr, "line:%d ",line);
    perror(err_string);
    exit(1);
}

/*登陆之后创建在线用户的链表*/
void create_online(void)
{
	onlhead = (struct user_online *) malloc (sizeof(struct user_online));   //申请空间
	onlhead -> next = NULL;       //空链表
}

/*添加一个在线用户*/
void add_online(char *username,int fd)
{
	struct user_online *s, *p;          //定义一个结构体指针
	p = onlhead;                        //将头指针赋给结构体指针，指向链表的第一个结点
	while(p -> next != NULL) {
		p = p -> next;
	}
	printf("%s %d\n",username,fd);
	s = (struct user_online *) malloc (sizeof(struct user_online));
	strcpy(s -> name,username);
	printf("%s %d\n",username,fd);
	s -> id = fd;
	s -> next = p->next;
	p -> next = s;
}

/*删除一个离线用户*/
void del_online(int fd)
{
	struct user_online *s, *p;
	p = onlhead;
	while((p->next != NULL) && (p -> id != fd)) {
		s = p;
		p = p->next;
	}
    if((p -> next == NULL) && (p -> id != fd)) {
		return ;
	}	
	else {
		s -> next = p -> next;
		free(p);
	}
}

//从文件中读取用户信息
struct data *read_inf()
{
    struct data *head,*p,*pnew;
    struct data *next;
    FILE *fp;
    if((fp = fopen("user.txt", "a+")) == NULL) {
        printf("读文件出错啦!\n");
        exit(1);
    }
    head = (struct data *) malloc (sizeof(struct data));
    head -> next = NULL;
    p = head;                        //保存头结点
    while(!feof(fp)) {
        //开辟空间，以存放读取到的信息
        pnew = (struct data *) malloc (sizeof(struct data));
        //存放读取信息
        fscanf(fp,"%s %s\n",pnew -> name, pnew -> news);                  //将文件的信息存入链表
        p -> next = pnew;
        p = pnew;
    }
    fclose(fp);
    return head;
}

//检验有无相同名字的账号注册过
int check_name(struct data recv_buf)
{
    char a[128]; 
    strcpy(a,"register name success!");
    char b[128];
    strcpy(b,"sorry,this name is already in use!");
    struct data *f;
    f = read_inf();              //调用函数read_buf,读取文件中已注册用户的信息
    f = f -> next;
    while (f != NULL) {
        if(strcmp(f -> name, recv_buf.name) == 0) {
	        char news[128];
	        char *time;
	        sprintf(news,"注册失败 %s帐号重复",recv_buf.name);
	        time = times();
	        log_file(time,news);
            if(send( conn_fd, b, sizeof(b),0) < 0) {
                my_err("send",__LINE__);
            }
	    return 0;
        }
	 f = f -> next;
    }
    FILE *fp;
    fp = fopen("user.txt","a+");
    if(fp == NULL) {
	    printf("error\n");
	    exit(1);
    }
    fprintf(fp,"%s %s\n",recv_buf.name,recv_buf.news);
    fclose(fp);
    char news[128];
    char *time;
    sprintf(news,"%s注册成功",recv_buf.name);
    time = times();
    log_file(time,news);
    add_online(recv_buf.name,conn_fd);   //因为注册过后直接就是在线，所以调用添加在线用户函数
    send(conn_fd,a,sizeof(a), 0);
}

//用于登录时匹配帐号和密码
int check_news(struct data recv_buf)
{
    char a[128];
    strcpy(a,"congratulation!login sucessful");
    char b[128];
    strcpy(b,"sorry,your input is wrong!");
     char c[128];
    strcpy(c,"sorry,user has logged in!");
    struct data *f;
    f = read_inf();
    f = f -> next;
    while(f !=  NULL) {
        if((strcmp(f -> name, recv_buf.name) == 0) && (strcmp(f -> news, recv_buf.news)) == 0) {
            struct user_online *p;
		    p = onlhead;
		while(p != NULL)
		{
			if(strcmp(p->name,recv_buf.name)==0){
			    	char news[128];
				    char *time;
				    sprintf(news,"%s登录失败，用户已重复登录",recv_buf.name);
				    time = times();
				    log_file(time,news);
				 if(send(conn_fd, c, sizeof(c), 0) < 0) {
					    my_err("send",__LINE__);
				    }
				return 0;
			}
			p = p -> next;
		}
		    char news[128];
		    char *time;
		    sprintf(news,"%s登录成功",recv_buf.name);
		    time = times();
		    log_file(time,news);
		    add_online(recv_buf.name,conn_fd);       //调用添加在线用户函数
		    if(send(conn_fd, a, sizeof(a), 0) < 0) {
		        my_err("send",__LINE__);
		    } 
	   	    return 0;  
        }
	f = f -> next;
    }
    char news[128];
    char *time;
    sprintf(news,"%s登录失败，密码错误",recv_buf.name);
    time = times();
    log_file(time,news);
    if(send(conn_fd, b, sizeof(b), 0) < 0) {
            my_err("send",__LINE__);
    }
}

//sprintf()是字符串格式化命令函数，把格式化的数据写入某个字符串缓冲区,也就是字符串的拼接，第一项为第二，三的拼接组合
//接收请求后将请求写入目标文件
void info_add(char name[],char username[]) 
{
	char news[128];
	char *time;
    sprintf(news,"%s请求添加%s为好友",username,name);
    time = times();
    log_file(time,news);
	FILE *fp;
	char names[40];                   //文件名
	sprintf(names,"%sadd",name);      //要请求的目标好友的请求文件 
	fp = fopen(names,"a+");           //以可追加方式打开好友请求文件
	fprintf(fp,"%s\n",username);      //写入那个用户请求文件
	fclose(fp);
}

//给双方好友文件中添加好友
void friend_add(char name[],char username[]) 
{
	char news[128];
    char *time;
    sprintf(news,"%s同意添加%s为好友",name,username);
    time = times();
    log_file(time,news);
	FILE *fp,*fp1;
	int flag=0,flag1=0;
	char str[40];
	char names[40];                    //字符数组，存放目标好友用户名的文件
	char users[40];                    //存放当前自己的用户名的文件
	sprintf(names,"%sfriend",name);    //(目标好友)用户的好友文件，对用户显示出的文件名为()friend; 
	sprintf(users,"%sfriend",username);     //另外一个人(自己)的好友文件 
	fp = fopen(names,"a+");             //打开目标好友的好友文件，以追加的方式写入新好友
	while(!feof(fp)){
		fscanf(fp,"%s\n",str);
		if(strcmp(str,username)==0)
			flag=1;
	}
	if(flag==0)
		fprintf(fp,"%s\n",username);        //写入好友(将好友名写入文件中)
	fclose(fp);
	fp1 = fopen(users,"a+");  
	while(!feof(fp1)){
		fscanf(fp1,"%s\n",str);
		if(strcmp(str,name)==0)
			flag1=1;
	}
	if(flag1==0)
		fprintf(fp1,"%s\n",name);           //将目标好友名写入自己当前用户名对应的好友文件中
	fclose(fp1);
}

//离线消息
void offline(char name[],char user[],char new[],char time[])
{
	FILE *fp;
	fp = fopen("offline.txt","a+");
	fprintf(fp,"%s %s %s %s\n",name,user,new,time);
	fclose(fp);
}

//修改密码
void changepassword(char name[],char pass[])
{
    struct data *f;
    int id;
    char b[128];
    strcpy(b,"change sucessful");
    f = read_inf();              //用户信息链表 
    f = f -> next;               //遍历链表
    FILE *fp;
    fp = fopen("user.txt", "w+");    //用w+的方式重写文件 
    while(f !=  NULL) {
        if(strcmp(f -> name,name) == 0) {   //找到修改的用户写入新密码 
   	    fprintf(fp,"%s %s\n",f->name,pass);
        }
	else
	    fprintf(fp,"%s %s\n",f->name,f->news); //原密码不变
	f = f -> next;
    }
    fclose(fp);
    char news[128];
    char *time;
    sprintf(news,"%s修改密码成功!",name);
    time = times();
    log_file(time,news);
    struct user_online *p; //调用在线用户链表 
	p = onlhead;
	while(p -> next != NULL)
	{ //查找对应用户id
		if(strcmp(p->name,name) == 0)
			id=p->id;
		p = p -> next;
	}
    if(send(id, b, sizeof(b), 0) < 0) {
            my_err("send",__LINE__);
    }
}

int k=0;
//私聊
void send_one(char name[],char news[],char users[])
{
	struct user_online *p,*head;      //定义结构体指针
	int flag=0;
	struct data msg;              //发送给用户的消息 
	struct data mse;              //返回给发送用户的 
	mse.type = 2;                 //返回给当前用户 
	k++;
	int ids;                      //当前用户的connid
	msg.type = 1;                 //发送给目标用户
	strcpy(msg.name,users);       //将当前用户名赋给name
	strcpy(msg.news,news);
	p = onlhead -> next;
	head = onlhead -> next;
	while(head != NULL) {          //查找当前用户的conn id
		if(strcmp(users,head -> name) == 0) {
			ids = head -> id;
			break;
		}
		head = head -> next;
	}
	while(p != NULL) {
		if(strcmp(name,p -> name) == 0) { //查找目标用户的id
			strcpy(mse.news,"发送成功!");
			char newss[128];
			char *time;
		    sprintf(newss,"%s给%s发送消息",users,name);
     	    time = times();
		    log_file(time,newss);
			flag=1;
			send(ids,&mse,sizeof(mse),0);             //发送给当前用户
			strcpy(msg.times,time);
			send(p -> id,&msg, sizeof(msg), 0);         // 发送给目标用户，p->id指在线用户链表中每个用户对应的id
		}
		p = p -> next;
	}
	if(flag==0) {//判断是否为离线消息 
		char newss[128];
		char *time;
	    sprintf(newss,"%s给%s发送离线消息",users,name);
	    time = times();
        log_file(time,newss);
		strcpy(mse.news,"用户不在线!,已发送离线消息!");
		offline(name,users,news,time);
		send(conn_fd,&mse,sizeof(mse),0);
	}
}

//发送文件
void send_file(char users[],char mess[],char name[],int size){
	struct user_online *p,*head;      //定义结构体指针
	int flag=0;
	struct data msg;             
	msg.size = size;
	int ids;                      //当前用户的connid
	msg.type = 3;                 //发送给目标用户
	strcpy(msg.users,users);
	strcpy(msg.name,name);
	strcpy(msg.mess,mess);
	p = onlhead -> next;
	while(p != NULL) {
		if(strcmp(users,p -> name) == 0) { //查找目标用户的id
			char newss[128];
			char *time;
		    sprintf(newss,"%sshou dao wen jian",users);
     	    		time = times();
		    log_file(time,newss);
			flag=1;
			send(p -> id,&msg, sizeof(msg), 0);         // 发送给目标用户，p->id指在线用户链表中每个用户对应的id
		}
		p = p -> next;
	}
	if(flag==0) {//判断是否为离线消息 
		char newss[128];
		char *time;
	    sprintf(newss,"发送失败!对方不在线!");
	    time = times();
       		 log_file(time,newss);
	}
}

//群聊
void send_all(char users[],char news[])
{
	struct user_online *p,*head;
	struct data msg;              //发送给用户的消息 
	struct data mse;              //反馈给发送用户的 
	mse.type = 2;                 //反馈给发送用户的 
	k++;
	int ids;
	printf("%d\n",k);
	msg.type = 1;
	strcpy(msg.name,users);
	strcpy(msg.news,news);
	p = onlhead -> next;
	head = onlhead -> next;
	while(head != NULL) {
		if(strcmp(users,p -> name) == 0) {
			ids = p -> id;
		}
		head = head -> next;
	}
	char newss[128];
	    char *time;
	    sprintf(newss,"%s给所有用户发消息",users);
	    time = times();
	    log_file(time,newss);
	strcpy(msg.times,time);
	while(p != NULL) { //给所有在线的id都发送信息 
		send(p -> id,&msg, sizeof(msg), 0);
		p = p -> next;
	}
	strcpy(mse.news,"发送成功!");
	send(ids,&mse,sizeof(mse),0);
}


int send_data(int conn_fd, const char *string);
//接收客户端发过来的消息
void recv_data(void *p)
{
    int fd=*(int *)p;
    printf("pthread=%d\n",fd);    
    int ret;
    while(1)
    {
        struct data data_buf;       //接收客户端信息,存客户端发过来的结构体
	    if(recv(fd, &data_buf, sizeof(data_buf),0)==0) {
            	printf("the %d quit\n",fd);
		 char news[128];
		    char *time;
		    sprintf(news,"%d退出连接",fd);
		    time = times();
		    log_file(time,news);
		    del_online(fd);               //通过接收客户端发来的消息，判断是否有用户退出，而去删除离线用户
		    close(fd);
		    return;
	    }
	    else {
		switch(data_buf.type) {			
		    case 1: check_name(data_buf);
		    break;
		    case 2: check_news(data_buf);
		    break;
		    case 3: info_add(data_buf.name,data_buf.users);                 //添加好友
		    break;
	 	    case 4: friend_add(data_buf.name,data_buf.users);              //查看好友申请
		    break;
                    case 5: changepassword(data_buf.name,data_buf.news);
		    break;
		    case 6: send_one(data_buf.name,data_buf.news,data_buf.users);   //私聊
		    break;
		    case 7: send_all(data_buf.users,data_buf.news);                //群聊
		    break;
                    case 8: send_file(data_buf.name,data_buf.mess,data_buf.news,data_buf.size);  //发送文件
                    break;
		}
	    }
    }
}

//向客户端发消息
int send_data(int conn_fd, const char *string)
{
    if (send(conn_fd, string, sizeof(string), 0) < 0) {
        my_err("send",__LINE__);
    }
}

//创建储存帐号密码的文件
void create_file()          
{
    FILE *fp;
    fp = fopen("user.txt","a+");
    if (fp == NULL) {
        printf("can not create a file!\n");
    }
}

//阻塞等待来自客户端的连接请求
void acceptconnect()
{
    while(1)
    {
        struct sockaddr_in client_addr;                    //定义套接字地址
        socklen_t client_len = sizeof(client_addr);        //接受连接请求，产生连接套解字
        //通过accept接受客户端的连接请求，并返回连接套接字用于收发数据
        conn_fd = accept (sock_fd,(struct sockaddr *)&client_addr,&client_len);
        printf("accept a new client,is %s\n",inet_ntoa(client_addr.sin_addr));
        if(conn_fd<0) {
            printf("error to connect\n");
            continue;                      //继续循环，处理连接
        }
        printf("a new connecter is %s\n",inet_ntoa(client_addr.sin_addr));
        printf("连接成功啦!\n");
	    printf("%d\n",conn_fd);
	    char news[128];
	    char *time;
	    sprintf(news,"%d连接成功",conn_fd);
	    time = times();
	    log_file(time,news);
        pthread_t thid;                 //线程id
        pthread_create(&thid, 0, (void *)recv_data, &conn_fd);         //创建子进程，用于接收信息
    }
}

//处理套接字
void ready()
{           
    sock_fd = socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0) {
        my_err("socket",__LINE__) ;
        exit(1);
    }
    struct sockaddr_in serv_addr;		    //定义套接字地址
    //初始化服务器端地址结构
    memset(&serv_addr, 0, sizeof (struct sockaddr_in));  
    serv_addr.sin_family = AF_INET;	                     //Ipv4
    serv_addr.sin_port = htons(SERV_PORT);				 //设置端口	
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);       //可以绑定到任何网络接口上，监控所有网卡过来的数据

    //将套接字绑定到本地端口			
    if(bind(sock_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        my_err("bind",__LINE__);
        exit(1);
    }

    //将套接字转化为监听套接字
    if(listen(sock_fd,LISTEN_NUM) < 0) {
        my_err("listen",__LINE__);
        exit(1);
    }
    char *time;
    time = times();
    log_file(time,"服务器启动!");
}

int main()
{
    ready();
    create_online();              //创建在线用户链表
    acceptconnect();
}

