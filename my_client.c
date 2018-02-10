/*************************************************************************
        > File Name: my_client1.c
      > Author: dongmengyuan
      > Mail: 1322762504@qq.com
      > Created Time: 2016年08月09日 星期二 08时33分30秒
 ************************************************************************/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#define MAX  1024
#define SERV_PORT   8888                  //服务器端的端口

int conn_fd;                              //定义套接字
char usernames[20];
int savefile_fd = -1;                     //文件描述符
struct data {
    int type;                             //消息类型．如：群聊，私聊，注册，登录
    char users[64];                       //存当前用户名;
    char name[64];                        //存目标用户名
    char news[255];                       //表示多种信息，如：密码，消息
    char times[64];
    int size;
    char mess[MAX];
};
//设置返回当地时间
char *times ()   
{   
	struct tm *ptm; 
	long ts;
	static char times[20];
    int y,m,d,h,n,s; 
	ts = time(NULL);
	ptm = localtime(&ts); 
	y = ptm -> tm_year + 1900;       //年 
	m = ptm -> tm_mon + 1;           //月 
	d = ptm -> tm_mday;              //日 
	h = ptm -> tm_hour;              //时 
	n = ptm -> tm_min;               //分 
	s = ptm -> tm_sec;               //秒
	sprintf(times,"%02d-%02d-%02d-%02d:%02d:%02d",y,m,d,h,n,s);                //格式化输出
	return times;
}

//消息记录文件
void message_file(char time[],char news[])
{
	char filename[40];
	FILE *fp;
	sprintf(filename,"%smessage\n",usernames);
	fp = fopen(filename,"a+");
	fprintf(fp,"%s %s\n",time,news);
	fclose(fp);
}

//自定义错误处理函数
void my_err(const char * err_string, int line)
{
    fprintf(stderr, "line:%d ", line);
    perror(err_string);
    exit(1);
}

//获取用户输入的信息并存入缓冲区buf中,buf的长度为len
int getin(int len ,char *buf)
{
    int i;
    char ch;
    if(buf == NULL) {
        return -1;
    }
    i = 0;
    while (((ch = getchar()) != '\n') && (ch != EOF) && (i < len - 2)) {
        buf[i++] = ch;
    }
    buf[i++] = '\n';
    buf[i++] = '\0';
    return 0;
}

//接收消息
void recvmassage()
{
    struct data recv_buf;
	FILE *fp;
    while(1) {
        char str[128];
    	if(recv(conn_fd,&recv_buf, sizeof(recv_buf),0)<=0) {
		return;
	    }
	    else {
            switch(recv_buf.type) {
                case 1:printf("%s\n%s发送消息%s\n",recv_buf.times,recv_buf.name,recv_buf.news);
			    sprintf(str,"%s对你发送消息%s",recv_buf.name,recv_buf.news);
		        message_file(recv_buf.times,str);
			    break;
	            case 2:printf("%s\n",recv_buf.news);
			    break;
                case 3:sprintf(str,"xin%s\n",recv_buf.name);
			    fp = fopen(str,"a+");
			    fprintf(fp,"%s",recv_buf.mess);
			    fclose(fp);
			    break;
		    }
     	}
    }
}

//添加新朋友
//将自己和将要添加的好友的用户名保存下来并发送给服务器端进行处理
void Add_friend(){
	struct data msg;
	int  flag=0;
	char names[20];
	char pass[20];
	msg.type = 3;                                //标志操作类型
    	char name[20];                               //存放目标好友用户名的文件
	printf("请输入您想要添加的好友的用户名:");
	scanf("%s",msg.name);
	strcpy(msg.users,usernames);                 //将当前(自己)的用户名储存到结构体，保存在存放当前用户名的文件里
	FILE *fp;
	fp = fopen("user.txt","rt");
	while(!feof(fp))
	{
		fscanf(fp,"%s %s\n",names,pass);
		if(strcmp(msg.name,names)==0){
			flag=1;
			break;
		}
	}
	if(flag==1)
		send(conn_fd,&msg,sizeof(msg),0);            //发送至服务器端处理
	else
		printf("无此人!\n");
}

