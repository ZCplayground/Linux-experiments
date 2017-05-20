
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>

#include<stdio.h>
#include<stdlib.h>
#include<time.h>


/*~~~~~~~~~~~~~~~~~~~~~~~信号量相关函数~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
typedef int MySem;

MySem newsem(int initVal); //创建新的信号量，初值为initVal，完成后返回信号量的ID
void psem(MySem semID);//对ID为semID的信号量做p
void vsem(MySem semID);//对ID为semID的信号量做v
void Freesem(MySem semID);//注销ID为semID的信号量

union semun{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

MySem newsem(int initval)
{
    int key;
    int newsemID;
    newsemID = semget(IPC_PRIVATE, 1, IPC_CREAT|0666);

    union semun sem_union;
    sem_union.val = initval;

    if(semctl(newsemID, 0, SETVAL, sem_union)==-1)
        return -1;
    else
        return newsemID;
}

void psem(MySem semID)
{
    struct sembuf MySembuf = {0, -1, SEM_UNDO};
    if(semop(semID, &MySembuf, 1)==-1){
    printf("error: P操作失败。\n");
    return;
    }
}


void vsem(MySem semID)
{
    struct sembuf MySembuf = {0, 1, SEM_UNDO};
    if(semop(semID, &MySembuf, 1)==-1){
    printf("error: V操作失败。\n");
    return;
    }
}

void Freesem(MySem semID)
{
    union semun sem_union;
    semctl(semID, 0, IPC_RMID, sem_union);
}

/*~~~~~~~~~~~~~~~~~~~进程相关操作~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int k;
int *buf;
int *in, *out;
//PC问题需要多个信号量：empty、full、mutex1、mute
MySem *empty, *full, *mutex1, *mutex2;
int shmID;


void init()
{
    printf("请输入仓库库存量k: ");
    scanf("%d",&k);
   // buf = (int *)calloc(k,sizeof(int)); 不可以直接为全局变量分配空间
    shmID = shmget(IPC_PRIVATE, sizeof(int)*(5+k), IPC_CREAT | 0666);//创建共享存储区
    buf = (int *)shmat(shmID, NULL, 0);//将共享存储区映射到进程空间

    empty = buf;
    full = buf + 1;
    mutex1 = buf + 2;
    mutex2 = buf + 3;
    in = buf + 4;
    out = buf + 5;

    *in = 0;
    *out = 0;

    *empty = newsem(k);
    *full = newsem(0);
    *mutex1 = newsem(1);
    *mutex2 = newsem(1);
}

void Pro()
{
    /* Pro()在开始显示：<进程类型：P> <进程ID> started
       结束时显示：P put an item to <仓库位置>       */

    printf("Producer: %d started.\n",getpid());

    empty = buf;
    full = buf + 1;
    mutex1 = buf + 2;

    int good = 1;//生产一个物品
    psem(*empty);
    psem(*mutex1);

    //buf[*in] = good;//放入仓库
    vsem(*mutex1);
    vsem(*full);

    printf("P put an item to position: %d.\n\n", *in);
    *in = (*in + 1) % k;
    return ;

}

void Con()
{
        /* Con()在开始显示：<进程类型：C> <进程ID> started
           结束时显示：C get an item from <仓库位置>     */

    printf("Consumer: %d started.\n",getpid());

    empty = buf;
    full = buf + 1;
    mutex2 = buf + 3;

    int takeout = 0;//拿走一个物品
    psem(*full);
    psem(*mutex2);

    //buf[*out] = takeout;//取走
    vsem(*mutex2);
    vsem(*empty);

    printf("C get an item from position: %d.\n\n", *out);
    *out = (*out + 1) % k;
    return ;

}

int main()
{
    init();

    srand((unsigned)time(NULL));
    int PCchoice;
    int delaytime;
    pid_t pid;

    while(1)
    {
    srand((unsigned)time(NULL));
    pid = fork();
    if(pid == -1)
    {
        printf("failed to create child process.\n");
        return -1;
    }
    else if(pid == 0)
    {
        PCchoice = rand()%2;
        if(PCchoice == 0)
        Pro();
        else if(PCchoice == 1)
        Con();
        exit(0);
    }

    else
    {
        sleep(2);
    }
    }
//    Freesem(empty);
    return 0;
}
