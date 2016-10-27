#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"Table.h"

void Word2Pinyin(char *word,char *buf,STable *ct){
    int len;
    int i;
    char ch[3];
    char *temp;
    len=strlen(word)/2;   // 汉字 两个字节
    buf[0]=0;
    ch[2]=0;
    for(i=0;i<len;i++){
        ch[0]=word[i*2];
        ch[1]=word[i*2+1];
        temp=GetSTableValue(ct,ch);
        if(temp!=NULL){
            strcat(buf,temp);
            strcat(buf," ");
        }
        else{
            strcat(buf,"a1 ");
        }
    }
}

//   
void Char2Pinyin(char **wordseq,char **pinyinseq,int nword,int *npinyin,STable *wt,STable *ct)
{
    char *temp,temp2[60];
    static char *buf=NULL;
    if(buf==NULL)
	{
        buf=(char *)malloc(sizeof(char)*1000);
    }
    buf[0]=0;
    int i;
    for(i=0;i<nword;i++)
	{
        temp=GetSTableValue(wt,wordseq[i]);
		// 直接找到这个词
        if(temp!=NULL)
		{
            strcat(buf,temp);
            strcat(buf," ");
            continue;
        }

		// 没有找到这个词  按照单字处理
		printf("word2pinyin---->%s\n",wordseq[i]);
        Word2Pinyin(wordseq[i],temp2,ct);
        strcat(buf,temp2);
    }

	// 将char * “zhong1 guo2” 
    Split(buf,pinyinseq,npinyin);
}
