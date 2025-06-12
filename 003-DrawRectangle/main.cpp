
// (3) DrawRectangle���� DirectX 12 ��һ������

#include<Windows.h>			// Windows ���ڱ�̺���ͷ�ļ�
#include<d3d12.h>			// DX12 ����ͷ�ļ�
#include<dxgi1_6.h>			// DXGI ͷ�ļ������ڹ����� DX12 �������������Ҫ�豸���� DXGI ������ ������
#include<DirectXColors.h>	// DirectX ��ɫ��
#include<DirectXMath.h>		// DirectX ��ѧ��
#include<d3dcompiler.h>		// DirectX Shader ��ɫ�������

#include<wrl.h>				// COM ���ģ��⣬����д DX12 �� DXGI ��صĽӿ�

#pragma comment(lib,"d3d12.lib")		// ���� DX12 ���� DLL
#pragma comment(lib,"dxgi.lib")			// ���� DXGI DLL
#pragma comment(lib,"dxguid.lib")		// ���� DXGI ��Ҫ���豸 GUID
#pragma comment(lib,"d3dcompiler.lib")	// ���� DX12 ��Ҫ����ɫ������ DLL

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


	ComPtr<ID3D12RootSignature> m_RootSignature;		// ��ǩ��
	ComPtr<ID3D12PipelineState> m_PipelineStateObject;	// ��Ⱦ����״̬


	ComPtr<ID3D12Resource> m_VertexResource;			// ������Դ
	struct VERTEX										// �������ݽṹ��
	{
		XMFLOAT4 position;								// ����λ��
		XMFLOAT4 color;									// ������ɫ
	};
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;			// ���㻺��������

	// �ӿ�
	D3D12_VIEWPORT viewPort = D3D12_VIEWPORT{ 0, 0, float(WindowWidth), float(WindowHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	// �ü�����
	D3D12_RECT ScissorRect = D3D12_RECT{ 0, 0, WindowWidth, WindowHeight };

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
					DXGI_ADAPTER_DESC1 adap = {};
					m_DXGIAdapter->GetDesc1(&adap);
					OutputDebugStringW(adap.Description);
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

	// ������ǩ��
	void CreateRootSignature()
	{
		ComPtr<ID3DBlob> SignatureBlob;			// ��ǩ���ֽ���
		ComPtr<ID3DBlob> ErrorBlob;				// �����ֽ��룬��ǩ������ʧ��ʱ�� OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); ���Ի�ȡ������Ϣ

		D3D12_ROOT_SIGNATURE_DESC rootsignatureDesc = {};	// ��ǩ����Ϣ�ṹ��
		rootsignatureDesc.NumParameters = 0;				// ����������������������ʱû����ԴҪ�󶨵���ɫ���ϣ��� 0
		rootsignatureDesc.pParameters = nullptr;			// ������ָ�룬����������ʱû����ԴҪ�󶨵���ɫ���ϣ��� nullptr
		rootsignatureDesc.NumStaticSamplers = 0;			// ��̬������������������ʱ�ò������� 0
		rootsignatureDesc.pStaticSamplers = nullptr;		// ��̬������ָ�룬������ʱ�ò������� nullptr
		// ��ǩ����־������������Ⱦ���߲�ͬ�׶��µ��������״̬��ע���������Ҫ�� IA �׶����붥�����ݣ�����Ҫͨ����ǩ����������Ⱦ��������� IA �׶ζ�������
		rootsignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		
		// �����ǩ�����ø�ǩ���ȱ���� GPU �ɶ��Ķ������ֽ���
		D3D12SerializeRootSignature(&rootsignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &SignatureBlob, &ErrorBlob);
		if (ErrorBlob)		// �����ǩ���������ErrorBlob �����ṩ������Ϣ
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// ������������ֽ��봴����ǩ������
		m_D3D12Device->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
	}

	// ������Ⱦ����״̬���� (Pipeline State Object, PSO)
	void CreatePSO()
	{
		// PSO ��Ϣ�ṹ��
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

		// Input Assembler ����װ��׶�
		D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};			// ������ʽ��Ϣ�ṹ��
		D3D12_INPUT_ELEMENT_DESC InputElementDesc[2] = {};		// ����Ԫ����Ϣ�ṹ������

		InputElementDesc[0].SemanticName = "POSITION";					// Ҫê��������
		InputElementDesc[0].SemanticIndex = 0;							// ����������Ŀǰ������ 0 ����
		InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;	// �����ʽ
		InputElementDesc[0].InputSlot = 0;								// ����۱�ţ�Ŀǰ������ 0 ����
		InputElementDesc[0].AlignedByteOffset = 0;						// ��������е�ƫ��
		// ���������ͣ�һ�������������õ� D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA �𶥵�������,����һ�ֽ���ʵ����������������ѧ
		InputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[0].InstanceDataStepRate = 0;					// ʵ�����ݲ����ʣ�Ŀǰ����û���õ�ʵ�������� 0


		InputElementDesc[1].SemanticName = "COLOR";											// Ҫê��������
		InputElementDesc[1].SemanticIndex = 0;												// ��������
		InputElementDesc[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;						// �����ʽ
		InputElementDesc[1].InputSlot = 0;													// ����۱��
		// ��������е�ƫ�ƣ���Ϊ position �� color ��ͬһ�����(0�������)
		// position �� float4���� 4 �� float ��ÿ�� float ռ 4 ���ֽڣ�����Ҫƫ�� 4*4=16 ���ֽڣ���������ȷ�� color ������λ�ã���Ȼװ���ʱ��Ḳ��ԭ�� position ������
		InputElementDesc[1].AlignedByteOffset = 16;											// ��������е�ƫ��
		InputElementDesc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;	// ����������
		InputElementDesc[1].InstanceDataStepRate = 0;										// ʵ�����ݲ�����


		InputLayoutDesc.NumElements = 2;						// ����Ԫ�ظ���
		InputLayoutDesc.pInputElementDescs = InputElementDesc;	// ����Ԫ�ؽṹ������ָ��
		PSODesc.InputLayout = InputLayoutDesc;					// ������Ⱦ���� IA �׶ε�������ʽ




		ComPtr<ID3DBlob> VertexShaderBlob;		// ������ɫ���������ֽ���
		ComPtr<ID3DBlob> PixelShaderBlob;		// ������ɫ���������ֽ���
		ComPtr<ID3DBlob> ErrorBlob;				// �����ֽ��룬��ǩ������ʧ��ʱ�� OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); ���Ի�ȡ������Ϣ

		// ���붥����ɫ�� Vertex Shader
		D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", NULL, NULL, &VertexShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// �����ɫ���������ErrorBlob �����ṩ������Ϣ
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// ����������ɫ�� Pixel Shader
		D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", NULL, NULL, &PixelShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// �����ɫ���������ErrorBlob �����ṩ������Ϣ
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		PSODesc.VS.pShaderBytecode = VertexShaderBlob->GetBufferPointer();		// VS �ֽ�������ָ��
		PSODesc.VS.BytecodeLength = VertexShaderBlob->GetBufferSize();			// VS �ֽ������ݳ���
		PSODesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();		// PS �ֽ�������ָ��
		PSODesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();			// PS �ֽ������ݳ���

		// Rasterizer ��դ��
		PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;		// �޳�ģʽ��ָ���Ƿ�������/����/���޳�������ѡ�����޳�
		PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;		// ���ģʽ��ָ���Ƿ�����ɫ/�߿���䣬����ѡ��ɫ���

		// ��һ�����ø�ǩ�������������ǽ���ǩ���� PSO �󶨣�������Ⱦ���ߵ��������״̬
		PSODesc.pRootSignature = m_RootSignature.Get();
		
		// ���û���ͼԪ����������������������
		PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// ������ȾĿ������������ֻ��һ����ȾĿ�� (��ɫ����) ��Ҫ������Ⱦ�������� 1
		PSODesc.NumRenderTargets = 1;
		// ������ȾĿ��ĸ�ʽ������Ҫ�ͽ�����ָ�����ڻ���ĸ�ʽһ�£������ 0 ָ������ȾĿ�������
		PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		// ���û�Ͻ׶� (����ϲ��׶�) �� RGBA ��ɫͨ���Ŀ����͹رգ�D3D12_COLOR_WRITE_ENABLE_ALL ��ʾ RGBA ��ɫͨ��ȫ������
		PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		// ���ò������������������� 1 ����
		PSODesc.SampleDesc.Count = 1;
		// ���ò������룬��������ڶ��ز����ģ�����ֱ����ȫ���� (UINT_MAX�����ǽ� UINT ���еı���λȫ�����Ϊ 1) ����
		PSODesc.SampleMask = UINT_MAX;

		// ���մ��� PSO ����
		m_D3D12Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_PipelineStateObject));
	}

	// ����������Դ
	void CreateVertexResource()
	{
		// CPU ���ٻ����ϵĶ�����Ϣ���飬ע������Ķ������궼�� NDC �ռ�����
		VERTEX vertexs[6] =
		{
			{{-0.75f, 0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Blue)},
			{{0.75f, 0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Yellow)},
			{{0.75f, -0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Red)},
			{{-0.75f, 0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Blue)},
			{{0.75f, -0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Red)},
			{{-0.75f, -0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Green)}
		};


		D3D12_RESOURCE_DESC VertexDesc = {};						// D3D12Resource ��Ϣ�ṹ��
		VertexDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// ��Դ���ͣ��ϴ��ѵ���Դ���Ͷ��� buffer ����
		VertexDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// ��Դ���֣�ָ����Դ�Ĵ洢��ʽ���ϴ��ѵ���Դ���� row major ���д洢
		VertexDesc.Width = sizeof(vertexs);							// ��Դ��ȣ��ϴ��ѵ���Դ�������Դ���ܴ�С
		VertexDesc.Height = 1;										// ��Դ�߶ȣ��ϴ��ѽ����Ǵ���������Դ�ģ����Ը߶ȱ���Ϊ 1
		VertexDesc.Format = DXGI_FORMAT_UNKNOWN;					// ��Դ��ʽ���ϴ�����Դ�ĸ�ʽ����Ϊ UNKNOWN
		VertexDesc.DepthOrArraySize = 1;							// ��Դ��ȣ������������������� 3D ����ģ��ϴ�����Դ����Ϊ 1
		VertexDesc.MipLevels = 1;									// Mipmap �ȼ����������������ģ��ϴ�����Դ����Ϊ 1
		VertexDesc.SampleDesc.Count = 1;							// ��Դ���������������� 1 ����
		
		// �ϴ������ԵĽṹ�壬�ϴ���λ�� CPU �� GPU �Ĺ����ڴ�
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// ������Դ��CreateCommittedResource ��Ϊ��Դ�Զ�����һ���ȴ�С����ʽ�ѣ������ʽ�ѵ�����Ȩ�ɲ���ϵͳ���������߲��ɿ���
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE,
			&VertexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexResource));

		

		// ���ڴ�����Դ��ָ��
		BYTE* TransferPointer = nullptr;
		// Map ��ʼӳ�䣬Map ������õ���� D3D12Resource �ĵ�ַ (�ڹ����ڴ���)�����ݸ�ָ�룬�������Ǿ���ͨ�� memcpy ��������������
		m_VertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		// �� CPU ���ٻ����ϵĶ������� ���Ƶ� �����ڴ��ϵ� D3D12Resource ��CPU ���ٻ��� -> �����ڴ�
		memcpy(TransferPointer, vertexs, sizeof(vertexs));
		// Unmap ����ӳ�䣬D3D12Resource ���ֻ��״̬���������ܼ��� GPU �ķ���
		m_VertexResource->Unmap(0, nullptr);


		// ��д VertexBufferView VBV ���㻺������������������� D3D12Resource���� GPU ֪������һ�����㻺��
		VertexBufferView.BufferLocation = m_VertexResource->GetGPUVirtualAddress();		// ���㻺����Դ�ĵ�ַ
		VertexBufferView.SizeInBytes = sizeof(vertexs);									// �������㻺����ܴ�С
		VertexBufferView.StrideInBytes = sizeof(VERTEX);								// ÿ������Ԫ�صĴ�С (����)
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

		// �ڶ������ø�ǩ�����������ý����� ��Ⱦ���߰󶨵ĸ�ǩ�� �� ����ĸ�ǩ�� �Ƿ�ƥ��
		// �Լ���ǩ��ָ������Դ�Ƿ���ȷ�󶨣������Ϻ����м򵥵�ӳ��
		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		// ������Ⱦ����״̬������������ m_CommandList->Reset() ��ʱ��ֱ���ڵڶ����������� PSO
		m_CommandList->SetPipelineState(m_PipelineStateObject.Get());

		// �����ӿ� (��դ���׶�)�����ڹ�դ�������Ļӳ��
		m_CommandList->RSSetViewports(1, &viewPort);
		// ���òü����� (��դ���׶�)
		m_CommandList->RSSetScissorRects(1, &ScissorRect);

		// �� RTV ���������ȾĿ��
		m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, nullptr);

		// ��յ�ǰ��ȾĿ��ı���Ϊ����ɫ
		m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);

		// ����ͼԪ���� (����װ��׶�)���������������������б�
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// ���� VBV ���㻺�������� (����װ��׶�) 
		m_CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);

		// Draw Call! ���ƾ���
		m_CommandList->DrawInstanced(6, 1, 0, 0);


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
				Sleep(10);
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

		engine.CreateRootSignature();
		engine.CreatePSO();
		engine.CreateVertexResource();

		engine.RenderLoop();
	}
};


// ������
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}

