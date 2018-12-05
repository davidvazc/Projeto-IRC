/*******************************************************************
 * SERVIDOR no porto 9000, à escuta de novos clientes.  Quando surjem
 * novos clientes os dados por eles enviados são lidos e descarregados no ecran.
 *******************************************************************/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>


#define BUF_SIZE   1024
#define SERVER_PORT     9002
#define SERVER_PORT2 9000
#define N_USERS   2
#define NMEDIA  6



void process_client(int client_fd);
void erro(char *msg);
void media_grupo(int client_fd);
void subscricoes(int client_fd,int i);
void* notificacoes(void* idp);

typedef struct{
    double calls_duracao;
    double calls_feitas;
    double calls_perdidas;
    double calls_recebidas;
    double sms_recebidas;
    double sms_enviadas;
}Grupo;



typedef struct{
    char id[BUF_SIZE];
    char atividade[BUF_SIZE];
    char localizacao[BUF_SIZE];
    int call_duracao;
    bool sub_call_duracao;
    int call_feitas;
    bool sub_call_feitas;
    int call_perdidas;
    bool sub_call_perdidas;
    int call_recebidas;
    bool sub_call_recebidas;
    char departamento[BUF_SIZE];
    int sms_recebidas;
    bool sub_sms_recebidas;
    int sms_enviadas;
    bool sub_sms_enviadas;
}Client;

Client total_pessoas[BUF_SIZE];
int nr_clientes;
int done=0;

Grupo grupo;
Grupo ogrupo;

void faz_client(){
    strcpy(total_pessoas[0].atividade, "Estudando");
    strcpy(total_pessoas[0].localizacao, "Portugal");
    strcpy(total_pessoas[0].departamento, "DEM");
    strcpy(total_pessoas[0].id , "teste1");
    total_pessoas[0].call_feitas = 7;
    total_pessoas[0].sub_call_feitas=false;
    total_pessoas[0].call_duracao = 2;
    total_pessoas[0].sub_call_duracao=false;
    total_pessoas[0].call_perdidas = 1;
    total_pessoas[0].sub_call_perdidas=false;
    total_pessoas[0].call_recebidas = 5;
    total_pessoas[0].sub_call_recebidas=false;
    total_pessoas[0].sms_recebidas = 6;
    total_pessoas[0].sub_sms_recebidas=false;
    total_pessoas[0].sms_enviadas = 6;
    total_pessoas[0].sub_sms_enviadas=false;
    nr_clientes++;
    
    strcpy(total_pessoas[1].atividade, "Bebendo");
    strcpy(total_pessoas[1].localizacao, "Brazil");
    strcpy(total_pessoas[1].departamento, "DEI");
    strcpy(total_pessoas[1].id , "teste2");
    total_pessoas[1].call_feitas = 5;
    total_pessoas[1].sub_call_feitas=false;
    total_pessoas[1].call_duracao = 3;
    total_pessoas[1].sub_call_duracao=false;
    total_pessoas[1].call_perdidas = 3;
    total_pessoas[1].sub_call_perdidas=false;
    total_pessoas[1].call_recebidas = 10;
    total_pessoas[1].sub_call_recebidas=false;
    total_pessoas[1].sms_recebidas = 4;
    total_pessoas[1].sub_sms_recebidas=false;
    total_pessoas[1].sms_enviadas =5;
    total_pessoas[1].sub_sms_enviadas=false;
    nr_clientes++;
}