//查看好友请求
void see_please()
{
	char names[40];           //文件名，字符数组存储字符串，用来表示未知的文件名
	char infor[10][64];       //文件信息，用二位数组和链表都可以实现的
	char user[20];            //将要同意的好友的用户名
   	 int i = 0,j;
	int flag=0;
	FILE *fp;
	sprintf(names,"%sadd",usernames);            //文件名
	fp = fopen(names,"rt");        //打开好友申请文件 
	if(fp == NULL) {
		return;	
	}
	while(!feof(fp)) {            //读取文件信息
		fscanf(fp,"%s\n",infor[i]);
		if(infor[i] == NULL) {
			printf("无好友请求\n");
			return;	
		}
		printf("%s请求添加您为好友!\n",infor[i]);
		i++;  
    	}
	fclose(fp);
	printf("请输入您将要同意的好友请求:\n");             
	scanf("%s",user);          //目标好友
	if(i == 1) {                   //如果一条请求就删除文件 
		remove(names);
	}
	fp = fopen(names,"w+");      //以可写的方式打开好友申请文件
	for(j = 0;j < i;j++){        //更新请求文件 
		if(strcmp(user,infor[j]) == 0){
			if(i == 1){
				remove(names);
			}
			struct data msg;
			msg.type = 4;
			strcpy(msg.users,user);               //将目标好友用户名存入结构体
			strcpy(msg.name,usernames);          //将自己的当前用户名写入结构体
			send(conn_fd,&msg,sizeof(msg),0);    //将回复的请求发送给服务器 		
			flag=1;
		}
		else{
		  fprintf(fp,"%s\n",infor[j]);
		}
	}
	if(flag==0)
		printf("请输入正确的回复!\n");
	fclose(fp);
}

//查看好友
void see_friend()
{
	FILE *fp;
	char name[40];
	char infor[20];                           //存放该用户的好友信息
	sprintf(name,"%sfriend",usernames);         //当前用户的好友文件
	fp = fopen(name,"rt");                    //以只读方式打开文件查看好友
	if(fp == NULL){
		printf("暂时没有好友!");
		return;	
	}
	while(!feof(fp)) {                  //读取好友文件
        fscanf(fp,"%s\n",infor);        //读出好友文件里的好友信息
		printf("%s is your friend!\n",infor);       
    	}
	fclose(fp);
}

//查找好友
int find_friend(char names[])
{
	FILE *fp;
    char name[40];                          //文件名
    char infor[20];                         //所有好友   
	int flag = 0;           
	sprintf(name,"%sfriend",usernames);     //格式化字符串函数，设置一个文件名的格式                
	fp = fopen(name,"rt");
	if(fp == NULL) {
		printf("文件为空!");
		return 0;	
	}
	while(!feof(fp)) {                        //读取文件信息 
        	fscanf(fp,"%s\n",infor);           //从好友文件中读出好友信息
		if(strcmp(infor,names) == 0)           //通过比较，判断是否存在该好友
			flag=1;
    	}
	fclose(fp);
	return flag;
}
//删除好友
void delete_friend(){
	char name[20]; //用户要删除的好友 
	char names[40]; //当前用户的好友文件名 
	char info[20][20]; //存储好友文件的信息 
	printf("请输入要删除的好友:\n");
	scanf("%s",name);
	FILE *fp;
	int i=0;
	int j;
	int flag = 0; //标志是否有此好友 
	sprintf(names,"%sfriend",usernames);
	fp = fopen(names,"rt"); //读取好友文件信息 
	if(fp == NULL)
	{
		printf("你所删除的好友不存在!\n");
		return;
	}
	while(!feof(fp)){ //将所有好友存储到变量数组里 
		fscanf(fp,"%s\n",info[i]);
		i++;
	}
	fclose(fp);
	fp = fopen(names,"w+"); //更新好友文件
	for(j=0;j<i;j++){
		if(strcmp(name,info[j])==0){ //如果是要删除的则不用存进文件 
			printf("删除好友成功!\n");
			flag=1;
		}
		else //其他的好友存进文件 
			fprintf(fp,"%s\n",info[j]);
	}
	fclose(fp);
	if(flag==0)
		printf("你所删除的好友不存在!\n");
	else
		if(i==1) //如果只有一个好友，删除后就删除文件 
			remove(names);
}
//私聊
void send_one()
{
	struct data msg;
	msg.type = 6;                 //给一个人发消息
	strcpy(msg.users,usernames);   //把当前用户名赋给users，存入结构体准备发送给服务器端处理
	printf("请输入发送的用户名");
	scanf("%s",msg.name);
	printf("请输入消息内容:");
	scanf("%s",msg.news);
	if(find_friend(msg.name) == 0) { //调用查找好友函数，查找有无要发送消息的目标好友
		printf("无此好友\n");	
	}
	else{
		char str[128];
		char *time;
		time=times();
		sprintf(str,"你对%s发送消息%s",msg.name,msg.news);
		message_file(time,str);//调用消息记录存储函数 
		if (send(conn_fd,&msg,sizeof(msg),0) < 0) {
			my_err("send", __LINE__);
		   }     //将各类信息发送给服务器端进行私聊处理
	}
}

