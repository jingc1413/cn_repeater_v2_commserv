#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "threadpool.h"
#include "utils.h"
#include "log_r.h"

#define DEFAULT_TIME 1 /* �����߳�ÿ��1����һ��������У���������ӻ����ٹ����߳� */

typedef struct tagTASKSTRU
{
	void *(*func)(void *);
	void *arg;
} TASKSTRU;

struct tagTHRPOOLSTRU
{
	pthread_mutex_t lock; /* �̳߳صĻ����� */
	pthread_cond_t QueueNotFull; /* �û��������������ʱ��������Ѿ����ˣ��ͻ�������ֱ�������� */
	pthread_cond_t QueueNotEmpty; /* �������û������ʱ�������̻߳�������ֱ�������ѣ��������ˣ�������Ҫ�������ˣ� */
	pthread_t *Threads; /* ���飬�������й����̵߳� thid */
	pthread_t AdjusterThid; /* �����̵߳� thid */
	TASKSTRU *pstruTaskQueue; /* ������� */
	int nMinThrNum; /* ��С�����߳��� */
	int nMaxThrNum; /* ������߳��� */
	int nLiveThrNum; /* �ִ�Ĺ����߳��� */
	int nBusyThrNum; /* ���ڴ�������Ĺ����߳�����������ֵ�������ֵС�ܶ࣬��Ҫ��������һЩ�����߳��� */
	int nWaitExitThrNum; /* ��Ҫ�����ٵĹ����߳��� */
	int nThrVary; /* �����߳�һ������ӵĹ����߳��� */
	int nQueueFront; /* ��ǰ��Ҫ��������������� */
	int nQueueRear; /* ������������һ������֮����������������ʱ���������� */
	int nQueueSize; /* ��������е�������� */
	int nQueueSoftSize; /* ����������д����������������ֵʱ�������߳̾ͻ���ӹ����߳� */
	int nQueueMaxSize; /* ���������󳤶ȣ����ԣ����Բ��������������������ֵ */
	int nShutdown; /* �����̳߳�ʱ����1 */
};

static int IsThreadAlive(pthread_t thid)
{
	int nRet;

	nRet = pthread_kill(thid, 0); /* 0�������źţ�������������߳��Ƿ��� */
	if (nRet == ESRCH) /* No such process */
		return 0;

	return 1;
}

static int FreeThreadPool(THRPOOLSTRU *pstruPool)
{
	if (pstruPool == NULL)
		return 0;

	if (pstruPool->pstruTaskQueue)
		free(pstruPool->pstruTaskQueue);

	if (pstruPool->Threads)
	{
		free(pstruPool->Threads);
		pthread_mutex_lock(&pstruPool->lock);
		pthread_mutex_destroy(&pstruPool->lock);
		pthread_cond_destroy(&pstruPool->QueueNotEmpty);
		pthread_cond_destroy(&pstruPool->QueueNotFull);
	}

	free(pstruPool);

	return 0;
}

/**
 * �����̣߳�������������ϵ�����
 */
