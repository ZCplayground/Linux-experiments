#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include<stdio.h>
#include<stdlib.h>
#include<time.h>


/*~~~~~~~~~~~~~~~~~~~~~~~信号量相关函数~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
typedef int MySem;

MySem newsem(int initVal, int sem_num); //创建新的信号量，初值为initVal，完成后返回信号量的ID
void psem(MySem semID, int sem_num);//对ID为semID的信号量做p
void vsem(MySem semID, int sem_num);//对ID为semID的信号量做v
void Freesem(MySem semID);//注销ID为semID的信号量

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

MySem newsem(int initval, int sem_num)
{
	int key;
	int newsemID;
	key = 1;
	newsemID = semget(key, 4, IPC_CREAT | 0666);

	union semun sem_union;
	sem_union.val = initval;

	if (semctl(newsemID, sem_num, SETVAL, sem_union) == -1)
		return -1;
	else
		return newsemID;
}

void psem(MySem semID, int sem_num)
{
	struct sembuf MySembuf = { sem_num, -1, SEM_UNDO };
	if (semop(semID, &MySembuf, 1) == -1) {
		printf("error: P操作失败。\n");
		return;
	}
}


void vsem(MySem semID, int sem_num)
{
	struct sembuf MySembuf = { sem_num, 1, SEM_UNDO };
	if (semop(semID, &MySembuf, 1) == -1) {
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
int in, out;
//PC问题需要多个信号量：empty、full、mutex1、mute
MySem empty, full, mutex1, mutex2;


void init()
{
	printf("请输入仓库库存量k: ");
	scanf("%d", &k);
	buf = (int *)calloc(k, sizeof(int));

	in = 0;
	out = 0;

	empty = newsem(k, 0);
	full = newsem(0, 1);
	mutex1 = newsem(1, 2);
	mutex2 = newsem(1, 3);
}

void Pro()
{
	/* Pro()在开始显示：<进程类型：P> <进程ID> started
	结束时显示：P put an item to <仓库位置>       */

	printf("Producer: %d started.\n", getpid());

	int good = 1;//生产一个物品
	psem(empty, 0);
	psem(mutex1, 2);

	buf[in] = good;//放入仓库
	vsem(mutex1, 2);
	vsem(full, 1);

	printf("P put an item to position: %d.\n\n", in);
	in = (in + 1) % k;
	return;

}

void Con()
{
	/* Con()在开始显示：<进程类型：C> <进程ID> started
	结束时显示：C get an item from <仓库位置>     */

	printf("Consumer: %d started.\n", getpid());

	int takeout = 0;//拿走一个物品
	psem(full, 1);
	psem(mutex2, 3);


	buf[out] = takeout;//取走
	vsem(mutex2, 3);
	vsem(empty, 0);

	printf("C get an item from position: %d.\n\n", out);
	out = (out + 1) % k;
	return;

}

int main()
{
	init();

	srand((unsigned)time(NULL));
	int PCchoice;
	int delaytime;
	pid_t pid;
	while (1)
	{
		pid = fork();
		if (pid == -1)
		{
			printf("failed to create child process.\n");
			return -1;
		}
		else if (pid == 0)
		{
			PCchoice = rand() % 2;
			if (PCchoice == 0)
				Pro();
			else if (PCchoice == 1)
				Con();
			exit(0);
		}

		else
		{
			sleep(5);
		}
	}
	return 0;
}

