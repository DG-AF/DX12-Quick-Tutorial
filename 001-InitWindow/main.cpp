
// (1) InitWindow����һ������

#include<Windows.h>		// Windows ���ڱ�̺���ͷ�ļ�
#include<d3d12.h>		// DX12 ����ͷ�ļ�
#include<dxgi1_6.h>		// DXGI ͷ�ļ������ڹ����� DX12 �������������Ҫ�豸���� DXGI ������ ������

#include<wrl.h>			// COM ���ģ��⣬����д DX12 �� DXGI ��صĽӿ�

#pragma comment(lib,"d3d12.lib")	// ���� DX12 ���� DLL
#pragma comment(lib,"dxgi.lib")		// ���� DXGI DLL
#pragma comment(lib,"dxguid.lib")	// ���� DXGI ��Ҫ���豸 GUID

using namespace Microsoft;
using namespace Microsoft::WRL;		// ʹ�� wrl.h ����������ռ�


// DX12 ����
class DX12Engine
{
private:

	int WindowWidth = 640;		// ���ڿ��
	int WindowHeight = 480;		// ���ڸ߶�
	HWND m_hwnd;				// ���ھ��

public:

	// ��ʼ������
	void InitWindow(HINSTANCE hins)
	{

		WNDCLASS wc = {};					// ���ڼ�¼��������Ϣ�Ľṹ��
		wc.hInstance = hins;				// ��������Ҫһ��Ӧ�ó����ʵ����� hinstance
		wc.lpfnWndProc = CallBackFunc;		// ��������Ҫһ���ص����������ڴ����ڲ�������Ϣ
		wc.lpszClassName = L"DX12 Game";	// �����������

		RegisterClass(&wc);					// ע�ᴰ���࣬��������¼�뵽����ϵͳ��

		// ʹ�����ĵĴ����ഴ������
		m_hwnd = CreateWindow(wc.lpszClassName, L"DX12������", WS_SYSMENU | WS_OVERLAPPED,
			10, 10, WindowWidth, WindowHeight,
			NULL, NULL, hins, NULL);

		// ��Ϊָ���˴��ڴ�С���ɱ�� WS_SYSMENU �� WS_OVERLAPPED��Ӧ�ò����Զ���ʾ���ڣ���Ҫʹ�� ShowWindow ǿ����ʾ����
		ShowWindow(m_hwnd, SW_SHOW);
	}

	// ��Ⱦѭ��
	void RenderLoop()
	{
		bool isExit = false;	// �Ƿ��˳�
		MSG msg = {};			// ��Ϣ�ṹ��

		while (isExit != true)
		{
			// �鿴��Ϣ�����Ƿ�����Ϣ������оͻ�ȡ�� PM_REMOVE ��ʾ��ȡ����Ϣ�������̽�����Ϣ����Ϣ�������Ƴ�
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				// �������û���յ��˳���Ϣ���������ϵͳ�����ɷ���Ϣ������
				if (msg.message != WM_QUIT)
				{
					TranslateMessage(&msg);					// ������Ϣ�������ⰴ��ֵת��Ϊ��Ӧ�� ASCII �� (���Ļὲ)
					DispatchMessage(&msg);					// �ɷ���Ϣ��֪ͨ����ϵͳ���ûص�����������Ϣ
				}
				else
				{
					isExit = true;							// �յ��˳���Ϣ�����˳���Ϣѭ��
				}
			}
		}
	}

	// �ص�����
	static LRESULT CALLBACK CallBackFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// �� switch ���ڶ�������������ÿ�� case �ֱ��Ӧһ��������Ϣ
		switch (msg)
		{
		case WM_DESTROY:			// ���ڱ����� (���������Ͻ� X �رմ���ʱ)
			PostQuitMessage(0);		// �����ϵͳ�����˳����� (WM_QUIT)��������Ϣѭ��
			break;

		// ������յ�������Ϣ��ֱ��Ĭ�Ϸ�����������
		default: return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return 0;	// ע�����
	}

	// ���д���
	static void Run(HINSTANCE hins)
	{
		DX12Engine engine;
		engine.InitWindow(hins);
		engine.RenderLoop();
	}
};


// ������
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}