static void *WorkerThread(void *ThreadPool)
{
	int nNeedBroadcast = 0;
	TASKSTRU struTask;
	THRPOOLSTRU *pstruPool = (THRPOOLSTRU *)ThreadPool;
    pthread_t ThreadId = pthread_self();
 
	while (1)
	{
		pthread_mutex_lock(&(pstruPool->lock));

		while ((pstruPool->nQueueSize == 0) && (!pstruPool->nShutdown))
		{ /* ���������û������ */
			pthread_cond_wait(&pstruPool->QueueNotEmpty, &pstruPool->lock);

			if (pstruPool->nWaitExitThrNum > 0)
			{ /* ��Ҫ���ټ��������߳� */
				pstruPool->nWaitExitThrNum--;
				if (pstruPool->nLiveThrNum > pstruPool->nMinThrNum)
				{ /* ��֤�����̲߳������������ */
					/* ΨһҪ���ģ�����Ĩȥ�Լ����ڵļ�¼���Լ��ͷ�����
					 * ���к��£����ͷ��ڴ��������������ƴ���������� */
					pstruPool->nLiveThrNum--;
					pthread_mutex_unlock(&(pstruPool->lock));
					pthread_exit(NULL);
				}
			}
		}

		if (pstruPool->nShutdown)
		{
			pthread_mutex_unlock(&(pstruPool->lock));
			pthread_exit(NULL);
		}

		/* ������������ģ����п�λ�ó���ʱ����Ҫ֪ͨ���������߳� */
		if (pstruPool->nQueueSize == pstruPool->nQueueMaxSize)
			nNeedBroadcast = 1;

		/* ��������л�ȡ���� */
		struTask.func = pstruPool->pstruTaskQueue[pstruPool->nQueueFront].func;
		/* ���ﲻ�������Ƴ����������� threadpool_add �����ͷ���ָ����ڴ� */
		struTask.arg = pstruPool->pstruTaskQueue[pstruPool->nQueueFront].arg;
		pstruPool->nQueueFront = (pstruPool->nQueueFront + 1) % pstruPool->nQueueMaxSize;
		pstruPool->nQueueSize--;

		if (nNeedBroadcast) /* ֪ͨȫ���磬���������λ�ÿճ����� */
		{
			pthread_cond_broadcast(&pstruPool->QueueNotFull);
		}


		pthread_mutex_unlock(&(pstruPool->lock));

		pthread_mutex_lock(&(pstruPool->lock));
		pstruPool->nBusyThrNum++;
		pthread_mutex_unlock(&(pstruPool->lock));
        PrintDebugLogR(DBG_HERE,"[%X] Start to work\n", ThreadId);
		(*(struTask.func))(struTask.arg); /* �����߳̿��������ܺ�ʱ��ܳ� */
        PrintDebugLogR(DBG_HERE,"[%X] Work complete\n", ThreadId);
		pthread_mutex_lock(&(pstruPool->lock));
		pstruPool->nBusyThrNum--;
		pthread_mutex_unlock(&(pstruPool->lock));
	}

	pthread_exit(NULL);
	return NULL;
}

/**
 * �������̣߳�����������е�״̬��������ӡ����ٹ����߳�
 */
static void *AdjusterThread(void *ThreadPool)
{
	int nThrVary = 0, nAdd, i;
	THRPOOLSTRU *pstruPool = (THRPOOLSTRU *)ThreadPool;

	while (!pstruPool->nShutdown)
	{
		sleep(DEFAULT_TIME); /* ��ʱ˯�� */

		pthread_mutex_lock(&(pstruPool->lock));
		/* �����������е�������������趨�ķ�ֵ��
		 * �����ִ�Ĺ����߳�����������߳����������һ�鹤���߳� */
		if (pstruPool->nQueueSize >= pstruPool->nQueueSoftSize
			&& pstruPool->nLiveThrNum < pstruPool->nMaxThrNum)
		{
			nAdd = 0;
			for (i = 0; i < pstruPool->nMaxThrNum; i++)
			{
				/* һ����ഴ�� nThrVary �������߳� */
				if (nAdd >= pstruPool->nThrVary)
					break;
				/* �ִ湤���̲߳��ܳ���������߳��� */
				if (pstruPool->nLiveThrNum >= pstruPool->nMaxThrNum)
					break;

				if (pstruPool->Threads[i] == 0 || !IsThreadAlive(pstruPool->Threads[i]))
				{
					pthread_create(&(pstruPool->Threads[i]), NULL, WorkerThread, (void *)pstruPool);
					nAdd++;
					pstruPool->nLiveThrNum++;
				}
			}
		}
		pthread_mutex_unlock(&(pstruPool->lock));

		pthread_mutex_lock(&(pstruPool->lock));
		/* ����ڸɻ���߳�ԶԶ���ڴ����̣߳����Ҵ���߳�������������������
		 * �Ǿ�ɾ��һЩ�����߳� */
		if ((pstruPool->nBusyThrNum * 2) < pstruPool->nLiveThrNum
			&& pstruPool->nLiveThrNum > pstruPool->nMinThrNum)
		{
			pstruPool->nWaitExitThrNum = pstruPool->nThrVary;
			nThrVary = pstruPool->nThrVary;
		}
		pthread_mutex_unlock(&(pstruPool->lock));
		if (nThrVary)
		{
			for (i = 0; i < nThrVary; i++)
			{ /* ���������ˣ�����û��������������̣߳��Ͻ�����Ȼ��ȥ���� */
				pthread_cond_signal(&(pstruPool->QueueNotEmpty));
			}
		}
	}
	return NULL;
}