void faz_ogrupo(){ //cria copia antiga do grupo que e comparada com a recente para enviar notificação caso algum valor modifique
    for(int i=0;i<N_USERS;i++){
        ogrupo.calls_duracao+=total_pessoas[i].call_feitas;
        ogrupo.calls_feitas+=total_pessoas[i].call_duracao;
        ogrupo.calls_perdidas+=total_pessoas[i].call_perdidas;
        ogrupo.calls_recebidas+=total_pessoas[i].call_recebidas;
        ogrupo.sms_enviadas+=total_pessoas[i].sms_recebidas;
        ogrupo.sms_recebidas+=total_pessoas[i].sms_enviadas;
    }
    //Media
    ogrupo.calls_duracao=ogrupo.calls_duracao/N_USERS;
    ogrupo.calls_feitas=ogrupo.calls_feitas/N_USERS;
    ogrupo.calls_perdidas=ogrupo.calls_perdidas/N_USERS;
    ogrupo.calls_recebidas=ogrupo.calls_recebidas/N_USERS;
    ogrupo.sms_enviadas=ogrupo.sms_enviadas/N_USERS;
    ogrupo.sms_recebidas=ogrupo.sms_recebidas/N_USERS;
}

void calcula_media(){ //calcula a media para a informação do grupo
    int i;
    //somar
    for (i=0; i<N_USERS; i++) {
        grupo.calls_duracao=0;
        grupo.calls_feitas=0;
        grupo.calls_perdidas=0;
        grupo.calls_recebidas=0;
        grupo.sms_enviadas=0;
        grupo.sms_recebidas=0;
    }
    for(i=0;i<N_USERS;i++){
        grupo.calls_duracao+=total_pessoas[i].call_feitas;
        grupo.calls_feitas+=total_pessoas[i].call_duracao;
        grupo.calls_perdidas+=total_pessoas[i].call_perdidas;
        grupo.calls_recebidas+=total_pessoas[i].call_recebidas;
        grupo.sms_enviadas+=total_pessoas[i].sms_recebidas;
        grupo.sms_recebidas+=total_pessoas[i].sms_enviadas;
    }
    //Media
    grupo.calls_duracao=grupo.calls_duracao/N_USERS;
    grupo.calls_feitas=grupo.calls_feitas/N_USERS;
    grupo.calls_perdidas=grupo.calls_perdidas/N_USERS;
    grupo.calls_recebidas=grupo.calls_recebidas/N_USERS;
    grupo.sms_enviadas=grupo.sms_enviadas/N_USERS;
    grupo.sms_recebidas=grupo.sms_recebidas/N_USERS;
}

int main() {
    
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;
    
    //    cria estrutura
    bzero((void *) &addr, sizeof(addr));
    
    // Configure settings of the server address struct
    // Address family = Internet
    addr.sin_family = AF_INET;
    //Set IP address to localhost
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //Set port number, using htons function to use proper byte order
    addr.sin_port = htons(SERVER_PORT);
    
    //Create the socket.
    if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    //Bind the address struct to the socket
    if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
        erro("na funcao bind");
    //Listen on the socket, with 5 max connection requests queued
    if( listen(fd, 5) < 0)
        erro("na funcao listen");
    
    faz_client(); //Cria os clientes para teste
    
    while (1) {
        
        while(waitpid(-1,NULL,WNOHANG)>0);
        
        client_addr_size = sizeof(client_addr);
        client = accept(fd,(struct sockaddr *)&client_addr,(socklen_t *)&client_addr_size); //aceita a conexão do cliente
        
        
        if (client > 0) { //caso haja conexão
            if (fork() == 0) {
                close(fd);
                process_client(client);
                exit(0);
            }
            close(client);
        }
    }
    return 0;
}

