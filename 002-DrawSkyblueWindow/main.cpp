
// (2) DrawSkyblueWindow���� DirectX 12 ��Ⱦһ������ɫ����

#include<Windows.h>			// Windows ���ڱ�̺���ͷ�ļ�
#include<d3d12.h>			// DX12 ����ͷ�ļ�
#include<dxgi1_6.h>			// DXGI ͷ�ļ������ڹ����� DX12 �������������Ҫ�豸���� DXGI ������ ������
#include<DirectXColors.h>	// DirectX ��ɫ��

#include<wrl.h>				// COM ���ģ��⣬����д DX12 �� DXGI ��صĽӿ�

#pragma comment(lib,"d3d12.lib")	// ���� DX12 ���� DLL
#pragma comment(lib,"dxgi.lib")		// ���� DXGI DLL
#pragma comment(lib,"dxguid.lib")	// ���� DXGI ��Ҫ���豸 GUID

using namespace Microsoft;
using namespace Microsoft::WRL;		// ʹ�� wrl.h ����������ռ䣬������Ҫ�õ������ Microsoft::WRL::ComPtr COM����ָ��
using namespace DirectX;			// DirectX �����ռ�


// DX12 ����
class DX12Engine
{
private:

	int WindowWidth = 640;		// ���ڿ��
	int WindowHeight = 480;		// ���ڸ߶�
	HWND m_hwnd;				// ���ھ��

	ComPtr<ID3D12Debug> m_D3D12DebugDevice;				// D3D12 ���Բ��豸
	UINT m_DXGICreateFactoryFlag = NULL;				// ���� DXGI ����ʱ��Ҫ�õ��ı�־

	ComPtr<IDXGIFactory5> m_DXGIFactory;				// DXGI ����
	ComPtr<IDXGIAdapter1> m_DXGIAdapter;				// ��ʾ������ (�Կ�)
	ComPtr<ID3D12Device4> m_D3D12Device;				// D3D12 �����豸

	ComPtr<ID3D12CommandQueue> m_CommandQueue;			// �������
	ComPtr<ID3D12CommandAllocator> m_CommandAllocator;	// ���������
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;	// �����б�

	ComPtr<IDXGISwapChain3> m_DXGISwapChain;			// DXGI ������
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;				// RTV ��������
	ComPtr<ID3D12Resource> m_RenderTarget[3];			// ��ȾĿ�����飬ÿһ����ȾĿ���Ӧһ�����ڻ�����
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;				// RTV ���������
	UINT RTVDescriptorSize = 0;							// RTV �������Ĵ�С
	UINT FrameIndex = 0;								// ֡��������ʾ��ǰ��Ⱦ�ĵ� i ֡ (�� i ����ȾĿ��)

	ComPtr<ID3D12Fence> m_Fence;						// Χ��
	UINT64 FenceValue = 0;								// ����Χ���ȴ���Χ��ֵ
	HANDLE RenderEvent = NULL;							// GPU ��Ⱦ�¼�
	D3D12_RESOURCE_BARRIER beg_barrier = {};			// ��Ⱦ��ʼ����Դ���ϣ����� -> ��ȾĿ��
	D3D12_RESOURCE_BARRIER end_barrier = {};			// ��Ⱦ��������Դ���ϣ���ȾĿ�� -> ����

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

	// �������Բ�
	void CreateDebugDevice()
	{
		::CoInitialize(nullptr);	// ע�����DX12 �������豸�ӿڶ��ǻ��� COM �ӿڵģ�������Ҫ��ȫ����ʼ��Ϊ nullptr

#if defined(_DEBUG)		// ����� Debug ģʽ�±��룬��ִ������Ĵ���

		// ��ȡ���Բ��豸�ӿ�
		D3D12GetDebugInterface(IID_PPV_ARGS(&m_D3D12DebugDevice));
		// �������Բ�
		m_D3D12DebugDevice->EnableDebugLayer();
		// �������Բ�󣬴��� DXGI ����Ҳ��Ҫ Debug Flag
		m_DXGICreateFactoryFlag = DXGI_CREATE_FACTORY_DEBUG;

#endif
	}