THRPOOLSTRU*
CreateThreadPool(unsigned int nMinThrNum, unsigned int nMaxThrNum,
				unsigned int nMaxQueueLen, unsigned int nSoftQueueLen,
				unsigned int nThrVary)
{
	int i;
	THRPOOLSTRU *pstruPool = NULL;

	if (!nMinThrNum || !nMaxThrNum || !nMaxQueueLen || !nSoftQueueLen || !nThrVary)
	{
		//PrintErrorLog(DBG_HERE, "�����̳߳�ʧ��[�����������]\n");
		return NULL;
	}

	/* ʹ�� do..while(0)��䣬����ʵ�� goto �Ĺ��� */
	do{
		pstruPool = (THRPOOLSTRU *)calloc(1, sizeof(THRPOOLSTRU));
		if (!pstruPool)
		{
			//PrintErrorLog(DBG_HERE, "�����̳߳�ʧ��[�����ڴ�ʧ��]\n");
			break;
		}

		pstruPool->nMinThrNum = nMinThrNum;
		pstruPool->nMaxThrNum = nMaxThrNum;
		pstruPool->nQueueMaxSize = nMaxQueueLen;
		pstruPool->nQueueSoftSize = nSoftQueueLen;
		pstruPool->nThrVary = nThrVary;

		pstruPool->Threads = (pthread_t *)calloc(1, sizeof(pthread_t) * nMaxThrNum);
		if (pstruPool->Threads == NULL)
		{
			//PrintErrorLog(DBG_HERE, "�����̳߳�ʧ��[�����ڴ�ʧ��]\n");
			break;
		}

		pstruPool->pstruTaskQueue = (TASKSTRU *)calloc(1, sizeof(TASKSTRU) * nMaxQueueLen);
		if (pstruPool->pstruTaskQueue == NULL)
		{
			//PrintErrorLog(DBG_HERE, "�����̳߳�ʧ��[�����ڴ�ʧ��]\n");
			break;
		}

		if (pthread_mutex_init(&pstruPool->lock, NULL) ||
			pthread_cond_init(&pstruPool->QueueNotEmpty, NULL) ||
			pthread_cond_init(&pstruPool->QueueNotFull, NULL))
		{
			//PrintErrorLog(DBG_HERE, "�����̳߳�ʧ��[��ʼ����ʧ��]\n");
			break;
		}

		/* �������������Ĺ����߳� */
		for (i = 0; i < nMinThrNum; i++)
		{
			pthread_create(&(pstruPool->Threads[i]), NULL, WorkerThread, (void *)pstruPool);
			pstruPool->nLiveThrNum++;
		}
		pthread_create(&(pstruPool->AdjusterThid), NULL, AdjusterThread, (void *)pstruPool);
		return pstruPool;
	} while(0);

	FreeThreadPool(pstruPool);
	return NULL;
}

/**
 * �������������
 */
