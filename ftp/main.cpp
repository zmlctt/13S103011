#include <iostream>
#include <winsock.h>
#include <stdio.h>
#include<fstream>
#include <string.h>

using namespace std;
#define	WSVERS	MAKEWORD(2, 0)
#define DEFAULT_LSTN_PORT    2416
#define WEB_PORT    1234
#define UdpFtpPort 2345

int fget(SOCKET ssock,char* recBuf,char *sendBuf);
void udpGet(SOCKET sock,char recBuf[],struct sockaddr_in fsin);
bool endWith(const char a[],const string b);

int main()
{
    struct	sockaddr_in fsin;
    struct  sockaddr_in sin;
    SOCKET	TcpFtpSock, UdpFtpSock,WebSock,ssock;
    int	alen;
    WSADATA wsadata;
    struct protoent *ppe;
    if (WSAStartup(WSVERS, &wsadata) != 0)
    {
        WSACleanup();
        cout<<"WSAStartup failed\n";
    }


    memset(&sin,0,sizeof(sin));                             //TcpFtp
    TcpFtpSock = socket(PF_INET,SOCK_STREAM,0);
    if(TcpFtpSock<0)
        cout<<"creating socket failed\n";
    sin.sin_family=AF_INET;
    sin.sin_port=htons(DEFAULT_LSTN_PORT);
    sin.sin_addr.s_addr=INADDR_ANY;
    if(bind(TcpFtpSock,(struct sockaddr*)&sin,sizeof(sin))<0)
        cout<<"Bind failed\n";
    listen(TcpFtpSock,3);

    memset(&sin,0,sizeof(sin));                            //UdpFtp
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr=INADDR_ANY;
    sin.sin_port=htons(UdpFtpPort);

    ppe = getprotobyname("udp");
    UdpFtpSock = socket(AF_INET, SOCK_DGRAM, ppe->p_proto);
    bind(UdpFtpSock,(struct sockaddr *)&sin, sizeof(sin));

    memset(&sin,0,sizeof(sin));
    sin.sin_family=AF_INET;
    sin.sin_port=htons(WEB_PORT);
    sin.sin_addr.s_addr=INADDR_ANY;
    WebSock=socket(PF_INET,SOCK_STREAM,0);
    bind(WebSock,(struct sockaddr*)&sin,sizeof(sin));
    listen(WebSock,3);


    int maxfd;
    fd_set allset, rset;
    maxfd = TcpFtpSock;
    if(UdpFtpSock > maxfd)
    {
        maxfd = UdpFtpSock;
    }
    if(WebSock > maxfd)
    {
        maxfd = WebSock;
    }

    FD_ZERO(&allset);
    FD_SET(TcpFtpSock, &allset);
    FD_SET(UdpFtpSock, &allset);
    FD_SET(WebSock, &allset);

    while(1)
    {
        rset = allset;
        select(maxfd + 1, &rset, NULL, NULL, NULL);
        if(FD_ISSET(TcpFtpSock, &rset))
        {
            char WelBuf[80]={'H','e','l','l','o'};
            char sendBuf[80];
            char recBuf[80];

            alen=sizeof(fsin);
            ssock=accept(TcpFtpSock,(struct sockaddr*)&fsin,&alen);
            if(ssock == INVALID_SOCKET)
                break;
            cout<<inet_ntoa(fsin.sin_addr)<<"is connected at port"<<ntohs(sin.sin_port)<<endl;
            send(ssock,WelBuf,strlen(WelBuf),0);

            while(1)
            {

                int cc=0;
                int n=0;
                while(1)
                {
                    cc=recv(ssock,&recBuf[n],1,0);

                    if(cc==0||cc<0)
                        continue;
                    cout<<recBuf[n]<<endl;
                    if(recBuf[n]=='$')
                    {
                        recBuf[n]='\0';
                        break;
                    }
                    if(recBuf[n]!='\r')
                        ++n;
                }
                cout<<"the command is"<<recBuf<<endl;
                if(strncmp(recBuf,"get",3)==0)
                    fget(ssock,recBuf,sendBuf);

                if(strncmp(recBuf,"quit",4)==0)
                {
                    sprintf(sendBuf,"Bye");
                    send(ssock,sendBuf,strlen(sendBuf),0);
                    break;
                }
            }

            closesocket(ssock);
        }
        if(FD_ISSET(UdpFtpSock, &rset))
        {
            char recBuf[100];
            alen=sizeof(fsin);
            while(1)
            {
                recvfrom(UdpFtpSock,recBuf,sizeof(recBuf),0,(struct sockaddr *)&fsin,&alen);
                if(strncmp(recBuf,"get",3)==0)
                {
                    udpGet(UdpFtpSock,recBuf,fsin);
                }
                if(strncmp(recBuf,"quit",4)==0)
                {
                    break;
                }
            }
        }

        if(FD_ISSET(WebSock, &rset))
        {
            char recvBuf[1024];
            alen=sizeof(fsin);
            ssock=accept(WebSock,(struct sockaddr *)&fsin,&alen);
            int rec;

            rec=recv(ssock,recvBuf,sizeof(recvBuf),0);
            recvBuf[rec]='\0';
            char *p=strtok(recvBuf," ");
            if(p)
            {
                p=strtok(NULL," ");
            }

            char type[256];
            if(endWith(p,".jpg"))
            {
                strcpy(type,"image/jpeg\r\nAccept-Ranges:bytes");
            }
            else if(endWith(p,".gif"))
            {
                strcpy(type,"image/gif");
            }
            else if(endWith(p,".js"))
            {
                strcpy(type,"text/javascript");
            }
            else if(endWith(p,".css"))
            {
                strcpy(type,"text/css");
            }
            else
            {
                strcpy(type,"text/html");
            }
            char content[1024];
            string path;
            path="D:\\";
            path+=p;
            cout<<path<<endl;
            FILE *f = fopen(path.c_str(), "rb");
            int filesize=0;
            int thisTimeNum=0;
            int nSend;
            while(true)
            {
                memset(content, 0, sizeof(content));
                thisTimeNum = fread(content, sizeof(char), sizeof(content), f);
                if(thisTimeNum <= 0)
                {
                    printf("read over!\n");
                    break;
                }
                nSend = send(ssock, content, thisTimeNum, 0);
                if(nSend == SOCKET_ERROR)
                {
                    printf("sock send error!\n");
                    break;
                }
                filesize += nSend;
                fseek(f, filesize, SEEK_SET);
            }
            fclose(f);
            closesocket(ssock);
        }
    }
    closesocket(TcpFtpSock);
    closesocket(UdpFtpSock);
    return 0;
}