void process_client(int client_fd){
    int cliente=-1;
    long nread = 0;
    int i;
    char buffer[BUF_SIZE],aux[BUF_SIZE*2];
    write(client_fd,"\n++++++++++++++++++++++++++++++++++++++++++\n+                                        +\n+   SEJA BEM VINDO AO SERVIDOR ISABELA   +\n+                                        +\n++++++++++++++++++++++++++++++++++++++++++\n\nIntoruza o seu id:",BUF_SIZE); //escreve mensagem de boas vindas e pede id
    
    nread = read(client_fd, buffer, BUF_SIZE);//le id
    buffer[nread] = '\0';
    printf("%s\n", buffer);
    
    for(i=0;i<nr_clientes;i++){
        if(strcmp(buffer,total_pessoas[i].id)==0){ //verifica se nome está presente
            cliente=i;
        }
    }
    if(cliente!=-1){
        write(client_fd,"Cliente existe",BUF_SIZE);//escreve resposta sobre id
    }
    else{
        write(client_fd,"\nO id introduzido não existe!\nIntroduza novamente:",BUF_SIZE);
        while(cliente==-1){ //enquanto o id nao existir na base de dados
            nread = read(client_fd, buffer, BUF_SIZE); //le id
            buffer[nread] = '\0';
            printf("%s\n", buffer);
            for(i=0;i<nr_clientes;i++){
                if(strcmp(buffer,total_pessoas[i].id)==0){ //se o id esta presente
                    cliente=i; //posição i do cliente na base de dados
                    write(client_fd,"Cliente existe",BUF_SIZE);
                }
            }
            if(cliente==-1)
                write(client_fd,"\nO id introduzido não existe!\nIntroduza novamente:",BUF_SIZE);
        }
    }
    pthread_t my_thread;
    int id=cliente;
    calcula_media();//Cria a informação do grupo
    faz_ogrupo(); //Cria a cópia que irá ser comparada no thread
    pthread_create(&my_thread,NULL,notificacoes,&id); //cria thread para enviar as notificações
    while(!(strcmp(buffer,"4")==0)){ //enquanto nao pedir para sair do menu
        write(client_fd,"\n+++++++++++++++++++++++++++++++++\n+        Escolha a opção        +\n+    1-Informações pessoais     +\n+    2-Media de grupo           +\n+    3-Subscrição               +\n+    4-Sair                     +\n+++++++++++++++++++++++++++++++++\n",BUF_SIZE); //envia o menu
        nread = read(client_fd, buffer, BUF_SIZE); //le o que o cliente pretende fazer
        buffer[nread] = '\0';
        printf("%s\n", buffer);
        if(strcmp(buffer, "1")==0){ // se quiser informações pessoais
            while(!(strcmp(buffer,"11")==0)){ //enquanto nao pedir para sair do sub menu das informações pessoais
                write(client_fd,"\n+++++++++++++++++++++++++++\n+  1- Actividade          +\n+  2- Localização         +\n+  3- Departamento        +\n+  4- Chamadas feitas     +\n+  5- Duração chamadas    +\n+  6- Chamadas perdidas   +\n+  7- Chamadas recebidas  +\n+  8- SMS recebidas       +\n+  9- SMS enviadas        +\n+  10- Mostrar todas      +\n+  11- Voltar             +\n+++++++++++++++++++++++++++\n",BUF_SIZE); //envia sub menu
                
                nread = read(client_fd, buffer, BUF_SIZE); //le o que o cliente pretende fazer no sub menu
                buffer[nread] = '\0';
                printf("%s\n", buffer);
                if(strcmp(buffer, "1")==0){ //Se quiser saber a sua atividade
                    sprintf(aux,"\n\n\n>>>   Actividade: %s   <<<\n",total_pessoas[cliente].atividade);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "2")==0){ //Se quiser saber a sua localização
                    sprintf(aux,"\n\n\n>>>   Localização: %s   <<<\n",total_pessoas[cliente].localizacao);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "3")==0){ //Se quiser saber o seu departamento
                    sprintf(aux,"\n\n\n>>>   Departamento: %s   <<<\n",total_pessoas[cliente].departamento);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "4")==0){ //Se quiser saber as suas chamadas feitas
                    sprintf(aux,"\n\n\n>>>   Chamadas feitas: %d   <<<\n",total_pessoas[cliente].call_feitas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "5")==0){ //Se quiser saber a duração das suas chamadas
                    sprintf(aux,"\n\n\n>>>   Duração chamadas: %d   <<<\n",total_pessoas[cliente].call_duracao);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "6")==0){ //Se quiser saber as suas chamadas perdidas
                    sprintf(aux,"\n\n\n>>>   Chamadas perdidas: %d   <<<\n",total_pessoas[cliente].call_perdidas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "7")==0){ //Se quiser saber as suas chamadas recebidas
                    sprintf(aux,"\n\n\n>>>   Chamadas recebidas: %d   <<<\n",total_pessoas[cliente].call_recebidas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "8")==0){ //Se quiser saber os seus SMS recebidos
                    sprintf(aux,"\n\n\n>>>   SMS recebidas: %d   <<<\n",total_pessoas[cliente].sms_recebidas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "9")==0){ //Se quiser saber os seus SMS enviados
                    sprintf(aux,"\n\n\n>>>   SMS enviadas: %d   <<<\n",total_pessoas[cliente].sms_enviadas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "10")==0){ //Se quiser saber as informações todas
                    sprintf(aux,"\n+++++++++++++++++++++++++++++++++++++++++\n+  Actividade: %s           \n+  Localização: %s          \n+  Departamento: %s         \n+  Chamadas feitas: %d      \n+  Duração de chamas: %d    \n+  Chamadas perdidas: %d    \n+  Chamadas recebidas: %d   \n+  SMS recebidas: %d        \n+  SMS envidas: %d          \n+++++++++++++++++++++++++++++++++++++++++",total_pessoas[cliente].atividade,total_pessoas[cliente].localizacao,total_pessoas[cliente].departamento,total_pessoas[cliente].call_feitas,total_pessoas[cliente].call_duracao,total_pessoas[cliente].call_perdidas,total_pessoas[cliente].call_recebidas,total_pessoas[cliente].sms_recebidas,total_pessoas[cliente].sms_enviadas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "11")==0){ //Se pedir para sair
                    write(client_fd,"\n", BUF_SIZE);
                } else{ //Se não escolher nenhuma das opções no sub menu
                    write(client_fd,"\n\n\n>>>ERRO: Escolha 1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ou 11!\n",BUF_SIZE);
                }
            }
        } else if(strcmp(buffer,"2")==0){ //Se quiser saber as informações de grupo
            while(!(strcmp(buffer,"8")==0)){ //Enquanto nao pedir para sair
                write(client_fd,"\n++++++++++++++++++++++++++++++++++++++++++++++++\n+  Selecioe a media que deseja ver:            +\n+  1- Chamadas recebidas                       +\n+  2- Chamdas feitas                           +\n+  3- Chamadas perdidas                        +\n+  4- Duração media de chamada                 +\n+  5- SMS enviados                             +\n+  6- SMS recebidos                            +\n+  7- Ver todas as opções                      +\n+  8- Voltar                                   +\n++++++++++++++++++++++++++++++++++++++++++++++++\n",BUF_SIZE);//envia o sub menu
                nread = read(client_fd, buffer, BUF_SIZE); //le o que o cliente pretende fazer
                buffer[nread] = '\0';
                printf("%s\n", buffer);
                if(strcmp(buffer, "1")==0){ //Se quiser a media das chamadas recebidas pelo grupo
                    sprintf(aux,"\n\n\n>>>   Media chamadas recebidas: %.2f   <<<\n",grupo.calls_recebidas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "2")==0){ //Se quiser a media das chamadas feitas pelo grupo
                    sprintf(aux,"\n\n\n>>>   Media chamadas feitas: %.2f   <<<\n",grupo.calls_feitas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "3")==0){//Se quiser a media das chamadas perdidas pelo grupo
                    sprintf(aux,"\n\n\n>>>   Media chamadas perdidas: %.2f   <<<\n",grupo.calls_perdidas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "4")==0){//Se quiser a duração media das chamadas do grupo
                    sprintf(aux,"\n\n\n>>>   Duração media de chamada: %.2f   <<<\n",grupo.calls_duracao);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "5")==0){ //Se quiser a media dos SMS enviados pelo grupo
                    sprintf(aux,"\n\n\n>>>   SMS enviados: %.2f   <<<\n",grupo.sms_enviadas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "6")==0){ //Se quiser a media dos SMS recebidos pelo grupo
                    sprintf(aux,"\n\n\n>>>   SMS recebidos: %.2f   <<<\n",grupo.sms_recebidas);
                    write(client_fd, aux, BUF_SIZE);
                } else if(strcmp(buffer, "7")==0){ ///Se quiser a informação toda
                    media_grupo(client_fd);
                } else if(strcmp(buffer, "8")==0){ //Se pedir para sair do sub menu
                    write(client_fd,"\n", BUF_SIZE);
                } else{//Se nao escolher nenhuma das opçoes do sub menu
                    write(client_fd,"\n\n\n>>>ERRO: Escolha 1 ,2 ,3 ,4 ,5 ,6 ,7 ou 8!\n",BUF_SIZE);
                }
            }
        } else if(strcmp(buffer,"3")==0){ //Se quiser subscrever a algum dado
            subscricoes(client_fd,cliente);
        } else if(strcmp(buffer,"4")==0){ //Se o cliente quiser sair
            printf("\n");
        } else{ //Se nao escolher nenhuma das opções que ta no menu
            write(client_fd,"\n\n>>ERRO: Escolha 1,2,3 ou 4!",BUF_SIZE);
        }
    }
    
    printf("Fechando conexão com o cliente...\n");
    done=1; //Termina o thread
    pthread_join(my_thread,NULL); //Espera que o thread termine
    exit(0); //Fecha o processo que trata do cliente
    
    
}