//群聊
void send_all()
{
	struct data msg;
	msg.type = 7; 
	strcpy(msg.users,usernames);  //把当前用户名赋给users，即存入结构体，便于发送给服务器端
	printf("请输入消息内容:");
	scanf("%s",msg.news);
	char str[128];
	char *time;
	time=times();
	sprintf(str,"你对所有用户发送消息%s",msg.news);
	message_file(time,str);
	send(conn_fd,&msg,sizeof(msg),0);	
}
//查看离线消息
void see_news()
{
	FILE *fp;
	char users[20][40]; //存储文件信息的变量
	char name[20][40];
	char news[20][40];
	char times[20][40];
	int flag=0; //标志是否有当前用户的离线信息
	int i=0;
	int j;
	fp = fopen("offline.txt","a+"); //打开离线信息文件 
	while(!feof(fp)) {
		fscanf(fp,"%s %s %s %s\n",users[i],name[i],news[i],times[i]);
		i++;
	}
	fclose(fp);
	fp = fopen("offline.txt","w+");//更新离线文件信息
	for(j=0;j<i;j++) { 
		if(strcmp(usernames,users[j])==0) {  //查找当前用户的离线信息
			printf("%s\n%s对你发送离线消息:%s\n",times[j],name[j],news[j]); //输出到文件 
			char str[128];
			sprintf(str,"%s对你发送离线消息%s",name[j],news[j]);
			message_file(times[j],str); //存入消息记录 
			flag=1;		
		}	
		else {
			fprintf(fp,"%s %s %s %s\n",users[j],name[j],news[j],times[j]);
		}
	}
	if(flag==0) {
		printf("无离线消息!\n");
	}
	fclose(fp);
}

//查看消息记录
void see_Message()
{
	FILE *fp;
	char time[128];
	char Mess[128];
	char filename[40]; //消息记录文件名 
	sprintf(filename,"%smessage\n",usernames);
	fp = fopen(filename,"rt");
	if(fp == NULL){
		printf("无消息记录!\n");
		return;
	}
	else{
		while(!feof(fp)) {
			fscanf(fp,"%s %s\n",time,Mess);
			printf("%s\n%s\n",time,Mess);
		}
	}
	fclose(fp);
}

void send_file();
//内部界面
void interface()
{
	pthread_t thid;                 //线程id
    pthread_create(&thid, 0, (void *)recvmassage, &conn_fd);     //创建子进程，用于接收信息
	char choice;
	do{
	system("clear");
    printf("\t\t**************************************************\n");
    printf("\t\t|              **CHAT ROOM**                     |\n");
    printf("\t\t--------------------------------------------------\n");
    printf("\t\t|                                                |\n");
    printf("\t\t***************欢迎来到聊天室!********************\n");
    printf("\t\t|\t ---------1.添加新朋友  ---------        |\n");
    printf("\t\t|\t ---------2.查看好友请求---------        |\n");
    printf("\t\t|\t ---------3.查看好友    ---------        |\n");
    printf("\t\t|\t ---------4.删除好友    ---------        |\n");
    printf("\t\t|\t ---------5.私聊        ---------        |\n");
    printf("\t\t|\t ---------6.群聊        ---------        |\n");
    printf("\t\t|\t ---------7.查看离线消息---------        |\n");
    printf("\t\t|\t ---------8.查看消息记录---------        |\n");
    printf("\t\t|\t ---------9.发文件      ---------        |\n");
    printf("\t\t|\t ---------0.退出系统    ---------        |\n");
    printf("\t\t|------------------------------------------------|\n");
    printf("\t\t**************************************************\n");
    printf("\t\t---输入您将使用的选项号:\n");
    choice = getchar();
    setbuf(stdin, NULL);                      //linux 里面的函数，主要用于打开和关闭缓冲机制
    }while(choice>'9' && choice<'0');
    switch(choice) {
        case '1':Add_friend();                //添加新朋友
		setbuf(stdin, NULL);
		printf("请按回车继续:\n");
		getchar();                            //两个getchar是为了界面停止在此处，等待操作
		getchar();
       	break;
	    case '2':see_please();                //查看好友请求
	    printf("请按回车继续:\n");
		getchar();
		getchar();
		break;
	    case '3':see_friend();                //查看好友
		printf("请按回车继续:\n");
		setbuf(stdin, NULL);
		getchar();
		getchar();
		break;
        case '4':delete_friend();
		printf("请按回车继续:\n");
		setbuf(stdin, NULL);
		getchar();
		getchar();
        break;
        case '5': send_one();
		printf("请按回车继续:\n");
		getchar();
		getchar();
		  break;
     	case '6':send_all();
		break;
        case '7':see_news();
		printf("请按回车继续:\n");
		getchar();
		getchar();
        break;
	    case '8':see_Message();
		printf("请按回车继续:\n");
		setbuf(stdin, NULL);
		getchar();
		getchar();
        break;
        case '9':send_file();
        printf("请按回车继续:\n");
        getchar();
        getchar();
        break;
	    case '0':printf("退出聊天室啦!!\n");
     	exit(0);
        break;
    }
    if(choice!='0')
    	interface();
}

