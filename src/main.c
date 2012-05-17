/*
 * File:   main.c
 * Author: nemo
 *
 * Created on August 23, 2011, 1:55 PM
 */

#define FTP_SERVER_ADDR         "193.1.1.221"
#define PORT_FTP                21
#define PORT_FTP_DATA           20
#define MAX_SIZE_PACK           2048
#define MAX_SIZE_LIST           8192
#define MAX_MESSAGE_SIZE        2048

#define FTP_USER                "vxuser\0"
#define FTP_PASSWORD            "vxpasswd\0"

#define SERVER_NAME             "nemo server 24.08.2011"

#define CODE_OK                         200
#define CODE_NOT_IMPLEMENTED_YET        202
#define CODE_FILE_SIZE_OK               213
#define CODE_MDTM_OK                    213
#define CODE_SYST_INFO                  215
#define CODE_CONNECTION_OK              220
#define CODE_QUIT                       221
#define CODE_CLOSING_DATA_CONNECTION    226
#define CODE_ENTERING_PASSIVE_MODE      227
#define CODE_ENTERING_EXT_PASS_MODE     229
#define CODE_USER_LOGGED_IN             230
#define CODE_REQUEST_ACTION_OK          250
#define CODE_CURRENT_DIR_OK             257
#define CODE_FILE_STATUS_OK             150
#define CODE_USER_OK_PASS_NEED          331
#define CODE_INVALID_USER_OR_PASS       430
#define CODE_USER_LOGIN_FAILED          530
#define CODE_UNEXPECTED_COMMAND         533
#define CODE_FILE_UNAVAILABLE           550
#define CODE_REQUESTED_ACTION_ABORTED_1 451
#define CODE_ACCESS_DENIED              450
#define CODE_CONNECTION_ALREADY         125

#define UNKN    "\0"
#define QUIT    "QUIT\0"
#define USER    "USER\0"
#define PASS    "PASS\0"
#define SYST    "SYST\0"
#define PWD     "PWD\0"
#define TYPE    "TYPE\0"
#define CWD     "CWD\0"
#define PASV    "PASV\0"
#define REST    "REST\0"
#define EPSV    "EPSV\0"
#define LIST    "LIST\0"
#define PORT    "PORT\0"
#define MODE    "MODE\0"
#define NLST    "NLST\0"
#define SIZE    "SIZE\0"
#define MDTM    "MDTM\0"
#define RETR    "RETR\0"
#define MKD     "MKD\0"
#define STOR    "STOR\0"
#define RMD     "RMD\0"
#define SITE    "SITE\0"
#define DELE    "DELE\0"
#define XPWD    "XPWD\0"
#define XMKD    "XMKD\0"
#define CDUP    "CDUP\0"
#define XCUP    "XCUP\0"
#define XRMD    "XRMD\0"

#define MAX_SIZE_USERNAME       10

#define ASCII                   0
#define BINARY                  1

#define FILE_    0
#define FOLDER_  1
#define NONE_    2

#define MAX_FILE_SIZE           900
#define MAX_LEN_HOMEDIR         512


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>

typedef struct{
    char cmd[8];
    char val[128];
}REQUEST;

void send_reply(int *, int, char *);
int create_socket(void);
void create_sockaddr(struct sockaddr_in *, char *, int );
void parse_request(REQUEST *, char *);
int ls(char *, char *);
char *get_file_perm(char *, int);
char *get_file_date(char *, int);
int get_file_size(char *, int);
int get_file_type(char *);
int remove_dir(char *);

