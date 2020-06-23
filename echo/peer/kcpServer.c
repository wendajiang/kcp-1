#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "ikcp.h"

#include <string.h>

#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <stdbool.h>


// int IsResvAddr(char *ip) {
//     char addrs[][20] = { 
//         "0.0.0.0", "10.0.0.0", "127.0.0.1", "169.254.0.0",
//         "172.16.0.0", "192.0.0.0", "192.0.2.0", "192.88.99.0",
//         "192.168.0.0", "198.18.0.0", "198.51.100.0", "203.0.113.0",
//         "224.0.0.0", "240.0.0.0", "255.255.255.255", "100.64.0.0",
//         "9.0.0.0"
//     };
//     in_addr_t addr;
//     int i;

//     addr = inet_addr(ip);
//     for (i = 0; i < (int) sizeof(addrs); i++) {
//         if (inet_addr(addrs[i]) == addr) {
//             return 1; // true
//         }
//     }

//     return 0;
// }

// int IsLanAddr(char *ip) {
//     unsigned int uiHostIP =  ntohl(inet_addr(ip));
//     /*
//      * A类  10.0.0.0    - 10.255.255.255
//      * B类  172.16.0.0  - 172.31.255.255
//      * C类  192.168.0.0 - 192.168.255.255
//      *     100.64.0.0/10 100.64.0.0 - 100.127.255.255
//      * 其他：9.0.0.0/8   9.0.0.1 - 9.255.255.255
//      * 环回 127.0.0.1
//      */

//     if ((uiHostIP >= 0x0A000000 && uiHostIP <= 0x0AFFFFFF ) ||
//         (uiHostIP >= 0xAC100000 && uiHostIP <= 0xAC1FFFFF ) ||
//         (uiHostIP >= 0xC0A80000 && uiHostIP <= 0xC0A8FFFF ) ||
//         (uiHostIP >= 0x64400000 && uiHostIP <= 0x647FFFFF ) ||
//         (uiHostIP >= 0x09000000 && uiHostIP <= 0x09FFFFFF ) ||
//         (uiHostIP == 0x7f000001)
//        )
//     {
//        return 1;
//     }

//     return 0;
// }

// int GetEthAddrs(char *ips[], int num) {
// 	struct ifconf ifc;
// 	struct ifreq ifr[64];
// 	struct sockaddr_in sa;
// 	int sock = -1;
// 	int cnt = 0;
// 	int i, n;

// 	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
// 		ifc.ifc_len = sizeof(ifr);
// 		ifc.ifc_buf = (caddr_t) ifr;
// 		ifc.ifc_req = ifr;

// 		if (ioctl(sock, SIOCGIFCONF, (char *) &ifc) == 0) {
// 			n = ifc.ifc_len / sizeof(struct ifreq);
// 			for (i = 0; i < n && i < num; i++) {
// 				if (ioctl(sock, SIOCGIFADDR, &ifr[i]) == 0) {
// 					memcpy(&sa, &ifr[i].ifr_addr, sizeof(sa));
// 					sprintf(ips[cnt++], "%s", inet_ntoa(sa.sin_addr));
// 				}
// 			}
// 		}
// 		close(sock);
// 	}

// 	return (sock >= 0 && cnt > 0) ? cnt : -1;
// }

// int GetLanAddr(char *ip, int len) {
// 	char aip[8][20];
// 	char *ips[8];
// 	int i, num;

// 	for (i = 0; i < (int) sizeof(ips); i++) {
// 		ips[i] = aip[i];
// 	}
// 	if ((num = GetEthAddrs(ips, sizeof(ips))) > 0) {
// 		for (i = 0; i < num; i++) {
// 			if (!IsResvAddr(ips[i]) && IsLanAddr(ips[i])) {
// 				snprintf(ip, len, "%s", ips[i]);

// 				return 0;
// 			}
// 		}
// 	}

// 	return -1;
// }



static int number = 0;

 typedef struct {
	unsigned char *ipstr;
	int port;
	
	ikcpcb *pkcp;
	
	int sockfd;
	
	struct sockaddr_in addr;//存放服务器信息的结构体
	struct sockaddr_in CientAddr;//存放客户机信息的结构体
	
	char buff[488];//存放收发的消息
	
}kcpObj;


/* get system time */
void itimeofday(long *sec, long *usec)
{
	struct timeval time;
	gettimeofday(&time, NULL);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;
}

/* get clock in millisecond 64 */
int64_t iclock64(void)
{
	long s, u;
	int64_t value;
	itimeofday(&s, &u);
	value = ((int64_t)s) * 1000 + (u / 1000);
	return value;
}

