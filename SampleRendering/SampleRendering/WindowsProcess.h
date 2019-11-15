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
	//��ȡӦ�ó����handle
	HINSTANCE Appinst()const { return mhAppInst; }
	//��ȡ�����ڵ�handle
	HWND MainWnd()const { return mhMainWnd; }
	//��ȡ�����
	float AspectRation()const { return static_cast<float>(mClientWidth) / mClientHeight; }
	//Windows����Ϣѭ��
	int Run();
	//���ڳ�ʼ��
	virtual bool Initialize();
	//ʵ��Ӧ�ó���ס���ڵĴ��ڹ��̺���
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnResize();

protected:
	//��ʼ��������
	bool InitMainWindow();

	//���������������Ϣ�Ĵ���
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	virtual void OnKeyboardInput();
	void CalculateFrameStats();

protected:
	static WindowsProcess * mApp;
	HINSTANCE mhAppInst = nullptr;//Ӧ�ó���ʵ�����
	HWND mhMainWnd = nullptr;//�����ھ��
	HWND mhChildWnd = nullptr;
	bool mAppPaused = false;//Ӧ�ó����Ƿ���ͣ
	bool mMinimized = false;//Ӧ�ó����Ƿ���С��
	bool mMaximized = false;//Ӧ�ó����Ƿ����
	bool mResizing = false;//��С�������Ƿ��ܵ���ק

	std::wstring mMainWndCaption = L"SamleRndering";

	GameTimer mTimer;//���ڼ�¼ "delta-time" ����Ϸ��ʱ��

	int mClientWidth = 1000;
	int mClientHeight = 750;

	int mWindowWith = 1000;
	int mWindowHeight = 750;
	//D3D��Ⱦ���
	std::shared_ptr<RenderBase> render;
	POINT mLastMousePos;
};