int main(int argc, char** argv) {
    int sFd, sFd2;
    int sFdFtp, sFdFtp2;
    struct sockaddr_in addrFtp, addrFtp2;
    struct sockaddr clientAddr;
    socklen_t socklen;
    int err;
    int szPack;
    char buf[MAX_SIZE_PACK];
    char mess[MAX_MESSAGE_SIZE];
    char username[MAX_SIZE_USERNAME];
    REQUEST req;
    int user_accepted;
    int user_logged_in;
    int waiting_pass;
    int type;
    int port;
    int passive_mode;
    int port_high;
    int port_low;
    char addr[16];
    int ip1, ip2, ip3, ip4;
    char list[MAX_SIZE_LIST];
    char size[12];
    char *body;
    int fd;
    int file_type;
    int size_body;
    char homedir[MAX_LEN_HOMEDIR];
    char *tmpdir;
    char tmpname[MAX_LEN_HOMEDIR];

    sFdFtp  = create_socket();
    create_sockaddr(&addrFtp,  FTP_SERVER_ADDR, PORT_FTP);
    err = bind(sFdFtp,  (struct sockaddr *)&addrFtp,  sizeof(addrFtp));
    err = listen(sFdFtp, 1);
    printf("initial: %s\n", strerror(errno));

    body = (char *)malloc(MAX_FILE_SIZE);

    err = (int)getcwd(homedir, sizeof(homedir));
    printf("Home dir: %s\n", homedir);
    sFdFtp2 = -1;
    while(1){
        waiting_pass   = 0;
        user_accepted  = 0;
        user_logged_in = 0;
        type=BINARY;
        port=0;
        passive_mode=0;
        strcpy(req.cmd, UNKN);
        strcpy(req.val, "");

        sFd = accept(sFdFtp, (struct sockaddr *)&clientAddr, &socklen);
        send_reply(&sFd, CODE_CONNECTION_OK, SERVER_NAME);

        while(strcmp(req.cmd, QUIT)!=0){
            strcpy(req.cmd, UNKN);
            strcpy(req.val, "");
            bzero(buf, sizeof(buf));
            printf("-------------------------------------\n");
            szPack = recv(sFd, &buf[0], MAX_SIZE_PACK, 0);
            if (szPack == 0){
                strcpy(req.cmd, QUIT);
                strcpy(req.val, "no_reply_quit");
            }
            else{
                if(szPack == -1){
                    close(sFd);
                    break;
                }
                else{
                    parse_request(&req, buf);
                }
            }
// CDUP, XCUP
            if((strcmp(req.cmd, CDUP)==0)||(strcmp(req.cmd, XCUP)==0)){
                if(user_logged_in){
                    err = chdir("..");
                    if(strlen(homedir) > strlen(getcwd(buf, sizeof(buf)))){
                        err = chdir(homedir);
                    }
                    sprintf(mess, "%s", getcwd(buf, sizeof(buf)));
                    if(strlen(mess) == strlen(homedir)){
                        send_reply(&sFd, CODE_REQUEST_ACTION_OK, "./");
                    }
                    else{
                        send_reply(&sFd, CODE_REQUEST_ACTION_OK, &mess[strlen(homedir)]);
                    }
                }
                else{
                    send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Not logged in");
                }
                continue;
            }
// CWD
            if(strcmp(req.cmd, CWD)==0){
                if(user_logged_in){
                    err = chdir(req.val);
                    if(strlen(homedir) > strlen(getcwd(buf, sizeof(buf)))){
                        err = chdir(homedir);
                    }
                    sprintf(mess, "%s", getcwd(buf, sizeof(buf)));
                    if(strlen(mess) == strlen(homedir)){
                        send_reply(&sFd, CODE_REQUEST_ACTION_OK, "./");
                    }
                    else{
                        send_reply(&sFd, CODE_REQUEST_ACTION_OK, &mess[strlen(homedir)]);
                    }
                }
                else{
                    send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Not logged in");
                }
                continue;
            }
// DELE
            if(strcmp(req.cmd, DELE)==0){
                if(user_logged_in){
                    if(remove(req.val)==0){
                        send_reply(&sFd, CODE_REQUEST_ACTION_OK, "File deleted");
                    }
                    else{
                        send_reply(&sFd, CODE_FILE_UNAVAILABLE, "File unavailable");
                    }
                }
                continue;
            }
// EPSV
            if(strcmp(req.cmd, EPSV)==0){
                if(user_logged_in){
                    port = 65530;
                    sFdFtp2 = create_socket();
                    create_sockaddr(&addrFtp2, FTP_SERVER_ADDR, port);
                    err = bind(sFdFtp2,  (struct sockaddr *)&addrFtp2,  sizeof(addrFtp2));
                    err = listen(sFdFtp2, 1);
                    sprintf(mess, "Entering extended passive mode (|||65530|)");
                    send_reply(&sFd, CODE_ENTERING_EXT_PASS_MODE, mess);
                    passive_mode=1;
                }
                else{
                    send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Not logged in");
                }
                continue;
            }
// LIST NLST
            if((strcmp(req.cmd, LIST)==0)||(strcmp(req.cmd, NLST)==0)){
                if(user_logged_in){
                    if(port!=0){
                        if(strlen(req.val) == 0){
                            err = ls(list, getcwd(buf, sizeof(buf)));
                        }
                        else{
                            err = ls(list, req.val);
                        }
                        if (err == 0){
                            if(passive_mode){
                                sFd2 = accept(sFdFtp2, (struct sockaddr *)&clientAddr, &socklen);
                            }
                            else{
                                sFd2 = create_socket();
                                create_sockaddr(&addrFtp2, addr, port);
                                err = connect(sFd2, (struct sockaddr *)&addrFtp2, sizeof(addrFtp2));
                            }
                            send_reply(&sFd, CODE_FILE_STATUS_OK, "File status okay");
                            send_reply(&sFd, CODE_CLOSING_DATA_CONNECTION, "Closing data connections");
                            send(sFd2, list, strlen(list), 0);
                            shutdown(sFdFtp2, SHUT_RDWR);
                            shutdown(sFd2, SHUT_RDWR);
                            close(sFdFtp2);
                            close(sFd2);
                        }
                        else{
                            send_reply(&sFd, CODE_ACCESS_DENIED, "Access denied");
                        }
                    }
                    else{
                        send_reply(&sFd, CODE_UNEXPECTED_COMMAND, "Try to LIST without port");
                        }
                    }
                passive_mode=0;
                port=0;
                continue;
            }


// MDTM
            if(strcmp(req.cmd, MDTM)==0){
                if(user_logged_in){
                    send_reply(&sFd, CODE_MDTM_OK, "20110825000000.000");
                }
                continue;
            }
// MKD
            if((strcmp(req.cmd, MKD)==0)||(strcmp(req.cmd, XMKD)==0)){
                if(user_logged_in){
                    err = mkdir(req.val, 0x777);
                    send_reply(&sFd, CODE_REQUEST_ACTION_OK, strerror(errno));
                }
                continue;
            }
// MODE
            if(strcmp(req.cmd, MODE)==0){
                if(user_logged_in){
                    send_reply(&sFd, CODE_NOT_IMPLEMENTED_YET, "Not implemented yet");
                }
                continue;
            }
// PASS
            if(strcmp(req.cmd, PASS)==0){
                if(waiting_pass==1){
                    if(strcmp(req.val, FTP_PASSWORD)==0){
                        if (user_accepted == 1){
                            user_logged_in = 1;
                            send_reply(&sFd, CODE_USER_LOGGED_IN, "User logged in");
                        }
                        else{
                            user_logged_in = 0;
                            send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Login failed");
                        }
                    }
                    else{
                        user_logged_in = 0;
                        send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Invalid username or password");
                    }
                }
                else{
                    send_reply(&sFd, CODE_UNEXPECTED_COMMAND, "Unexpected command");
                }
                waiting_pass = 0;
                user_accepted = 0;
                continue;
            }
// PASV
            if(strcmp(req.cmd, PASV)==0){
                if(passive_mode == 0){
                    if(user_logged_in){
                        close(sFdFtp2);
                        port = 65530;
                        sFdFtp2 = create_socket();
                        create_sockaddr(&addrFtp2, FTP_SERVER_ADDR, port);
                        err = bind(sFdFtp2,  (struct sockaddr *)&addrFtp2,  sizeof(addrFtp2));
                        err = listen(sFdFtp2, 1);
                        sprintf(mess, "Entering passive mode (193,1,1,221,255,250)");
                        send_reply(&sFd, CODE_ENTERING_PASSIVE_MODE, mess);
                        passive_mode=1;
                    }
                    else{
                        send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Not logged in");
                    }
                }
                continue;
            }
// PORT
            if(strcmp(req.cmd, PORT)==0){
                if(user_logged_in){
                    sscanf(strtok(req.val, ","), "%d", &ip1);
                    sscanf(strtok(0, ","), "%d", &ip2);
                    sscanf(strtok(0, ","), "%d", &ip3);
                    sscanf(strtok(0, ","), "%d", &ip4);
                    sscanf(strtok(0, ","),"%d", &port_high);
                    sscanf(strtok(0, ","),"%d", &port_low);
                    sprintf(addr, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
                    port = port_high*256+port_low;
                    printf("%s:%d\n", addr, port);
                    send_reply(&sFd, CODE_OK, "PORT Command successful");
                    passive_mode=0;
                }
                else{
                    send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Not logged in");
                }
                continue;
            }
// PWD
            if((strcmp(req.cmd, PWD)==0)||(strcmp(req.cmd, XPWD)==0)){
                if(user_logged_in){
                    sprintf(mess, "%s", getcwd(buf, sizeof(buf)));
                    if(strlen(mess) == strlen(homedir)){
                        send_reply(&sFd, CODE_CURRENT_DIR_OK, "./");
                    }
                    else{
                        send_reply(&sFd, CODE_CURRENT_DIR_OK, &mess[strlen(homedir)]);
                    }
                }
                else{
                    send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Not logged in");
                }
                continue;
            }
// QUIT
            if(strcmp(req.cmd, QUIT)==0){
                send_reply(&sFd, CODE_QUIT, "End of session.");
                close(sFd);
                continue;
            }
// REST
            if(strcmp(req.cmd, REST)==0){
                if(user_logged_in){
                    send_reply(&sFd, CODE_NOT_IMPLEMENTED_YET, "Not implemented yet");
                }
                continue;
            }
// RETR
            if(strcmp(req.cmd, RETR)==0){
                if(user_logged_in){
                    if(port!=0){
                        if(passive_mode){
                            sFd2 = accept(sFdFtp2, (struct sockaddr *)&clientAddr, &socklen);
                        }
                        else{
                            sFd2 = create_socket();
                            create_sockaddr(&addrFtp2, addr, port);
                            err = connect(sFd2, (struct sockaddr *)&addrFtp2, sizeof(addrFtp2));
                        }
                        file_type = get_file_type(req.val);
                        size_body = get_file_size(req.val, file_type);
                        fd = open(req.val, O_RDONLY);
                        if (fd!=-1){
                            send_reply(&sFd, CODE_FILE_STATUS_OK, "File status okay");
                            while(size_body >= MAX_FILE_SIZE){
                                err = read(fd, body, MAX_FILE_SIZE);
                                send(sFd2, body, MAX_FILE_SIZE, 0);
                                size_body -= MAX_FILE_SIZE;
                            }
                            err = read(fd, body, size_body);
                            send(sFd2, body, size_body, 0);
                            send_reply(&sFd, CODE_CLOSING_DATA_CONNECTION, "Closing data connections");
                        }
                        else{
                            send_reply(&sFd, CODE_FILE_UNAVAILABLE, "File not available");
                        }
                        close(fd);
                        close(sFd2);
                        close(sFdFtp2);
                    }
                    else{
                        send_reply(&sFd, CODE_UNEXPECTED_COMMAND, "RETR without PORT");
                    }
                }
                passive_mode=0;
                port=0;
                continue;
            }
// RMD, XRMD
            if((strcmp(req.cmd, RMD)==0)||(strcmp(req.cmd, XRMD)==0)){
                if(user_logged_in){
                    err = remove_dir(req.val);
                    if (err == 0){
                        send_reply(&sFd, CODE_REQUEST_ACTION_OK, "Directory successfully removed");
                    }
                    else{
                        send_reply(&sFd, CODE_REQUESTED_ACTION_ABORTED_1, "Directory not empty");
                    }
                }
                continue;
            }
// SITE
            if(strcmp(req.cmd, SITE)==0){
                if(user_logged_in){
                    send_reply(&sFd, CODE_NOT_IMPLEMENTED_YET, "Not implemented yet");
                }
                continue;
            }
// SIZE
            if(strcmp(req.cmd, SIZE)==0){
                if(user_logged_in){
                    switch(get_file_type(req.val)){
                        case NONE_:
                        case FOLDER_:
                                        send_reply(&sFd, CODE_ACCESS_DENIED, "Access denied");
                                        break;
                        case FILE_:     sprintf(size, "%d", get_file_size(req.val, FILE_));
                                        send_reply(&sFd, CODE_FILE_SIZE_OK, size);
                                        break;
                    }
                }
                continue;
            }
// STOR
            if(strcmp(req.cmd, STOR)==0){
                if(user_logged_in){
                    if(port!=0){
                        if(passive_mode){
                            sFd2 = accept(sFdFtp2, (struct sockaddr *)&clientAddr, &socklen);
                        }
                        else{
                            sFd2 = create_socket();
                            create_sockaddr(&addrFtp2, addr, port);
                            err = connect(sFd2, (struct sockaddr *)&addrFtp2, sizeof(addrFtp2));
                        }
                        sprintf(tmpname, "%s", strrchr(req.val, '/')+1);
                        req.val[strlen(req.val) - strlen(strrchr(req.val, '/'))] = 0;
                        tmpdir = getcwd(buf, sizeof(buf));
                        err = chdir(req.val);
                        fd = open(tmpname, O_CREAT|O_RDWR, S_IRWXO);
                        if (fd!=-1){
                            send_reply(&sFd, CODE_FILE_STATUS_OK, "File status okay");
                            size_body = recv(sFd2, body, MAX_FILE_SIZE, MSG_WAITALL);
                            while(size_body != 0){
                                err = write(fd, body, size_body);
                                size_body = recv(sFd2, body, MAX_FILE_SIZE, MSG_WAITALL);
                            }
                            send_reply(&sFd, CODE_CLOSING_DATA_CONNECTION, "Closing data connections");
                        }
                        else{
                            send_reply(&sFd, CODE_FILE_UNAVAILABLE, "Unable to create file");
                        }
                        close(fd);
                        err = chdir(tmpdir);
                        close(sFd2);
                        close(sFdFtp2);
                    }
                    else{
                        send_reply(&sFd, CODE_UNEXPECTED_COMMAND, "Try to RETR without port");
                    }
                }
                passive_mode=0;
                port=0;
                continue;
            }
// SYST
            if(strcmp(req.cmd, SYST)==0){
                if(user_logged_in){
                    send_reply(&sFd, CODE_SYST_INFO, "UNIX Type: L8");
                }
                else{
                    send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Not logged in");
                }
                continue;
            }
// TYPE
            if(strcmp(req.cmd, TYPE)==0){
                if(user_logged_in)
                    if(strcmp(req.val, "A\0")==0){
                        type = ASCII;
                        send_reply(&sFd, CODE_OK, "Type set to ASCII");
                    }
                    else{
                        type = BINARY;
                        send_reply(&sFd, CODE_OK, "type set to BINARY");
                    }
                else{
                    send_reply(&sFd, CODE_USER_LOGIN_FAILED, "Not logged in");
                }
                continue;
            }
// USER
            if(strcmp(req.cmd, USER)==0){
                if(strlen(req.val)<MAX_SIZE_USERNAME){
                    strcpy(username, req.val);
                }
                else{
                    memset(username, 0, sizeof(username));
                }

                if (strcmp(username, FTP_USER)==0){
                    user_accepted = 1;
                }
                else{
                    user_accepted = 0;
                }
                send_reply(&sFd, CODE_USER_OK_PASS_NEED, "User name received, need password");
                waiting_pass = 1;
                continue;
            }
        }
    }

    free(body);
    close(sFdFtp);
    return (EXIT_SUCCESS);
}

int remove_dir(char *name){
    DIR *dir;
    struct dirent *filefromdir;
    int flag = 0;
    /*int err;*/
    /*char buf[MAX_SIZE_PACK];*/

    if((dir = opendir(name)) != NULL){
        filefromdir = readdir(dir);
        while(filefromdir != NULL){
            if((strcmp(filefromdir->d_name, ".") != 0)&&(strcmp(filefromdir->d_name, "..") != 0)){
                flag = 1;
                break;
            }
            filefromdir = readdir(dir);
        }
        if(flag == 0){
            rmdir(name);
        }
    }
    closedir(dir);
    return flag;
}

int create_socket(void){
    int sFd;
    int val=1;

    sFd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(sFd, SOL_SOCKET, SO_REUSEADDR, (char *) & val, sizeof (val));

    return sFd;
}

void create_sockaddr(struct sockaddr_in *addr, char *server_addr, int port){
    bzero(addr, sizeof(struct sockaddr_in));

    addr->sin_family = PF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(server_addr);
}

void send_reply(int *sFd, int code, char *text){
    char message[MAX_MESSAGE_SIZE];

    sprintf(message, "%d %s\n", code, text);
    send(*sFd, message, strlen(message), 0);
    printf("REPLY: %s",message);
}

void parse_request(REQUEST *req, char *buf){
    int i;
    sscanf(buf, "%s %s", req->cmd, req->val);

    for(i=0; i<strlen(req->cmd); i++){
        req->cmd[i] = toupper(req->cmd[i]);
    }
    printf("COMMAND: %s  %s\n", req->cmd, req->val);
}

int ls(char *list, char *path){
    DIR *dir;
    struct dirent *filefromdir;
    int tmpfiletype;
    int tmpfilesize;
    /*int tmpfiledate;*/

    list[0]=0;

    printf("%s\n", path);

    if (get_file_type(path) == NONE_) return 1;

    if (get_file_type(path) == FILE_){
        sprintf(&list[strlen(list)], "-");
        sprintf(&list[strlen(list)], "%9s", get_file_perm(path, FILE_));
        sprintf(&list[strlen(list)], "    ");
        sprintf(&list[strlen(list)], " vxuser    ");
        sprintf(&list[strlen(list)], " vxuser    ");
        sprintf(&list[strlen(list)], " %10d", get_file_size(path, FILE_));
        sprintf(&list[strlen(list)], " %12s", get_file_date(path, FILE_));
        sprintf(&list[strlen(list)], " %s\r\n", path);
        return 0;
    }

    if ((dir = opendir(path)) != NULL) {
        filefromdir = readdir(dir);
        while (filefromdir != NULL){
            tmpfiletype = get_file_type(filefromdir->d_name);
            tmpfilesize = get_file_size(filefromdir->d_name, tmpfiletype);

            if (tmpfiletype == FILE_){
                sprintf(&list[strlen(list)], "-");
            }
            else{
                sprintf(&list[strlen(list)], "d");
            }
            sprintf(&list[strlen(list)], "%9s", get_file_perm(filefromdir->d_name, tmpfiletype));
            sprintf(&list[strlen(list)], "    ");
            sprintf(&list[strlen(list)], " vxuser    ");
            sprintf(&list[strlen(list)], " vxuser    ");
            sprintf(&list[strlen(list)], " %10d", tmpfilesize);
            sprintf(&list[strlen(list)], " %12s", get_file_date(filefromdir->d_name, tmpfiletype));
            sprintf(&list[strlen(list)], " %s\r\n", filefromdir->d_name);

            filefromdir = readdir(dir);
        }
    }

    closedir(dir);
    return 0;
}

int get_file_size(char *path, int filetype) {
    int file;
    int szFile = 0;

    if (filetype == FILE_) {
        file = open(path, O_RDONLY);
        szFile = lseek(file, 0, SEEK_END);
        close(file);
    }

    return szFile;
}

int get_file_type(char *path) {
    int filetype = FILE_;
    int file;
    DIR *dir;

    if ((dir = opendir(path)) == NULL) {
        if ((file = open(path, O_RDONLY)) == -1) {
            filetype = NONE_;
        } else {
            close(file);
        }
    } else {
        filetype = FOLDER_;
        closedir(dir);
    }
    return filetype;
}

char *get_file_perm(char *path, int type){
    int fd;

    if(type == FOLDER_){
        return("---------");
    }
    else{
        if ((fd = open(path, O_RDWR))!=-1){
            close(fd);
            return("rw-rw-rw-");
        }
        else{
            if ((fd = open(path, O_RDONLY))!=-1){
                close(fd);
                return("r--r--r--");
            }
            else{
                if ((fd = open(path, O_WRONLY))!=-1){
                    close(fd);
                    return("-w--w--w-");
                }
                else{
                    return("---------");
                }
            }
        }
    }

}

char *get_file_date(char *path, int type){
    return "Aug 25 18:43";
}

/*
 * формат записи по файлу
 *
 *    permiss  lnk owner      group            size
 *  .rwxrwxrwx_<->_<-------->_<-------->_<-------->_MMM_dd_hh:mm_<filename>\r\n
 *                                                 _MMM_dd__YYYY_
 *
 *
 * */
/*
1  ABOR
2  ACCT
3  ADAT
4  ALLO
5  APPE
6  AUTH
7  CCC
8  CDUP    - OK
9  CONF
10 CWD     - OK
11 DELE    - OK
12 ENC
13 EPRT
14 EPSV    - OK
15 FEAT
16 HELP
17 LANG
18 LIST    - OK
19 LPRT
20 LPSV
21 MDTM    - OK
22 MIC
23 MKD     - OK
24 MLSD
25 MLST
26 MODE    - Not implemented yet
27 NLST    - OK
28 NOOP
29 OPTS
30 PASS    - OK
31 PASV    - OK
32 PBSZ
33 PORT    - OK
34 PROT
35 PWD     - OK
36 QUIT    - OK
37 REIN
38 REST    - Not implemented yet
39 RETR    - OK
40 RMD     - Not implemented yet
41 RNFR
42 RNTO
43 SITE    - not implemented yet
44 SIZE    - OK
45 SMNT
46 STAT
47 STOR    - OK
48 STOU
49 STRU
50 SYST    - OK
51 TYPE    - OK
52 USER    - OK
53 XCUP    - OK
54 XMKD    - OK
55 XPWD    - OK
56 XRCP
57 XRMD
58 XRSQ
59 XSEM
60 XSEN
*/
