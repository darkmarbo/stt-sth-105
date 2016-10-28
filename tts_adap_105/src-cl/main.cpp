#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HTS_engine.h"
#include "Vocoder.h"
#include "Table.h"
#include "Char2Pinyin.h"
#include "ProsodicAnalysis.h"
#include "getLabel.h"


void trimWordTailTagger(char **wordseq, const int nWords)
{
    int i = 0;
    for(i = 0; i<nWords; i++)
    {
        int j=0;
        while(1)
        {
            if(wordseq[i][j]=='/')
            {
                wordseq[i][j] = 0;
                j = 0;
                break;
            }
            j++;
        }
    }
}


void getPosTagger(const char *line, int nWords, char **tagseq)
{
    int i=0, j=0;
    const char* tmp = line;

    for(; *tmp!=0; tmp++)
    {
        if( *tmp != '/')
        {
            continue;
        }
        tmp++;//skip '/'
        do{
            tagseq[i][j++] = *tmp;
            tmp++;
        }while(*tmp!=' ');

        tagseq[i][j] = 0;
        i++;
        j=0;
    }
}

int GetWordSegmentAndPosTagger(char *tline, char **wordseq, char **posseq){
    int nWords;
    Split(tline,wordseq,&nWords);
    getPosTagger(tline, nWords, posseq);
    trimWordTailTagger(wordseq, nWords);
    return nWords;
}



