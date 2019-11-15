#pragma once
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include"GameTimer.h"
#include "Common/d3dUtil.h"
#include <windows.h>
class RenderBase;

class WindowsProcess
{
public:
	WindowsProcess(HINSTANCE hInstance);
	WindowsProcess(const WindowsProcess& rhs) = delete;
	WindowsProcess & operator=(const WindowsProcess& rhs) = delete;
	~WindowsProcess()
	{
		
	}
public:
	
	static WindowsProcess * GetAPP();
	//获取应用程序的handle
	HINSTANCE Appinst()const { return mhAppInst; }
	//获取主窗口的handle
	HWND MainWnd()const { return mhMainWnd; }
	//获取长宽比
	float AspectRation()const { return static_cast<float>(mClientWidth) / mClientHeight; }
	//Windows的消息循环
	int Run();
	//窗口初始化
	virtual bool Initialize();
	//实现应用程序住窗口的窗口过程函数
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnResize();

protected:
	//初始化主窗口
	bool InitMainWindow();

	//鼠标键盘输入相关信息的处理
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	virtual void OnKeyboardInput();
	void CalculateFrameStats();

protected:
	static WindowsProcess * mApp;
	HINSTANCE mhAppInst = nullptr;//应用程序实例句柄
	HWND mhMainWnd = nullptr;//主窗口句柄
	HWND mhChildWnd = nullptr;
	bool mAppPaused = false;//应用程序是否暂停
	bool mMinimized = false;//应用程序是否最小化
	bool mMaximized = false;//应用程序是否最大化
	bool mResizing = false;//大小调整栏是否受到推拽

	std::wstring mMainWndCaption = L"SamleRndering";

	GameTimer mTimer;//用于记录 "delta-time" 和游戏总时间

	int mClientWidth = 1000;
	int mClientHeight = 750;

	int mWindowWith = 1000;
	int mWindowHeight = 750;
	//D3D渲染相关
	std::shared_ptr<RenderBase> render;
	POINT mLastMousePos;
};