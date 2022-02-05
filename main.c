#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
//commands
#define PEER "peer"
#define RUMAR "rumar"
#define CLEAR "clear"
#define CONS "cons"
#define MESS "mess"
#define MAX_CON 1000
#define MAX_MSG 2048
#define BUFFER 2048

typedef struct net_peer{
    uint16_t port;    
    char ip[20];    
} NetP;

typedef struct peer{
    char name[20];  
    NetP *netp;
} Peer;

typedef struct peer_message{
    Peer *peer;
    char message[BUFFER];
}PMsg;
//json print
typedef struct _json{
    char *key;
    char *value;
    int type;
}Json;
typedef struct LIST_STRUCT
{
   void** items;
   size_t size;
   size_t item_size;
} list_T;
typedef struct application {
    Peer *root;
    pthread_t tid;
    int server_fd;
} App;

pthread_mutex_t con_lock;
int cur_con = 0;
Peer cons[MAX_CON];

pthread_mutex_t msg_lock;
int cur_msg = 0;
PMsg msg_list[MAX_MSG];

App *app;

/* declaration Segment */ 
//modules 
char *itos(int i);
void  print_str(char *str);
void  println_str(char *str);
char *level_tap(int level);
char *clone_str(char *buffer);
//net peer
NetP *netp_init(const char *ip,uint16_t port);
bool  netp_cmp(NetP *p1,NetP *p2);
NetP *netp_on_addr(char *addr);
char *netp_to_addr(NetP *netp);
char *netp_tostring(NetP *netp,int level);
//peer
Peer *peer_init(const char *name,NetP *netp);
bool  peer_cmp(Peer p1,Peer p2);
void  peer_cpy(Peer *p1,Peer *p2);
int   peer_con_index(Peer *p);
int   peer_add_con(Peer *p);
Peer *peer_rm_con_index(int index);
bool peer_exist(NetP *netp);
char *peer_tostring(Peer *p,int level);
void  peer_print_all();
void  peer_test();
//pmsg
PMsg *pmsg_init(Peer *from,char *msg);
void  pmsg_cpy(PMsg *pm1,PMsg *pm2);
int   pmsg_add(PMsg *pm);
bool  pmsg_exist(char *msg);
char *pmsg_tostring(PMsg pm,int level);
void  pmsg_print_all();
void  pmsg_test();
//list type
list_T* list_init(size_t item_size);
void list_push(list_T* list, void* item);
//json
Json* json_init(char *key,char *value,int type);
void json_cpy(Json *j1,Json *j2);
int jlist_add(list_T* list,char *key,char *value,int type);
int jlist_rm(list_T* list,char *key);
char* jlist_tostring(list_T* list,int level);
//color
char *conc_color(char *code,char *str);
char *red(char *str);
char *yellow(char *str);
char *reset(char *str);
char *black(char *str);
char *blue(char *str);
char *green(char *str);
char *purple(char *str);
char *cyan(char *str);
char *white(char *str);
char *str_dqute(char *str);
char *str_qute(char *str);

/* Application */
void listen_app();
void downed_app();
App *init_app(const char *name,const char *ip,int port);
void init_root(int argc,char *argv[]);
void sending(NetP *netp,const char *message);
void receiving(int server_fd);
void *receive_thread(void *server_fd);
//cmd
bool  command();
void  command_start();
char *command_gen(const char *cmd,const char *data);
char *command_not_found(char *cmd);
void  send_handler(char * cmd,char *data);
void  recive_handler(char * buffer);
void  send_to_all(char *msg);
void  send_all_msg(NetP *netp);

/**
 * @brief main 
 * 
 * @return ** int 
 */

