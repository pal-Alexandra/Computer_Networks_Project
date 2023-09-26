#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <dirent.h>
#include <wait.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

const int COMMAND_NR = 14;
const char COMMAND_NAME[][20] = {"help","exit", "login", "logout", "location", "list", "my_chdir", "my_mkdir", "my_rm", "my_delete", "my_rename", "my_mv", "my_download", "my_upload"};
const int COMMAND_LEN_ARGS[] = {  1,    1,       3,       1,        1,          1,      2,          2,          2,       2,           3,           3,       3,             3}; 
int clt_nr;
char key[]="SANTA";

extern int errno;

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

int getCommandType(char comArgs[10][1001], int lenArgs)
{
    if(lenArgs == 0)
        return -3; 
    
    for(int i=0; i<COMMAND_NR; i++)
    {
        if(strcmp(comArgs[0], COMMAND_NAME[i]) ==0 )
        {
            if(lenArgs == COMMAND_LEN_ARGS[i])
                return i+1;
            else
                return -2; //comanda nu are destule argumente
        }
    }
    
    return -1; //comanda necunoscuta
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

void decrypt(char msg[], char k[])
{
    int len_msg=strlen(msg);
    for(int i=0; i<len_msg; i++)
    {
        msg[i]=msg[i]-k[i];
    }
}

int look_for(char u[], char p[])
{
    FILE *f = fopen("login.txt", "r");
    if (f == NULL)
    {
        perror("[SERVER]: eroare la deschidere login.txt\n");
        exit(EXIT_FAILURE);
    }

    char file_u[101], file_pas[201];
    for (int i = 0; i < 3; i++)
    {
        memset(file_u, 0, 101);
        memset(file_pas, 0, 201);
        fscanf(f, "%s %s", file_u, file_pas);

        if (strcmp(file_u, u) == 0 && strcmp(file_pas, p) == 0)
        {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

int look_for_white(char u[])
{
    FILE *f = fopen("white_list.txt", "r");
    if (f == NULL)
    {
        perror("[SERVER]: eroare la deschidere white_list.txt\n");
        exit(EXIT_FAILURE);
    }

    char file_u[101];
    memset(file_u, 0, 101);
    for (int i = 0; i < 2; i++)
    {
        memset(file_u, 0, 101);
        fscanf(f, "%s", file_u);

        if (strcmp(file_u, u) == 0)
        {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void is_logout(int fd)
{
    printf("Clientul trebuie sa fie logat pentru a introduce aceasta comanda\n");
    char ans[1001];
    int size;
    memset(ans, 0, 1001);
    strcpy(ans, "Esti delogat. Nu poti executa aceasta comanda.");
    size = strlen(ans);

    if (write(fd, &size, sizeof(int)) == -1)
    {
        perror("[SERVER] eroare la write(size_ans) catre client.\n");
    }
    if (write(fd, ans, size) == -1)
    {
        perror("[SERVER] eroare la write(ans) spre client.\n");
        exit(EXIT_FAILURE);
    }
}

void is_black(int fd)
{
    printf("Clientul este blackListed => nu are permisiunea de a introduce aceasta comanda\n");
    char ans[1001];
    int size;
    memset(ans, 0, 1001);
    strcpy(ans, "Esti blacklisted. Nu poti executa aceasta comanda.");
    size = strlen(ans);

    if (write(fd, &size, sizeof(int)) == -1)
    {
        perror("[SERVER] eroare la write(size_ans) catre client.\n");
    }
    if (write(fd, ans, size) == -1)
    {
        perror("[SERVER] eroare la write(ans) spre client.\n");
        exit(EXIT_FAILURE);
    }
}

int list(char path[], int clientSd)
{
    DIR *d;
    d=opendir(path);
    struct dirent *dir;
    struct stat fstat;
    
    char content[10001];
    memset(content,0,10001);
    strcat(content,"CONTINUT: \n");
    char local_path[1001];
    char file[101];
    memset(file,0,101);
    int size=0;
    
    if(d != NULL)
    {
        while(dir = readdir(d))
        {
            if(strcmp(dir->d_name,".") != 0 && strcmp(dir->d_name,"..") != 0)
            {
                strcpy(file, dir->d_name);
                strcat(file," ");
                
                memset(local_path,0,1001);
                strcpy(local_path,path);
                strcat(local_path,"/");
                strcat(local_path,dir->d_name);
                stat(local_path,&fstat);
                if(S_ISDIR(fstat.st_mode) != 0)
                {
                    strcat(file,"directory");
                }
                if(S_ISREG(fstat.st_mode) != 0)
                {
                    strcat(file,"file");
                }
                
                strcat(content,file);
                strcat(content,"\n");
            }
        }

        size=strlen(content);
        if(write(clientSd, &size, sizeof(int)) == -1)
        {
            perror("[SERVER] eroare la write(size) spre client.\n");
            return errno;
        }
        if(write(clientSd, content, size) == -1)
        {
            perror("[SERVER] eroare la write(content) spre client.\n");
            return errno;
        } 
    }else
    {
        strcpy(content,"Nu am putut deschide locatia");
        size=strlen(content);
        if(write(clientSd, &size, sizeof(int)) == -1)
        {
            perror("[SERVER] eroare la write(size) spre client.\n");
            return errno;
        }
        if(write(clientSd, content, size) == -1)
        {
            perror("[SERVER] eroare la write(content) spre client.\n");
            return errno;
        } 
    }
    return 0;
}

char* my_chdir(char path[], char local_path[], char to_change[])
{
    char *response;
    char new_path[2001];
    memset(new_path,0,2001);
    
    if(strcmp(to_change, "..") ==0)
    {
        if(strcmp(path,local_path) == 0)
        {
            response =  "FAIL.Sunteti in radacina. Nu puteti merge la un nivel superior";
            return response;
        }else
        {
            int index = -1;
            for(int i=strlen(local_path)-1; i>=0 && index==-1; i--)
            {
                if(local_path[i] == '/')
                {
                    index=i;
                }
            }
            strncat(new_path,local_path,index);
            response=new_path;
            return response;
        }
    }else
    {
        //lipesc la local_path com[1] si incerc sa l deschid, daca-l pot deschide, atunci com[1] e nume de director valid
        strcat(new_path,local_path);
        strcat(new_path,"/");
        strcat(new_path,to_change);

        struct stat filestat;
        stat(new_path,&filestat);
        DIR *d;
        d=opendir(new_path);
        if(d == NULL)
        {
            response = "FAIL.Numele introdus nu reprezinta un director valid";
            return response;
        }

        response = new_path;
        return response;
    }    
}

int my_mkdir(char local_path[], char dir[])
{
    
    char new_dir[1001];
    memset(new_dir,0,1001);
    strcpy(new_dir, local_path);
    strcat(new_dir,"/");
    strcat(new_dir, dir);
    
    
    if (mkdir(new_dir, 0777) == -1)
    {
        printf("Eroare la creare director\n");
        return 0;
    }

    return 1;
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

int my_rm(char *to_delete)
{
    
    DIR *d = opendir(to_delete);

    int to_deleteLen = strlen(to_delete);
    int stop = -1;

    if(d != NULL)
    {
        struct dirent *dd;

        stop = 0;
        while(!stop && (dd=readdir(d)))
        {
            int stop_2 = -1;
            char *newToDelete;
            int len;

            if(!strcmp(dd->d_name,".") || !strcmp(dd->d_name,".."))
                continue; //nu vreau sa merg recursiv in . si in ..
            
            len = to_deleteLen + strlen(dd->d_name) + 2;
            newToDelete = malloc(len);

            if(newToDelete)
            {
                struct stat statNewToDelete;
                snprintf(newToDelete, len, "%s/%s", to_delete, dd->d_name);
                if(!stat(newToDelete, &statNewToDelete))
                {
                    if(S_ISDIR(statNewToDelete.st_mode))
                        stop_2 = my_rm(newToDelete);
                    else
                        stop_2 = unlink(newToDelete);
                }
                free(newToDelete);
            }
            stop = stop_2;
        }
        closedir(d);
    }

    if(stop == 0)
        stop = rmdir(to_delete);
    
    return stop;
}

int my_delete(char local_path[], char file[])
{
    char to_delete[1001];
    strcpy(to_delete, local_path);
    strcat(to_delete, "/");
    strcat(to_delete, file);

    if(is_directory(to_delete) == 1)
    {
        return -1;
    }
    
    if(unlink(to_delete) == -1)
    {
        return -2;
    }
    else
    {
        return 1;
    } 
}

int my_rename(char local_path[], char oldName[], char newName[])
{
    char to_rename[1001];
    strcpy(to_rename, local_path);
    strcat(to_rename, "/");
    strcat(to_rename, oldName);

    if(is_directory(to_rename) == 1)
    {
        return -1;
    }

    char newPath[1001];
    strcpy(newPath, local_path);
    strcat(newPath, "/");
    strcat(newPath, newName);

    if(rename(to_rename, newPath) == 0)
    {
        return 1;
    }

    return 0;
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

void path_absolut(char local_path[], char path[])
{
    char aux[1001];
    strcpy(aux,local_path);
    strcat(aux,path);
    strcpy(path,aux);
}

int my_mv(char local_path[], char to_move[], char destination[])
{
    //verific daca am cale relativa
    printf("Suntem in my_mv\n");
    char path1[1001]; //path-ul care ne ajuta pt cale relativa
    memset(path1, 0, 1001);
    getcwd(path1,sizeof(path1));
    //printf(" path1: %s\n", path1); 

    if(to_move[0] == '.')
    {
        path_relativ(local_path,to_move);
    }
    else if(to_move[0] == '/')
    {
        path_absolut(path1,to_move);
    }
    else
    {
        //printf("path-ul pt to_move nu este valid\n");
        return -1;
    }

    if(destination[0] == '.')
    {
        path_relativ(local_path,destination);
    }
    else if(destination[0] == '/')
    {
        path_absolut(path1,destination);
    }
    else
    {
        //printf("path-ul pt destinatie nu este valid\n");
        return -1;
    }
    
    //printf("INCEP VERIFICARILE\n");
    if(is_directory(destination) == 0)
    {
        //printf("%s nu este director\n", destination);
        return -2;
    }
    
    if(is_directory(to_move) == 1)
    {
        //printf("%s nu este fisier, ci director\n", to_move);
        return -2;
    }

    if(file_exists(to_move) == 0)
    {
        //printf("%s nu exista\n", to_move);
        return -3;
    }

    //printf("Am terminat verificarile\n");
    //printf("Incep mutarea\n");

    char destination_file[1001];
    strcpy(destination_file,destination);
    strcat(destination_file,"/");
    int index = -1;
    for(int i=strlen(to_move)-1; i>=0 && index==-1; i--)
    {
        if(to_move[i] == '/')
        {
            index=i;
        }
    }
    char name[1001];
    strcpy(name,to_move+index+1);
    strcat(destination_file,name);

    FILE *f1 = fopen(to_move,"rb");
    FILE *f2 = fopen(destination_file,"wb");
    char ch[1];
    memset(ch,0,1); 
    while (!feof(f1))
    {
        fread(ch, 1, 1, f1);
 
        fwrite(ch,1,1,f2);
    }

    fclose(f1);
    fclose(f2);
    unlink(to_move);
    return 1;
}

void my_download(char local_path[], char to_download[], char destination[], int sd)
{
    char path1[1001]; 
    memset(path1, 0, 1001);
    getcwd(path1,sizeof(path1));

    int status;
    if (read(sd, &status, sizeof(int)) == -1)
    {
        perror("[SERVER] eroare la read(status_1) de la client.\n");
        return;
    }
    
    if(status == 0)
    {
        printf("EROARE: destinatia %s nu este director\n", destination);
        return;
    }
    
    if(to_download[0] == '.')
    {
        path_relativ(local_path,to_download);
    }
    else if(to_download[0] == '/')
    {
        path_absolut(path1,to_download);
    }

    
    //verific daca to_download e director
    status = is_directory(to_download);
    if (write(sd, &status, sizeof(int)) == -1)
    {
        perror("[SERVER] eroare la write(status_2) de la client.\n");
        return;
    }
    if(status == 1)
    {
        printf("EROARE: to_download: %s nu este fisier, ci director\n", to_download);
        return;
    }

    //verific daca to_download exista
    status = file_exists(to_download);
    if (write(sd, &status, sizeof(int)) == -1)
    {
        perror("[SERVER] eroare la write(status_3) de la client.\n");
        return;
    }
    if(status == 0)
    {
        printf("EROARE: ce vrea sa descarce clientul: %s nu exista\n", to_download);
        return;
    }

    //iau doar numele efectiv al fisierului si il trimit
    int index = -1;
    for(int i=strlen(to_download)-1; i>=0 && index==-1; i--)
    {
        if(to_download[i] == '/')
        {
            index=i;
        }
    }
    char name[1001];
    strcpy(name,to_download+index+1);
    int size=strlen(name);
    if (write(sd, &size, sizeof(int)) == -1)
    {
        perror("[SERVER] eroare la write(size_name) catre client.\n");
       return;
    }
    if (write(sd, name, size) == -1)
    {
        perror("[SERVER] eroare la write(name) spre client.\n");
        return;
    }

    //citesc status ul daca exista sau nu deja un fisier cu numele la fel in destinatie
    if (read(sd, &status, sizeof(int)) == -1)
    {
        perror("[SERVER] eroare la write(status_4) de la client.\n");
        return;
    }

    if(status == 0)
    {
        //pot sa incep transferul
        if (read(sd, &status, sizeof(int)) == -1)
        {
            perror("[SERVER] eroare la write(status_5) de la client.\n");
            return;
        }
        if(status == 1)
        {
            //ACUM INCEP TRANSFERUL
            char ch[1];
            int len;
            FILE *f1 = fopen(to_download, "rb");
            if(f1 != NULL)
            {
                while (1)
                {
                    memset(ch,0,1);
                    len = fread(ch, 1, 1, f1);

                    if(len > 0)
                    { 
                        write(sd, &len, sizeof(int));
                        write(sd, ch, len);
                        continue;
                    }
                    else if (len == 0)
                    {
                        //am ajuns la finalul fisierului
                        if(feof(f1))
                            len = -1;
                        write(sd, &len, sizeof(int));
                        break;
                    }
                    
                }

            }
            fclose(f1);
            return;
        }
        else
        {
            printf("in client nu s a putut crea fisierul\n");
            return;
        } 
    }
    else
    {
        printf("in destinatie exista deja un fis cu numele: %s\n", name);
        return;
    }

}

void my_upload(char local_path[], char file[], char destination[], int sd)
{

    int status;
    if (read(sd, &status, sizeof(int)) == -1)
    {
        perror("[SERVER] eroare la read(status_1) de la client.\n");
        return;
    }

    if(status == 1)
    {
        //fisierul sursa exista
        //ma ocup de destinatie: path rel/abs
        char path1[1001];
        memset(path1, 0, 1001);
        getcwd(path1,sizeof(path1));
        if(destination[0] == '.')
        {
            path_relativ(local_path,destination);
        }
        else if(destination[0] == '/')
        {
            path_absolut(path1,destination);
        }
        //exista destinatia? 1/0
        status=is_directory(destination);
        if (write(sd, &status, sizeof(int)) == -1)
        {
            perror("[SERVER] eroare la write(status_1) la client.\n");
            return;
        }

        if(status == 1)
        {
            //destinatia exista
            //nu mai testez daca exista in director un fisier cu acelasi nume
            //de data asta o sa faca overwrite
            
            //prelucrez denumirea
            char destination_file[1001];
            strcpy(destination_file,destination);
            strcat(destination_file,"/");
            //iau doar numele efectiv al fisierului si il trimit
            int index = -1;
            for(int i=strlen(file)-1; i>=0 && index==-1; i--)
            {
                if(file[i] == '/')
                {
                    index=i;
                }
            }
            char name[1001];
            strcpy(name,file+index+1);
            strcat(destination_file,name);

            //incep tranfserul
            FILE *f1 = fopen(destination_file,"w");
            if(f1 == NULL)
            {
                printf("EROARE la crearea fisierului\n");
                status = 0;
            }
            else
                status = 1;
            
            //scriu clientului status-ul
            if (write(sd, &status, sizeof(int)) == -1)
            {
                perror("[SERVER] eroare la write(status_3) la client.\n");
                return;
            }

            if(status == 1)
            {
                //ACUM INCEP
                char ch[1];
                int len;
                while (1)
                {
                    memset(ch,0,1);
                    read(sd, &len, sizeof(int));

                    if(len == -1)
                    {
                        printf("DONE. %s a fost incarcat\n", name);
                        break;
                    }
                    if(len > 0)
                    {
                        read(sd, ch, len);
                        fwrite(ch, 1, len, f1);
                    }                
                }
                fclose(f1);
                return;
            }
            else
            {
                printf("Eroare.Nu s a putut incarca fisierul\n");
                return;
            }
        }
        else
        {
            //destinatia nu exista
            printf("destinatia nu exista\n");
            return;
        }
    }
    else
    {
        //fisierul sursa nu exista
        printf("Nu exista fisierul sursa\n");
        return;
    }

}


int main()
{
    printf("******SERVER_ul_myFileTransferProtocol******\n");
    printf("******Introdu help ca sa aflii metodologia******\n\n");

    struct sockaddr_in server; // structura folosita de server
    struct sockaddr_in from;   // informatii de la clienti

    int serverSkt; // decriptorul de socket

    // creare socket
    if ((serverSkt = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[SERVER] eroare la socket().\n");
        return errno;
    }

    // pregatim structurile de date
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    // umplem structura folosita de server
    server.sin_family = AF_INET;                // familia de socket-uri
    server.sin_addr.s_addr = htonl(INADDR_ANY); // atasez socket-ul la adresa IP locala
    server.sin_port = htons(2035);              // vom astepta la portul 2035

    // vreau sa pot reutiliza portul
    int option_value = 1;
    if (setsockopt(serverSkt, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) < 0)
    {
        perror("[SERVER] eroare la setsockopt().\n");
        return errno;
    }

    // atasam socket-ul
    if (bind(serverSkt, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[SERVER] eroare la bind().\n");
        return errno;
    }

    // punem server-ul sa asculte daca vin clienti sa se conecteze
    if (listen(serverSkt, 5) == -1)
    {
        perror("[SERVER] eroare la listen().\n");
        return errno;
    }

    // incepem sa acceptam clienti in mod concurent:
    while (1)
    {
        printf("[SERVER] Asteptam la portul %d ... \n", 2035);
        fflush(stdout);

        int clientSkt; // decriptorul de socket al clientului
        int length = sizeof(from);

        clientSkt = accept(serverSkt, (struct sockaddr *)&from, &length); // acceptam clienti
        if (clientSkt < 0)
        {
            perror("[SERVER] eroare la accept().\n");
            continue;
        }

        // pentru ca server-ul sa fie concurent, trebuie sa facem fork()
        int pid;
        pid = fork();
        if (pid == -1)
        {
            perror("[SERVER] eroare la fork().\n");
            return errno;
        }

        clt_nr++; // contorizam clientii

        if (pid == 0)
        {
            // copil
            printf("A venit clientul cu numarul %d\n", clt_nr);

            char command[1001];
            char ans[1001];
            char *white_list = "Daca aveti cont si sunteti white_listed puteti face: exit, login, logout, list, my_mkdir, my_rm, my_chdir, my_delete, my_rename, my_mv, my_download. Daca aveti cont si sunteti black_listed veti primi un mesaj";
            int size;
            char username[1001];
            char password[2001];
            int isLogged = 0;
            int isBlack = 0;
            char path[1001]; //path-ul initial
            memset(path, 0, 1001);
            getcwd(path,sizeof(path)); 
            strcat(path,"/server_dir"); //doar in acest director clientii vor putea crea/sterge alte directoare/fisiere, get/rename/delete/mv un fisier si chdir doar in acest director
            char local_path[2001];     //directorul curent unde opereaza clientul
            memset(local_path,0,2001);
            strcpy(local_path,path); //la inceput directorul curent este radacina=/server_dir
            char path_for_client[2001]; //ceea ce trimit catre client ca director actual
            memset(path_for_client,0,2001);
            strcpy(path_for_client,"/server_dir"); //directorul initial


            while (1) // cat timp clientul trimite comenzi
            {
                memset(command, 0, 1001);
                memset(ans, 0, 1001);
                //printf("CITESC COMANDA\n");

                // citesc de la client, pe rand: lungimea comenzii, comanda in sine
                int bytesRead = 0;
                if ((bytesRead = read(clientSkt, &size, sizeof(int))) == -1)
                {
                    perror("[SERVER] eroare la read(size_msg) de la client.\n");
                    continue;
                }
                if(bytesRead == 0)
                {
                    close(clientSkt);
                    printf("Clientul %d a iesit\n", clt_nr);
                    exit(1); // daca un client nu introduce nicio comanda, ii fac drop
                }
                if (read(clientSkt, command, size) == -1)
                {
                    perror("[SERVER] eroare la read(msg) de la client.\n");
                    continue;
                }
                
                //printf("AM CITIT COMANDA\n");

                command[size]='\0';
                //printf("comanda : %s\n",command);
                char comArgs[10][1001];
                int lenArgs=0;
                memset(comArgs,0,sizeof(comArgs));
                getCommandArgs(comArgs, command, &lenArgs);

                int commandType = getCommandType(comArgs, lenArgs);

                // verific ce comanda am primit
                if(commandType == -1)
                {
                    memset(ans, 0, 101);
                    strcpy(ans, "Comanda necunoscuta");
                    size = strlen(ans);
                    if (write(clientSkt, &size, sizeof(int)) == -1)
                    {
                        perror("[SERVER] eroare la write(size_ans) catre client.\n");
                        continue;
                    }
                    if (write(clientSkt, ans, size) == -1)
                    {
                        perror("[SERVER] eroare la write(ans) spre client.\n");
                        return errno;
                    }
                    continue;
                }
                else if(commandType == -2)
                {
                    memset(ans, 0, 101);
                    strcpy(ans, "Comanda nu are destule argumente");
                    size = strlen(ans);
                    if (write(clientSkt, &size, sizeof(int)) == -1)
                    {
                        perror("[SERVER] eroare la write(size_ans) catre client.\n");
                        continue;
                    }
                    if (write(clientSkt, ans, size) == -1)
                    {
                        perror("[SERVER] eroare la write(ans) spre client.\n");
                        return errno;
                    }
                    continue;
                }
                else if (commandType == 1) //help
                {
                    memset(ans, 0, 101);
                    strcpy(ans, white_list);
                    size = strlen(ans);
                    if (write(clientSkt, &size, sizeof(int)) == -1)
                    {
                        perror("[SERVER] eroare la write(size_ans) catre client.\n");
                        continue;
                    }
                    if (write(clientSkt, ans, size) == -1)
                    {
                        perror("[SERVER] eroare la write(ans) spre client.\n");
                        return errno;
                    }
                    continue;
                }
                else if (commandType == 2) //exit
                {
                    printf("A iesit clientul %d\n", clt_nr);
                    close(clientSkt);
                    exit(1);
                }
                else if(commandType == 3) //login
                {

                    if(isLogged == 0)
                    {
                        memset(username, 0, 1001);
                        memset(password, 0, 2001);
                        
                        strcpy(username,comArgs[1]);
                        strcpy(password,comArgs[2]);

                        //decriptare username, parola
                        char key_aux[1001];
                        memset(key_aux,0,1001);
                        strcpy(key_aux,key);
                        generate_key(username,key_aux);
                        char user_decryp[1001];
                        memset(user_decryp,0,1001);
                        strcpy(user_decryp,comArgs[1]);
                        decrypt(user_decryp,key_aux);
                        memset(username,0,1001);
                        strcpy(username,user_decryp);

                        char pass_decryp[1001];
                        memset(pass_decryp,0,1001);
                        memset(key_aux,0,1001);
                        strcpy(key_aux,key);
                        generate_key(password, key_aux);
                        strcpy(pass_decryp,password);
                        decrypt(pass_decryp,key_aux);
                        memset(password,0,1001);
                        strcpy(password,pass_decryp);

                        memset(ans,0,101);
                        if (look_for(username, password) == 1)
                        {
                            strcat(ans, "Login cu succes");
                            isLogged = 1;

                            if (look_for_white(username) == 0)
                            {
                                isBlack = 1;
                                //printf("Clientul %d este blackListed\n", clt_nr);
                                strcat(ans," Sunteti blacklisted. Puteti introduce doar: help, login, logout, exit");
                            }else
                            {
                                isBlack = 0;
                                //printf("Clientul %d este whiteListed\n", clt_nr);
                                strcat(ans," Sunteti whitelisted.");
                            }

                            memset(local_path,0,2001);
                            strcpy(local_path,path); 
                            memset(path_for_client,0,2001);
                            strcpy(path_for_client,"/server_dir");
                            
                        }else
                        {
                            strcat(ans,"Login fara succes. Nu aveti cont");
                            isLogged=0;
                            isBlack=0;
                        }

                        size=strlen(ans);
                        if (write(clientSkt, &size, sizeof(int)) == -1)
                        {
                            perror("[SERVER] eroare la write(size_ans) catre client.\n");
                            continue;
                        }
                        if (write(clientSkt, ans, size) == -1)
                        {
                            perror("[SERVER] eroare la write(ans) spre client.\n");
                            return errno;
                        }
                        continue;
                    }else
                    {
                        memset(ans,0,1001);
                        strcat(ans,"Sunteti deja logat cu ");
                        strcat(ans, username);
                        size=strlen(ans);
                        if (write(clientSkt, &size, sizeof(int)) == -1)
                        {
                            perror("[SERVER] eroare la write(size_ans) catre client.\n");
                            continue;
                        }
                        if (write(clientSkt, ans, size) == -1)
                        {
                            perror("[SERVER] eroare la write(ans) spre client.\n");
                            return errno;
                        }
                        continue;

                    }
                    
                }
                else if (commandType == 4) //logout
                {
                    if (isLogged == 1)
                    {
                        memset(ans, 0, 1001);
                        strcpy(ans, "Logout cu succes");
                        size = strlen(ans);
                        if (write(clientSkt, &size, sizeof(int)) == -1)
                        {
                            perror("[SERVER] eroare la write(size_ans) catre client.\n");
                            continue;
                        }
                        if (write(clientSkt, ans, size) == -1)
                        {
                            perror("[SERVER] eroare la write(ans) spre client.\n");
                            return errno;
                        }
                        isLogged = 0;
                        isBlack = 0;
                        memset(username,0,1001);
                        memset(password,0,2001);
                        
                        memset(local_path,0,2001);
                        strcpy(local_path,path); 
                        memset(path_for_client,0,2001);
                        strcpy(path_for_client,"/server_dir"); 
                    
                        continue;
                    }
                    else
                    {
                        is_logout(clientSkt);
                        isLogged=0;
                        continue;
                    }
                }
                else if(commandType == 5) //location
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            memset(ans,0,1001);
                            strcpy(ans,path_for_client);
                            size = strlen(ans);
                            if (write(clientSkt, &size, sizeof(int)) == -1)
                            {
                                perror("[SERVER] eroare la write(size_ans) catre client.\n");
                                continue;
                            }
                            if (write(clientSkt, ans, size) == -1)
                            {
                                perror("[SERVER] eroare la write(ans) spre client.\n");
                                return errno;
                            }
                            continue;

                        }else
                        {
                            is_black(clientSkt);
                            continue;
                        }
                    }else
                    {
                        is_logout(clientSkt);
                        continue;
                    }
                }
                else if(commandType == 6) //list
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            list(local_path,clientSkt);
                            continue;

                        }else
                        {
                            is_black(clientSkt);
                            continue;
                        }
                    }else
                    {
                        is_logout(clientSkt);
                        continue;
                    }
                }
                else if(commandType == 7) //my_chdir
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            //com[1]: fie .. pentru nivel superior, fie nume de director din directorul curent
                            memset(ans,0,1001);
                            strcpy(ans,my_chdir(path,local_path,comArgs[1])); 
                            if(strstr(ans,"FAIL") == NULL)
                            {   
                                strcpy(local_path,ans);
                                char *p = strstr(ans,"/server_dir");
                                memset(path_for_client,0,2001);
                                strcpy(path_for_client,p);

                                strcpy(ans, "Succes. Introduceti location pentru a afla noua locatie, list pentru a afla ce aveti la dispozitie in ea.");
                            }
                            
                            size = strlen(ans);
                            if (write(clientSkt, &size, sizeof(int)) == -1)
                            {
                                perror("[SERVER] eroare la write(size_ans) catre client.\n");
                                continue;
                            }
                            if (write(clientSkt, ans, size) == -1)
                            {
                                perror("[SERVER] eroare la write(ans) spre client.\n");
                                return errno;
                            }
                            continue;
                        }else
                        {
                            is_black(clientSkt);
                            continue;
                        }
                    }else
                    {
                        is_logout(clientSkt);
                        continue;
                    }

                }
                else if(commandType == 8) //my_mkdir
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            if(my_mkdir(local_path, comArgs[1]) == 1)
                            {
                                strcpy(ans, "Directorul a fost creat. Introduceti list pentru a verifica.");
                            }
                            else
                            {
                                strcpy(ans, "Eroare. Directorul nu a fost creat.");
                            }
                            size = strlen(ans);
                            if (write(clientSkt, &size, sizeof(int)) == -1)
                            {
                                perror("[SERVER] eroare la write(size_ans) catre client.\n");
                                continue;
                            }
                            if (write(clientSkt, ans, size) == -1)
                            {
                                perror("[SERVER] eroare la write(ans) spre client.\n");
                                return errno;
                            }
                            continue;

                        }
                        else
                        {
                            is_black(clientSkt);
                            continue;
                        }

                    }
                    else
                    {
                        is_logout(clientSkt);
                        continue;
                    }
                }
                else if(commandType == 9) //my_rm
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            //echivalent rm -r 
                            char to_delete[1001];
                            strcpy(to_delete,local_path);
                            strcat(to_delete,"/");
                            strcat(to_delete,comArgs[1]);
                            
                            if(is_directory(to_delete) == 1)
                            {
                                if(my_rm(to_delete) == 0)
                                {
                                    strcpy(ans,"Director sters. Introdu list pentru a verifica");
                                }
                                else
                                {
                                    strcpy(ans,"Stergere esuata. Introdu list pentru a verifica");
                                }
                            }
                            else
                            {
                                strcpy(ans,"Nume de director invalid");
                            }
                            size = strlen(ans);
                            if (write(clientSkt, &size, sizeof(int)) == -1)
                            {
                                perror("[SERVER] eroare la write(size_ans) catre client.\n");
                                continue;
                            }
                            if (write(clientSkt, ans, size) == -1)
                            {
                                perror("[SERVER] eroare la write(ans) spre client.\n");
                                return errno;
                            }
                            continue;
                            
                        }
                        else
                        {
                            is_black(clientSkt);
                            continue;
                        }
                    }
                    else
                    {
                        is_logout(clientSkt);
                        continue;
                    }
                }
                else if(commandType == 10)  //incep lucrul cu fisiere, my_delete
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
    
                            int status = my_delete(local_path, comArgs[1]);
                            if(status == -1)
                            {
                                strcpy(ans, "Stergere respinsa. Numele introdus este un director.");
                            }
                            else if(status == -2)
                            {
                                strcpy(ans, "Eroare. Stergerea nu a putut fi efectuata");
                            }
                            else if(status == 1)
                            {
                                strcpy(ans,"Stergere cu succes. Introduceti list pentru a verifica.");
                            }
                            
                            size = strlen(ans);
                            if (write(clientSkt, &size, sizeof(int)) == -1)
                            {
                                perror("[SERVER] eroare la write(size_ans) catre client.\n");
                                continue;
                            }
                            if (write(clientSkt, ans, size) == -1)
                            {
                                perror("[SERVER] eroare la write(ans) spre client.\n");
                                return errno;
                            }
                            continue;

                        }
                        else
                        {
                            is_black(clientSkt);
                            continue;
                        }

                    }
                    else
                    {
                        is_logout(clientSkt);
                        continue;
                    }
                }
                else if(commandType == 11) //my_rename
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            int status = my_rename(local_path, comArgs[1], comArgs[2]);
                            if(status == -1)
                            {
                                strcpy(ans, "Redenumire respinsa. Numele introdus este un director.");
                            }
                            else if(status == 0)
                            {
                                strcpy(ans, "Eroare. Redenumirea nu a putut fi efectuata");
                            }
                            else if(status == 1)
                            {
                                strcpy(ans,"Redenumire cu succes. Introduceti list pentru a verifica.");
                            }
                            
                            size = strlen(ans);
                            if (write(clientSkt, &size, sizeof(int)) == -1)
                            {
                                perror("[SERVER] eroare la write(size_ans) catre client.\n");
                                continue;
                            }
                            if (write(clientSkt, ans, size) == -1)
                            {
                                perror("[SERVER] eroare la write(ans) spre client.\n");
                                return errno;
                            }
                            continue;
                        }
                        else
                        {
                            is_black(clientSkt);
                            continue;
                        }
                    }
                    else
                    {
                        is_logout(clientSkt);
                        continue;
                    }
                }
                else if(commandType == 12) //my_mv 
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            int status = my_mv(local_path, comArgs[1], comArgs[2]);
                            if(status == -1)
                            {
                                strcpy(ans,"EROARE.Numele sursei sau al destinatiei este invalid");
                            }
                            else if (status == -2)
                            {
                                strcpy(ans,"EROARE. Fie sursa nu este fisier fie destinatia nu este director.");
                            }
                            else if (status == -3)
                            {
                                strcpy(ans,"EROARE. Fisierul ce se vrea a fi mutat nu exista");
                            }
                            else if(status == 1)
                            {
                                strcpy(ans,"SUCCES.Navigati intre directoare si verificati");
                            }

                            size = strlen(ans);
                            if (write(clientSkt, &size, sizeof(int)) == -1)
                            {
                                perror("[SERVER] eroare la write(size_ans) catre client.\n");
                                continue;
                            }
                            if (write(clientSkt, ans, size) == -1)
                            {
                                perror("[SERVER] eroare la write(ans) spre client.\n");
                                return errno;
                            }
                            continue;
                        }
                        else
                        {
                            is_black(clientSkt);
                            continue;
                        }
                    }
                    else
                    {
                        is_logout(clientSkt);
                        continue;
                    }
                }
                else if(commandType == 13) //my_download
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            my_download(local_path,comArgs[1],comArgs[2], clientSkt);
                            continue;
                        }
                        else
                        {
                            is_black(clientSkt);
                            continue;
                        }

                    }
                    else
                    {
                        is_logout(clientSkt);
                        continue;
                    }
                }
                else if(commandType == 14) //my_upload
                {
                    if(isLogged == 1)
                    {
                        if(isBlack == 0)
                        {
                            //comArgs[1]=numele fis pe care il incarc
                            //comArgs[2]=numele directorului destinatie
                            my_upload(local_path,comArgs[1],comArgs[2], clientSkt);
                            continue;

                        }
                        else
                        {
                            is_black(clientSkt);
                            continue;
                        }

                    }
                    else
                    {
                        is_logout(clientSkt);
                        continue;
                    }

                }
                              
            }
            
            exit(2); //termin copilul
        }
        else
        {
            // parinte
            ;
        }
    }

    return 0;
}