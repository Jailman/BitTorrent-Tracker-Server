#include <iostream>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <conio.h>
#include <fstream>
#include <windows.h>
#include <vector>
using namespace std;
char bittorrentss[10000000];
char *bitrorrents=&bittorrentss[0];

struct info_one
{
    long lenth;
    string name;
    string name_utf_8;
    long piece_length;
    string pieces;
    string publisher;
    string publisher_url;
    string publisher_url_utf_8;
};

struct files
{
    long length;
    string path;
    string path_utf_8;
};

struct info_more
{
    files file;
    string name;
    string name_utf_8;
    long piece_length;
    string pieces;
    string publisher;
    string publisher_url;
    string publisher_url_utf_8;
    string publisher_utf_8;
};



class bittorrent
{
private:
    string announcetracker;
    string announcelist[1000];
    long creationdate;
    string comment;
    string ceratedby;
    info_one infoone;
    info_more infomore;
    string nodes;
public:
    int getannouncetracker(char *bittorrents);
    int getannouncelist(int nowplace,char *bittorrents);
    int getcreationdate();
    int getcomment();
    int getceratedby();
    int getinfoone_lenth();
    int getinfoone_name();
    int getinfoone_name_utf_8();
    int getinfoone_piece_length();
    int getinfoone_pieces();
    int getinfoone_publisher();
    int getinfoone_publisher_url();
    int getinfoone_publisher_url_utf_8();
    int getinfomore_files_length();
    int getinfomore_files_path();
    int getinfomore_files_path_utf_8();
    int getinfomore_name();
    int getinfomore_name_utf_8();
    int getinfomore_piece_length();
    int getinfomore_pieces();
    int getinfomore_publisher();
    int getinfomore_publisher_url();
    int getinfomore_publisher_url_utf_8();
    int getinfomore_publisher_utf_8();
    int getnodes();
};



int bittorrent::getannouncetracker(char *bittorrents)
{int midj;
for(int i=0;i<11;i++)
{bittorrents[i]='\0';}
char net[6];
for(int j=0;j<6;j++)
{
    char a=bittorrents[j+11];
    if(a!=':')
        net[j]=a;
    else midj=j;j=6;//midj中记录了url的位数的位数大小
}

int mid=midj;//mid用于控制循环，以便用midj进行位数计算
int numb=0;int l;
for(int k=0;k<mid;k++)
{int num=net[k]-48;
    for(l=0;l<midj-1;l++)
    {num*=10;}
numb+=num;midj--;
}
int m;char anourl[1000];int place=11+mid+2;
for(m=0;m<numb-8;m++)//去掉尾部的announce
{
    anourl[m]=bittorrents[place+m-1];

}
announcetracker=anourl;
int nowplace=place+numb;

return nowplace;//下一个的第一位是从1开始数的nowplace位
}

int bittorrent::getannouncelist(int nowplace,char *bittorrents)
{int nowlistplace=0;//用于记录当前录入的list位置
int midj=0;
char urlnum[6];
char num[2];int othernowplace=nowplace;
num[0]=bittorrents[nowplace-1];
num[1]=bittorrents[nowplace];
if(num[0]=='1'&&num[1]=='3')
{
    nowplace+=17;
if(bittorrents[nowplace-1]=='l')
{
    int i=nowplace;//bittorrents[nowplace]即表示l后第一个数字的bt数组内的编号
    for(int j=0;j<6;j++)
    {char a=bittorrents[i];
     if (a!=':')
     urlnum[j]=a;//urlnum记录了地址位数
     else if (a==':')
        midj=j;//midj记录了地址位数的位数
     }
int mid=midj;
int numb=0;int num=0;
for(int k=0;k<mid;k++)
{num=urlnum[k]-48;
    for(int l=0;l<midj-1;l++)
    {num*=10;}
    numb+=num;midj--;
}
     nowplace+=mid+2;char anolist[1000];//
for(int m=0;m<numb-8;m++)
   {
     anolist[m]=bittorrents[m+nowplace-1];
   }
announcelist[nowlistplace]=anolist;
nowlistplace++;
nowplace+=numb+1;
}

if(bittorrents[nowplace-1]<=57&&bittorrents[nowplace-1]>=48)
{
    int n=nowplace;
    for(int o=0;o<6;o++)
    {
        char b=bittorrents[n];
        if(n!=':')
            urlnum[o]=b;
        else if (b==':')
            midj=o;
    }
int midd=midj;
int numbb=0;
int numm=0;

for(int q=0;q<midd;q++)
{numm=urlnum[q]-48;
for(int p=0;p<midd;p++)
{numm*=10;}

numbb+=numm;midj--;}
nowplace+=midd+1;char anolistt[1000];//
for(int q=0;q<numbb-8;q++)
{
    anolistt[1000]=bittorrents[q+nowlistplace-1];
}
announcelist[nowlistplace]=anolistt;nowlistplace++;
nowplace+=numbb+1;
}


}
else {announcelist[0]="unfind";nowplace=othernowplace;}
return nowplace;
}

int main()
{
    cout<<"hellow.world";
}
