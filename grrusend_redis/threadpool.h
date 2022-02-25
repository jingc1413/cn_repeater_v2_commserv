/**
 * �̳߳���3��������ɣ�������С������̡߳������̡߳�
 * �����̸߳�������������е����񣬹����̸߳���̬��ӻ����ٹ����߳�
 * ʹ����ֻ��Ҫ3�������������̳߳ء�������������������̳߳ء�
 */
#ifndef __THREADPOOL_H
#define __THREADPOOL_H

typedef struct tagTHRPOOLSTRU THRPOOLSTRU;

/**
 * @desc: �����̳߳�
 * @param[in] nMinThrNum: ��С�����߳���
 * @param[in] nMaxThrNum: ������߳���
 * @param[in] nMaxQueueLen: ������г���
 * @param[in] nSoftQueueLen: ���ԣ�����������д�����������������������ֵʱ����ӹ����߳�
 * @param[in] nThrVary: ���ԣ�һ������Ӷ��ٹ����߳��������� 10
 * @return: �ɹ� - �����̳߳ص�ָ�룻ʧ�� - ���� NULL
 */
THRPOOLSTRU *CreateThreadPool(unsigned int nMinThrNum, unsigned int nMaxThrNum,
				unsigned int nMaxQueueLen, unsigned int nSoftQueueLen,
				unsigned int nThrVary);

/**
 * @desc: ���̳߳������һ������ע�⣺�����������������˺����ᱻ������
 * @param[in] pstruPool: �̳߳�
 * @param[in] func: ��������ָ��
 * @param[in] arg: ���������Ĳ���
 * @return: ������ӳɹ� - ����0��ʧ�� - ����-1
 */
int AddTaskToThreadPool(THRPOOLSTRU *pstruPool, void*(*func)(void *arg), void *arg);

/**
 * @desc: �����̳߳�
 * @param[in] pstruPool: �̳߳�
 * @return: ���ٳɹ� - ����0��ʧ�� - ����-1
 */
int DestroyThreadPool(THRPOOLSTRU *pstruPool);

/**
 * @desc: ��ȡ���й����߳�����
 * @param[in] pstruPool: �̳߳�
 * @return: ���й����߳�����
 */
int GetAllThreadNum(THRPOOLSTRU *pstruPool);

/**
 * @desc: ��ȡ����������Ĺ����߳�����
 * @param[in] pstruPool: �̳߳�
 * @return: �����߳�����
 */
int GetBusyThreadNum(THRPOOLSTRU *pstruPool);

#endif