	// �����豸
	bool CreateDevice()
	{
		// ���� DXGI ����
		CreateDXGIFactory2(m_DXGICreateFactoryFlag, IID_PPV_ARGS(&m_DXGIFactory));

		// DX12 ֧�ֵ����й��ܰ汾������Կ������Ҫ֧�� 11.0
		const D3D_FEATURE_LEVEL dx12SupportLevel[] =
		{
			D3D_FEATURE_LEVEL_12_2,		// 12.2
			D3D_FEATURE_LEVEL_12_1,		// 12.1
			D3D_FEATURE_LEVEL_12_0,		// 12.0
			D3D_FEATURE_LEVEL_11_1,		// 11.1
			D3D_FEATURE_LEVEL_11_0		// 11.0
		};


		// �� EnumAdapters1 �ȱ��������ϵ�ÿһ���Կ�
		// ÿ�ε��� EnumAdapters1 �ҵ��Կ����Զ����� DXGIAdapter �ӿڣ������� S_OK
		// �Ҳ����Կ��᷵�� ERROR_NOT_FOUND

		for (UINT i = 0; m_DXGIFactory->EnumAdapters1(i, &m_DXGIAdapter) != ERROR_NOT_FOUND; i++)
		{
			// �ҵ��Կ����ʹ��� D3D12 �豸���Ӹߵ��ͱ������й��ܰ汾�������ɹ�������
			for (const auto& level : dx12SupportLevel)
			{
				// ���� D3D12 ���Ĳ��豸�������ɹ��ͷ��� true
				if (SUCCEEDED(D3D12CreateDevice(m_DXGIAdapter.Get(), level, IID_PPV_ARGS(&m_D3D12Device))))
				{
					return true;
				}
			}
		}

		// ����Ҳ����κ���֧�� DX12 ���Կ������˳�����
		if (m_D3D12Device == nullptr)
		{
			MessageBox(NULL, L"�Ҳ����κ���֧�� DX12 ���Կ��������������ϵ�Ӳ����", L"����", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	// ��������������
	void CreateCommandComponents()
	{
		// ������Ϣ�ṹ�壬����ֻ��Ҫ����е����� type ������
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		// D3D12_COMMAND_LIST_TYPE_DIRECT ��ʾ�����ֱ�ӷŽ������������������
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		// �����������
		m_D3D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));

		// ������������������������ǿ����ڴ棬�洢�����б��ϵ����ע����������Ҫһ��
		m_D3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator));