int main(int argc,char *argv[]){

    pthread_mutex_init(&con_lock, NULL);
    pthread_mutex_init(&msg_lock, NULL);

    init_root(argc,argv);
    listen_app();
    pthread_create(
        &app->tid, 
        NULL, 
        &receive_thread, 
        &app->server_fd
    ); 
    while(command());

    pthread_mutex_destroy(&con_lock);
    pthread_mutex_destroy(&msg_lock); 
    
    downed_app(app);
}
/* Modules */
char *itos(int i)
{
    char *s = malloc(sizeof(char) * 40);
    
    sprintf(s,"%d",i);

    return s;
}
void print_str(char *str)
{
    printf("%s",str);
}
void println_str(char *str)
{
    printf("%s\n",str);
}
char *level_tap(int level)
{
    char *tab = (char *)malloc(sizeof(char) * level + 1);
    strcat(tab,"");
    
    while (level-- > 0) strcat(tab,"  ");

    return tab;
}
char *clone_str(char * buffer)
{
    char *clone = malloc((strlen(buffer)+1) * sizeof(char));
    
    sprintf(clone,"%s%s",buffer,"\0");
    
    return clone;
}
char *get_line()
{
    char *string; // character array pointer
    size_t size = 1024; // initial size of char array
    string = (char*) malloc (size);//dynamic memory allocation of character array
    if(string == NULL)
    {
        printf("unable to allocate memory\n");
        exit(1);
    }
    char **string_pointer = &string; // double pointer to char array
    size_t characters = getline(string_pointer,&size,stdin);
    
    string[strlen(string)-1] = 0;

    return string;
}

