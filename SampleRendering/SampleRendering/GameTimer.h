#pragma once
class GameTimer
{
public:
	GameTimer();
	float TotalTime()const;//
	float DeltaTime()const
	{
		return (float)mDeltaTime;
	}//��Ϊ��λ

	void Reset();//�ڿ�ʼ��Ϣѭ��ǰ����
	void Start();//�����ͣ��ʱ��ʱ����
	void Stop();//��ͣ������ʱ����
	void Tick();//ÿһ֡����
private:
	double mSecondsPerCount;
	double mDeltaTime;

	_int64 mBaseTime;
	_int64 mPasuedTime;
	_int64 mStopTime;
	_int64 mPerTime;
	_int64 mCurrTime;

	bool mStopped;
};