int AddTaskToThreadPool(THRPOOLSTRU *pstruPool, void*(*func)(void *arg), void *arg)
{
    pthread_t ThreadId = pthread_self();

	if (!pstruPool || !func || !arg)
	{
		//PrintErrorLog(DBG_HERE, "�������ʧ��[�����������]\n");
		return -1;
	}

	pthread_mutex_lock(&pstruPool->lock);

    PrintDebugLogR(DBG_HERE,"[%X] Waiting for queue\n", ThreadId);
	while ((pstruPool->nQueueSize >= pstruPool->nQueueMaxSize) && (!pstruPool->nShutdown))
	{
		/* ����������ˣ������������ֱ��������г��ֿ���λ�� */
		pthread_cond_wait(&pstruPool->QueueNotFull, &pstruPool->lock);
	}
    PrintDebugLogR(DBG_HERE,"[%X] Catch a queue\n", ThreadId);
	if (pstruPool->nShutdown)
	{
        PrintDebugLogR(DBG_HERE,"[%X] pool already shutdown\n", ThreadId);
		pthread_mutex_unlock(&pstruPool->lock);
		//PrintErrorLog(DBG_HERE, "�������ʧ��[�̳߳�������]\n");
		return -1;
	}

	/* ��ʼ����������������.
	 * ����Ҫ�ͷ� arg ָ��ָ����ڴ棬����ڴ���뽻��������ȥ�ͷţ���Ϊ��
	 * 1. ���� arg ָ��һ���������ﲻ�����ͷŸɾ���
	 * 2. ���� arg ָ��һ����ڴ��е�һ����ַ��
	 *    int *a = malloc(sizeof(a)*10), arg=&a[5]�������ͷ��ڴ�ͻ����
	 * ���ԣ�������ֱ�Ӹ��������ָ�� */
	pstruPool->pstruTaskQueue[pstruPool->nQueueRear].func = func;
	pstruPool->pstruTaskQueue[pstruPool->nQueueRear].arg = arg;
	pstruPool->nQueueRear = (pstruPool->nQueueRear+1) % pstruPool->nQueueMaxSize;
	pstruPool->nQueueSize++;
    PrintDebugLogR(DBG_HERE,"[%X] nQueueRear: %d; nQueueSize: %d\n", ThreadId, pstruPool->nQueueRear, pstruPool->nQueueSize);

	/* ֪ͨȫ���磬����������������� */
	pthread_mutex_unlock(&pstruPool->lock);
	//pthread_cond_signal(&pstruPool->QueueNotEmpty);
	pthread_cond_broadcast(&pstruPool->QueueNotEmpty);

	return 0;
}


int DestroyThreadPool(THRPOOLSTRU *pstruPool)
{
	int i;

	if (!pstruPool)
	{
		//PrintErrorLog(DBG_HERE, "�����̳߳�ʧ��[�����������]\n");
		return -1;
	}

	pstruPool->nShutdown = 1;
	/* �ȴ��������߳��˳� */
	pthread_join(pstruPool->AdjusterThid, NULL);

	/* ����������ն��ж����������̣߳����ǿ���ȥ���� */
	pthread_cond_broadcast(&(pstruPool->QueueNotEmpty));
	for (i = 0; i < pstruPool->nMinThrNum; i++)
	{ /* �ȴ����й����߳��˳� */
		pthread_join(pstruPool->Threads[i], NULL);
	}
	FreeThreadPool(pstruPool);
	return 0;
}

int GetAllThreadNum(THRPOOLSTRU *pstruPool)
{
	int nAll = 0;
	pthread_mutex_lock(&pstruPool->lock);
	nAll = pstruPool->nLiveThrNum;
	pthread_mutex_unlock(&pstruPool->lock);
	return nAll;
}

int GetBusyThreadNum(THRPOOLSTRU *pstruPool)
{
	int nBusy = 0;
	pthread_mutex_lock(&pstruPool->lock);
	nBusy = pstruPool->nBusyThrNum;
	pthread_mutex_unlock(&(pstruPool->lock));
	return nBusy;
}
