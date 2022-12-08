#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include "inodemap.h"

typedef struct {
    int create, extract, print, if_file, if_dir;
    char *archiveFile;
    char *input_dir;
}Options;

Options parseArgs(int argc, char *argv[]);
void checkModes (int numTrans);
void create(FILE *archiveFile, char *inputDirectoryString);
void print(char *archiveFileString);
void extract(FILE *archiveFile);

int main(int argc, char *argv[]) {
    Options opts = parseArgs(argc, argv);
    if (opts.create == 1){
        FILE *archiveFile = fopen(opts.archiveFile,"w+");
        int32_t magic_number = 0x7261746D;
        fwrite(&magic_number, 4, 1, archiveFile);
        struct stat file_stat;
        lstat(opts.input_dir, &file_stat);
        int64_t inode_num = (int64_t) file_stat.st_ino;
        fwrite(&inode_num , 8, 1, archiveFile);
        set_inode(file_stat.st_ino, opts.input_dir);
        int32_t file_name_length = strlen(opts.input_dir);
        fwrite(&file_name_length, 4, 1, archiveFile);
        fwrite(opts.input_dir, file_name_length, 1, archiveFile);
        int32_t mode = (int32_t) file_stat.st_mode;
        fwrite(&mode, 4, 1, archiveFile);
        int64_t time_num = (int64_t) file_stat.st_mtim.tv_sec;
        fwrite(&time_num, 8, 1, archiveFile);
        create(archiveFile, opts.input_dir);
        fclose(archiveFile);
    }
    if (opts.print == 1){
        print(opts.archiveFile);
    }
    if (opts.extract == 1){
        FILE *archiveFile = fopen(opts.archiveFile,"r");
        int32_t magic_number;
        fread(&magic_number, 4, 1, archiveFile);
        if (magic_number != 0x7261746D){
            fprintf(stderr, "Error: Bad magic number (%d), should be: %d.\n", magic_number, 0x7261746D);
            exit(-1);
        }
        extract(archiveFile);
        fclose(archiveFile);
    }
    exit(0);
}


void create(FILE *tar_file, char *input_dir){
    struct dirent *de;
    struct stat stat;
    char * fullname;

    DIR *input_directory = opendir(input_dir);
    fullname = (char *) malloc((strlen(input_dir)+258));

    for (de = readdir(input_directory); de != NULL; de = readdir(input_directory)) {
        sprintf(fullname, "%s/%s", input_dir, de->d_name);
        lstat(fullname, &stat);
        if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0 && !S_ISLNK(stat.st_mode)){
            int64_t inode_number = (int64_t) stat.st_ino;
            fwrite(&inode_number, 8, 1, tar_file);
            int32_t name_length = strlen(fullname);
            fwrite(&name_length, 4, 1, tar_file);
            fwrite(fullname, name_length, 1, tar_file);
            if(!get_inode( stat.st_ino )) {
                set_inode(stat.st_ino, fullname);
                int32_t mode = (int32_t) stat.st_mode;
                fwrite(&mode, 4, 1, tar_file);
                int64_t time = (int64_t) stat.st_mtim.tv_sec;
                fwrite(&time, 8, 1, tar_file);
                if (!S_ISDIR(stat.st_mode)){
                    int64_t file_size = (int64_t) stat.st_size;
                    fwrite(&file_size, 8, 1, tar_file);
                    if (file_size != 0) {
                        FILE *inputFile = fopen(fullname, "r");
                        char *content = malloc(stat.st_size * sizeof(char));
                        fread(content, stat.st_size, 1, inputFile);
                        fwrite(content, stat.st_size, 1, tar_file);
                        fclose(inputFile);
                    }
                }
            }
        }
        if (S_ISDIR(stat.st_mode) && strcmp(de->d_name, ".") !=0 && strcmp(de->d_name, "..") !=0 ) {
            create(tar_file, fullname);
        }
    }
    closedir(input_directory);
}

void extract(FILE *tar_file){

    int64_t temp;
    while(fread(&temp, 8, 1, tar_file) == 1){
        ino_t inode_number = (ino_t) temp;
        int32_t name_length;
        fread(&name_length, 4, 1, tar_file);
        char *file_name =  malloc(name_length+1 * sizeof(char));
        fread(file_name, name_length, 1, tar_file);
        file_name[name_length] = '\0';
        
        if(get_inode(inode_number) != NULL){
            char *src_name = (char *) malloc((strlen(get_inode(inode_number)) + 1) * sizeof(char));
            strcpy(src_name, get_inode(inode_number));
            link(src_name, file_name);
            free(src_name);
        }else{
            int32_t mode;
            int64_t mod_time;
            fread(&mode, 4, 1, tar_file);
            fread(&mod_time, 8, 1, tar_file);

            if (S_ISDIR((mode_t) mode)){
                mkdir(file_name, (mode_t) mode);
                set_inode(inode_number, file_name);
            }else{
                int64_t size;
                fread(&size, 8, 1, tar_file);
                FILE *currentFile = fopen(file_name, "w+");
                if (size != 0) {
                    char *content = (char *) malloc(size * sizeof(char));
                    fread(content, size, 1, tar_file);
                    fwrite(content, size, 1, currentFile);
                    free(content);
                }

                chmod(file_name, (mode_t) mode);

                struct timeval time_val;
                struct timeval time_mod;
                gettimeofday(&time_val, NULL);
                time_mod.tv_sec = (time_t) mod_time;
                time_mod.tv_usec = 0;
                struct timeval time_arr[2] = {time_val, time_mod};
                utimes(file_name, time_arr);

                set_inode(inode_number, file_name);
                fclose(currentFile);
            }
        }
    }
}

