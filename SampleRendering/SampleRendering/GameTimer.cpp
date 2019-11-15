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
		//至于为什么有mPasuedTime，
		//如果是第一次暂停的话，mPausedTIME=0，不是的话就需要减去之前暂停时度过的时间的总和。
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
	//获得本帧开始显示的时刻
	_int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER *)&currTime);
	mCurrTime = currTime;
	//获得前一帧与本一帧的时间差
	mDeltaTime = (mCurrTime - mPerTime)*mSecondsPerCount;
	//准备计算本帧与下一帧的时间差
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
