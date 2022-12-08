//THIS  CODE  IS  OUR  OWN  WORK,  IT  WAS  WRITTEN  WITHOUT  CONSULTING  A  TUTOR  OR  CODE  WRITTEN  BY OTHER STUDENTS OUTSIDE OF OUR TEAM. - Yulin Hu 
#include "pbm.h"
#include <stdio.h> 
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

struct Options{
    char mode;
    char *arg;
    int var;
    char *infile_name;
    char *outfile_name;
};

void bitmap(struct Options option){

    PPMImage *input = read_ppmfile(option.infile_name);
    PBMImage *output = new_pbmimage(input->width, input->height);

    for (int i=0; i< input->height; i++){
        for (int j = 0; j < input->width; j++){
            double avg = (input->pixmap[0][i][j] + input->pixmap[1][i][j] + input->pixmap[2][i][j]) /3.0;
            if (avg < input->max/2.0){
                output->pixmap[i][j] = 1;
            }
            else{
                output->pixmap[i][j] = 0;
            }
        }
    }
    write_pbmfile(output, option.outfile_name);
    del_ppmimage(input);
    del_pbmimage(output);
}

void grayscale(struct Options option){

    PPMImage *input = read_ppmfile(option.infile_name);
    PGMImage *output = new_pgmimage(input->width, input->height, option.var);

    for (int i=0; i< input->height; i++){
        for (int j=0; j< input->width; j++){
            double avg = (input->pixmap[0][i][j] + input->pixmap[1][i][j] + input->pixmap[2][i][j]) /3.0;
            output->pixmap[i][j] = (avg/input->max)*option.var;
        }
    }
    write_pgmfile(output, option.outfile_name);
    del_ppmimage(input);
    del_pgmimage(output);
}
void isolate(struct Options option){
    PPMImage *input = read_ppmfile(option.infile_name);
    PPMImage *output = new_ppmimage(input->width, input->height, input->max);

    for(int i=0; i<3; i++){
        for (int j=0; j< input->height; j++){
            for (int k=0; k< input->width; k++){
                output->pixmap[i][j][k] = input->pixmap[i][j][k];
            }
        }
    }

    if(strcmp("red", option.arg)==0){
        for (int i=0; i<3; i++){
            for (int j=0; j< output->height; j++){
                for (int k=0; k< output->width; k++){
                    if (i==1 || i==2){
                        output->pixmap[i][j][k] = 0;
                    }
                }
            }
        }
    }
    if(strcmp("green", option.arg)==0){
        for (int i=0; i<3; i++){
            for (int j=0; j< output->height; j++){
                for (int k=0; k< output->width; k++){
                    if (i==0 || i==2){
                        output->pixmap[i][j][k] = 0;
                    }
                }
            }
        }
    }
    if(strcmp("blue", option.arg)==0){
        for (int i=0; i<3; i++){
            for (int j=0; j< output->height; j++){
                for (int k=0; k< output->width; k++){
                    if (i==0 || i==1){
                        output->pixmap[i][j][k] = 0;
                    }
                }
            }
        }
    }

    write_ppmfile(output, option.outfile_name);
    del_ppmimage(input);
    del_ppmimage(output);
}

void rmv_color(struct Options option){
    PPMImage *input = read_ppmfile(option.infile_name);
    PPMImage *output = new_ppmimage(input->width, input->height, input->max);

    for(int i=0; i<3; i++){
        for (int j=0; j< input->height; j++){
            for (int k=0; k< input->width; k++){
                output->pixmap[i][j][k] = input->pixmap[i][j][k];
            }
        }
    }

    if(strcmp("red", option.arg)==0){
        for (int i=0; i<3; i++){
            for (int j=0; j< output->height; j++){
                for (int k=0; k< output->width; k++){
                    if (i==0){
                        output->pixmap[i][j][k] = 0;
                    }
                }
            }
        }
    }
    if(strcmp("green", option.arg)==0){
        for (int i=0; i<3; i++){
            for (int j=0; j< output->height; j++){
                for (int k=0; k< output->width; k++){
                    if (i==1){
                        output->pixmap[i][j][k] = 0;
                    }
                }
            }
        }
    }
    if(strcmp("blue", option.arg)==0){
        for (int i=0; i<3; i++){
            for (int j=0; j< output->height; j++){
                for (int k=0; k< output->width; k++){
                    if (i==2){
                        output->pixmap[i][j][k] = 0;
                    }
                }
            }
        }
    }

    write_ppmfile(output, option.outfile_name);
    del_ppmimage(input);
    del_ppmimage(output);
}

void sepia(struct Options option){

    PPMImage *input = read_ppmfile(option.infile_name);
    PPMImage *output = new_ppmimage(input->width, input->height, input->max);
    
    for (int i=0; i< input->height; i++){
        for (int j=0; j< input->width; j++){
            output->pixmap[0][i][j] = (double)((0.393*input->pixmap[0][i][j]) + (0.769*input->pixmap[1][i][j]) + (0.189*input->pixmap[2][i][j]));
            output->pixmap[1][i][j] = (double)((0.349*input->pixmap[0][i][j]) + (0.686*input->pixmap[1][i][j]) + (0.168*input->pixmap[2][i][j]));
            output->pixmap[2][i][j] = (double)((0.272*input->pixmap[0][i][j]) + (0.534*input->pixmap[1][i][j]) + (0.131*input->pixmap[2][i][j]));
        }
    }

    int max = 0;
    for(int i=0; i<3; i++){
        for (int j=0; j< input->height; j++){
            for (int k=0; k< input->width; k++){
                if(output->pixmap[i][j][k]>max){
                    max = output->pixmap[i][j][k];
                }
            }
        }
    }
    output->max = max;

    write_ppmfile(output, option.outfile_name);
    del_ppmimage(input);
    del_ppmimage(output);
}