void print(char *tar_dir){
    FILE *tar_file = fopen(tar_dir,"r");
    int32_t magic_number;
    fread(&magic_number, 4, 1, tar_file);
    if (magic_number != 0x7261746D){
        fprintf(stderr, "Error: Bad magic number (%d), should be: %d.\n", magic_number, 0x7261746D);
        exit(-1);
    }
    ino_t inode_number = 0;
    int64_t inode_number_int;
    while(fread(&inode_number_int, 8, 1, tar_file) == 1){
        inode_number = (ino_t) inode_number_int;
        int32_t name_length;
        fread(&name_length, 4, 1, tar_file);
        char *name = (char *) malloc((name_length) * sizeof(char));
        fread(name, name_length, 1, tar_file);
        name[name_length] = '\0';
        if (get_inode(inode_number) != NULL){
            printf("%s -- inode: %lu\n", name, inode_number);
        }else{
            mode_t mode;
            int32_t mode_int;
            fread(&mode_int, 4, 1, tar_file);
            mode = (mode_t) mode_int;
            time_t mtime;
            int64_t mtime_int;
            fread(&mtime_int, 8, 1, tar_file);
            mtime = (time_t) mtime_int;
            set_inode(inode_number, name);
            if (S_ISDIR(mode)){
                printf("%s/ -- inode: %lu, mode: %o, mtime: %llu\n", name, inode_number, mode, (unsigned long long) mtime);
            }else {
                off_t size;
                int64_t size_int;
                fread(&size_int, 8, 1, tar_file);
                size = (off_t) size_int;
                if (size != 0) {
                    char *read_contents = (char *) malloc(size * sizeof(char));
                    fread(read_contents, size, 1, tar_file);
                }
                if ((mode & S_IXUSR) || (mode & S_IXGRP) || (mode & S_IXOTH)){
                    printf("%s* -- inode: %lu, mode: %o, mtime: %llu, size: %lu\n", name, inode_number, mode,
                           (unsigned long long) mtime, size);
                }else{
                    printf("%s -- inode: %lu, mode: %o, mtime: %llu, size: %lu\n", name, inode_number, mode,
                           (unsigned long long) mtime, size);
                }
            }
        }
    }
    fclose(tar_file);
}


Options parseArgs(int argc, char *argv[]){
    Options opts;
    opts.create = 0;
    opts.extract = 0;
    opts.print = 0;
    opts.if_file = 0;
    opts.if_dir = 0;
    int o;
    int numOptions = 0;

    while ((o = getopt(argc, argv, "cxtf:")) != -1) {
            switch (o) {
            case 'c':
                numOptions++;
                checkModes(numOptions);
                opts.create = 1;
                break;
            case 'x':
                numOptions++;
                checkModes(numOptions);
                opts.extract = 1;
                break;
            case 't':
                numOptions++;
                checkModes(numOptions);
                opts.print = 1;
                break;
            case 'f':
                opts.archiveFile = optarg;
                opts.if_file = 1;
                break;
            default:
                fprintf(stderr, "Error: No tarfile specified.\n");
                    exit(-1);
            }
    }
    if (optind <  argc){
        for(; optind < argc; optind++){
            if (opts.create) {
                opts.input_dir = argv[optind];
                opts.if_dir = 1;
            }
        }
    }

    if (numOptions == 0){
        fprintf(stderr, "Error: No mode specified.\n");
        exit(-1);
    }
    if (opts.create && opts.input_dir == NULL){
        fprintf(stderr, "Error: No directory target specified.\n");
        exit(-1);
    }
    if (opts.archiveFile == NULL){
        fprintf(stderr, "Error: No tarfile specified.\n");
        exit(-1);
    }
    if (opts.create && opts.input_dir != NULL){
        struct stat file_stat;
        if (lstat(opts.input_dir, &file_stat) != 0) {
            fprintf(stderr, "Error: Specified target (\"%s\") does not exist.\n", opts.input_dir);
            exit(-1);
        }else{
            if (!S_ISDIR(file_stat.st_mode)) {
                fprintf(stderr, "Error: Specified target (\"%s\") is not a directory.\n", opts.input_dir);
                exit(-1);
            }
        }
    }
    return opts;
}

void checkModes (int numTrans){
    if (numTrans > 1){
        fprintf(stderr, "Error: Multiple modes specified.\n");
        exit(1);
    }
}