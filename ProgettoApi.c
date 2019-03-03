#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DIM_BUFFER_GRAPH 64  //da provare con i test, modificare se necessario
#define DIM_BUFFER_ACC 16  //da provare con i test, modificare se necessario
#define DIM_HT_ADJ_VERTEX 8192  //da provare con i test, modificare se necessario
#define DIM_ADJ_READC 63  //per indiricizzare le mosse per carattere
#define DIM_HT_ACC 64 //da provare con i test, modificare se necessario
#define FIRST_BLANK 256  //costante moltiplicativa per l'aumento del nastro
#define BLANK 1024  //costante moltiplicativa per l'aumento del nastro


//strutture dati
typedef struct adj_s{
    char writec;  //read char non mi serve, ci arrivo con la ht
    unsigned int tapemove;
    unsigned int nextvertex;
    struct adj_s*next;
}adj_t;
typedef struct ht_adj_s{
    unsigned int numvertex;
    adj_t**adj;  //punta all'array che indicizza le mosse in base al carattere
    struct ht_adj_s*next;  //per il concatenamento
}ht_adj_t;
typedef struct ht_acc_s{
    unsigned int numvertex;
    struct ht_acc_s*next;
}ht_acc_t;
typedef struct tape_s{
    char*right_tape;
    char*left_tape;
    size_t right_len;
    size_t left_len;
    unsigned int users;
}tape_t;
typedef struct queue_s{
    unsigned int numvertex;
    tape_t*tape_pointer;
    unsigned int chirality;
    unsigned int offset;
    struct queue_s*next;
}queue_t;

//prototipi funzioni
static inline void graph_create(ht_adj_t*[]);
static inline adj_t**h_adj_special(unsigned int,ht_adj_t*[]);  //come sotto solo che ritorna il puntatore a tutta la lista di tutte le mosse di tutti i caratteri, ritorna -1 se manca il nodo
static inline adj_t*h_adj(unsigned int,char,ht_adj_t*[],unsigned int*);  //riceve vertice e carattere da leggere e rtorna il puntatore alla lista delle mosse che posso fare con quel carattere da leggere, ritorna NULL se manca qualcosa, l'ultimo int* se 1 dice che ho il vertice, se 0 non lo ho
void add_ht_adj_vertex(unsigned int,ht_adj_t*[]);  //aggiunge il vertice alla ht e mette a null l'array per l'inirizzamento diretto
void add_ht_adj_move(adj_t**,char,char,char,unsigned int);  //aggiunge la mossa all'array indicizzato per caratteri
void read_acc(ht_acc_t**);  //funzione che riceve in input la ht degli stati di acc e la popola
static inline int h_acc(unsigned int,ht_acc_t**);  //funzione che riceve in input la ht delle accettazioni e il numero del vertice e ritorna 1 se è di acc, 0 se non lo è
void add_ht_acc(unsigned int,ht_acc_t**);  //funzione che riceve il numero dello stato di acc e la ht e lo aggiunge
unsigned int read_max();  //legge e ritorna il numero massimo di mosse, la chiamo nella chiamata della visita del grafo
void run(unsigned int,ht_adj_t**,ht_acc_t**);
static inline tape_t*input_read(unsigned int*);
void bfs(unsigned int,ht_adj_t**,ht_acc_t**,tape_t*);
static inline queue_t*enqueue(queue_t*,unsigned int,tape_t*,unsigned int,unsigned int);  //non creo spazio per un nuovo puntatore ma copio quello che ho da prima
static inline queue_t*enqueue_special(queue_t*,unsigned int,tape_t*,unsigned int,unsigned int,queue_t*);  //serve a riciclare la memoria, le funzioni special enqueue e write to tape sono economiche
static inline queue_t*dequeue(queue_t**);
static inline void write_to_tape(queue_t*,adj_t*,tape_t**,unsigned int*,unsigned int*,char); //l'ultimo intero dice se la transizione è deterministica
static inline void write_to_tape_special(queue_t*,adj_t*,tape_t**,unsigned int*,unsigned int*,char);
tape_t*copy_tape(queue_t*);  //copia il nastro in caso di cow e ritorna il puntatore a quello nuovo
void tape_increase(tape_t*,unsigned int);


