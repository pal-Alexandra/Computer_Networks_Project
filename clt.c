#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <wait.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>


extern int errno;
int clientSkt; //decriptorul de socket
char key[]="SANTA";

void exit_client(int signal)
{
    close(clientSkt);
    exit(0);
}

void getCommandArgs(char args[10][1001], char*source, int *count)
{
    char cuv[1001];
    int k=0;
    for(int j=0; j<strlen(source); j++)
    {
        if(source[j] != ' ')
        {
            cuv[k++]=source[j];
        }
        else
        {
            cuv[k]='\0';
            if(k>0)
            {
                strcpy(args[(*count)++],cuv);
                k=0;
            }
        }
    }

    if(k>0)
    {
        cuv[k]='\0';
        strcpy(args[(*count)++],cuv);
        k=0;
    }
}
void generate_key(char msg[], char k[])
{
    int len_msg=strlen(msg);
    int len_key=strlen(k);
    if(len_msg > len_key)
    {
        char aux[1001];
        strcpy(aux,k);
        int len_aux=len_key;
        int i=0;
        while(len_aux < len_msg)
        {
            aux[len_aux++]=k[i++];
            if(i==len_key)
                i=0;   
        }
        aux[len_aux]='\0';
        strcpy(k,aux);
    }
}

void encrypt(char msg[], char k[])
{
    int len_msg=strlen(msg);
    for(int i=0; i<len_msg; i++)
    {
        msg[i]=msg[i]+k[i];
    }
}

