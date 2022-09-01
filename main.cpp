#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>

#if defined ( __cplusplus)
extern "C"
{
#include "x264.h"
};
#else
#include "x264.h"
#endif

int width=640;
int height=480;
int csp=X264_CSP_I420;

using namespace std;
string file_yuv;
string file_h264;
void readTxt()
{
    ifstream fin("./encoder_info.txt");
    if(!fin)
    {
        printf("read ./encoder_info.txt error");
        return;
    }
    string s;
    int i = 0;
    while(fin>>s){
        if(i == 0)
        {
            cout<<"input file is : "<<s<<endl;
            file_yuv = s;
        }else{
            cout<<"output file is : "<<s<<endl;
            file_h264 = s;
        }
        i++;
    }
    fin.close();
    s.clear();
}

int main(int argc, char** argv)
{
         int ret;
         int y_size;
         int i,j;
         readTxt();
         FILE* fp_src  = fopen(file_yuv.data(), "rb");
         FILE* fp_dst = fopen(file_h264.data(), "wb");

         //Encode frame number
         //if set 0, encode all frame
         int frame_num=0;

         int iNal   = 0;
         x264_nal_t* pNals = NULL;
         x264_t* pHandle   = NULL;
         x264_picture_t* pPic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
         x264_picture_t* pPic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
         x264_param_t* pParam = (x264_param_t*)malloc(sizeof(x264_param_t));

         //Check
         if(fp_src==NULL||fp_dst==NULL){
                   printf("Error open files.\n");
                   return -1;
         }
         printf("fopen sucess\n");

         x264_param_default(pParam);   //给参数结构体a赋默认值
         x264_param_default_preset(pParam, "fast" , "zerolatency" );  //设置preset和tune

         //修改部分参数
         pParam->i_csp=csp;
         pParam->i_width   = width;   // 宽度
         pParam->i_height  = height;  // 高度
         pParam->i_fps_num  = 25;     // 设置帧率（分子）
         pParam->i_fps_den  = 1;      // 设置帧率时间1s（分母）

         pParam->i_threads  = X264_SYNC_LOOKAHEAD_AUTO;
         pParam->i_keyint_max = 10;              //在此间隔设置IDR关键帧

         pParam->rc.i_bitrate = 1200;       // 设置码率,在ABR(平均码率)模式下才生效，且必须在设置ABR前先设置bitrate
         pParam->rc.i_rc_method = X264_RC_ABR;  // 码率控制方法，CQP(恒定质量)，CRF(恒定码率,缺省值23)，ABR(平均码率)

         //set profile
         //x264_param_apply_profile(pParam, "baseline");
         x264_param_apply_profile(pParam, NULL);//

         //open encoder
         printf("X264_encoder_open start\n");
         pHandle = x264_encoder_open(pParam);

         printf("x264_picture_init\n");
         x264_picture_init(pPic_out);
         printf("x264_picture_alloc\n");
         x264_picture_alloc(pPic_in, csp, pParam->i_width, pParam->i_height);

         //ret = x264_encoder_headers(pHandle, &pNals, &iNal);

         y_size = pParam->i_width * pParam->i_height;
         //detect frame number
         if(frame_num==0){
                   fseek(fp_src,0,SEEK_END);
                   switch(csp){
                   case X264_CSP_I444:frame_num=ftell(fp_src)/(y_size*3);break;
                   case X264_CSP_I422:frame_num=ftell(fp_src)/(y_size*2);break;
                   case X264_CSP_I420:frame_num=ftell(fp_src)/(y_size*3/2);break;
                   default:printf("Colorspace Not Support.\n");return -1;
                   }
                   fseek(fp_src,0,SEEK_SET);
         }

         //Loop to Encode
         for( i=0;i<frame_num;i++){
                   switch(csp){
                   case X264_CSP_I444:{
                            fread(pPic_in->img.plane[0],y_size,1,fp_src);         //Y
                            fread(pPic_in->img.plane[1],y_size,1,fp_src);         //U
                            fread(pPic_in->img.plane[2],y_size,1,fp_src);         //V
                            break;}
                   case X264_CSP_I422:{
                            int index = 0;
                            int y_i=0,u_i=0,v_i=0;
                            for(index = 0 ; index < y_size*2 ;){
                                fread(&pPic_in->img.plane[0][y_i++],1,1,fp_src);   //Y
                                index++;
                                fread(&pPic_in->img.plane[1][u_i++],1,1,fp_src);   //U
                                index++;
                                fread(&pPic_in->img.plane[0][y_i++],1,1,fp_src);   //Y
                                index++;
                                fread(&pPic_in->img.plane[2][v_i++],1,1,fp_src);   //V
                                index++;
                            }break;}

                   case X264_CSP_I420:{
                            fread(pPic_in->img.plane[0],y_size,1,fp_src);         //Y
                            fread(pPic_in->img.plane[1],y_size/4,1,fp_src);       //U
                            fread(pPic_in->img.plane[2],y_size/4,1,fp_src);       //V
                            //FFMPEG编码可能需要修改
//                            memcpy(m_pFrame->data[0], m_pBuf, m_ySize);
//                            memcpy(m_pFrame->data[1], m_pBuf + m_ySize, m_ySize/4);
//                            memcpy(m_pFrame->data[2], m_pBuf + m_ySize*5/4, m_ySize/4);
                            break;}

                   default:{
                            printf("Colorspace Not Support.\n");
                            return -1;}
                   }
                   pPic_in->i_pts = i;

                   ret = x264_encoder_encode(pHandle, &pNals, &iNal, pPic_in, pPic_out);
                   if (ret< 0){
                            printf("Error.\n");
                            return -1;
                   }

                   printf("Succeed encode frame: %5d\n",i);

                   for ( j = 0; j < iNal; ++j){
                             fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
                   }
         }
         i=0;
         //flush encoder
         while(1){
                   ret = x264_encoder_encode(pHandle, &pNals, &iNal, NULL, pPic_out);
                   if(ret==0){
                            break;
                   }
                   printf("Flush 1 frame.\n");
                   for (j = 0; j < iNal; ++j){
                            fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
                   }
                   i++;
         }
         x264_picture_clean(pPic_in);
         x264_encoder_close(pHandle);
         pHandle = NULL;

         free(pPic_in);
         free(pPic_out);
         free(pParam);

         fclose(fp_src);
         fclose(fp_dst);

         printf("encoder yuv to h264 end\n");
         return 0;
}
