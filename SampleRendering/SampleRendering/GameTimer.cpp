#include "GameTimer.h"
#include <windows.h>
GameTimer::GameTimer():mSecondsPerCount(0),mDeltaTime(-1.0f),mBaseTime(0),
mStopped(false),mPerTime(0),mCurrTime(0), mPasuedTime(0)
{
	_int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER *)&countsPerSec);
	mSecondsPerCount = 1.0f / (double)countsPerSec;
}

float GameTimer::TotalTime() const
{
	if (mStopped)
	{
		return (float)((mStopTime - mPasuedTime - mBaseTime)*mSecondsPerCount);
		//����Ϊʲô��mPasuedTime��
		//����ǵ�һ����ͣ�Ļ���mPausedTIME=0�����ǵĻ�����Ҫ��ȥ֮ǰ��ͣʱ�ȹ���ʱ����ܺ͡�
	}
	else
	{
		return (float)(((mCurrTime - mPasuedTime) - mBaseTime)*mSecondsPerCount);
	}
}

void GameTimer::Tick()
{
	if (mStopped)
	{
		mDeltaTime = 0.0;
		return;
	}
	//��ñ�֡��ʼ��ʾ��ʱ��
	_int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER *)&currTime);
	mCurrTime = currTime;
	//���ǰһ֡�뱾һ֡��ʱ���
	mDeltaTime = (mCurrTime - mPerTime)*mSecondsPerCount;
	//׼�����㱾֡����һ֡��ʱ���
	mPerTime = mCurrTime;
	if (mDeltaTime<0.0f)
	{
		mDeltaTime = 0.0f;
	}
}
void GameTimer::Reset()
{
	_int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	mBaseTime = currTime;
	mPerTime = currTime;
	mStopTime = 0;
	mStopped = false;
}

void GameTimer::Start()
{
	if (mStopped)
	{
		_int64 startTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
		mPasuedTime += (startTime - mStopTime);
		mPerTime = startTime;
		mStopTime = 0;
		mStopped = false;
	}
	
}

void GameTimer::Stop()
{
	if (!mStopped)
	{
		_int64 stopTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&stopTime);
		mStopTime = stopTime;
		mStopped = true;
	}
}