int is_directory(char dir[])
{
    DIR *dd;
    dd = opendir(dir);

    if(dd != NULL)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int exists(char path[], char file[])
{
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if(d != NULL)
    {
        while((dir = readdir(d)))
        {
            if(strcmp(dir->d_name,file) == 0)
            {
                return 1; //exista fisierul in director
            }
        }
    }
    return 0; //nu exista fisierul in director

}

int file_exists(char file[])
{
    int index = -1;
    for(int i=strlen(file)-1; i>=0 && index==-1; i--)
    {
        if(file[i] == '/')
        {
            index=i;
        }
    }
    
    char file_dir[1001];
    memset(file_dir,0,sizeof(file_dir));
    strncat(file_dir,file,index);
    char name[1001];
    memset(name,0,1001);
    strcpy(name,file+index+1);

    DIR* d;
    d=opendir(file_dir);
    struct dirent *dir;
    if(d!=NULL)
    {
        while(dir=readdir(d))
        {
            //printf("%s\n",dir->d_name);
            if(strcmp(dir->d_name, name) == 0)
            {
                return 1;
            }
        }
        closedir(d);
    }
    
    return 0;
}

void path_relativ(char local_path[], char path[])
{
    char aux[1001];
    strcpy(aux,local_path);
    strcat(aux,path+1);
    strcpy(path,aux);

}

int main()
{   
    if(signal(SIGINT,exit_client) == SIG_ERR)
    {
        perror("[CLIENT]: eroare la signal()\n");
        return errno;
    }
   
   
    struct sockaddr_in server; //structura folosita pentru conectare

    //cream socket-ul
    if((clientSkt= socket(AF_INET, SOCK_STREAM, 0)) ==-1)
    {
        perror("[CLIENT]: eroare la socket() \n");
        return errno;
    }
    

    //umplem structura folosita pentru conectare
    server.sin_family = AF_INET; //familia de socket-uri
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); //adresa IP a serverului: masina locala
    server.sin_port = htons(2035); //portul de conectare

    //ne conectam la server
    if(connect(clientSkt, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[CLIENT]: eroare la connect() \n");
        return errno;
    }

    char cmd[1001];
    char msg[2001];
    char msg1[1001];
    char msg2[1001];
    int size;
    char username[1001];
    char password[1001];
    int isLogged=0;
    int isBlack=0;

    while(1)
    {
        printf("Introduceti o comanda: ");
        memset(cmd, 0, 1001);
        fgets(cmd,sizeof(cmd),stdin);
        cmd[(int)strlen(cmd)-1] = '\0';

        char comArgs[10][1001];
        int lenArgs=0;
        //printf("comanda: %s \n", cmd);
        memset(comArgs,0,sizeof(comArgs));
        getCommandArgs(comArgs, cmd, &lenArgs);

        //scriu catre server pe rand: lungimea comenzii, comanda in sine
        if(strcmp(comArgs[0],"login") == 0)
        {
            //criptez username si parola
            char key_aux[1001];
            memset(key_aux,0,1001);
            strcpy(key_aux,key);
            generate_key(comArgs[1],key_aux);
            
            char user_cryp[1001];
            memset(user_cryp,0,1001);
            strcpy(user_cryp,comArgs[1]);
            encrypt(user_cryp,key_aux);

            char pass_cryp[1001];
            memset(pass_cryp,0,1001);
            memset(key_aux,0,1001);
            strcpy(key_aux,key);
            generate_key(comArgs[2], key_aux);
            strcpy(pass_cryp,comArgs[2]);
            encrypt(pass_cryp,key_aux);

            char cmd_aux[1001];
            memset(cmd_aux,0,1001);
            strcpy(cmd_aux,comArgs[0]);
            strcat(cmd_aux," ");
            strcat(cmd_aux,user_cryp);
            strcat(cmd_aux," ");
            strcat(cmd_aux,pass_cryp);
            strcpy(cmd,cmd_aux);
        }

        size = strlen(cmd);
        if(write(clientSkt, &size, sizeof(int)) <= 0)
        {
            perror("[CLIENT]: eroare la write(size) catre server \n");
            return errno;
        }
        if(write(clientSkt, cmd, size) <= 0)
        {
            perror("[CLIENT]: eroare la write(cmd) catre server \n");
            return errno;
        }
                
        if(strcmp(comArgs[0],"my_download") == 0)
        {
            char destination[1001];
            strcpy(destination,comArgs[2]);

            //tratez path absolut
            char path1[1001]; //path-ul care ne ajuta pt cale abs
            memset(path1, 0, 1001); //path-ul actual
            getcwd(path1,sizeof(path1));
            if(destination[0] == '.')
            {
                path_relativ(path1,destination);
            }
            
            //verific destinatia, ii scriu server ului 0/1 
            int status = is_directory(destination);
            if(write(clientSkt, &status, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la write(status_1) catre server \n");
                return errno;
            }
            if(status == 0)
            {
                printf("EROARE: %s nu este director\n", destination);
                continue; 
            }

            //citesc status de la server: to_dwonload este director sau nu : 1/0
            if(read(clientSkt, &status, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la read(status_2) de la server \n");
                return errno;
            }
            
            if(status == 1)
            {
                printf("EROARE: %s nu este fisier, ci director\n", comArgs[1]);
                continue; 
            }
            //citesc status de la server: to_download exista sau nu : 1/0
            if(read(clientSkt, &status, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la read(status_3) de la server \n");
                return errno;
            }
            if(status == 0)
            {
                printf("EROARE: %s nu este exista \n", comArgs[1]);
                continue; 
            }
            
            //citesc doar numele fisierului to_download
            memset(msg, 0, 2001);
            if(read(clientSkt, &size, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la read(size) de la server \n");
                return errno;
            }
            if(read(clientSkt, msg, size) <= 0)
            {
                perror("[CLIENT]: eroare la read(ans) de la server \n");
                return errno;
            }
           
            char name[1001];
            memset(name,0,1001);
            strcpy(name,msg);
            //verific daca in destinatie mai exista un fisier cu acelasi nume, daca da, trimit 1 si trec la alta comanda
            status = exists(destination, name);
            if(write(clientSkt, &status, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la write(status) de la server \n");
                return errno;
            }
            if(status == 0)
            {
                //pot incepe transferul
                char destination_file[1001];
                strcpy(destination_file,destination);
                strcat(destination_file,"/");
                strcat(destination_file,name);
                FILE *f1 = fopen(destination_file,"wb");
                if(f1 ==  NULL)
                {
                    printf("EROARE la crearea fisierului\n");
                    status = 0;
                }
                else
                    status = 1; //fisierul a putut fi creat
                
                if(write(clientSkt, &status, sizeof(int)) <= 0)
                {
                    perror("[CLIENT]: eroare la write(status) de la server \n");
                    return errno;
                }
                if(status == 0)
                {
                    continue;
                }
                else
                {
                    //ACUM INCEP TRANSFERUL
                    char ch[1];
                    int len;
                    if(f1 != NULL)
                    {
                        while (1)
                        {
                            memset(ch,0,1);
                            read(clientSkt, &len, sizeof(int));

                            if(len == -1)
                            {
                                printf("DONE. %s a fost descarcat\n", name);
                                break;
                            }
                            if(len > 0)
                            { 
                                read(clientSkt, ch, len);
                                fwrite(ch, 1, len, f1);
                            }                
                        }
                    }
                    fclose(f1);
                    continue;
                }
            }
            else
            {
                //mai exista un fisier cu acelasi nume in destinatie => alta comanda
                printf("Mai exista un fis cu acelasi nume in destinatie => introduceti alta comanda\n");
                continue;
            }
        }
        else if(strcmp(comArgs[0],"my_upload") == 0)
        {
            //facem upload unui fisier de pe masina clientului -> pe server
            char upload_file[1001];
            strcpy(upload_file,comArgs[1]); //fisierul pe care vreau sa l incarc

            //tratez path relativ
            char path1[1001];
            memset(path1, 0, 1001); //path-ul actual
            getcwd(path1,sizeof(path1));
            if(upload_file[0] == '.')
            {
                path_relativ(path1, upload_file);
            }

            //verific sursa, ii scriu server ului daca exista sau nu 1/0
            int status = file_exists(upload_file);
            if(write(clientSkt, &status, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la write(status_1) catre server \n");
                return errno;
            }
            if(status == 0)
            {
                printf("EROARE: fisierul %s nu exista \n", upload_file);
                continue; 
            }

            //citesc de la server daca exista sau nu destinatia
            if(read(clientSkt, &status, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la read(status_2) de la server \n");
                return errno;
            }
            if(status == 0)
            {
                printf("EROARE: destinatia %s nu exista \n", comArgs[2]);
                continue; 
            }

            //incep transferul
            //citesc de la server daca a putut crea sau nu fisierul
            if(read(clientSkt, &status, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la read(status_2) de la server \n");
                return errno;
            }

            if(status == 1)
            {
                FILE *f1= fopen(upload_file,"rb");
                int len;
                char ch[1];
                if(f1!=NULL)
                {
                    while (1)
                    {
                        memset(ch,0,1);
                        len = fread(ch, 1, 1, f1);

                        if(len > 0)
                        {
                            write(clientSkt, &len, sizeof(int));
                            write(clientSkt, ch, len);
                            continue;
                        }
                        else if (len == 0)
                        {
                            //am ajuns la finalul fisierului
                            if(feof(f1))
                                len = -1;
                            write(clientSkt, &len, sizeof(int));
                            break;
                        }
                        
                    }
                }
                fclose(f1);
                printf("DONE.Fisierul a fost incarcat\n");
                continue;;
            }
            else
            {
                printf("Server-ul nu a putut incarca fisierul\n");
                continue;
            }
        }
        else if(strcmp(comArgs[0],"exit") == 0)
        {
            close(clientSkt);
            return 0;
        }
        else
        {
            memset(msg, 0, 2001);
            if(read(clientSkt, &size, sizeof(int)) <= 0)
            {
                perror("[CLIENT]: eroare la read(size) de la server \n");
                return errno;
            }
            if(read(clientSkt, msg, size) <= 0)
            {
                perror("[CLIENT]: eroare la read(ans) de la server \n");
                return errno;
            }
            printf("Am primit de la server mesajul: < %s > \n",msg);
        }
        
    }
    
    close(clientSkt);
    return 0;
}