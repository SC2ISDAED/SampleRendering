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
	//ֻ������һ��App
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
	//�����ڱ�����������ǻ״̬ʱ��WM_ACTIVATE�ᷢ��
//��������ʱ����mAppPausedΪ false ��Ϊ�ǻ״̬ʱ����Ϊtrue
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
	//	�����ڵ�����Сʱ����ϣ������ͼ��������������ÿһ�θ��Ĵ��ڴ�С��
				//��Ҫ���û������Ĵ�С�빤�������η�Χ�Ĵ�С
		 		mClientWidth = LOWORD(lParam);
		 		mClientHeight = HIWORD(lParam);
				if (!render->DeviceIsNull())
				{
					if (wParam == SIZE_MINIMIZED)
					{
						//��������С��ʱ
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
						//������С����ԭ
						if (mMinimized)
						{
							mAppPaused = false;
							mMinimized = false;
							OnResize();
						}
						//������󻯸�ԭ
						else if (mMaximized)
						{
							mAppPaused = false;
							mMaximized = false;
							OnResize();
						}
						else if (mResizing)
						{
							//����û�������ק��С�������Ⲣ����������buffers��
							//��Ϊ���û�������קʱ����һֱ���� WM_SIZE��Ϣ������
							//������������ʱ���û����С���ͻᵼ�µ㻯���Һ���
							//��������ʹ��WM_EXITSIZEMOVE message����
						}
						else
						{
							OnResize();
						}
					}
				}
				return 0;
	case WM_ENTERSIZEMOVE:
		//�����Ϣʱ�û� ---ץȡ������------ʱ����
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;
	case WM_EXITSIZEMOVE:
		//�����Ϣʱ�û�,---�ͷŵ�����---,ʱ����
		//�ڴ˴������µĴ��ڴ�С������ض���
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;
	case WM_DESTROY:
		//--------��������-----------------
		PostQuitMessage(0);
		return 0;
	case WM_MENUCHAR:
		//��ĳһ�˵����ڼ�������û����µĲ��� mnemonic key ������ acceleratorkey 
		//���� alt-enter ʱ����������
		return MAKELRESULT(0, MNC_CLOSE);
	case WM_GETMINMAXINFO:
		//��ֹ���ڱ�ù�С
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;
		//���溯���Ǵ��������Ӧ����
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
	//��дWNDCLASS �ṹ����Ҫ������
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
	//ע��һ��WNDCLASS ��һ��ʵ��
	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}
	//���������������ߴ�ļ��㴰�ڳ����εĳߴ�
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
	//����������ÿ���ƽ��֡����Ҳ������ÿһ֡��ƽ����Ⱦʱ��
	frameCnt++;
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt;// fps = frameCnt / t����������ȡt=1s
		float mspf = 1000.0f / fps;//ת���Ժ���Ϊ��λ
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
		//�����Ա���һ��ƽ��ֵ
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

