#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    FILE *fpt;
    int rows,cols,bytes,rows2,cols2,bytes2,sum,temp,temp2,temp3,FP_temp,TP_temp;
    int i=3,j,r,c,r2,c2,mean=0,min=0,max=0,oc,or,TP=0,FP=0,TN=0,FN=0;
    char header[320], header2[320], gt_letter[3], letter='e';
    unsigned char *img, *template;
    unsigned char *bin_img;
    unsigned char *norm_img;
    int *zero_temp, *MSF;
    int threshold[10]={60,80,100,120,140,160,180,200,220,240};

    if (argc!=4){
        printf("Usage: ./lab2.exe parenthood.ppm parenthood_e_template.ppm groundtruth.txt\n\n");
        exit(0);
    }
    
    //image
    if ((fpt=fopen(argv[1],"rb"))==NULL) {
        printf("Unable to open image for reading\n");
        exit(0);
    }

    fscanf(fpt,"%s %d %d %d",header,&cols,&rows,&bytes);

    if (strcmp(header,"P5")!= 0 || bytes!=255) {
        printf("Not a greyscale 8-bit PPM image\n");
        exit(0);
    }

    img=(unsigned char *)calloc((rows*cols),sizeof(unsigned char));
    header[0]=fgetc(fpt);
    fread(img,1,cols*rows,fpt);
    fclose(fpt);

    //template
    if ((fpt=fopen(argv[2],"rb"))==NULL) {
        printf("Unable to open image for reading\n");
        exit(0);
    }

    fscanf(fpt,"%s %d %d %d",header2,&cols2,&rows2,&bytes2);

    if (strcmp(header2,"P5")!= 0 || bytes2!=255) {
        printf("Not a greyscale 8-bit PPM image\n");
        exit(0);
    }

    template=(unsigned char *)calloc((rows2*cols2),sizeof(unsigned char));
    header2[0]=fgetc(fpt);
    fread(template,1,cols2*rows2,fpt);
    fclose(fpt);

    //zero-mean filter    
    zero_temp = (int *)calloc((rows2*cols2),sizeof(int));
    sum=0;
    for (c=0; c<(rows2*cols2); c++) {
        sum=sum+template[c];
    }
    mean=sum/(rows2*cols2);
    //printf("\nmean=%d",mean);

    for (c=0; c<(rows2*cols2); c++) {
        zero_temp[c]=template[c] - mean;
        //printf("val=%d\n",zero_temp[c]);
    }

    //convolution for MSF
    MSF = (int *)calloc((rows*cols),sizeof(int));    
    for (r=7; r<(rows-7); r++) {
        for (c=4; c<(cols-4); c++) {
            sum=0;
            temp=0;
            temp2=0;
            temp3=0;
            for (r2=-7; r2<=7; r2++) {
                for (c2=-4; c2<=4; c2++) {
                    temp=(r+r2)*cols+(c+c2);
                    temp2=(r2+7)*cols2+(c2+4);
                    temp3=(int)img[temp];
                    sum=sum+(temp3*zero_temp[temp2]);
                    //printf("img=%d temp=%d\n",img[temp],zero_temp[temp2]);
                }
            }
            MSF[r*cols+c]=sum;
        }
    }

    //finding max and min
    for (c=0; c<(rows*cols); c++) {
        if (c==0){
            min=MSF[c];
            max=MSF[c];
        }
        else {
            if (min > MSF[c]){
                min=MSF[c];
            }
            if (max < MSF[c]){
                max=MSF[c];
            }
        }
    }

    //printf("min=%d,max=%d\n",min,max);
    
    //normalize to 8-bit
    norm_img=(unsigned char *)calloc((rows*cols),sizeof(unsigned char));
    for (r=0; r<rows; r++){
        for (c=0; c<cols; c++){
            norm_img[r*cols+c]=((MSF[r*cols+c]-min)*255)/(max-min);
            //printf("norm=%d\n",norm_img[c]);
        }
    }
    fpt=fopen("MSF.ppm","w");
    fprintf(fpt,"P5 %d %d 255\n",cols,rows);
    fwrite(norm_img,cols*rows,1,fpt);
    fclose(fpt);

    //4a
    bin_img = (unsigned char *)calloc((rows*cols),sizeof(unsigned char));

    for (r= 0; r<rows; r++) {
        for (c=0; c<cols; c++){
            if(norm_img[r*cols+c] > threshold[8]) {
                bin_img[r*cols+c]=255;
            }
            else {
                bin_img[r*cols+c]=0;
            }
        }
    }
    
    fpt=fopen("binimg.ppm","w");
    fprintf(fpt,"P5 %d %d 255\n",cols,rows);
    fwrite(bin_img,cols*rows,1,fpt);
    fclose(fpt); 

    printf("\nGenerated: Binary Image of MSF at Threshold = %d\n\n",threshold[8]);

    //4b
    for (j=0; j<10; j++) {
        fpt=fopen(argv[3],"r");
        i=3;
        TP=0;
        FP=0;
        FN=0;
        TN=0;
        while(i == 3) {
            i=fscanf(fpt,"%s %d %d",gt_letter, &oc, &or);
            //printf("gtletter=%s\tletter=%c\n",&gt_letter[0],letter);
            if (i!=3) {
                break;
            }
            if (gt_letter[0] == letter) {
                TP_temp = TP;
                for(r=(or-7); r<=(or+7);r++){
                    for(c=(oc-4); c<=(oc+4); c++) {
                        if (norm_img[r*cols+c] > threshold[j]){
                            TP++;
                            r=(or+8);
                            c=(oc+5);
                        }
                    }
                }
                if(TP==TP_temp) {
                    FN++;
                }
            }
            else {
                FP_temp = FP;
                for(r=(or-7); r<=(or+7);r++){
                    for(c=(oc-4); c<=(oc+4); c++) {
                        if (norm_img[r*cols+c] > threshold[j]){
                            FP++;
                            r=(or+8);
                            c=(oc+4);
                        }
                    }
                }
                if (FP == FP_temp){
                    TN++;
                }
            }
        }
        printf("For Threshold T=%d\tTP=%d\tFP=%d\tTN=%d\tFN=%d\n",threshold[j],TP,FP,TN,FN);
        fclose(fpt);
    }

    free(img);
    free(template);
    free(zero_temp);
    free(MSF);
    free(norm_img);
    free(bin_img);


}