FILE*fp;

//main
int main(){
    //fp=freopen("inpout_pubblici/in6.txt","r",stdin);
    ht_adj_t*ht_adj[DIM_HT_ADJ_VERTEX];  //array di puntatori agli elementi ht adj t
    ht_acc_t*ht_acc[DIM_HT_ACC];  //array di puntatori agli elementi ht acc t
    graph_create(ht_adj);
    read_acc(ht_acc);
    run(read_max(),ht_adj,ht_acc);

    //fclose(fp);
    return 0;
}

//funzioni
static inline void graph_create(ht_adj_t*ht_adj[DIM_HT_ADJ_VERTEX]){
    char buffer[DIM_BUFFER_GRAPH+1];
    char acc[4]="acc\0";
    unsigned int i,num;
    char readc;
    char writec;
    char tempmove;
    unsigned int nextvertex;
    adj_t**adjlist=NULL;
    for(i=0;i<DIM_HT_ADJ_VERTEX;i++){
        ht_adj[i]=NULL;
    }
    fgets(buffer,DIM_BUFFER_GRAPH+1,stdin);  //acquisisco tr e me ne frego
    fgets(buffer,DIM_BUFFER_GRAPH+1,stdin);  //acquisisco la prima mossa
    while(1){
        sscanf(buffer,"%d %c %c %c %d",&num,&readc,&writec,&tempmove,&nextvertex);
	//qui controllo il tipo di loop;
        adjlist=h_adj_special(num,ht_adj);
        if(adjlist==NULL){
            add_ht_adj_vertex(num,ht_adj);
            adjlist=h_adj_special(num,ht_adj);  //aggiorno adj
        }
        add_ht_adj_move(adjlist,readc,writec,tempmove,nextvertex);
        fgets(buffer,DIM_BUFFER_GRAPH+1,stdin);  //acquisisco nuova mossa
        for(i=0;i<3;i++){
            if(buffer[i]!=acc[i])
                break;
        }
        if(i==3)
            break;  //controllo se sono arrivato alla fine
    }
}
static inline adj_t**h_adj_special(unsigned int num,ht_adj_t*ht[DIM_HT_ADJ_VERTEX]){  //controllare
    unsigned int val;
    ht_adj_t*p=NULL;
    val=(num&(DIM_HT_ADJ_VERTEX-1)); //questo e l'indice dell'array in cui si deve trovare vertex
    if(ht[val]!=NULL){
        for(p=ht[val];p;p=p->next){
            if(p->numvertex==num){
                return p->adj;  //mi serve per add adj move
            }
        }
    }
    return NULL;
}
static inline adj_t*h_adj(unsigned int num,char readc,ht_adj_t*ht[DIM_HT_ADJ_VERTEX],unsigned int*vertex_presence){  //controllare
    unsigned int val;
    unsigned int readc_int;
    *vertex_presence=0;
    readc_int=(unsigned int)readc;
    ht_adj_t*p=NULL;
    val=(num&(DIM_HT_ADJ_VERTEX-1)); //questo e l'indice dell'array in cui si deve trovare vertex
    if(ht[val]!=NULL){
        for(p=ht[val];p;p=p->next){
            if(p->numvertex==num){
                *vertex_presence=1;  //mi serve in bfs per capire se il null è perchè non c'è la mossa per quel carattere o perchè non c'è proprio il vertice
                if(readc_int<=57)
                    return *(p->adj+readc_int-48);
                if(readc_int<=90)  //sicuramente è maggiore di 57 dalla condizione precedente
                    return *(p->adj+readc_int-55);
                if(readc_int==95)
                    return *(p->adj+readc_int-59);
                return *(p->adj+readc_int-60);
            }
        }
    }
    return NULL;
}
void add_ht_adj_vertex(unsigned int num,ht_adj_t*ht[DIM_HT_ADJ_VERTEX]){
    unsigned int val;
    ht_adj_t*temp=NULL,*headtemp=NULL;
    val=(num&(DIM_HT_ADJ_VERTEX-1));
    if((temp=malloc(sizeof(ht_adj_t)))){
        /*
        for(int i=0;i<DIM_ADJ_READC;i++){
            *(temp->adj+i)=NULL;
        }
        */
        if((temp->adj=calloc(DIM_ADJ_READC,sizeof(adj_t*)))){
            temp->numvertex=num;
            headtemp=ht[val];
            ht[val]=temp;
            ht[val]->next=headtemp;
        }
        else {
            printf("errore memoria in add_ht_adj_vertex\n");
            return;
        }
    }
    else{
        printf("errore memoria in add_ht_adj_vertex\n");
        return;
    }
}
void add_ht_adj_move(adj_t*adj[],char readc,char writec,char tempmove,unsigned int nextvertex){
    adj_t*temp=NULL,*headtemp=NULL;
    unsigned int readc_int;
    readc_int=(unsigned int)readc;
    if((temp=malloc(sizeof(adj_t)))){
        temp->writec=writec;
        switch (tempmove) {
            case 'R':
                temp->tapemove=2;
                break;
            case 'S':
                temp->tapemove=1;
                break;
            case 'L':
                temp->tapemove=0;
                break;
            default:
                break;
        }
        temp->nextvertex=nextvertex;
        if(readc_int<=57){
            headtemp=*(adj+readc_int-48);
            *(adj+readc_int-48)=temp;
            (*(adj+readc_int-48))->next=headtemp;
        }
        else if(readc_int<=90){
            headtemp=*(adj+readc_int-55);
            *(adj+readc_int-55)=temp;
            (*(adj+readc_int-55))->next=headtemp;
        }
        else if(readc_int==95){
            headtemp=*(adj+readc_int-59);
            *(adj+readc_int-59)=temp;
            (*(adj+readc_int-59))->next=headtemp;
        }
        else{
            headtemp=*(adj+readc_int-60);
            *(adj+readc_int-60)=temp;
            (*(adj+readc_int-60))->next=headtemp;
        }
    }
    else{
        printf("errore memoria in add ht adj move\n");
        return;
    }
}
void read_acc(ht_acc_t*ht[]){
    char buffer[DIM_BUFFER_ACC+1];  //stesso gioco di graph_create
    char max[4]="max\0";
    unsigned int i,num;
    for(i=0;i<DIM_HT_ACC;i++){
        ht[i]=NULL;
    }
    fgets(buffer,DIM_BUFFER_ACC+1,stdin);
    for(i=0;i<3;i++)   //gestione del caso in cui non ci sia nessuno stato di accettazione
        if(buffer[i]!=max[i])
            break;
    if(i==3)
        return;   //fino a qui

    while(1){
        sscanf(buffer,"%d",&num);
        add_ht_acc(num,ht);
        fgets(buffer,DIM_BUFFER_ACC+1,stdin);
        for(i=0;i<3;i++)   //gestione del caso in cui non ci sia nessuno stato di accettazione
            if(buffer[i]!=max[i])
                break;
        if(i==3)
            break;
    }
}
static inline int h_acc(unsigned int num,ht_acc_t*ht[]){
    unsigned int val;
    ht_acc_t*p=NULL;
    val=(num&(DIM_HT_ACC-1));
    if(ht[val]!=NULL){
        for(p=ht[val];p;p=p->next){
            if(p->numvertex==num)
                return 1;
        }
    }
    return 0;
}
void add_ht_acc(unsigned int num,ht_acc_t*ht[]){
    unsigned int val;
    ht_acc_t*temp=NULL,*headtemp=NULL;
    val=(num&(DIM_HT_ACC-1));
    if((temp=malloc(sizeof(ht_acc_t)))){
        temp->numvertex=num;
        headtemp=ht[val];
        ht[val]=temp;
        ht[val]->next=headtemp;
    }
    else{
        printf("errore add_ht_acc memoria\n");
        return;
    }
}