int main()
{



	int num_interp = 1;
    //声学模型相关文件的存储位置,写入字符串中
    char *durname="model\\dur.pdf";  // -md dur.pdf
    char *durtreename="model\\tree-dur.inf";  // -td tree-dur.inf
    char *lspname="model\\mgc.pdf";  // -mm mgc.pdf 
    char *lsptreename="model\\tree-mgc.inf"; // -tm tree-mgc.inf
    char *lspwinfn1="model\\mgc.win1";
    char *lspwinfn2="model\\mgc.win2";
    char *lspwinfn3="model\\mgc.win3";
    char **lspwinfn=(char**)malloc(3*sizeof(char*));  // -dm mgc.win1 -dm mgc.win2 -dm mgc.win3
    lspwinfn[0]=lspwinfn1;
    lspwinfn[1]=lspwinfn2;
    lspwinfn[2]=lspwinfn3;
    //char *gvname="model\\gv-mgc.pdf";
    char *lf0name="model\\lf0.pdf"; //  -mf lf0.pdf
    char *lf0treename="model\\tree-lf0.inf"; // -tf tree-lf0.inf
    char *lf0winfn1="model\\lf0.win1";
    char *lf0winfn2="model\\lf0.win2";
    char *lf0winfn3="model\\lf0.win3";
    char **lf0winfn=(char**)malloc(3*sizeof(char*)); // -df lf0.win1 -df lf0.win2 -df lf0.win3
    lf0winfn[0]=lf0winfn1;
    lf0winfn[1]=lf0winfn2;
    lf0winfn[2]=lf0winfn3;


    //申请内存并初始化
    HTS_Engine *engine=(HTS_Engine *)malloc(sizeof(HTS_Engine));
    HTS_Engine_initialize(engine,3); // mod


    HTS_Engine_set_sampling_rate(engine,16000);  //设置采样率
    HTS_Engine_set_log_gain(engine,FALSE);       //是否采用对数增益
    HTS_Engine_set_fperiod(engine,80);           //帧移初始化
    HTS_Engine_set_msd_threshold(engine,1,0.5);  //设置msd概率




    //以下部分读取相应的模型
	/* load duration model */
    HTS_Engine_load_duration_from_fn(engine,&durname,&durtreename,1);
	/* load stream[0] (spectrum model) */
    HTS_Engine_load_parameter_from_fn(engine,&lspname,&lsptreename,lspwinfn,0,FALSE,3,1);
    free(lspwinfn);
	/* load stream[1] (lf0 model) */
    HTS_Engine_load_parameter_from_fn(engine, &lf0name,&lf0treename, lf0winfn,1,TRUE,3,1);
    free(lf0winfn);

	// mod 新增加的 
	char *fn_ms_lpf = "model\\lpf.pdf"; 
	char *fn_ts_lpf = "model\\tree-lpf.inf";
	char *fn_ws_lpf = "model\\lpf.win1 ";
	HTS_Engine_load_parameter_from_fn(engine, &fn_ms_lpf, 
		&fn_ts_lpf, &fn_ws_lpf, 2, FALSE, 1, 1);

	// mod 新增加的
	/* load gv[0] (GV for spectrum) */
	char *fn_ms_gvm = "model\\gv-mgc.pdf";
	char *fn_ts_gvm = "model\\tree-gv-mgc.inf"; 
	HTS_Engine_load_gv_from_fn(engine, &fn_ms_gvm, &fn_ts_gvm, 0, 1);


    double *gen,*lgn,*lf0; //初始化相关内存
    lgn=(double*)malloc(sizeof(double) * 20000);
    lf0=(double*)malloc(sizeof(double) * 20000);
    int len, i, msd_frame;             //信号处理模块采用的循环变量以及记录长度的变量

   

    //韵律词、韵律短语、语调短语模型
    Table * pw,*pp,*ip;
    pw= ReadTable("./resource/ProsodicWordModel.txt");   //韵律词
    pp= ReadTable("./resource/ProsodicPhraseModel.txt");  //韵律短语
    ip= ReadTable("./resource/IntPhraseModel.txt");       //语调短语


    STable *wt, *ct;
    wt=ReadSTable("./resource/dict.txt");   //字典
    ct=ReadSTable("./resource/char2pinyin.txt");  //字音对照表

    FILE *fp_txt=fopen("test.txt","r");   //测试文本
    char tline[1024]="0";                  //用于一行一行读入测试文本

    int *pwr,*ppr;
    short *ptag;
    char *teof;
    pwr=(int *)malloc(sizeof(int)*60);
    ppr=(int *)malloc(sizeof(int)*60);
    ptag=(short *)malloc(sizeof(short)*60);

    char **wordseq,**posseq,**pinyinseq;
    wordseq=(char**)malloc(sizeof(char *)*150);
    posseq=(char**)malloc(sizeof(char *)*150);
    pinyinseq=(char**)malloc(sizeof(char *)*150);
    for(i=0;i<150;i++){
        wordseq[i]=(char *)malloc(sizeof(char)*30);  //单词序列，每一个wordseq[i]存储一个单词
        posseq[i]=(char *)malloc(sizeof(char)*30);   //词性序列
        pinyinseq[i]=(char *)malloc(sizeof(char)*30); //字音转换后得到的拼音序列
    }
    int nWord,nChar;
    short temp;


    teof=fgets(tline,1023,fp_txt);
    dropReturnTag(tline);

    FILE * fp_wav=fopen("./test.pcm","wb");//二进制写入，用于存储合成的语音

    TTS_Label_Init(); //初始化Label模块

	int c_test = 0;
	FILE * fp_log=fopen("./test_log.txt","w");
    while(teof){

        nWord=GetWordSegmentAndPosTagger(tline,wordseq,posseq);

        ProsodicWordAnalysis(wordseq,posseq,nWord,pwr,pw);  //韵律词分析
        ProsodicPhraseAnalysis(wordseq,posseq,nWord,pwr,ppr,pp,ip); //一级、二级韵律短语

        GetProsodicTag(wordseq,nWord,pwr,ppr,ptag);
        //统一转换格式，生成ptag，ptag数字1，2，3 分别表示韵律词尾，二级韵短尾，一级韵短尾
        //0表示非韵律词尾


        //字音转换
        Char2Pinyin(wordseq, pinyinseq, nWord, &nChar, wt, ct);

        //根据字音转换信息、韵律信息，生成每个字的完整信息，并写入cif
		 //记录字符信息的数据结构及其初始化
		TtsLabelCharInfo *cif=(TtsLabelCharInfo *)malloc(sizeof(TtsLabelCharInfo)*300);
        TtsLabel_ObtainLabelCharSeq(cif,pinyinseq,nChar,ptag);
        //打印出标准HTS格式label文件，并写入label.txt
        PrintLabel(cif, nChar,"./label.txt");
		free(cif);

        //至此，前端模块结束
        //以下输出韵律信息，供测试人员分析
		for(int ii=0; ii<nChar; ii++)
		{
			printf("%s#",pinyinseq[ii]);
		}
		printf("\n");
        for(i=0;i<nWord;i++){
            printf("%s/%s  ",wordseq[i],posseq[i]);
        }
        printf("\n");
        for(i=0;i<nWord;i++){
            printf("%s",wordseq[i]);
            if(ppr[i]==1){
                printf("|");
            }
            if(ppr[i]==2){
                printf("$");
            }
            else if(pwr[i]==1){
                printf(" ");
            }
        }
        printf("\n\n");


        //后端HTS读如label文件。
        HTS_Engine_load_label_from_fn(engine,"./label.txt");

        //参数规划过程
        HTS_Engine_create_sstream(engine);
        HTS_Engine_create_pstream(engine);
		fprintf(fp_log,"lf0\n");
        for(i=0,msd_frame=0;i<engine->pss.total_frame;i++)
		{
            if ( engine->pss.pstream[1].msd_flag[i] ) 
			{
                lf0[i]=engine->pss.pstream[1].par[msd_frame][0];
				fprintf(fp_log,"%f\n",lf0[i]);
                msd_frame++;
            }
            else
			{
                lf0[i]=0;
            }
        }

        //合成过程
        gen=LPCSynth(engine->pss.pstream[0].par+5, lf0+5,
                     engine->pss.pstream[0].static_length-1,engine->pss.total_frame-5,&len);
        //gen=MGCSynth(engine->pss.pstream[0].par+5, lf0+5, engine->pss.pstream[0].static_length-1,engine->pss.total_frame-5,&len);

        //将合成结果写入文件（需转成short型再写入）
		c_test++;
		fprintf(fp_log,"gen\n");
        for(i=0;i<len-456;i++)
		{
			
			fprintf(fp_log,"%f\n",gen[i]);
			
            temp = (short)(gen[i]*32700);
			//temp = (short)(gen[i]*20000);
            fwrite(&temp,sizeof(short),1,fp_wav);
			//printf("\n%d\n",sizeof(short));
        }
		fprintf(fp_log,"\n");
		
        free(gen);
		short short0 = 0;
        for(i=0;i<6000;i++){
            fwrite(&short0,sizeof(short),1,fp_wav);
        }

        //读如下一行，进入下一轮循环
        teof=fgets(tline,1023,fp_txt);
        dropReturnTag(tline);

        //更新HTS引擎状态
        HTS_Engine_refresh(engine);


    }

    printf("process ok!\n");

    fclose(fp_wav);
	fclose(fp_log);
    free(engine);
    free(lgn);
    free(lf0);

    return 1;
}
