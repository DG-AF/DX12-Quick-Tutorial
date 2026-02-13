
// (3) DrawRectangle：用 DirectX 12 画一个矩形

#include<Windows.h>			// Windows 窗口编程核心头文件
#include<d3d12.h>			// DX12 核心头文件
#include<dxgi1_6.h>			// DXGI 头文件，用于管理与 DX12 相关联的其他必要设备，如 DXGI 工厂和 交换链
#include<DirectXColors.h>	// DirectX 颜色库
#include<DirectXMath.h>		// DirectX 数学库
#include<d3dcompiler.h>		// DirectX Shader 着色器编译库

#include<wrl.h>				// COM 组件模板库，方便写 DX12 和 DXGI 相关的接口

#pragma comment(lib,"d3d12.lib")		// 链接 DX12 核心 DLL
#pragma comment(lib,"dxgi.lib")			// 链接 DXGI DLL
#pragma comment(lib,"dxguid.lib")		// 链接 DXGI 必要的设备 GUID
#pragma comment(lib,"d3dcompiler.lib")	// 链接 DX12 需要的着色器编译 DLL

using namespace Microsoft;
using namespace Microsoft::WRL;		// 使用 wrl.h 里面的命名空间，我们需要用到里面的 Microsoft::WRL::ComPtr COM智能指针
using namespace DirectX;			// DirectX 命名空间


// DX12 引擎
class DX12Engine
{
private:

	int WindowWidth = 640;		// 窗口宽度
	int WindowHeight = 480;		// 窗口高度
	HWND m_hwnd;				// 窗口句柄

	ComPtr<ID3D12Debug> m_D3D12DebugDevice;				// D3D12 调试层设备
	UINT m_DXGICreateFactoryFlag = NULL;				// 创建 DXGI 工厂时需要用到的标志

	ComPtr<IDXGIFactory5> m_DXGIFactory;				// DXGI 工厂
	ComPtr<IDXGIAdapter1> m_DXGIAdapter;				// 显示适配器 (显卡)
	ComPtr<ID3D12Device4> m_D3D12Device;				// D3D12 核心设备

	ComPtr<ID3D12CommandQueue> m_CommandQueue;			// 命令队列
	ComPtr<ID3D12CommandAllocator> m_CommandAllocator;	// 命令分配器
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;	// 命令列表

	ComPtr<IDXGISwapChain3> m_DXGISwapChain;			// DXGI 交换链
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;				// RTV 描述符堆
	ComPtr<ID3D12Resource> m_RenderTarget[3];			// 渲染目标数组，每一副渲染目标对应一个窗口缓冲区
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;				// RTV 描述符句柄
	UINT RTVDescriptorSize = 0;							// RTV 描述符的大小
	UINT FrameIndex = 0;								// 帧索引，表示当前渲染的第 i 帧 (第 i 个渲染目标)

	ComPtr<ID3D12Fence> m_Fence;						// 围栏
	UINT64 FenceValue = 0;								// 用于围栏等待的围栏值
	HANDLE RenderEvent = NULL;							// GPU 渲染事件
	D3D12_RESOURCE_BARRIER beg_barrier = {};			// 渲染开始的资源屏障，呈现 -> 渲染目标
	D3D12_RESOURCE_BARRIER end_barrier = {};			// 渲染结束的资源屏障，渲染目标 -> 呈现


	ComPtr<ID3D12RootSignature> m_RootSignature;		// 根签名
	ComPtr<ID3D12PipelineState> m_PipelineStateObject;	// 渲染管线状态


	ComPtr<ID3D12Resource> m_VertexResource;			// 顶点资源
	struct VERTEX										// 顶点数据结构体
	{
		XMFLOAT4 position;								// 顶点位置
		XMFLOAT4 color;									// 顶点颜色
	};
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;			// 顶点缓冲描述符