unsigned int read_max(){
    unsigned int maxmoves=0;
    char bin[5];  //lo uso per leggere e scartare run prima delle stringhe
    scanf("%u\n",&maxmoves);  //acquisisco il numero massimo di mosse
    fgets(bin,5,stdin);
    return maxmoves;
}
void run(unsigned int movesleft,ht_adj_t*ht_adj[],ht_acc_t*ht_acc[]){
    unsigned int written=1;
    tape_t*firsttape=NULL;
    while(1){
        firsttape=input_read(&written);
        if(written==0)
            break;
        bfs(movesleft,ht_adj,ht_acc,firsttape);
    }
}
static inline tape_t*input_read(unsigned int*written){
    tape_t*input=NULL;
    if((input=malloc(sizeof(tape_t)))){
        if(scanf("%ms\n",&(input->right_tape))<=0){  //quando non leggo piu niente mi fermo, come giusto che sia
            *written=0;
            free(input);
            return NULL;
        }
        input->right_len=strlen(input->right_tape);
        (input->right_tape)[input->right_len]='\0';  //per gestione futura
        if((input->left_tape=malloc(sizeof(char)*(FIRST_BLANK+1)))){
            input->left_tape=memset(input->left_tape,'_',FIRST_BLANK);
            (input->left_tape)[FIRST_BLANK]='\0';  //mi serve per gestirla come stringa
            input->left_len=strlen(input->left_tape);
            input->users=1;  //il nodo zero la usa subito
        }
        else{
            printf("errore memoria in input_read\n");
            return NULL;
        }
    }
    else{
        printf("errore memoria in input_read\n");
        return NULL;
    }
    return input;
}

