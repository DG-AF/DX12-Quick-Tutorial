
// (1) InitWindow：做一个窗口

#include<Windows.h>		// Windows 窗口编程核心头文件
#include<d3d12.h>		// DX12 核心头文件
#include<dxgi1_6.h>		// DXGI 头文件，用于管理与 DX12 相关联的其他必要设备，如 DXGI 工厂和 交换链

#include<wrl.h>			// COM 组件模板库，方便写 DX12 和 DXGI 相关的接口

#pragma comment(lib,"d3d12.lib")	// 链接 DX12 核心 DLL
#pragma comment(lib,"dxgi.lib")		// 链接 DXGI DLL
#pragma comment(lib,"dxguid.lib")	// 链接 DXGI 必要的设备 GUID

using namespace Microsoft;
using namespace Microsoft::WRL;		// 使用 wrl.h 里面的命名空间


// DX12 引擎
class DX12Engine
{
private:

	int WindowWidth = 640;		// 窗口宽度
	int WindowHeight = 480;		// 窗口高度
	HWND m_hwnd;				// 窗口句柄

public:

	// 初始化窗口
	void InitWindow(HINSTANCE hins)
	{

		WNDCLASS wc = {};					// 用于记录窗口类信息的结构体
		wc.hInstance = hins;				// 窗口类需要一个应用程序的实例句柄 hinstance
		wc.lpfnWndProc = CallBackFunc;		// 窗口类需要一个回调函数，用于处理窗口产生的消息
		wc.lpszClassName = L"DX12 Game";	// 窗口类的名称

		RegisterClass(&wc);					// 注册窗口类，将窗口类录入到操作系统中

		// 使用上文的窗口类创建窗口
		m_hwnd = CreateWindow(wc.lpszClassName, L"DX12画窗口", WS_SYSMENU | WS_OVERLAPPED,
			10, 10, WindowWidth, WindowHeight,
			NULL, NULL, hins, NULL);

		// 因为指定了窗口大小不可变的 WS_SYSMENU 和 WS_OVERLAPPED，应用不会自动显示窗口，需要使用 ShowWindow 强制显示窗口
		ShowWindow(m_hwnd, SW_SHOW);
	}

	// 渲染循环
	void RenderLoop()
	{
		bool isExit = false;	// 是否退出
		MSG msg = {};			// 消息结构体

		while (isExit != true)
		{
			// 查看消息队列是否有消息，如果有就获取。 PM_REMOVE 表示获取完消息，就立刻将该消息从消息队列中移除
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				// 如果程序没有收到退出消息，就向操作系统发出派发消息的命令
				if (msg.message != WM_QUIT)
				{
					TranslateMessage(&msg);					// 翻译消息，将虚拟按键值转换为对应的 ASCII 码 (后文会讲)
					DispatchMessage(&msg);					// 派发消息，通知操作系统调用回调函数处理消息
				}
				else
				{
					isExit = true;							// 收到退出消息，就退出消息循环
				}
			}
		}
	}

	// 回调函数
	static LRESULT CALLBACK CallBackFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// 用 switch 将第二个参数分流，每个 case 分别对应一个窗口消息
		switch (msg)
		{
		case WM_DESTROY:			// 窗口被销毁 (当按下右上角 X 关闭窗口时)
			PostQuitMessage(0);		// 向操作系统发出退出请求 (WM_QUIT)，结束消息循环
			break;

		// 如果接收到其他消息，直接默认返回整个窗口
		default: return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return 0;	// 注意这里！
	}

	// 运行窗口
	static void Run(HINSTANCE hins)
	{
		DX12Engine engine;
		engine.InitWindow(hins);
		engine.RenderLoop();
	}
};


// 主函数
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}