	// 视口
	D3D12_VIEWPORT viewPort = D3D12_VIEWPORT{ 0, 0, float(WindowWidth), float(WindowHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	// 裁剪矩形
	D3D12_RECT ScissorRect = D3D12_RECT{ 0, 0, WindowWidth, WindowHeight };

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
		m_hwnd = CreateWindow(wc.lpszClassName, L"DX12画矩形", WS_SYSMENU | WS_OVERLAPPED,
			10, 10, WindowWidth, WindowHeight,
			NULL, NULL, hins, NULL);

		// 因为指定了窗口大小不可变的 WS_SYSMENU 和 WS_OVERLAPPED，应用不会自动显示窗口，需要使用 ShowWindow 强制显示窗口
		ShowWindow(m_hwnd, SW_SHOW);
	}

	// 创建调试层
	void CreateDebugDevice()
	{
		::CoInitialize(nullptr);	// 注意这里！DX12 的所有设备接口都是基于 COM 接口的，我们需要先全部初始化为 nullptr

#if defined(_DEBUG)		// 如果是 Debug 模式下编译，就执行下面的代码

		// 获取调试层设备接口
		D3D12GetDebugInterface(IID_PPV_ARGS(&m_D3D12DebugDevice));
		// 开启调试层
		m_D3D12DebugDevice->EnableDebugLayer();
		// 开启调试层后，创建 DXGI 工厂也需要 Debug Flag
		m_DXGICreateFactoryFlag = DXGI_CREATE_FACTORY_DEBUG;

#endif
	}

	// 创建设备
	bool CreateDevice()
	{
		// 创建 DXGI 工厂
		CreateDXGIFactory2(m_DXGICreateFactoryFlag, IID_PPV_ARGS(&m_DXGIFactory));

		// DX12 支持的所有功能版本，你的显卡最低需要支持 11.0
		const D3D_FEATURE_LEVEL dx12SupportLevel[] =
		{
			D3D_FEATURE_LEVEL_12_2,		// 12.2
			D3D_FEATURE_LEVEL_12_1,		// 12.1
			D3D_FEATURE_LEVEL_12_0,		// 12.0
			D3D_FEATURE_LEVEL_11_1,		// 11.1
			D3D_FEATURE_LEVEL_11_0		// 11.0
		};


		// 用 EnumAdapters1 先遍历电脑上的每一块显卡
		// 每次调用 EnumAdapters1 找到显卡会自动创建 DXGIAdapter 接口，并返回 S_OK
		// 找不到显卡会返回 ERROR_NOT_FOUND

		for (UINT i = 0; m_DXGIFactory->EnumAdapters1(i, &m_DXGIAdapter) != ERROR_NOT_FOUND; i++)
		{
			// 找到显卡，就创建 D3D12 设备，从高到低遍历所有功能版本，创建成功就跳出
			for (const auto& level : dx12SupportLevel)
			{
				// 创建 D3D12 核心层设备，创建成功就返回 true
				if (SUCCEEDED(D3D12CreateDevice(m_DXGIAdapter.Get(), level, IID_PPV_ARGS(&m_D3D12Device))))
				{
					DXGI_ADAPTER_DESC1 adap = {};
					m_DXGIAdapter->GetDesc1(&adap);
					OutputDebugStringW(adap.Description);
					return true;
				}
			}
		}

		// 如果找不到任何能支持 DX12 的显卡，就退出程序
		if (m_D3D12Device == nullptr)
		{
			MessageBox(NULL, L"找不到任何能支持 DX12 的显卡，请升级电脑上的硬件！", L"错误", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	// 创建命令三件套
	void CreateCommandComponents()
	{
		// 队列信息结构体，这里只需要填队列的类型 type 就行了
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		// D3D12_COMMAND_LIST_TYPE_DIRECT 表示将命令都直接放进队列里，不做其他处理
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		// 创建命令队列
		m_D3D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));

		// 创建命令分配器，它的作用是开辟内存，存储命令列表上的命令，注意命令类型要一致
		m_D3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator));

		// 创建图形命令列表，注意命令类型要一致
		m_D3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(),
			nullptr, IID_PPV_ARGS(&m_CommandList));