void bfs(unsigned int movesleft,ht_adj_t*ht_adj[],ht_acc_t*ht_acc[],tape_t*firsttape){
    unsigned int printed=0;
    char readc;
    unsigned int vertex_presence;
    unsigned int recicled=0;
    //int deterministic;
    tape_t*father_tape;
    queue_t*u=NULL;
    queue_t*frontier=NULL;
    queue_t*next=NULL;
    queue_t*temp=NULL;
    adj_t*adj=NULL;
    tape_t*temp_pointer=NULL;
    unsigned int chirality,offset;
    chirality=1;
    offset=0;
    frontier=enqueue(frontier,0,firsttape,chirality,offset);
    while(movesleft&&!printed){
        u=dequeue(&frontier);
        switch (u->chirality){
            case 1:
                readc=*(u->tape_pointer->right_tape+u->offset);
                break;
            case 0:
                readc=*(u->tape_pointer->left_tape+u->offset);
                break;
            default: readc='_';
                break;
        }
        adj=h_adj(u->numvertex,readc,ht_adj,&vertex_presence);
        /*if(adj&&adj->next==NULL)
            deterministic=1;
        else
            deterministic=0;*/
        father_tape=u->tape_pointer;
        switch (vertex_presence) {
            case 1:
                if(adj){
                    while(adj->next!=NULL){

                        write_to_tape(u,adj,&temp_pointer,&chirality,&offset,readc);


                        next=enqueue(next,adj->nextvertex,temp_pointer,chirality,offset);
                        adj=adj->next;
                    }
                    write_to_tape_special(u,adj,&temp_pointer,&chirality,&offset,readc);
                    next=enqueue_special(next,adj->nextvertex,temp_pointer,chirality,offset,u);
                    recicled=1;
                }
            case 0:
                if(h_acc(u->numvertex,ht_acc)){
                    printf("1\n");
                    printed=1;
                    //forse devo liberare anche frontier
                    while(frontier){
                        temp=frontier->next;
                        frontier->tape_pointer->users=frontier->tape_pointer->users-1;
                        if(frontier->tape_pointer->users==0) {
                            free(frontier->tape_pointer->right_tape);
                            free(frontier->tape_pointer->left_tape);
                            free(frontier->tape_pointer);
                        }
                        free(frontier);
                        frontier=temp;
                    }
                    while(next){
                        temp=next->next;
                        next->tape_pointer->users=next->tape_pointer->users-1;
                        if(next->tape_pointer->users==0) {
                            free(next->tape_pointer->right_tape);
                            free(next->tape_pointer->left_tape);
                            free(next->tape_pointer);
                        }
                        free(next);
                        next=temp;
                    }
                }
                break;
            default: break;
        }
        father_tape->users=father_tape->users-1;
        if(father_tape->users==0){
            free(father_tape->right_tape);
            free(father_tape->left_tape);
            free(father_tape);
        }
        if(!recicled) {
            free(u);
        }
        recicled=0;
        if(frontier==NULL){
            if(next){
                frontier=next;
                next=NULL;
                movesleft=movesleft-1;
            }
            else if(!printed){
                printf("0\n");
                printed=1;
            }
        }
    }
    if(movesleft==0){
        while(frontier){
            u=dequeue(&frontier);
            if(h_acc(u->numvertex,ht_acc)){
                printf("1\n");
                printed=1;
            }
            u->tape_pointer->users=u->tape_pointer->users-1;
            if(u->tape_pointer->users==0) {
                free(u->tape_pointer->right_tape);
                free(u->tape_pointer->left_tape);
                free(u->tape_pointer);
            }
            free(u);
        }
    }
    if(!printed){
        printf("U\n");
    }
    while(frontier){  //forse while
        temp=frontier->next;
        free(frontier);
        frontier=temp;
    }
}

