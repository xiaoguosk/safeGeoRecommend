#include <windows.h>


class TimerCounter
{
public:
	TimerCounter(void);//���캯��
	~TimerCounter(void);//��������

private:
	LARGE_INTEGER startCount;//��¼��ʼʱ��

	LARGE_INTEGER endCount;//��¼����ʱ��

	LARGE_INTEGER freq;//����CPUʱ��Ƶ��

public:
	double dbTime;//�������е�ʱ�䱣��������

public:
	void Start();//�������ʼ�㴦��ʼ��ʱ
	void Stop();//�����������㴦������ʱ
};