		// 命令列表创建时处于 Record 录制状态，我们需要关闭它，这样下文的 Reset 才能成功
		m_CommandList->Close();
	}

	// 创建渲染目标，将渲染目标设置为窗口
	void CreateRenderTarget()
	{
		// 创建 RTV 描述符堆 (Render Target View，渲染目标描述符)
		D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
		RTVHeapDesc.NumDescriptors = 3;							// 渲染目标的数量
		RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// 描述符堆的类型：RTV
		// 创建一个 RTV 描述符堆，创建成功后，会自动开辟三个描述符的内存
		m_D3D12Device->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&m_RTVHeap));


		// 创建 DXGI 交换链，用于将窗口缓冲区和渲染目标绑定
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.BufferCount = 3;								// 缓冲区数量
		swapchainDesc.Width = WindowWidth;							// 缓冲区 (窗口) 宽度
		swapchainDesc.Height = WindowHeight;						// 缓冲区 (窗口) 高度
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// 缓冲区格式，指定缓冲区每个像素的大小
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// 交换链类型，有 FILP 和 BITBLT 两种类型
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// 缓冲区的用途，这里表示把缓冲区用作渲染目标的输出
		swapchainDesc.SampleDesc.Count = 1;							// 缓冲区像素采样次数

		// 临时低版本交换链接口，用于创建高版本交换链，因为下文的 CreateSwapChainForHwnd 不能直接用于创建高版本接口
		ComPtr<IDXGISwapChain1> _temp_swapchain;

		// 创建交换链，将窗口与渲染目标绑定
		// 注意：交换链需要绑定到命令队列来刷新，所以第一个参数要填命令队列，不填会创建失败！
		m_DXGIFactory->CreateSwapChainForHwnd(m_CommandQueue.Get(), m_hwnd,
			&swapchainDesc, nullptr, nullptr, &_temp_swapchain);

		// 通过 As 方法，将低版本接口的信息传递给高版本接口
		_temp_swapchain.As(&m_DXGISwapChain);


		// 创建完交换链后，我们还需要令 RTV 描述符 指向 渲染目标
		// 因为 ID3D12Resource 本质上只是一块数据，它本身没有对数据用法的说明
		// 我们要让程序知道这块数据是一个渲染目标，就得创建并使用 RTV 描述符

		// 获取 RTV 堆指向首描述符的句柄
		RTVHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
		// 获取 RTV 描述符的大小
		RTVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (UINT i = 0; i < 3; i++)
		{
			// 从交换链中获取第 i 个窗口缓冲，创建第 i 个 RenderTarget 渲染目标
			m_DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTarget[i]));

			// 创建 RTV 描述符，将渲染目标绑定到描述符上
			m_D3D12Device->CreateRenderTargetView(m_RenderTarget[i].Get(), nullptr, RTVHandle);

			// 偏移到下一个 RTV 句柄
			RTVHandle.ptr += RTVDescriptorSize;
		}
	}

	// 创建围栏和资源屏障，用于 CPU-GPU 的同步
	void CreateFenceAndBarrier()
	{
		// 创建 CPU 上的等待事件
		RenderEvent = CreateEvent(nullptr, false, true, nullptr);

		// 创建围栏，设定初始值为 0
		m_D3D12Device->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));


		// 设置资源屏障
		// beg_barrier 起始屏障：Present 呈现状态 -> Render Target 渲染目标状态
		beg_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;					// 指定类型为转换屏障		
		beg_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		beg_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		// end_barrier 终止屏障：Render Target 渲染目标状态 -> Present 呈现状态
		end_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		end_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		end_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	}

	// 创建根签名
	void CreateRootSignature()
	{
		ComPtr<ID3DBlob> SignatureBlob;			// 根签名字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码，根签名创建失败时用 OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); 可以获取报错信息

		D3D12_ROOT_SIGNATURE_DESC rootsignatureDesc = {};	// 根签名信息结构体
		rootsignatureDesc.NumParameters = 0;				// 根参数数量，我们这里暂时没有资源要绑定到着色器上，填 0
		rootsignatureDesc.pParameters = nullptr;			// 根参数指针，我们这里暂时没有资源要绑定到着色器上，填 nullptr
		rootsignatureDesc.NumStaticSamplers = 0;			// 静态采样器数量，我们暂时用不到，填 0
		rootsignatureDesc.pStaticSamplers = nullptr;		// 静态采样器指针，我们暂时用不到，填 nullptr
		// 根签名标志，可以设置渲染管线不同阶段下的输入参数状态。注意这里！我们要从 IA 阶段输入顶点数据，所以要通过根签名，设置渲染管线允许从 IA 阶段读入数据
		rootsignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		
		// 编译根签名，让根签名先编译成 GPU 可读的二进制字节码
		D3D12SerializeRootSignature(&rootsignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &SignatureBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果根签名编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// 用这个二进制字节码创建根签名对象
		m_D3D12Device->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
	}

	// 创建渲染管线状态对象 (Pipeline State Object, PSO)
	void CreatePSO()
	{
		// PSO 信息结构体
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

		// Input Assembler 输入装配阶段
		D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};			// 输入样式信息结构体
		D3D12_INPUT_ELEMENT_DESC InputElementDesc[2] = {};		// 输入元素信息结构体数组

		InputElementDesc[0].SemanticName = "POSITION";					// 要锚定的语义
		InputElementDesc[0].SemanticIndex = 0;							// 语义索引，目前我们填 0 就行
		InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;	// 输入格式
		InputElementDesc[0].InputSlot = 0;								// 输入槽编号，目前我们填 0 就行
		InputElementDesc[0].AlignedByteOffset = 0;						// 在输入槽中的偏移
		// 输入流类型，一种是我们现在用的 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA 逐顶点输入流,还有一种叫逐实例输入流，后面再学
		InputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[0].InstanceDataStepRate = 0;					// 实例数据步进率，目前我们没有用到实例化，填 0


		InputElementDesc[1].SemanticName = "COLOR";											// 要锚定的语义
		InputElementDesc[1].SemanticIndex = 0;												// 语义索引
		InputElementDesc[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;						// 输入格式
		InputElementDesc[1].InputSlot = 0;													// 输入槽编号
		// 在输入槽中的偏移，因为 position 与 color 在同一输入槽(0号输入槽)
		// position 是 float4，有 4 个 float ，每个 float 占 4 个字节，所以要偏移 4*4=16 个字节，这样才能确定 color 参数的位置，不然装配的时候会覆盖原先 position 的数据
		InputElementDesc[1].AlignedByteOffset = 16;											// 在输入槽中的偏移
		InputElementDesc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;	// 输入流类型
		InputElementDesc[1].InstanceDataStepRate = 0;										// 实例数据步进率


		InputLayoutDesc.NumElements = 2;						// 输入元素个数
		InputLayoutDesc.pInputElementDescs = InputElementDesc;	// 输入元素结构体数组指针
		PSODesc.InputLayout = InputLayoutDesc;					// 设置渲染管线 IA 阶段的输入样式




		ComPtr<ID3DBlob> VertexShaderBlob;		// 顶点着色器二进制字节码
		ComPtr<ID3DBlob> PixelShaderBlob;		// 像素着色器二进制字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码，根签名创建失败时用 OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); 可以获取报错信息

		// 编译顶点着色器 Vertex Shader
		D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", NULL, NULL, &VertexShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// 编译像素着色器 Pixel Shader
		D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", NULL, NULL, &PixelShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		PSODesc.VS.pShaderBytecode = VertexShaderBlob->GetBufferPointer();		// VS 字节码数据指针
		PSODesc.VS.BytecodeLength = VertexShaderBlob->GetBufferSize();			// VS 字节码数据长度
		PSODesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();		// PS 字节码数据指针
		PSODesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();			// PS 字节码数据长度

		// Rasterizer 光栅化
		PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;		// 剔除模式，指定是否开启背面/正面/不剔除，这里选背面剔除
		PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;		// 填充模式，指定是否开启纯色/线框填充，这里选纯色填充

		// 第一次设置根签名！本次设置是将根签名与 PSO 绑定，设置渲染管线的输入参数状态
		PSODesc.pRootSignature = m_RootSignature.Get();
		
		// 设置基本图元，这里我们设置三角形面
		PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// 设置渲染目标数量，我们只有一副渲染目标 (颜色缓冲) 需要进行渲染，所以填 1
		PSODesc.NumRenderTargets = 1;
		// 设置渲染目标的格式，这里要和交换链指定窗口缓冲的格式一致，这里的 0 指的是渲染目标的索引
		PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		// 设置混合阶段 (输出合并阶段) 下 RGBA 颜色通道的开启和关闭，D3D12_COLOR_WRITE_ENABLE_ALL 表示 RGBA 四色通道全部开启
		PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		// 设置采样次数，我们这里填 1 就行
		PSODesc.SampleDesc.Count = 1;
		// 设置采样掩码，这个是用于多重采样的，我们直接填全采样 (UINT_MAX，就是将 UINT 所有的比特位全部填充为 1) 就行
		PSODesc.SampleMask = UINT_MAX;

		// 最终创建 PSO 对象
		m_D3D12Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_PipelineStateObject));
	}

	// 创建顶点资源
	void CreateVertexResource()
	{
		// CPU 高速缓存上的顶点信息数组，注意这里的顶点坐标都是 NDC 空间坐标
		VERTEX vertexs[6] =
		{
			{{-0.75f, 0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Blue)},
			{{0.75f, 0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Yellow)},
			{{0.75f, -0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Red)},
			{{-0.75f, 0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Blue)},
			{{0.75f, -0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Red)},
			{{-0.75f, -0.75f, 0.0f, 1.0f}, XMFLOAT4(Colors::Green)}
		};


		D3D12_RESOURCE_DESC VertexDesc = {};						// D3D12Resource 信息结构体
		VertexDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 资源类型，上传堆的资源类型都是 buffer 缓冲
		VertexDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// 资源布局，指定资源的存储方式，上传堆的资源都是 row major 按行存储
		VertexDesc.Width = sizeof(vertexs);							// 资源宽度，上传堆的资源宽度是资源的总大小
		VertexDesc.Height = 1;										// 资源高度，上传堆仅仅是传递线性资源的，所以高度必须为 1
		VertexDesc.Format = DXGI_FORMAT_UNKNOWN;					// 资源格式，上传堆资源的格式必须为 UNKNOWN
		VertexDesc.DepthOrArraySize = 1;							// 资源深度，这个是用于纹理数组和 3D 纹理的，上传堆资源必须为 1
		VertexDesc.MipLevels = 1;									// Mipmap 等级，这个是用于纹理的，上传堆资源必须为 1
		VertexDesc.SampleDesc.Count = 1;							// 资源采样次数，这里填 1 即可
		
		// 上传堆属性的结构体，上传堆位于 CPU 和 GPU 的共享内存
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建资源，CreateCommittedResource 会为资源自动创建一个等大小的隐式堆，这个隐式堆的所有权由操作系统管理，开发者不可控制
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE,
			&VertexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexResource));

		

		// 用于传递资源的指针
		BYTE* TransferPointer = nullptr;
		// Map 开始映射，Map 方法会得到这个 D3D12Resource 的地址 (在共享内存上)，传递给指针，这样我们就能通过 memcpy 操作复制数据了
		m_VertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		// 将 CPU 高速缓存上的顶点数据 复制到 共享内存上的 D3D12Resource ，CPU 高速缓存 -> 共享内存
		memcpy(TransferPointer, vertexs, sizeof(vertexs));
		// Unmap 结束映射，D3D12Resource 变成只读状态，这样做能加速 GPU 的访问
		m_VertexResource->Unmap(0, nullptr);


		// 填写 VertexBufferView VBV 顶点缓冲描述符，描述上面的 D3D12Resource，让 GPU 知道这是一个顶点缓冲
		VertexBufferView.BufferLocation = m_VertexResource->GetGPUVirtualAddress();		// 顶点缓冲资源的地址
		VertexBufferView.SizeInBytes = sizeof(vertexs);									// 整个顶点缓冲的总大小
		VertexBufferView.StrideInBytes = sizeof(VERTEX);								// 每个顶点元素的大小 (步长)
	}

	// 渲染
	void Render()
	{
		// 获取 RTV 堆首句柄
		RTVHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
		// 获取当前渲染的后台缓冲序号
		FrameIndex = m_DXGISwapChain->GetCurrentBackBufferIndex();
		// 偏移 RTV 句柄，找到对应的 RTV 描述符
		RTVHandle.ptr += FrameIndex * RTVDescriptorSize;


		// 先重置命令分配器
		m_CommandAllocator->Reset();
		// 再重置命令列表，Close 关闭状态 -> Record 录制状态
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);

		// 将起始转换屏障的资源指定为当前渲染目标
		beg_barrier.Transition.pResource = m_RenderTarget[FrameIndex].Get();
		// 调用资源屏障，将渲染目标由 Present 呈现(只读) 转换到 RenderTarget 渲染目标(只写)
		m_CommandList->ResourceBarrier(1, &beg_barrier);

		// 第二次设置根签名！本次设置将会检查 渲染管线绑定的根签名 与 这里的根签名 是否匹配
		// 以及根签名指定的资源是否被正确绑定，检查完毕后会进行简单的映射
		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		// 设置渲染管线状态，可以在上面 m_CommandList->Reset() 的时候直接在第二个参数设置 PSO
		m_CommandList->SetPipelineState(m_PipelineStateObject.Get());

		// 设置视口 (光栅化阶段)，用于光栅化里的屏幕映射
		m_CommandList->RSSetViewports(1, &viewPort);
		// 设置裁剪矩形 (光栅化阶段)
		m_CommandList->RSSetScissorRects(1, &ScissorRect);

		// 用 RTV 句柄设置渲染目标
		m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, nullptr);

		// 清空当前渲染目标的背景为天蓝色
		m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);

		// 设置图元拓扑 (输入装配阶段)，我们这里设置三角形列表
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 设置 VBV 顶点缓冲描述符 (输入装配阶段) 
		m_CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);

		// Draw Call! 绘制矩形
		m_CommandList->DrawInstanced(6, 1, 0, 0);


		// 将终止转换屏障的资源指定为当前渲染目标
		end_barrier.Transition.pResource = m_RenderTarget[FrameIndex].Get();
		// 再通过一次资源屏障，将渲染目标由 RenderTarget 渲染目标(只写) 转换到 Present 呈现(只读)
		m_CommandList->ResourceBarrier(1, &end_barrier);

		// 关闭命令列表，Record 录制状态 -> Close 关闭状态，命令列表只有关闭才可以提交
		m_CommandList->Close();

		// 用于传递命令用的临时 ID3D12CommandList 数组
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// 执行上文的渲染命令！
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);

		// 向命令队列发出交换缓冲的命令，此命令会加入到命令队列中，命令队列执行到该命令时，会通知交换链交换缓冲
		m_DXGISwapChain->Present(1, NULL);




		// 将围栏预定值设定为下一帧
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		// 命令队列执行到这里会修改围栏值，表示渲染已完成，"击中"围栏
		m_CommandQueue->Signal(m_Fence.Get(), FenceValue);
		// 设置围栏的预定事件，当渲染完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		m_Fence->SetEventOnCompletion(FenceValue, RenderEvent);
	}

	// 渲染循环
	void RenderLoop()
	{
		bool isExit = false;	// 是否退出
		MSG msg = {};			// 消息结构体

		while (isExit != true)
		{
			// MsgWaitForMultipleObjects 用于多个线程的无阻塞等待，返回值是激发事件 (线程) 的 ID
			// 经过该函数后 RenderEvent 也会自动重置为无信号状态，因为我们创建事件的时候指定了第二个参数为 false
			DWORD ActiveEvent = ::MsgWaitForMultipleObjects(1, &RenderEvent, false, INFINITE, QS_ALLINPUT);

			switch (ActiveEvent - WAIT_OBJECT_0)
			{
			case 0:				// ActiveEvent 是 0，说明渲染事件已经完成了，进行下一次渲染
				Sleep(10);
				Render();
				break;

			case 1:				// ActiveEvent 是 1，说明渲染事件未完成，CPU 主线程同时处理窗口消息，防止界面假死
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
				break;

			case WAIT_TIMEOUT:	// 渲染超时
				break;
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


// 主函数
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}

