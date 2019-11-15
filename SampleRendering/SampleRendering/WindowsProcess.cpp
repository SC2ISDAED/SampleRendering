#include "WindowsProcess.h"
#include "windowsx.h"
#include <memory.h>
#include "RenderBase.h"

using namespace std;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam,LPARAM lParam)
{
	return WindowsProcess::GetAPP()->MsgProc(hwnd, msg, wParam, lParam);
}

WindowsProcess *WindowsProcess::mApp = nullptr;

WindowsProcess::WindowsProcess(HINSTANCE hInstance):mhAppInst(hInstance)
{
	//只允许构造一个App
	assert(mApp == nullptr);
	mApp = this;
	render = std::make_shared<RenderBase>(mClientWidth, mClientHeight);
	
}

WindowsProcess * WindowsProcess::GetAPP()
{
	return mApp;
}

int WindowsProcess::Run()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			mTimer.Tick();

			if (!mAppPaused)
			{
				CalculateFrameStats();
				render->Update(mTimer);
				render->Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool WindowsProcess::Initialize()
{

	if (!InitMainWindow())
	{
		return false;
	}
	render->SetMainWnd(mhMainWnd);
	if (!render->initDirect3D())
	{
		return false;
	}
	OnResize();

	return true;
}

LRESULT WindowsProcess::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//当窗口被被激活或进入非活动状态时，WM_ACTIVATE会发送
//当被激活时设置mAppPaused为 false 变为非活动状态时设置为true
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;
	case WM_SIZE:
	//	当窗口调整大小时，不希望发生图像拉伸现象，所以每一次更改窗口大小，
				//都要更该缓冲区的大小与工作区矩形范围的大小
		 		mClientWidth = LOWORD(lParam);
		 		mClientHeight = HIWORD(lParam);
				if (!render->DeviceIsNull())
				{
					if (wParam == SIZE_MINIMIZED)
					{
						//当窗口最小化时
						mAppPaused = true;
						mMinimized = true;
						mMaximized = false;
					}
					else if (wParam == SIZE_MAXIMIZED)
					{
						mAppPaused = false;
						mMinimized = false;
						mMaximized = true;
						OnResize();
					}
					else if (wParam == SIZE_RESTORED)
					{
						//当从最小化复原
						if (mMinimized)
						{
							mAppPaused = false;
							mMinimized = false;
							OnResize();
						}
						//当从最大化复原
						else if (mMaximized)
						{
							mAppPaused = false;
							mMaximized = false;
							OnResize();
						}
						else if (mResizing)
						{
							//如果用户正在拖拽大小条，在这并不重新设置buffers，
							//因为当用户持续拖拽时，会一直发送 WM_SIZE信息到窗口
							//如果在这里调整时重置缓冲大小，就会导致点化而且很慢
							//所以我们使用WM_EXITSIZEMOVE message处理。
						}
						else
						{
							OnResize();
						}
					}
				}
				return 0;
	case WM_ENTERSIZEMOVE:
		//这个消息时用户 ---抓取调整栏------时发出
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;
	case WM_EXITSIZEMOVE:
		//这个消息时用户,---释放调整栏---,时发出
		//在此处根据新的窗口大小重置相关对象
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;
	case WM_DESTROY:
		//--------窗口销毁-----------------
		PostQuitMessage(0);
		return 0;
	case WM_MENUCHAR:
		//当某一菜单处于激活，并且用户按下的不是 mnemonic key 或者是 acceleratorkey 
		//按下 alt-enter 时发出蜂鸣声
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_GETMINMAXINFO:
		//防止窗口变得过小
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;
		//下面函数是处理鼠标相应函数
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			render->SetMSAA();
		else if ((int)wParam == VK_F1)
			render->SetDebugRendering();
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void WindowsProcess::OnResize()
{
	render->OnResize(mClientWidth, mClientHeight);
}

bool WindowsProcess::InitMainWindow()
{
	WNDCLASS wc;
	//填写WNDCLASS 结构体需要的内容
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";
	//注册一个WNDCLASS 的一个实例
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}
	//基于请求服务区域尺寸的计算窗口长方形的尺寸
	RECT R = { 0,0,mWindowWith,mWindowHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;
	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);
	return true;
}

void WindowsProcess::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
	render->OnMouseDown(x,y,mLastMousePos);
}

void WindowsProcess::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void WindowsProcess::OnMouseMove(WPARAM btnState, int x, int y)
{

	if ((btnState & MK_LBUTTON) != 0)
	{
		render->OnMouseMove(x, y, mLastMousePos);
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void WindowsProcess::OnKeyboardInput()
{

}

void WindowsProcess::CalculateFrameStats()
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;
	//不仅计算了每秒的平均帧数，也计算了每一帧的平均渲染时间
	frameCnt++;
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt;// fps = frameCnt / t，这里我们取t=1s
		float mspf = 1000.0f / fps;//转到以毫秒为单位
		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);
		wstring locatonx= to_wstring(render->mCamera.mLocation.x);
		wstring locatony = to_wstring(render->mCamera.mLocation.y);
		wstring locatonz = to_wstring(render->mCamera.mLocation.z);
		wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr+
			L"x: " + locatonx+
			L"y: " + locatony+
			L"z: " + locatonz;
		SetWindowText(mhMainWnd, windowText.c_str());
		//重置以便下一组平均值
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