char view();
//修改密码
void changepasswd()
{
    char recv_bufs[128];
    char *password;                          //旧密码
    char *password1;                         //新密码
    char *password2;                         //新密码
    char buf[128];
    struct data msg;
    msg.type = 2;                            //标记操作类型
    printf("please input your name:");
    scanf("%s",msg.name);
    password = getpass("please input your old password:");          //密码加密输入
    strcpy(msg.news, password);                                     
    send(conn_fd, &msg, sizeof(msg), 0);              //发送到服务器端处理，判断是否登录成功
    if (recv(conn_fd, recv_bufs, sizeof(recv_bufs), 0) <= 0) {
        return;
    } 
	printf("%s\n",recv_bufs);           //服务器端的返回消息
    if (strcmp(recv_bufs, "congratulation!login sucessful") == 0) {
	    struct data changepass;               //结构体
        printf("congratulation!login sucessful\n");         //登录成功
        printf("您已登录成功，下来可以进行修改密码操作:\n");
        //输入并向服务器端发送你的新密码
        password1 = getpass("please input your new password:");
        password2 = getpass("please input your new password again:");
        strcpy(buf, password1);                //避免加密函数覆盖第一次输入的密码
        if (strcmp(buf, "") == 0) {            //密码不为空
            printf("your password can not be null\n");
            printf("请按回车继续:\n");
            getchar();
            getchar();
            view();
        }
        else {
            if (strcmp(buf, password2) == 0) {
		         changepass.type = 5;              //标志操作类型
		         strcpy(changepass.name,msg.name);        //用户名
                 strcpy(changepass.news,password1);       //新密码
                 if (send(conn_fd, &changepass, sizeof(changepass), 0) < 0) {
                      my_err("send",__LINE__);
                 }
		         if (recv(conn_fd, recv_bufs, sizeof(recv_bufs), 0) <= 0) {
			          return;
		         } //接收服务器的修改消息
	      	     printf("%s\n",recv_bufs);             //输出服务器端的返回信息
            }
            else {
                printf("请注意:您两次输入的密码不一致!\n");
            }
        }
    }
    printf("请按回车继续:\n");
    getchar();
    getchar();
    view();
}

//发文件
void send_file()
{
    struct data filedata;
    filedata.type = 8;
    char file_name[100];
    printf("请输入您要发送的文件名:\n");
    scanf("%s",file_name);
    printf("请输入将要发送的用户:\n");
    scanf("%s",filedata.name);
    if((savefile_fd=open(file_name,O_RDONLY))<0)
	{
		savefile_fd = -1;
		printf("打开失败,文件不存在!\n");
						
	}
    else{
	    do{
			memset(filedata.mess,0,sizeof(filedata.mess));
			strcpy(filedata.news,file_name);
			filedata.size = read(savefile_fd,filedata.mess,500);
			if (filedata.size == 0) {
			    printf("传输文件成功\n");
			}
			else if (filedata.size > 0) {
			    send(conn_fd, &filedata, sizeof(filedata), 0);
			    usleep(1000);
			}
			else {
			    printf("文件发送失败\n");
			    break;
			}
		} while(filedata.size > 0);
		close(savefile_fd);
		savefile_fd = -1;
	 }
}