//Media do grupo
void media_grupo(int client_fd){
    
    //Passar para String
    char buff[BUF_SIZE];
    calcula_media();
    
    sprintf(buff, "\n++++++++++++++++++++++++++++++++++++\n+  Media chamadas recebidas: %.2f  +\n+  Media chamadas feitas: %.2f     +\n+  Media chamadas perdidas: %.2f   +\n+  Duração media de chamada: %.2f  +\n+  SMS enviadas: %.2f              +\n+  SMS recebidas: %.2f             +\n++++++++++++++++++++++++++++++++++++",grupo.calls_recebidas,grupo.calls_feitas,grupo.calls_perdidas,grupo.calls_duracao,grupo.sms_enviadas,grupo.sms_recebidas);
    //Envia para cliente
    write(client_fd,buff, BUF_SIZE);
    
}


void subscricoes(int client_fd,int i){
    char buffer[BUF_SIZE];
    long nread=0;
    do{
        write(client_fd,"++++++++++++++++++++++++++++++++++++++++++++++++\n+  Selecione a que deseja subscrever/cancelar: +\n+  1- Chamadas recebidas                       +\n+  2- Chamdas feitas                           +\n+  3- Chamadas perdidas                        +\n+  4- Duração media de chamada                 +\n+  5- SMS enviados                             +\n+  6- SMS recebidos                            +\n+  7- Subscrever todas as opções               +\n+  8- Voltar                                   +\n++++++++++++++++++++++++++++++++++++++++++++++++\n",BUF_SIZE); //envia o sub menu
        nread=read(client_fd,buffer,BUF_SIZE); //le o que o cliente pretende subscrever
        buffer[nread]='\0';
        printf("%s\n", buffer);
        
        if(strcmp(buffer,"1")==0){ //Se quiser subscrever//cancelar a subscrição às chamadas recebidas
            if(total_pessoas[i].sub_call_recebidas==false){
                total_pessoas[i].sub_call_recebidas=true;
                write(client_fd,"\n\n\n>>>   Está agora subscrito.   <<<\n",BUF_SIZE);
            } else{
                total_pessoas[i].sub_call_recebidas=false;
                write(client_fd,"\n\n\n>>>   Deixou de estar subscrito.   <<<\n",BUF_SIZE);
                
            }
        } else if(strcmp(buffer,"2")==0){//Se quiser subscrever/cancelar a subscrição às chamadas efetuadas
            if(total_pessoas[i].sub_call_feitas==false){
                total_pessoas[i].sub_call_feitas=true;
                write(client_fd,"\n\n\n>>>   Está agora subscrito.   <<<\n",BUF_SIZE);
            } else{
                total_pessoas[i].sub_call_feitas=false;
                write(client_fd,"\n\n\n>>>   Deixou de estar subscrito.   <<<\n",BUF_SIZE);
            }
        } else if(strcmp(buffer,"3")==0){//Se quiser subscrever/cancelar a subscrição às chamadas perdidas
            if(total_pessoas[i].sub_call_perdidas==false){
                total_pessoas[i].sub_call_perdidas=true;
                write(client_fd,"\n\n\n>>>   Está agora subscrito.   <<<\n",BUF_SIZE);
            } else{
                total_pessoas[i].sub_call_perdidas=false;
                write(client_fd,"\n\n\n>>>   Deixou de estar subscrito.   <<<\n",BUF_SIZE);
            }
        } else if(strcmp(buffer,"4")==0){//Se quiser subscrever/cancelar a subscrição à duração das chamadas
            if(total_pessoas[i].sub_call_duracao==false){
                total_pessoas[i].sub_call_duracao=true;
                write(client_fd,"\n\n\n>>>   Está agora subscrito.   <<<\n",BUF_SIZE);
            } else{
                total_pessoas[i].sub_call_duracao=false;
                write(client_fd,"\n\n\n>>>   Deixou de estar subscrito.   <<<\n",BUF_SIZE);
            }
        } else if(strcmp(buffer,"5")==0){ //Se quiser subscrever/cancelar a subscrição às SMS recebidas
            if(total_pessoas[i].sub_sms_enviadas==false){
                total_pessoas[i].sub_sms_enviadas=true;
                write(client_fd,"\n\n\n>>>   Está agora subscrito.   <<<\n",BUF_SIZE);
            } else{
                total_pessoas[i].sub_sms_enviadas=false;
                write(client_fd,"\n\n\n>>>   Deixou de estar subscrito.   <<<\n",BUF_SIZE);
            }
        } else if(strcmp(buffer,"6")==0){ //Se quiser subscrever/cancelar a subscrição às SMS efetuadas
            if(total_pessoas[i].sub_sms_recebidas==false){
                total_pessoas[i].sub_sms_recebidas=true;
                write(client_fd,"\n\n\n>>>   Está agora subscrito.   <<<\n",BUF_SIZE);
            } else{
                total_pessoas[i].sub_sms_recebidas=false;
                write(client_fd,"\n\n\n>>>   Deixou de estar subscrito.   <<<\n",BUF_SIZE);
            }
            
        }else if(strcmp(buffer,"7")==0){ //Se quiser subscrever a tudo
            if(total_pessoas[i].sub_sms_recebidas==false){
                total_pessoas[i].sub_sms_recebidas=true;
            }if(total_pessoas[i].sub_sms_enviadas==false){
                total_pessoas[i].sub_sms_enviadas=true;
            }if(total_pessoas[i].sub_call_duracao==false){
                total_pessoas[i].sub_call_duracao=true;
            }if(total_pessoas[i].sub_call_perdidas==false){
                total_pessoas[i].sub_call_perdidas=true;
            }if(total_pessoas[i].sub_call_feitas==false){
                total_pessoas[i].sub_call_feitas=true;
            }if(total_pessoas[i].sub_call_recebidas==false){
                total_pessoas[i].sub_call_recebidas=true;
            }
            write(client_fd, "Está agora subscrito a tudo!", BUF_SIZE);
        
         }else if(strcmp(buffer,"8")==0){ //Se quiser sair do sub menu
            write(client_fd,"\n", BUF_SIZE);
        } else{ //Caso nao escolha nenhuma opção que esta no sub menu
            write(client_fd,"\n\n\n>>>ERRO: Escolha 1,2,3,4,5,6,7 ou 8!\n",BUF_SIZE);
        }
    }while(strcmp(buffer,"8")!=0); //Enquanto nao pedir para sair do menu
}



