#include "TimerCounter.h"
#include <iostream>


using namespace std;
TimerCounter::TimerCounter(void)
{
	QueryPerformanceFrequency(&freq);//��ȡ����CPUʱ��Ƶ��
}

TimerCounter::~TimerCounter(void)
{
}

void TimerCounter::Start()
{
	QueryPerformanceCounter(&startCount);//��ʼ��ʱ
}

void TimerCounter::Stop()
{
	QueryPerformanceCounter(&endCount);//ֹͣ��ʱ

	dbTime = ((double)endCount.QuadPart - (double)startCount.QuadPart) / (double)freq.QuadPart;//��ȡʱ���

}