		// ����ͼ�������б�ע����������Ҫһ��
		m_D3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(),
			nullptr, IID_PPV_ARGS(&m_CommandList));

		// �����б���ʱ���� Record ¼��״̬��������Ҫ�ر������������ĵ� Reset ���ܳɹ�
		m_CommandList->Close();
	}

	// ������ȾĿ�꣬����ȾĿ������Ϊ����
	void CreateRenderTarget()
	{
		// ���� RTV �������� (Render Target View����ȾĿ��������)
		D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
		RTVHeapDesc.NumDescriptors = 3;							// ��ȾĿ�������
		RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// �������ѵ����ͣ�RTV
		// ����һ�� RTV �������ѣ������ɹ��󣬻��Զ������������������ڴ�
		m_D3D12Device->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&m_RTVHeap));


		// ���� DXGI �����������ڽ����ڻ���������ȾĿ���
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.BufferCount = 3;								// ����������
		swapchainDesc.Width = WindowWidth;							// ������ (����) ���
		swapchainDesc.Height = WindowHeight;						// ������ (����) �߶�
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// ��������ʽ��ָ��������ÿ�����صĴ�С
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// ���������ͣ��� FILP �� BITBLT ��������
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// ����������;�������ʾ�ѻ�����������ȾĿ������
		swapchainDesc.SampleDesc.Count = 1;							// ���������ز�������

		// ��ʱ�Ͱ汾�������ӿڣ����ڴ����߰汾����������Ϊ���ĵ� CreateSwapChainForHwnd ����ֱ�����ڴ����߰汾�ӿ�
		ComPtr<IDXGISwapChain1> _temp_swapchain;

		// ����������������������ȾĿ���
		// ע�⣺��������Ҫ�󶨵����������ˢ�£����Ե�һ������Ҫ��������У�����ᴴ��ʧ�ܣ�
		m_DXGIFactory->CreateSwapChainForHwnd(m_CommandQueue.Get(), m_hwnd,
			&swapchainDesc, nullptr, nullptr, &_temp_swapchain);

		// ͨ�� As ���������Ͱ汾�ӿڵ���Ϣ���ݸ��߰汾�ӿ�
		_temp_swapchain.As(&m_DXGISwapChain);

		
		// �����꽻���������ǻ���Ҫ�� RTV ������ ָ�� ��ȾĿ��
		// ��Ϊ ID3D12Resource ������ֻ��һ�����ݣ�������û�ж������÷���˵��
		// ����Ҫ�ó���֪�����������һ����ȾĿ�꣬�͵ô�����ʹ�� RTV ������

		// ��ȡ RTV ��ָ�����������ľ��
		RTVHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
		// ��ȡ RTV �������Ĵ�С
		RTVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (UINT i = 0; i < 3; i++)
		{
			// �ӽ������л�ȡ�� i �����ڻ��壬������ i �� RenderTarget ��ȾĿ��
			m_DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTarget[i]));

			// ���� RTV ������������ȾĿ��󶨵���������
			m_D3D12Device->CreateRenderTargetView(m_RenderTarget[i].Get(), nullptr, RTVHandle);

			// ƫ�Ƶ���һ�� RTV ���
			RTVHandle.ptr += RTVDescriptorSize;
		}
	}

	// ����Χ������Դ���ϣ����� CPU-GPU ��ͬ��
	void CreateFenceAndBarrier()
	{
		// ���� CPU �ϵĵȴ��¼�
		RenderEvent = CreateEvent(nullptr, false, true, nullptr);
		
		// ����Χ�����趨��ʼֵΪ 0
		m_D3D12Device->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
		

		// ������Դ����
		// beg_barrier ��ʼ���ϣ�Present ����״̬ -> Render Target ��ȾĿ��״̬
		beg_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;					// ָ������Ϊת������		
		beg_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		beg_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		// end_barrier ��ֹ���ϣ�Render Target ��ȾĿ��״̬ -> Present ����״̬
		end_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		end_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		end_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	}

	// ��Ⱦ
	void Render()
	{
		// ��ȡ RTV ���׾��
		RTVHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
		// ��ȡ��ǰ��Ⱦ�ĺ�̨�������
		FrameIndex = m_DXGISwapChain->GetCurrentBackBufferIndex();
		// ƫ�� RTV ������ҵ���Ӧ�� RTV ������
		RTVHandle.ptr += FrameIndex * RTVDescriptorSize;


		// ���������������
		m_CommandAllocator->Reset();
		// �����������б�Close �ر�״̬ -> Record ¼��״̬
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);

		// ����ʼת�����ϵ���Դָ��Ϊ��ǰ��ȾĿ��
		beg_barrier.Transition.pResource = m_RenderTarget[FrameIndex].Get();
		// ������Դ���ϣ�����ȾĿ���� Present ����(ֻ��) ת���� RenderTarget ��ȾĿ��(ֻд)
		m_CommandList->ResourceBarrier(1, &beg_barrier);


		// �� RTV ���������ȾĿ��
		m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, nullptr);

		// ��յ�ǰ��ȾĿ��ı���Ϊ����ɫ
		m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);


		// ����ֹת�����ϵ���Դָ��Ϊ��ǰ��ȾĿ��
		end_barrier.Transition.pResource = m_RenderTarget[FrameIndex].Get();
		// ��ͨ��һ����Դ���ϣ�����ȾĿ���� RenderTarget ��ȾĿ��(ֻд) ת���� Present ����(ֻ��)
		m_CommandList->ResourceBarrier(1, &end_barrier);
		
		// �ر������б�Record ¼��״̬ -> Close �ر�״̬�������б�ֻ�йرղſ����ύ
		m_CommandList->Close();

		// ���ڴ��������õ���ʱ ID3D12CommandList ����
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// ִ�����ĵ���Ⱦ���
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);

		// ��������з������������������������뵽��������У��������ִ�е�������ʱ����֪ͨ��������������
		m_DXGISwapChain->Present(1, NULL);




		// ��Χ��Ԥ��ֵ�趨Ϊ��һ֡
		FenceValue++;
		// ��������� (��������� GPU ��) ����Χ��Ԥ��ֵ�����������뵽���������
		// �������ִ�е�������޸�Χ��ֵ����ʾ��Ⱦ����ɣ�"����"Χ��
		m_CommandQueue->Signal(m_Fence.Get(), FenceValue);
		// ����Χ����Ԥ���¼�������Ⱦ���ʱ��Χ����"����"������Ԥ���¼������¼������ź�״̬ת�������ź�״̬
		m_Fence->SetEventOnCompletion(FenceValue, RenderEvent);
	}

	// ��Ⱦѭ��
	void RenderLoop()
	{
		bool isExit = false;	// �Ƿ��˳�
		MSG msg = {};			// ��Ϣ�ṹ��

		while (isExit != true)
		{
			// MsgWaitForMultipleObjects ���ڶ���̵߳��������ȴ�������ֵ�Ǽ����¼� (�߳�) �� ID
			// �����ú����� RenderEvent Ҳ���Զ�����Ϊ���ź�״̬����Ϊ���Ǵ����¼���ʱ��ָ���˵ڶ�������Ϊ false
			DWORD ActiveEvent = ::MsgWaitForMultipleObjects(1, &RenderEvent, false, INFINITE, QS_ALLINPUT);

			switch (ActiveEvent - WAIT_OBJECT_0)
			{
			case 0:				// ActiveEvent �� 0��˵����Ⱦ�¼��Ѿ�����ˣ�������һ����Ⱦ
				Render();
				break;

			case 1:				// ActiveEvent �� 1��˵����Ⱦ�¼�δ��ɣ�CPU ���߳�ͬʱ��������Ϣ����ֹ�������
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
				break;

			case WAIT_TIMEOUT:	// ��Ⱦ��ʱ
				break;
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
		engine.CreateDebugDevice();
		engine.CreateDevice();
		engine.CreateCommandComponents();
		engine.CreateRenderTarget();
		engine.CreateFenceAndBarrier();

		engine.RenderLoop();
	}
};


// ������
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}