void mirror(struct Options option){
    PPMImage *input = read_ppmfile(option.infile_name);

    for (int i=0; i<3; i++){
        for (int j=0; j< input->height; j++){
            for (int k=0; k< input->width/2; k++){
                input->pixmap[i][j][input->width-1-k] = input->pixmap[i][j][k];
            }
        }
    }

    write_ppmfile(input, option.outfile_name);
    del_ppmimage(input);
}

void thumbnail(struct Options option){
    PPMImage *input = read_ppmfile(option.infile_name);
    PPMImage *output = new_ppmimage(input->width/option.var, input->height/option.var, input->max);

    for (int i=0; i<3; i++){
        for (int j=0; j< input->height/option.var; j++){
            for (int k=0; k< input->width/option.var; k++){
                output->pixmap[i][j][k] = input->pixmap[i][j*option.var][k*option.var];
            }
        }
    }

    write_ppmfile(output, option.outfile_name);
    del_ppmimage(input);
    del_ppmimage(output);
}

void tile(struct Options option){
    PPMImage *input = read_ppmfile(option.infile_name);
    PPMImage *output = new_ppmimage(input->width, input->height, input->max);
    PPMImage *thumbnail = new_ppmimage(input->width/option.var, input->height/option.var, input->max);

    for (int i=0; i<3; i++){
        for (int j=0; j< input->height/option.var; j++){
            for (int k=0; k< input->width/option.var; k++){
                thumbnail->pixmap[i][j][k] = input->pixmap[i][j*option.var][k*option.var];
            }
        }
    }

    for (int i=0; i<3; i++){
        for (int a=0; a< option.var; a++){
            for (int b=0; b< option.var; b++){
                for (int j=0; j< input->height/option.var; j++){
                    for (int k=0; k< input->width/option.var; k++){
                        output->pixmap[i][a*thumbnail->height+j][b*thumbnail->width+k] = thumbnail->pixmap[i][j][k];
                    }
                }
            }
        }
    }

    write_ppmfile(output, option.outfile_name);
    del_ppmimage(input);
    del_ppmimage(output);
    del_ppmimage(thumbnail);
}


int main( int argc, char *argv[] )
{
    int opt;
    struct Options option;
    int flag1 = 0;
    int flag2 = 0;
    option.infile_name = argv[argc-1];
    if (argc == 4){
        option.mode = 'b';
    }
    while((opt = getopt(argc, argv, "bg:i:r:smt:n:o:")) != -1){

        if (argc<4 || argc>6){
            printf("Usage: ppmcvt [-bgirsmtno] [FILE]\n");
            exit(0);
        }

        switch(opt){
            case 'b':
                option.mode = 'b';
                flag2++;
                break;
            case 'g':
                option.mode = 'g';
                if (atoi(optarg) < 1 || atoi(optarg) > 65535){
                    printf("Error: Invalid max grayscale pixel value: %s; must be less than 65,536\n", optarg);
                    exit(0);
                }
                option.var = atoi(optarg);
                flag2++;
                break;
            case 'i':
                option.mode = 'i';
                if ((strcmp(optarg,"red") && strcmp(optarg,"green") && strcmp(optarg,"blue")) == 1){
                    printf("Error: Invalid channel specification: %s; should be 'red', 'green' or 'blue'\n", optarg);
                    exit(0);
                }
                
                option.arg = optarg;
                flag2++;
                break;
            case 'r':
                option.mode = 'r';
                if ((strcmp(optarg,"red") && strcmp(optarg,"green") && strcmp(optarg,"blue")) == 1){
                    printf("Error: Invalid channel specification: %s; should be 'red', 'green' or 'blue'\n", optarg);
                    exit(0);
                }

                option.arg = optarg;
                flag2++;
                break;
            case 's':
                option.mode = 's';
                flag2++;
                break;
            case 'm':
                option.mode = 'm';
                flag2++;
                break;
            case 't':
                option.mode = 't';
                if (atoi(optarg) < 1 || atoi(optarg) > 8){
                    printf("Error: Invalid scale factor: %d; must be 1-8\n", atoi(optarg));
                    exit(0);
                }
                option.var = atoi(optarg);
                flag2++;
                break;
            case 'n':
                option.mode = 'n';
                if (atoi(optarg) < 1 || atoi(optarg) > 8){
                    printf("Error: Invalid scale factor: %d; must be 1-8\n", atoi(optarg));
                    exit(0);
                }
                option.var = atoi(optarg);
                flag2++;
                break;
            case 'o':
                flag1 = 1;
                option.outfile_name = optarg;
                break;
            default:
                printf("Usage: ppmcvt [-bgirsmtno] [FILE]\n");
                exit(0);
        }
    }
    
    if (optind>argc){
        printf("Error: No input file specified\n");
        exit(0);
    }
    if (flag1 == 0){
        printf("Error: No output file specified\n");
        exit(0);
    }
    if (flag2>1){
        printf("Error: Multiple transformations specified\n");
        exit(0);
    }

    switch(option.mode){
        case 'b':
            bitmap(option);
            break;
        case 'g':
            grayscale(option);
            break;
        case 'i':
            isolate(option);
            break;
        case 'r':
            rmv_color(option);
            break;
        case 's':
            sepia(option);
            break;
        case 'm':
            mirror(option);
            break;
        case 't':
            thumbnail(option);
            break;
        case 'n':
            tile(option);
            break;
        default:
            break;
    }

    return 0;
}