//用户注册
void registered()
{
    char recv_bufs[128];                    //接收并保存服务器端的返回消息，以用来判断注册，登录是否成功
    char *password1;
    char *password2;
    char buf[12];
    struct data msg;
    msg.type = 1;
    printf("please input your name:");
    scanf("%s",msg.name);
    password1 = getpass("please input your password:");
    password2 = getpass("please input your password again:");
    strcpy(buf,password1);
    if(strcmp(buf,"") == 0) {
        printf("your password can not be null\n");
	    printf("请按回车继续:\n");
	    getchar();
	    getchar();
	    view();
    }
    else {
	    if(strcmp(buf,password2) == 0) {
            strcpy(msg.news,password1);
		   if(send(conn_fd, &msg, sizeof(msg),0) < 0) {
               my_err("send",__LINE__);
		    }    
	    }
	    else {
        printf("请注意:您两次输入的密码不一致!\n");
	    } 
    }
    while(1) {
        if(recv(conn_fd,recv_bufs, sizeof(recv_bufs),0) <= 0) {         
            return;
        }
        if(strcmp(recv_bufs,"register name success!") == 0) {         //通过此比较语句，判断是否注册成功
            strcpy(usernames,msg.name);             //                                                 
 	        printf("register name success!\n");
	        printf("请按回车继续:\n");
	        getchar();
	        getchar();
	        interface();
        }
        if(strcmp(recv_bufs,"sorry,this name is already in use!") == 0) {
            printf("sorry,this name is already in use!\n");
	        printf("请按回车继续:\n");
	        getchar();
	        getchar();
	        view();
        }
    }
    setbuf(stdin, NULL);     //清除缓冲区，不然缓冲区的回车换行会影响到下一个的输入
}

//用户登录
void login()
{
    char *password;
    struct data msg;
    char recv_bufs[128];
    msg.type = 2;
    printf("please input your name:");
    scanf("%s",msg.name);
    password = getpass("please input your password:");
    strcpy(msg.news,password);
    if (send(conn_fd, &msg, sizeof(msg), 0) < 0) {
        my_err("send",__LINE__);
    }
    while(1) {
        if(recv(conn_fd,recv_bufs, sizeof(recv_bufs),0) <= 0) {
            return;
        }
        if(strcmp(recv_bufs,"congratulation!login sucessful") == 0) {
            printf("congratulation!login sucessful\n");
	        strcpy(usernames,msg.name);
	        printf("请按回车继续:\n");
	        getchar();                         //用两个getchar是为了界面停止在此处，等待下面的操作
	        getchar();
	        interface();
		break;
        }
        if(strcmp(recv_bufs,"sorry,your input is wrong!") == 0) {
            printf("sorry,your input is wrong!\n");
	        printf("请按回车继续:\n");
	        getchar();
	        getchar();
	        view();
		break;
        }
	else {
		printf("%s\n",recv_bufs);
		 printf("请按回车继续:\n");
	        getchar();
	        getchar();
	        view();
		break;
		
	}
    }
    setbuf(stdin, NULL);               //清除缓冲区
}

char view()
{
    char choice;
    do {
    printf("\t\t**************************************************\n");
    printf("\t\t|              **CHAT ROOM**                     |\n");
    printf("\t\t--------------------------------------------------\n");
    printf("\t\t|                                                |\n");
    printf("\t\t***************欢迎来到聊天室!********************\n");
    printf("\t\t|\t ---------1.注册用户---------            |\n");
    printf("\t\t|\t ---------2.用户登录---------            |\n");
    printf("\t\t|\t ---------3.修改密码---------            |\n");
    printf("\t\t|\t ---------4.退出系统---------            |\n");
    printf("\t\t|------------------------------------------------|\n");
    printf("\t\t---输入您将使用的选项号:\n");
    choice = getchar();
    } while (choice!='1' && choice!='2' && choice!='3' && choice!='4' );    
    switch(choice) {
        case '1':registered();
        break;
        case '2':login();
     	break;
        case '3':changepasswd();
        break;
        case '4':printf("退出聊天室啦!!\n");
        break;
    }
}

int main(int argc, char *argv[])
{
  	printf("启动连接!\n");
	struct sockaddr_in serv_addr;			//定义套接字地址
    int sock_fd;
    if ((sock_fd = socket(AF_INET,SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    //初始化服务器端地址结构
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));	
	serv_addr.sin_family = AF_INET;                            //Ipv4
    serv_addr.sin_port=htons(SERV_PORT);                       //设置端口
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");        //绑定本机ip
    conn_fd = socket(AF_INET,SOCK_STREAM,0);	               //设置端口和地址

	//向服务器端发送连接请求，连接服务器端地址
	if(connect(conn_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0) {
		my_err("connect", __LINE__);
	}

    char a;
    view();
    sleep(1);
}