uint32_t iclock()
{
	return (uint32_t)(iclock64() & 0xfffffffful);
}

/* sleep in millisecond */
void isleep(unsigned long millisecond)
{
	/* usleep( time * 1000 ); */
	struct timespec ts;
	ts.tv_sec = (time_t)(millisecond / 1000);
	ts.tv_nsec = (long)((millisecond % 1000) * 1000000);
	/*nanosleep(&ts, NULL);*/
	usleep((millisecond << 10) - (millisecond << 4) - (millisecond << 3));
}


int udpOutPut(const char *buf, int len, ikcpcb *kcp, void *user){
   
    kcpObj *send = (kcpObj *)user;

	//发送信息
    int n = sendto(send->sockfd, buf, len, 0, (struct sockaddr *)&send->CientAddr, sizeof(struct sockaddr_in));
    if (n >= 0)       
   	{       
		//会重复发送，因此牺牲带宽
		printf("udpOutPut-send: size =%d bytes   buff=[%s]\n", n ,buf + 24);//24字节的KCP头部
        return n;
    } 
	else 
	{
        printf("error: %d bytes send, error\n", n);
        return -1;
    }
}

int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

int init(kcpObj *send)
{	
	send->sockfd = socket(AF_INET,SOCK_DGRAM,0);
	
	if(send->sockfd<0)
	{
		perror("socket error！");
		exit(1);
	}
	
	bzero(&send->addr, sizeof(send->addr));
	
	send->addr.sin_family = AF_INET;
	send->addr.sin_addr.s_addr = htonl(INADDR_ANY);//INADDR_ANY
	send->addr.sin_port = htons(send->port);
		
	printf("server socket: %d  port:%d\n",send->sockfd,send->port);
	
	if(send->sockfd < 0){
		perror("socket error！");
		exit(1);
	}
	// setnonblocking(send->sockfd);
	if(bind(send->sockfd,(struct sockaddr *)&(send->addr),sizeof(struct sockaddr_in))<0)
	{
		perror("bind");
		exit(1);
	}
	
}

void loop(kcpObj *send)
{
	unsigned int len = sizeof(struct sockaddr_in);
	int n,ret;	
	//接收到第一个包就开始循环处理
	char buf[1024]={0};
	while(1)
	{	
		ikcp_update(send->pkcp, iclock());
		
	
		//处理收消息
    	n = recvfrom(send->sockfd, buf, sizeof(buf), 0,(struct sockaddr *)&send->CientAddr,&len);		

		// sendto(send->sockfd, buf, sizeof(buf), 0,(struct sockaddr *)&send->CientAddr,sizeof(send->CientAddr));
		
		if(n < 0)//检测是否有UDP数据包: kcp头部+data
			continue;
		else {
			//预接收数据:调用ikcp_input将裸数据交给KCP，这些数据有可能是KCP控制报文，并不是我们要的数据。 
			//kcp接收到下层协议UDP传进来的数据底层数据buffer转换成kcp的数据包格式
			ret = ikcp_input(send->pkcp, buf, n);

			// while(true)	{
				//kcp将接收到的kcp数据包还原成之前kcp发送的buffer数据		
				ret = ikcp_recv(send->pkcp, buf, n);//从 buf中 提取真正数据，返回提取到的数据大小
				// if(ret < 0) 
				// 	break;
			// }

			//kcp提取到真正的数据	
			printf("Data from Client: %s size = %d\n", buf, ret);
			//kcp收到交互包，则回复			
			ret = ikcp_send(send->pkcp, buf, ret);
			if (ret > 0) {
				ikcp_flush(send->pkcp);
			}
		}
		
	}	
}

int main(int argc,char *argv[])
{
	printf("this is kcpServer\n");
	if(argc <2 )
	{
		printf("Please input server port\n");
		return -1;
	}
	
	kcpObj send;
	send.port = atoi(argv[1]);
	send.pkcp = NULL;
	
	bzero(send.buff,sizeof(send.buff));
	char Msg[] = "Server:Hello!";//与客户机后续交互	
	memcpy(send.buff,Msg,sizeof(Msg));

	ikcpcb *kcp = ikcp_create(0x1, (void *)&send);//创建kcp对象把send传给kcp的user变量
	kcp->output = udpOutPut;//设置kcp对象的回调函数
	ikcp_nodelay(kcp, 0, 10, 0, 0);//1, 10, 2, 1
	ikcp_wndsize(kcp, 128, 128);
	
	send.pkcp = kcp;

	init(&send);//服务器初始化套接字
	loop(&send);//循环处理
	
	return 0;	
}