/* Application */
App *init_app(const char *name,const char *ip,int port)
{
    App *app = malloc(sizeof(App));
    app->root = peer_init(name,netp_init(ip,port));

    return app;
}
void init_root(int argc,char *argv[])
{
    char *port="8796",*ipaddr="0.0.0.0",*name="Peer";
    
    char *dkey,*dval; 
    for (int i = 1; i < argc; i+=2)
    {
        dkey = clone_str(argv[i]);
        dval = clone_str(argv[i+1]);
        if(!strcmp(dkey,"-i")){ //ip
            ipaddr = dval;
        }
        else if(!strcmp(argv[i],"-p")){ //port
            port = dval;
        }
        else if(!strcmp(argv[i],"-n")){ // name
            name = dval;
        }
        else {
            command_not_found(dkey);
            exit(1);
        }
    }

    app = init_app(name,ipaddr,atoi(port));
}
void listen_app()
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int k = 0;
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(app->root->netp->port);
    
    //Printed the server socket addr and port
    printf("IP address is: %s\n", inet_ntoa(address.sin_addr));
    printf("port is: %d\n", (int)ntohs(address.sin_port));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    app->server_fd = server_fd;
}
void downed_app()
{
    close(app->server_fd);
}
/* Command */ 
void command_start(){
    if(!strcmp(app->root->name,"")){
        printf("%s  \n",cyan("command$"));
    }
    else {
        printf("%s%s%s%s%s \n",
            red("-> "),
            blue("["),
            cyan(netp_to_addr(app->root->netp)),
            blue("]"),
            cyan("$")
        );
    }
}
bool command()
{
    command_start();
    char *buf = get_line();
    if(!strcmp(buf,"exit")) 
        return false;
    
    char *cmd = strtok (buf," ");
    char *data = strtok (NULL,"");
    send_handler(cmd,data);

    return true;
}
char *command_gen(const char *cmd,const char *data)
{
    char *buffer = malloc((strlen(cmd) + strlen(data) + 1) * sizeof(char) + 1);

    sprintf(buffer,"%s %s",cmd,data);

    return buffer;
}
char *command_not_found(char *cmd)
{
    printf("%s : %s\n",purple(cmd),red("command not found"));   
}   
void send_handler(char * cmd,char *data)
{
    if(!strcmp(cmd,PEER)){
        NetP *tar =  netp_on_addr(data);
        if(netp_cmp(app->root->netp,tar)) 
        {
            printf("%s : you can not connect with yourself\n",red("warning"));
            return;
        }
        char *result = command_gen(PEER,netp_to_addr(app->root->netp));
        sending(tar,result);
    }
    else if (!strcmp(cmd,RUMAR)){
        char *result = command_gen(RUMAR, data);
        send_to_all(result);
    }
    else if (!strcmp(cmd,CLEAR)){
        system("clear");
    }
    else if (!strcmp(cmd,CONS)){
        peer_print_all();
    }
    else if (!strcmp(cmd,MESS)){
        pmsg_print_all();
    }
    else{
        command_not_found(cmd);
        return;
    }
}
void recive_handler(char *buffer)
{
    printf("\n%s %s\n",yellow("receved"),buffer);
    char *cp = clone_str(buffer);
    char *cmd = strtok (cp," ");    
    char *data = strtok (NULL,"");

    if(!strcmp(cmd,PEER)){
        NetP *clinet = netp_on_addr(data);
        pthread_mutex_lock(&con_lock);

        if(peer_exist(clinet)) {
            printf("%s %s %s\n",
                blue("ip address"),
                str_dqute(red(data)),
                blue("previusly connected!")
            );
            pthread_mutex_unlock(&con_lock);
        }
        else {
            peer_add_con(peer_init("",clinet));
            peer_print_all();
            pthread_mutex_unlock(&con_lock);
            pthread_mutex_lock(&msg_lock);
            send_all_msg(clinet);
            pthread_mutex_unlock(&msg_lock);
        }
    }
    else if (!strcmp(cmd,RUMAR)){
        pthread_mutex_lock(&msg_lock);
        if(pmsg_exist(data)) 
        {
            printf("%s %s %s\n",
                blue("message"),
                str_dqute(data),
                blue("exist!")
            );
            pthread_mutex_unlock(&msg_lock);
        }
        else {
            pmsg_add(pmsg_init(peer_init("",netp_init("",0)),data));
            pmsg_print_all();
            pthread_mutex_unlock(&msg_lock);
            pthread_mutex_lock(&con_lock);
            send_to_all(buffer);               
            pthread_mutex_unlock(&con_lock);
        }
    }
    else{
        printf("%s: command not found\n",cmd);   
        return;
    }
    command_start();
}
void send_all_msg(NetP *netp)
{
    char *buf;
    for (int i = 0; i < cur_msg; i++)
    {
        buf = command_gen(RUMAR,msg_list[i].message);
        sending(netp,buf);
    }
}
void send_to_all(char *msg)
{
    for (int i = 0; i < cur_con ; i++)
    {
        sending(cons[i].netp,msg);
    }
}
void sending(NetP *netp,const char *buffer)
{
    int sock = 0, valread;
    struct sockaddr_in serv_addr; 
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY always gives an IP of 0.0.0.0
    serv_addr.sin_port = htons(netp->port);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }

    send(sock, buffer, BUFFER * 2 , 0);
    printf("%s %s\n",green("sended"),buffer);
    close(sock);
}
//Calling receiving every 2 seconds
void *receive_thread(void *server_fd)
{
    int s_fd = *((int *)server_fd);
    while (1)
    {
        sleep(2);
        receiving(s_fd);
    }
}
//Receiving messages on our port
void receiving(int server_fd)
{
    struct sockaddr_in address;
    int valread;
    char buffer[BUFFER * 2] = {0};
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;

    //Initialize my current set
    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);
    int k = 0;
    while (1)
    {
        k++;
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
        {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {

                if (i == server_fd)
                {
                    int client_socket;

                    if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
                                                (socklen_t *)&addrlen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(client_socket, &current_sockets);
                }
                else
                {
                    valread = recv(i, buffer, sizeof(buffer), 0);
                    recive_handler(buffer);
                    FD_CLR(i, &current_sockets);
                }
            }
        }

        if (k == (FD_SETSIZE * 2))
            break;
    }
}
/* Test Module*/
void  peer_test()
{
    peer_add_con(peer_init("sjd",netp_init("10.10.10.10",5050)));
    peer_add_con(peer_init("mtt",netp_init("10.10.10.10",8585)));
    peer_add_con(peer_init("mhd",netp_init("10.10.10.10",7575)));
    peer_print_all();
}
void  pmsg_test()
{
    pmsg_add(pmsg_init(&cons[0],"sallam man sajjad hastam"));
    pmsg_add(pmsg_init(&cons[1],"sallam man matt hastam"));
    pmsg_add(pmsg_init(&cons[1],"haleton chetore"));
    pmsg_add(pmsg_init(&cons[2],"khobi da to chetori"));
    pmsg_add(pmsg_init(&cons[0],"manam khobam"));
    pmsg_print_all();    
}
/* net peer */
NetP *netp_init(const char *ip,uint16_t port)
{
    NetP *np = malloc(sizeof(NetP));
    
    strcpy(np->ip,ip);
    np->port = port; 
    
    return np;
}
bool  netp_cmp(NetP *n1,NetP *n2)
{
    return n1->port == n2->port && !strcmp(n1->ip,n2->ip); 
}
NetP *netp_on_addr(char *addr)
{
    char *cp = malloc(strlen(addr) * sizeof(char) + 1);
    strcpy(cp,addr);

    char *ip = strtok(cp,":");
    char *port = strtok(NULL,"");

    return netp_init(ip,atoi(port));
}
char *netp_to_addr(NetP *netp)
{
    char *buf = malloc((strlen(netp->ip)+6) * sizeof(char)  + 1);
    
    sprintf(buf,"%s:%s",netp->ip,itos(netp->port));

    return buf;
}
char* netp_tostring(NetP *netp,int level)
{
    list_T* list = list_init(3);
    jlist_add(list,"NETP","NetP",0);
    jlist_add(list,"ip",netp->ip,0);
    jlist_add(list,"port",itos(netp->port),1);

    return jlist_tostring(list,level);
}
/* Peer Segment */
Peer* peer_init(const char *name,NetP *netp)
{
    Peer *p = malloc(sizeof(Peer));
    
    strcpy(p->name,name);    
    p->netp = netp;

    return p;
}
bool peer_cmp(Peer p1,Peer p2)
{
    return p1.netp->port == p2.netp->port && 
        !strcmp(p1.netp->ip,p2.netp->ip);
}
void peer_cpy(Peer *p1,Peer *p2)
{
    strcpy(p1->name,p2->name);
    p1->netp = p2->netp;
}
int peer_con_index(Peer *p)
{
    
    for(int i=0;i<cur_con;i++)
        if(peer_cmp(cons[i],*p))
            return i;

    return -1;
}
int peer_add_con(Peer *p)
{
    int ind;
    if(MAX_CON <= cur_con){
        printf("full connection : can not add new\n");
        return -1;
    }
    if((ind=peer_con_index(p)) != -1)
    {
        return ind;
    }
    peer_cpy(&cons[cur_con],p);
    //return current & next index
    return cur_con++;
}
int peer_rm_con(Peer *p)
{
    int ind = peer_con_index(p);
    peer_rm_con_index(ind);
    return ind;
}
Peer *peer_rm_con_index(int index)
{
    if(index <0 || index >= cur_con){
        return NULL;
    }
    Peer *p = malloc(sizeof(Peer));
    peer_cpy(&cons[index],p);

    cur_con--;//decreas item count
    for (int i = index; i < cur_con; i++)
        cons[i] = cons[i+1]; 
    
    return p;
}
bool peer_exist(NetP *netp)
{
    for(int i = 0;i< cur_con;i++)
        if(netp->port == cons[i].netp->port  && 
            !strcmp(cons[i].netp->ip,netp->ip))
            return true;
    
    return false;
}
char* peer_tostring(Peer *p,int level)
{
    list_T* list = list_init(3);
    jlist_add(list,"PEER","Peer",0);
    jlist_add(list,"name",p->name,0);
    char *netp = netp_tostring(p->netp,level+1);
    jlist_add(list,"netp",netp,2);
    
    char *s = jlist_tostring(list,level);
    
    return s;
}
void peer_print_all()
{   
    for(int i=0;i < cur_con;i++)
        println_str(peer_tostring(&cons[i],0));    
}
/* Peer Message Segment */
PMsg* pmsg_init(Peer *from,char *msg)
{
    PMsg *p = malloc(sizeof(PMsg));
    p->peer = from; 
    strcpy(p->message,msg); 
    return p;
}
void pmsg_cpy(PMsg *pm1,PMsg *pm2)
{
    pm1->peer = pm2->peer;
    strcpy(pm1->message,pm2->message);
}
int pmsg_add(PMsg *pm)
{ 
    int pind;
    if(MAX_MSG <= cur_msg){
        printf("full message : can not add new message\n");
        return -1;
    }

    pmsg_cpy(&msg_list[cur_msg],pm);

    cur_msg++;

    return cur_msg-1;
}
bool pmsg_exist(char *msg)
{
    for(int i = 0; i < cur_msg;i++)
        if(!strcmp(msg_list[i].message,msg))
            return true;
    
    return false;
}
char* pmsg_tostring(PMsg pm,int level)
{   
    list_T* list = list_init(3);
    jlist_add(list,"PMSG","PMsg",0);
    jlist_add(list,"port",peer_tostring(pm.peer,level+1),2);
    jlist_add(list,"message",pm.message,1);

    return jlist_tostring(list,level);
}
void pmsg_print_all()
{
    for(int i=0;i < cur_msg;i++)
        println_str(pmsg_tostring(msg_list[i],0));    
}
/**/
list_T* list_init(size_t item_size){
   list_T* list = calloc(1,sizeof(struct LIST_STRUCT));
   list->size = 0;
   list->item_size = item_size;
   list->size = 0;

   return list;
}
void list_push(list_T* list, void* item){
   list->size +=1;
   
   if(!list->items)
      list->items = calloc(1,list->item_size);
   else 
      list->items = realloc(list->items,(list->size * list->item_size));
   
   list->items[list->size-1] = item;

} 
/* */ 
Json* json_init(char *key,char *value,int type)
{
    Json* j = malloc(sizeof(Json *));

    j->key = malloc(strlen(key)+1);
    strcpy(j->key,key);
    j->value = malloc(strlen(value)+1);
    strcpy(j->value,value);
    j->type = type;
    return j;
}
void json_cpy(Json *j1,Json *j2)
{
    strcpy(j1->key,j2->key);
    strcpy(j1->value,j2->value);
    j1->type = j1->type; 
}
int jlist_add(list_T* list,char *key,char *value,int type)
{
    list_push(list,json_init(key,value,type));
    return list->size - 1;
}
int jlist_rm(list_T* list,char *key)
{
    int ind;

    return ind;
}
void jlist_sort_by_key(list_T* list)
{
    for (size_t i = 1; i < list->size; i++)
    {
        Json *a =(Json *) list->items[i];
        for (int j = i + 1; j < list->size; j++)
        {
            Json *b =(Json *) list->items[j];
            if(strcmp(a->key, b->key) > 0)
            {
                void* tmp = list->items[i];
                list->items[i] = list->items[j];
                list->items[j] = tmp;
            }
        }
    }
}
char* jlist_tostring(list_T* list,int level)
{
    int sz = list->size;
    if(list->size == 0){
        return "";
    }

    Json *root = list->items[0];
    char *tab0 = level_tap(level);
    char *tab1 = level_tap(level+1);
    
    jlist_sort_by_key(list);
    
    char *indside = malloc(1); 
    for (size_t i = 1; i < sz; i++)
    {
        Json *j =(Json *) list->items[i];
        int ssz = (
            strlen(indside) +
            strlen(green(j->key)) + 
            2 * strlen(yellow("\"")) +
            2 * strlen(green("\"")) +
            strlen(red(j->value))
        );

        indside = realloc(indside,ssz * sizeof(char) + 40);

        strcat(indside,"\n");
        strcat(indside,tab1);
        sprintf(indside,"%s%s%s%s",indside,green("\""),green(j->key),green("\""));
        strcat(indside,":");

        if(j->type == 0)
            strcat(indside,red(j->value));  
        else if(j->type == 1)
            strcat(indside,str_dqute(j->value));  
        else
            strcat(indside,j->value);  
    }
    
    long s_sz = strlen(indside) + strlen(root->value) + 20;
    char *buffer = malloc(s_sz * sizeof(char)); 


    sprintf(buffer,
        "%s %s%s\n%s%s",
        purple(root->value),
        purple("{"),
        indside,
        tab0,
        purple("}")
    );

    return buffer;
}
char *conc_color(char *code,char *str)
{
    int ssz = strlen(code) + strlen(str) + strlen("\033[0m") + 1;
    char *buf = malloc(ssz * sizeof(char));
    
    sprintf(buf,"%s%s%s",code,str,"\033[0m");

    return buf;
}
char * red (char *str)
{
  return conc_color("\033[1;31m",str);
}
char *yellow(char *str)
{
  return conc_color("\033[1;33m",str);
}
char *reset (char *str) {
  return conc_color("\033[0m",str);
}
char *black(char *str)
{
  return conc_color("\033[1;30m",str);
}
char *blue(char *str)
{
  return conc_color("\033[1;34m",str);
}
char *green(char *str)
{
  return conc_color("\033[1;32m",str);
}
char *purple(char *str)
{
  return conc_color("\033[1;35m",str);
}
char *cyan(char *str)
{
  return conc_color("\033[1;36m",str);
}
char *white(char *str)
{
  return conc_color("\033[1;37m",str);
}
char *str_dqute(char *str)
{
    char *buf = malloc((strlen(yellow(str)) + strlen(yellow("\"")) * 2) +1);

    sprintf(buf,"%s%s%s",yellow("\""),yellow(str),yellow("\""));
    
    return buf;
}
char *str_qute(char *str)
{

    char *buf = malloc((strlen(yellow(str)) + strlen(yellow("\'")) * 2) +1);

    sprintf(buf,"%s%s%s",yellow("\'"),yellow(str),yellow("\'"));
    
    return buf;
}

