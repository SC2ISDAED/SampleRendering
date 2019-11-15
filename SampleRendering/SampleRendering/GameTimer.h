#pragma once
class GameTimer
{
public:
	GameTimer();
	float TotalTime()const;//
	float DeltaTime()const
	{
		return (float)mDeltaTime;
	}//秒为单位

	void Reset();//在开始消息循环前调用
	void Start();//解除暂停计时器时调用
	void Stop();//暂停计数器时条用
	void Tick();//每一帧调用
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