void* notificacoes(void* idp){ //Thread que envia notificações
    int fd2;
    int noti_fd=0;
    struct sockaddr_in addr2, client_addr2;
    int client_addr_size;
    int i=*((int*) idp);
    
    //    cria estrutura
    bzero((void *) &addr2, sizeof(addr2));
    
    // Configure settings of the server address struct
    // Address family = Internet
    addr2.sin_family = AF_INET;
    //Set IP address to localhost
    addr2.sin_addr.s_addr = htonl(INADDR_ANY);
    //Set port number, using htons function to use proper byte order
    addr2.sin_port = htons(SERVER_PORT2);
    
    //Create the socket.
    if ( (fd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    //Bind the address struct to the socket
    if ( bind(fd2,(struct sockaddr*)&addr2,sizeof(addr2)) < 0)
        erro("na funcao bind");
    //Listen on the socket, with 5 max connection requests queued
    if( listen(fd2, 5) < 0)
        erro("na funcao listen");
    while(noti_fd<=0){
        client_addr_size = sizeof(client_addr2);
        noti_fd=accept(fd2,(struct sockaddr *)&client_addr2,(socklen_t *)&client_addr_size); //Aceita a conexão do thread cliente
    }
    close(fd2);
    write(noti_fd,"Connected\n",BUF_SIZE);
    
    while(done==0){ //enquanto o cliente nao pedir para sair do server
        calcula_media();//Atualiza a informação do grupo
        if(ogrupo.calls_recebidas!=grupo.calls_recebidas){ //Se houver alteração da informação
            ogrupo.calls_recebidas=grupo.calls_recebidas; //Atualiza os dados antigos
            if(total_pessoas[i].sub_call_recebidas==true){ //Se o cliente estiver subscrito a este tipo de dados
                write(noti_fd,"O valor das chamadas recebidas foi alterado!",BUF_SIZE); //Envia a notificação
            }
        } else if(ogrupo.calls_feitas!=grupo.calls_feitas){//Se houver alteração da informação
            ogrupo.calls_feitas=grupo.calls_feitas;//Atualiza os dados antigos
            if(total_pessoas[i].sub_call_feitas==true){//Se o cliente estiver subscrito a este tipo de dados
                write(noti_fd,"O valor das chamadas feitas foi alterado!",BUF_SIZE);//Envia a notificação
            }
        } else if(ogrupo.calls_duracao!=grupo.calls_duracao){//Se houver alteração da informação
            ogrupo.calls_duracao=grupo.calls_duracao;//Atualiza os dados antigos
            if(total_pessoas[i].sub_call_duracao==true){//Se o cliente estiver subscrito a este tipo de dados
                write(noti_fd,"O valor da duração das chamadas foi alterado!",BUF_SIZE);//Envia a notificação
            }
        } else if(ogrupo.calls_perdidas!=grupo.calls_perdidas){//Se houver alteração da informação
            ogrupo.calls_perdidas=grupo.calls_perdidas;//Atualiza os dados antigos
            if(total_pessoas[i].sub_call_perdidas==true){//Se o cliente estiver subscrito a este tipo de dados
                write(noti_fd,"O valor das chamadas perdidas foi alterado!",BUF_SIZE);//Envia a notificação
            }
        } else if(ogrupo.sms_recebidas!=grupo.sms_recebidas){//Se houver alteração da informação
            ogrupo.sms_recebidas=grupo.sms_recebidas;//Atualiza os dados antigos
            if(total_pessoas[i].sub_sms_recebidas==true){//Se o cliente estiver subscrito a este tipo de dados
                write(noti_fd,"O valor das SMS recebidas foi alterado!",BUF_SIZE);//Envia a notificação
            }
        } else if(ogrupo.sms_enviadas!=grupo.sms_enviadas){//Se houver alteração da informação
            ogrupo.sms_enviadas=grupo.sms_enviadas;//Atualiza os dados antigos
            if(total_pessoas[i].sub_sms_enviadas==true){//Se o cliente estiver subscrito a este tipo de dados
                write(noti_fd,"O valor das SMS enviadas foi alterado!",BUF_SIZE);//Envia a notificação
            }
        }
        sleep(1);
    }
    close(noti_fd);
    pthread_exit(NULL);
    return NULL;
    
    
}

void erro(char *msg)
{
    printf("Erro: %s\n", msg);
    exit(-1);
}