static inline queue_t*enqueue(queue_t*queue,unsigned int numvertex,tape_t*tape,unsigned int chirality,unsigned int offset){
    queue_t*new=NULL;
    if((new=malloc(sizeof(queue_t)))){
        new->numvertex=numvertex;
        new->tape_pointer=tape;
        new->chirality=chirality;
        new->offset=offset;
        new->next=queue;
        queue=new;
    }
    else
        printf("errore memoria in enqueue\n");
    return queue;
}

static inline queue_t*enqueue_special(queue_t*queue,unsigned int numvertex,tape_t*tape,unsigned int chirality,unsigned int offset,queue_t*father){
    father->numvertex=numvertex;
    father->tape_pointer=tape;
    father->chirality=chirality;
    father->offset=offset;
    father->next=queue;
    return father;
}

static inline queue_t*dequeue(queue_t**queue_add){
    queue_t*temp=NULL;
    temp=(*queue_add);
    (*queue_add)=(*queue_add)->next;
    return temp;
}

static inline void write_to_tape(queue_t*u,adj_t*adj,tape_t**temp_pointer_add,unsigned int*chirality_add,unsigned int*offset_add, char readc){
    if(readc==adj->writec){
           (*temp_pointer_add)=u->tape_pointer;
           (*temp_pointer_add)->users++;
    }
    else{  //cow
        *(temp_pointer_add)=copy_tape(u);
        switch (u->chirality) {
            case 1: *((*temp_pointer_add)->right_tape+u->offset)=adj->writec;
                break;
            case 0: *((*temp_pointer_add)->left_tape+u->offset)=adj->writec;
                break;
            default:
                break;
        }
    }
    (*chirality_add)=u->chirality;
    (*offset_add)=u->offset;
    switch (adj->tapemove) {
        case 1:
            break;
        case 2:
            switch (*chirality_add) {
                case 1:
                    if ((*offset_add) == ((*temp_pointer_add)->right_len - 1))
                        tape_increase((*temp_pointer_add), 1);
                    (*offset_add)++;
                    break;
                case 0:
                    if ((*offset_add) == 0)
                        (*chirality_add) = 1;  //vado dall'altra parte
                    else
                        (*offset_add)--;
                    break;
                default:
                    break;
            }
            break;
        case 0:
            switch (*chirality_add) {
                case 1:
                    if ((*offset_add) == 0)
                        (*chirality_add) = 0;
                    else
                        (*offset_add)--;
                    break;
                case 0:
                    if ((*offset_add) == ((*temp_pointer_add)->left_len - 1))
                        tape_increase((*temp_pointer_add), 0);
                    (*offset_add)++;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static inline void write_to_tape_special(queue_t*u,adj_t*adj,tape_t**temp_pointer_add,unsigned int*chirality_add,unsigned int*offset_add,char readc){
    if(u->tape_pointer->users==1){
        (*temp_pointer_add)=u->tape_pointer;
        if(readc!=adj->writec) {
            switch (u->chirality) {
                case 1: *((*temp_pointer_add)->right_tape+u->offset)=adj->writec;
                    break;
                case 0: *((*temp_pointer_add)->left_tape+u->offset)=adj->writec;
                    break;
                default:
                    break;
            }
        }
        (*temp_pointer_add)->users++;
    }
    else{
        if(readc==adj->writec){
            (*temp_pointer_add)=u->tape_pointer;
            (*temp_pointer_add)->users++;
        }
        else{  //cow
            *(temp_pointer_add)=copy_tape(u);
            switch (u->chirality) {
                case 1: *((*temp_pointer_add)->right_tape+u->offset)=adj->writec;
                    break;
                case 0: *((*temp_pointer_add)->left_tape+u->offset)=adj->writec;
                    break;
                default:
                    break;
            }
        }
    }
    (*chirality_add)=u->chirality;
    (*offset_add)=u->offset;
    switch (adj->tapemove) {
        case 1:
            break;
        case 2:
            switch (*chirality_add) {
                case 1:
                    if((*offset_add)==((*temp_pointer_add)->right_len-1))
                        tape_increase((*temp_pointer_add),1);
                    (*offset_add)++;
                    break;
                case 0:
                    if((*offset_add)==0)
                        (*chirality_add)=1;  //vado dall'altra parte
                    else
                        (*offset_add)--;
                    break;
                default:break;
            }
            break;
        case 0:
            switch (*chirality_add) {
                case 1:
                    if((*offset_add)==0)
                        (*chirality_add)=0;
                    else
                        (*offset_add)--;
                    break;
                case 0:
                    if((*offset_add)==((*temp_pointer_add)->left_len-1))
                        tape_increase((*temp_pointer_add),0);
                    (*offset_add)++;
                    break;
                default:break;
            }
            break;
        default:break;
    }

}

tape_t*copy_tape(queue_t*u){
    tape_t*new_tape=NULL;
    size_t rlen,llen;
    rlen=u->tape_pointer->right_len;
    llen=u->tape_pointer->left_len;
    if((new_tape=malloc(sizeof(tape_t)))){
        new_tape->users=1;  //lo usa solo uno per ora;
        if((new_tape->right_tape=malloc(sizeof(char)*(rlen+1)))&&(new_tape->left_tape=malloc(sizeof(char)*(llen+1)))){
            new_tape->right_tape=memcpy(new_tape->right_tape,u->tape_pointer->right_tape,rlen+1);
            new_tape->left_tape=memcpy(new_tape->left_tape,u->tape_pointer->left_tape,llen+1);
            new_tape->right_len=rlen;
            new_tape->left_len=llen;
        }
        else{
           printf("errore memoria in copy_tape\n");
           return NULL;
        }
    }
    else{
        printf("errore memoria in copy_tape\n");
        return NULL;
    }
    return new_tape;
}

void tape_increase(tape_t*tape,unsigned int chirality){
    size_t rlen,llen;
    rlen=tape->right_len;
    llen=tape->left_len;
    switch (chirality) {
        case 1:
            if((tape->right_tape=realloc(tape->right_tape,rlen+BLANK+1))){
                memset(tape->right_tape+rlen,'_',BLANK);
                *(tape->right_tape+rlen+BLANK)='\0';
                tape->right_len=(rlen+BLANK);
            }
            else{
                printf("errore memoria in tape_increase\n");
                return;
            }
            break;
        case 0:
            if((tape->left_tape=realloc(tape->left_tape,llen+BLANK+1))){
                memset(tape->left_tape+llen,'_',BLANK);
                *(tape->left_tape+llen+BLANK)='\0';
                tape->left_len=(llen+BLANK);
            }
            else{
                printf("errore memoria in tape_increase\n");
                return;
            }
            break;
        default:break;
    }
}

