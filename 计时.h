#pragma once
#include <windows.h>
class �߾��ȼ�ʱ
{
public:
	�߾��ȼ�ʱ(void);
	~�߾��ȼ�ʱ(void);//��������

public:
	LARGE_INTEGER ��ʼʱ��;

	LARGE_INTEGER ����ʱ��;

	LARGE_INTEGER CPUƵ��;

public:
	double _��ʼʱ��;
	double _����ʱ��;
	double _���;

public:
	void ��ʼ();
	void ����();
};



�߾��ȼ�ʱ::�߾��ȼ�ʱ(void)
{
	QueryPerformanceFrequency(&CPUƵ��);
}

�߾��ȼ�ʱ::~�߾��ȼ�ʱ(void)
{
}

void �߾��ȼ�ʱ::��ʼ()
{
	QueryPerformanceCounter(&��ʼʱ��);
}

void �߾��ȼ�ʱ::����()
{
	QueryPerformanceCounter(&����ʱ��);

	_��ʼʱ�� = ((double)��ʼʱ��.QuadPart) / (double)CPUƵ��.QuadPart;
	_����ʱ�� = ((double)����ʱ��.QuadPart) / (double)CPUƵ��.QuadPart;
	_��� = ((double)����ʱ��.QuadPart - (double)��ʼʱ��.QuadPart) / (double)CPUƵ��.QuadPart;

}