int fget(SOCKET ssock,char* recBuf,char *sendBuf)
{
    int i=4,k=0;
    char FileName[20],tempBuf[80];
    cout<<"required file is:";
    int bytes;
    while(1)
    {
        bytes=recv(ssock,&recBuf[i],1,0);
        if((bytes<0)||(bytes==0))
            break;
        FileName[k]=recBuf[i];
        if(recBuf[i]=='\0')
        {
            FileName[k]='\0';
            break;
        }
        if(recBuf[i]!='\r')
        {
            ++i;
            ++k;
        }
    }
    cout<<"\n";
    string path;
    path="D:\\";
    path+=FileName;
    cout<<path<<endl;
    ifstream fin(path,ios::in);
    if(!fin)
    {
        sprintf(sendBuf,"sorry,cant open%s",FileName);
        bytes=send(ssock,sendBuf,strlen(sendBuf),0);
        return 1;
    }
    else
    {
        cout<<"file"<<FileName<<"is found"<<endl;
        cout<<"#########################"<<endl;
        while(fin.get(tempBuf,80,'\0')!=NULL)
        {
            sprintf(sendBuf,"%s",tempBuf);
            send(ssock,sendBuf,80,0);
        }
        fin.close();
        sprintf(sendBuf,"finished!!!");
        send(ssock,sendBuf,strlen(sendBuf),0);
    }
    return 0;
}

void udpGet(SOCKET sock,char recBuf[],struct sockaddr_in fsin)
{
    char fileName[20];
    char sendBuf[100];
    int i=0,k=0;
    while(recBuf[i]!='$')
    {
        ++i;
    }
    ++i;
    while(recBuf[i]!='\0')
    {
        fileName[k]=recBuf[i];
        ++k;
        ++i;
    }
    fileName[k]='\0';
    char path[20]="D:\\";
    strcat(path,fileName);
    FILE *p;
    p=fopen(path,"rb");
    cout<<path<<endl;
    int read_len;
    int send_len;

    while(true)
    {
        read_len=fread(sendBuf,sizeof(char),100,p);
        cout<<sendBuf<<" "<<read_len<<endl;
        if(read_len<=0)
        {
            cout<<"read <=0"<<endl;
            break;
        }
        send_len=sendto(sock,sendBuf,read_len,0,(struct sockaddr*)&fsin,sizeof(fsin));
        if(send_len==SOCKET_ERROR)
            break;
    }
    sendto(sock,sendBuf,0,0,(struct sockaddr*)&fsin,sizeof(fsin));
    fclose(p);
}
bool endWith(const char a[],const string b)
{
    int alen=strlen(a);
    int blen=b.length();
    bool ret=true;
    for(int i=0; i<blen; ++i)
    {
        if(a[alen-blen+i]!=b.at(i))
        {
            ret=false;
            break;
        }
    }
    return ret;
}
