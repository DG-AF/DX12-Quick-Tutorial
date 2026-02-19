
// (16) D2DWithDX12: 认识新版本的 Direct2D，学会使用 Direct2D 绘制简单的 UI 界面，并与 DirectX 12 互动


// C++ 17 开始把 std::codecvt_utf8 给取消了，直接使用会报错 (和 scanf 一样)，加这个宏可以绕过报错，继续使用
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING


#include<Windows.h>			// Windows 窗口编程核心头文件
#include<d3d12.h>			// DX12 核心头文件
#include<dxgi1_6.h>			// DXGI 头文件，用于管理与 DX12 相关联的其他必要设备，如 DXGI 工厂和 交换链
#include<DirectXColors.h>	// DirectX 颜色库
#include<DirectXMath.h>		// DirectX 数学库
#include<d3dcompiler.h>		// DirectX Shader 着色器编译库
#include<wincodec.h>		// WIC 图像处理框架，用于解码编码转换图片文件


#include<d3d11_4.h>			// 最新版本的 DX11 头文件，DX12 与 D2D 的互操作需要 DX11 搭桥引线，需要用到 D3D11Device
#include<d3d11on12.h>		// DX11On12 就是我们 "搭桥引线" 要用的过渡设备，它包含了过渡设备运行在 DX12 底层的所有必要声明
#include<d2d1_3.h>			// 最新版本的 Direct 2D 图形库，包含了很多新旧版本的组件和函数，我们需要拿它绘制 UI


#include<wrl.h>				// COM 组件模板库，方便写 DX12 和 DXGI 相关的接口
#include<string>			// C++ 标准 string 库
#include<sstream>			// C++ 字符串流处理库
#include<functional>		// C++ 标准函数对象库，用于下文的 std::function 函数包装器与 std::bind 绑定回调函数
#include<fstream>			// C++ 文件流处理库
#include<vector>			// C++ STL vector 容器库
#include<codecvt>			// C++ 字符编码转换库，用于 string 转 wstring
#include<iomanip>			// C++ 输入输出控制格式化库，用于 CallBackFunc 的 std::fixed 与 std::setprecision


#pragma comment(lib,"d3d12.lib")			// 链接 DX12 核心 DLL
#pragma comment(lib,"dxgi.lib")				// 链接 DXGI DLL
#pragma comment(lib,"dxguid.lib")			// 链接 DXGI 必要的设备 GUID
#pragma comment(lib,"d3dcompiler.lib")		// 链接 DX12 需要的着色器编译 DLL
#pragma comment(lib,"windowscodecs.lib")	// 链接 WIC DLL
#pragma comment(lib,"d2d1.lib")				// 链接 D2D1 DLL
#pragma comment(lib,"d3d11.lib")			// 链接 D3D11 核心 DLL


using namespace Microsoft;
using namespace Microsoft::WRL;		// 使用 wrl.h 里面的命名空间，我们需要用到里面的 Microsoft::WRL::ComPtr COM智能指针
using namespace DirectX;			// DirectX 命名空间



// ---------------------------------------------------------------------------------------------------------------



// 用于绑定回调函数的中间层
class CallBackWrapper
{
public:

	// 用于保存 DX12Engine 类的成员回调函数的包装器
	inline static std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> Broker_Func;

	// 用于传递到 lpfnWndProc 的静态成员函数，内部调用保存 DX12Engine::CallBackFunc 的函数包装器
	// 静态成员函数属于类，不属于类实例对象，所以没有 this 指针，可以直接赋值给 C-Style 的函数指针
	static LRESULT CALLBACK CallBackFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return Broker_Func(hwnd, msg, wParam, lParam);
	}

};



// ---------------------------------------------------------------------------------------------------------------



// D2D 引擎
class D2DEngine
{
private:

	UINT m_D3D11CreateDeviceFlag = NULL;	// 创建 D3D11 设备时需要用到的标志

	// 最高版本的 D3D11 核心设备，用于创建 D2D 相关设备 (中间人)，在整个 DX11 中负责所有资源的创建，类似 DX12 的 m_D3D12Device
	ComPtr<ID3D11Device5> m_D3D11Device;
	// 最高版本的 D3D11 设备上下文，在整个 DX11 中负责设置渲染状态，更新资源数据，以及发出指令，类似 DX12 的 m_CommandList + m_CommandQueue
	ComPtr<ID3D11DeviceContext4> m_D3D11DeviceContext;
	// 较高版本的 D3D11On12 过渡设备，D2D 与 D3D12 交互的核心设备，用于创建 DXGIDevice，以及管理 Warp 包装器资源
	ComPtr<ID3D11On12Device1> m_D3D11On12Device;

	// 创建 D2D 工厂需要用到的选项 (标志)
	D2D1_FACTORY_OPTIONS m_D2DCreateFactoryOptions = {};

	// 创建 D2D 设备上下文需要用到的选项 (标志)，D2D1_DEVICE_CONTEXT_OPTIONS_NONE 表示不使用多线程设备上下文
	D2D1_DEVICE_CONTEXT_OPTIONS m_D2DCreateDeviceContextOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;


	// 较高版本的 D2D 核心工厂，它是 D2D 渲染的起点和资源管理中心，负责创建所有 D2D 绘图需要的对象，并管理全局调试选项
	ComPtr<ID2D1Factory7> m_D2DFactory;
	// 较高版本的 D2D 设备，负责创建 D2DDeviceContext，管理全局性资源与状态
	ComPtr<ID2D1Device6> m_D2DDevice;
	// 较高版本的 D2D 设备上下文，负责绘图命令，管理绘图状态，连接资源与目标
	ComPtr<ID2D1DeviceContext6> m_D2DDeviceContext;

	UINT DPI = 0;	// 窗口 DPI

	// D3D11 用于包装的渲染目标资源 (后台缓冲)，数量和 D3D12 一致
	// Wrapped 包装，是一种软件设计思想，意思类似于将 D3D12 资源层层包装 (转换) 成 D2D 能用的资源接口
	// 但 D2D 内部并不拥有这个资源的真实显存，只是得到了 D3D12 资源翻译后的使用权，资源还是 D3D12 的，一点都没变过
	// Wrapped 包装本质上只是做了一个翻译，没有创建新的资源，和上面的 CallBackWrapper 转换回调函数并塞进 Win32API 原理是一样的
	ComPtr<ID3D11Resource> m_D3D11WrappedRenderTarget[3];
	// D2D 渲染目标资源 (后台缓冲)，数量和 D3D12 一致，别看接口类型不同，实际指向的显存 (数据来源) 仍然是 D3D12RenderTarget
	ComPtr<ID2D1Bitmap1> m_D2DRenderTarget[3];



	// ---------------------------------------------------------------------------------------------------------------



	ComPtr<IWICImagingFactory> m_WICFactory;				// WIC 工厂
	ComPtr<IWICBitmapDecoder> m_WICBitmapDecoder;			// 位图解码器
	ComPtr<IWICBitmapFrameDecode> m_WICBitmapDecodeFrame;	// 由解码器得到的单个位图帧
	ComPtr<IWICFormatConverter> m_WICFormatConverter;		// 位图转换器
	ComPtr<IWICBitmapFlipRotator> m_WICBitmapFlipRotator;	// 位图镜像翻转器，用于翻转 HUD 位图绘制右侧饱食度


	// 物品栏所属的位图，包括 9 格快捷物品栏，和一个选中框
	ComPtr<ID2D1Bitmap> m_InventoryBitmap;
	// HUD (Heads-up display，抬头显示器) 界面元素所属的位图，包含生命值、护甲值、饥饿值、经验槽等
	// HUD 在游戏中指的就是一直叠加在游戏画面上，为你实时显示各种状态信息的界面元素
	ComPtr<ID2D1Bitmap> m_HUDBitmap;
	// 水平镜像翻转的 HUD 界面位图，用于绘制右侧饱食度
	ComPtr<ID2D1Bitmap> m_FlippedHUDBitmap;


	// m_InventoryBitmap 的图片文件名 (相对路径)
	std::wstring InventoryBitmapFileName = L"resource/widgets.png";
	// m_HUDBitmap 和 m_FlippedHUDBitmap 的图片文件名 (相对路径)
	std::wstring HUDBitmapFileName = L"resource/icons.png";



	// ---------------------------------------------------------------------------------------------------------------


	// 误区三：D2D_RECT_F 左上角 (left, top) 和右下角 (right, bottom) 的点 指的是几何坐标上的 "角点" 坐标
	// 请注意，这两个点不是像素网格上 "包含首尾的像素区间"，D2D_RECT_F 定义的是 "几何角点坐标"，而不是 "像素行列"
	// D2D_RECT_F 都是浮点数了，不要把它认为是某行某列像素，D2D 和 D3D12 一样有纹理插值采样方法 (DrawBitmap 第四个参数)
	// 当指定左上角 (0, 0) 和右上角 (0, 0)，你得到的是一个 0 面积的矩形，而不是一个 1x1 的像素块
	// 如果你想得到一个 (w - 1) * (h - 1) 的像素区域，你需要指定左上角 (0, 0) 和右下角 (w, h)
	// 这就是作者为什么下面的 right 和 bottom 不 -1 的原因，直接加上宽高长度，因为 -1 了会少一行一列像素

	
	float Components_Scale_Rate = 2;					// 组件放大倍数，这个可以调整，默认 2 倍
	UINT Selected_Slot_Index = 0;						// 当前选择框的索引 (0-8)


	const float HotBar_Width = 180.0f;					// 快捷物品栏宽度 (不要黑底)
	const float HotBar_Height = 20.0f;					// 快捷物品栏高度 (不要黑底)
	D2D_RECT_F Source_HotBarInventory_Rect = {};		// 9 格快捷物品栏的源图区域范围
	D2D_RECT_F Destination_HotBarInventory_Rect = {};	// 9 格快捷物品栏的目标区域范围


	const float Slot_Width = 22.0f;						// 物品选择框的宽度 (不要黑底)
	const float Slot_Height = 22.0f;					// 物品选择框的高度 (不要黑底)
	D2D_RECT_F Source_Slot_Rect = {};					// 物品选择框的源图范围
	D2D_RECT_F Destination_Slot_Rect = {};				// 物品选择框的目标范围


	const float XPBar_Width = 182.0f;					// 经验条宽度
	const float XPBar_Height = 5.0f;					// 经验条高度
	D2D_RECT_F Source_XPBar_Rect = {};					// 经验条的源图范围
	D2D_RECT_F Destination_XPBar_Rect = {};				// 经验条的目标范围


	const float Heart_Width = 9.0f;						// HUD 生命值宽度 (空心/实心 共用)
	const float Heart_Height = 9.0f;					// HUD 生命值高度 (空心/实心 共用)
	D2D_RECT_F Source_EmptyHeart_Rect = {};				// HUD 生命值空心槽的源图范围 (不共用)
	D2D_RECT_F Source_FullHeart_Rect = {};				// HUD 生命值实心的源图范围 (不共用)
	D2D_RECT_F Destination_Heart_Rect = {};				// HUD 生命值空心槽/实心目标范围 (空心/实心 共用)


	const float HurgerBar_Width = 9.0f;					// HUD 饥饿值宽度
	const float HurgerBar_Height = 9.0f;				// HUD 饥饿值高度
	D2D_RECT_F Source_HurgerBar_Rect = {};				// HUD 饥饿值槽的源图范围
	D2D_RECT_F Destination_HurgerBar_Rect = {};			// HUD 饥饿值槽的目标范围


	const float CrossHair_Width = 9.0f;					// 十字准心宽度
	const float CrossHair_Height = 9.0f;				// 十字准心高度
	D2D_RECT_F Source_CrossHair_Rect = {};				// 十字准心的源图范围
	D2D_RECT_F Destination_CrossHair_Rect = {};			// 十字准心的目标范围



	// ---------------------------------------------------------------------------------------------------------------

public:

	// 利用 D3D11On12 函数搭桥引线，在 D3D12Device 的基础上创建 D3D11Device 和 D3D11On12 设备
	// 利用此函数创建的 D3D11Device 可以操作 D3D12Device 拥有的所有资源 (例如渲染目标)
	// 这意味着利用这个 D3D11Device 创建的 D2DDevice 可以直接画到渲染目标上，实现 D2D 与 D3D12 的同屏显示
	void D2D_STEP01_CreateD3D11Device(ComPtr<ID3D12Device4>& m_D3D12Device, ComPtr<ID3D12CommandQueue>& m_CommandQueue)
	{
#if defined(_DEBUG)		// 如果是 DEBUG 调试，增加调试标志
		m_D3D11CreateDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		// D2D 的默认格式是 DXGI_FORMAT_B8G8R8A8_UNORM，加上这个标志，让 D3D11 创建的渲染目标支持 BGRA 格式
		m_D3D11CreateDeviceFlag |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;

		// 临时创建的 D3D11 低版本设备，当工具人创建高版本设备用的
		ComPtr<ID3D11Device> _temp_D3D11Device;
		// 临时创建的 D3D11 低版本设备上下文，当工具人创建高版本设备上下文用的
		ComPtr<ID3D11DeviceContext> _temp_D3D11DeviceContext;


		// 在 D3D12Device 的基础上创建 D3D11Device，为后续的 D2D 设备创建打下基础
		// 注意，第五个参数不能使用 &m_CommandQueue，& = ReleaseAndGetAddressOf()
		// 第五个参数填命令队列，和上面交换链原理是一样的，都是将交换链/设备上下文刷新并绑定命令队列
		D3D11On12CreateDevice(m_D3D12Device.Get(), m_D3D11CreateDeviceFlag,
			nullptr, 0, reinterpret_cast<IUnknown**>(m_CommandQueue.GetAddressOf()),
			1, 0, &_temp_D3D11Device, &_temp_D3D11DeviceContext, nullptr);


		// 通过 As 方法将数据继承到高版本设备接口
		_temp_D3D11Device.As(&m_D3D11Device);
		_temp_D3D11DeviceContext.As(&m_D3D11DeviceContext);

		// 注意这里！这里要把 D3D11Device 的数据继承给 D3D11On12Device！
		m_D3D11Device.As(&m_D3D11On12Device);
	}



	// 利用第一步创建的 D3D11On12 设备，创建 D2D 相关设备
	void D2D_STEP02_CreateD2DDevice()
	{
#if defined(_DEBUG)		// 如果是 DEBUG 调试，D2D 工厂选项增加调试等级，D2D1_DEBUG_LEVEL_INFORMATION 表示提供全部调试信息
		m_D2DCreateFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

		// 创建 D2D 工厂 (单线程)，注意我们用了这个函数的模板函数重载版本，使用这个版本可以直接在模板参数指定创建工厂的接口类型
		// 模板参数 <ID2D1Factory7> 就是我们指定要创建的工厂接口类型
		D2D1CreateFactory<ID2D1Factory7>(D2D1_FACTORY_TYPE_SINGLE_THREADED,
			m_D2DCreateFactoryOptions, &m_D2DFactory);


		// 临时创建的 DXGIDevice (工具人)，D2D 需要依赖 DXGI 来确保与底层的 D3D 设备相关联，获得底层设备的完整能力，并保持兼容性
		ComPtr<IDXGIDevice> _temp_DXGIDevice;
		// D3D11On12 设备的数据继承到 DXGIDevice，传递这个可以验证传入设备是否支持 D2D 所需功能
		m_D3D11On12Device.As(&_temp_DXGIDevice);


		// D2D 工厂创建 D2DDevice 设备，注意这个需要传递 _temp_DXGIDevice
		m_D2DFactory->CreateDevice(_temp_DXGIDevice.Get(), &m_D2DDevice);
		// D2D 设备创建 D2DDeviceContext 设备上下文
		m_D2DDevice->CreateDeviceContext(m_D2DCreateDeviceContextOptions, &m_D2DDeviceContext);
	}



	// 在上面各种设备的基础上，逐步翻译、包装、转化并绑定 D3D12RenderTarget 到 D2DRenderTarget 上
	void D2D_STEP03_CreateD2DRenderTarget(HWND MainWindowHwnd, ComPtr<ID3D12Resource> (&m_D3D12RenderTarget)[3])
	{
		// 获取 DPI (Dots Per Inch 每英寸点数)，它描述了显示设备的像素密度，即在一英寸的长度内可以排列多少个像素点
		// 在 Direct2D 以及一般的 UI 开发中，DPI 至关重要，因为它直接影响着文字、图形和 UI 元素在不同显示器上的物理尺寸
		// DPI 与屏幕分辨率是紧密相关的，如果不考虑 DPI，同一个应用在低分辨率和高分辨率屏幕上显示时，
		// 元素要么太小 (在高 DPI 屏上) 要么太大 (在低 DPI 屏上)，用户体验会很差

		// 利用 GetDpiForWindow 获取窗口的 DPI
		DPI = GetDpiForWindow(MainWindowHwnd);


		// D2D 位图属性，渲染目标其实就是一个特殊纹理 (2D 位图)，这一点在 D3D11 和 D2D 道理也是一样的
		D2D1_BITMAP_PROPERTIES1 BitmapProperties = {};
		// 设置位图选项 (标志)
		// D2D1_BITMAP_OPTIONS_TARGET 表示该位图可以被设置为渲染目标
		// D2D1_BITMAP_OPTIONS_CANNOT_DRAW 表示该位图不能作为绘制操作的来源，
		// 不能用于 ID2D1DeviceContext::DrawBitmap 做输入参数，更不能用于创建 ID2D1BitmapBrush 位图画刷 (后面的教程再涉及)
		// 指定这两个标志，表示创建一个专用渲染目标，作为最终输出的画板，而不是作为中间资源被反复利用
		// Direct2D 驱动层可能会因此进行一些优化，比如不需要为它创建着色器资源描述符 (SRV Descriptor)，从而节省资源
		BitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

		// 渲染目标的 DXGI_FORMAT 指定 DXGI_FORMAT_UNKNOWN，会自动匹配表面格式
		BitmapProperties.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
		// 指定像素颜色值采用预乘 alpha 格式存储，相当于渲染目标上色会自动进行 SrcRGB * SrcA 的操作
		// 传统的直接 alpha (或称非预乘、straight alpha) 格式中，RGB 分量是独立于 alpha 存储的，合成时需要实时计算
		// 预乘 alpha 可以避免在合成时因颜色与 alpha 相乘而产生的色偏或边缘锯齿问题
		// 例如，在绘制带有半透明边缘的纹理时，直接 alpha 可能导致边缘出现暗色光晕，而预乘格式已经将颜色与透明度融合，合成结果更自然
		// 预乘后的颜色可以直接与背景进行加法混合 (SrcRGB + DstRGB * (1 - SrcA))，可以节省一次乘法操作 (虽然性能提升微乎其微)
		// 许多 GPU 内部处理纹理时更倾向于预乘格式，可以减少着色器中的计算
		BitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;

		// 设置渲染目标 x,y 轴的 DPI，两个都是一样的
		BitmapProperties.dpiX = DPI;
		BitmapProperties.dpiY = DPI;

		// 用于色彩空间转换的颜色上下文，我们不需要，填 nullptr
		BitmapProperties.colorContext = nullptr;


		// 先创建 D3D11 的渲染目标，D3D12 有多少后台窗口缓冲 (渲染目标)，D3D11 和 D2D 也要创建多少接口
		for (UINT i = 0; i < 3; i++)
		{
			// 用于 D3D11 包装渲染目标资源的标志
			D3D11_RESOURCE_FLAGS D3D11WrappedBackBufferFlag = { D3D11_BIND_RENDER_TARGET };

			// 通过 D3D11On12 设备创建包装层资源，D2D 只能识别 DXGI 风格的资源，我们需要进行第一次翻译
			// 我们需要通过 D3D11On12 设备将 D3D12RenderTarget 接口包装 (翻译) 成 D3D11WrappedResource 接口
			// 注意要填转换前后状态：转换前是 RENDER_TARGET 渲染目标状态，转换后是 PRESENT 呈现状态，后面有用！
			m_D3D11On12Device->CreateWrappedResource(m_D3D12RenderTarget[i].Get(),
				&D3D11WrappedBackBufferFlag, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT,
				IID_PPV_ARGS(&m_D3D11WrappedRenderTarget[i]));

			// 用于临时转化和翻译用的 DXGISurface 接口，对于上面那几个渲染目标资源，D2D 只认 DXGISurface，我们需要先转化成这个
			// DXGISurface 是 DXGI 为所有图形 API 定义的，用于表示 2D 图像数据的统一接口 (如何转化不同接口就是另外一回事了)
			ComPtr<IDXGISurface> _temp_DXGISurface;

			// 将 D3D11WrappedResource 的数据继承到 DXGISurface
			m_D3D11WrappedRenderTarget[i].As(&_temp_DXGISurface);

			// 利用 D2DDeviceContext 将 DXGISurface 接口转化成 D2DRenderTarget，这样我们就完成了渲染目标从 D3D12 到 D2D 的翻译
			m_D2DDeviceContext->CreateBitmapFromDxgiSurface(_temp_DXGISurface.Get(),
				BitmapProperties, &m_D2DRenderTarget[i]);
		}
	}



	// ---------------------------------------------------------------------------------------------------------------



	// WIC 读取图集，然后通过 D2DDeviceContext 的成员方法创建并转化成 D2DBitmap
	// Atlas 图集，相当于包含所有界面小元素的大图，是纹理图片的一种形式
	// 后续教程我们都要将 WIC 的相关组件和功能搬到 D2DEngine，这样可以适当减少代码复杂度
	bool D2D_STEP04_LoadAtlasIntoD2DBitmaps()
	{
		// 先创建 WIC 工厂，WIC 工厂每个进程实例只能持有一次，重复创建会报错
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_WICFactory));

		// 创建 m_InventoryBitmap
		{
			// 读取图片数据并创建解码器
			HRESULT hr = m_WICFactory->CreateDecoderFromFilename(InventoryBitmapFileName.c_str(), nullptr, GENERIC_READ,
				WICDecodeMetadataCacheOnDemand, &m_WICBitmapDecoder);


			std::wostringstream output_str;		// 用于格式化字符串
			switch (hr)
			{
				case S_OK: break;	// 解码成功，直接 break 进入下一步即可

				case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):	// 文件找不到
					output_str << L"找不到文件 " << InventoryBitmapFileName << L" ！请检查文件路径是否有误！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				case HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT):	// 文件句柄正在被另一个应用进程占用
					output_str << L"文件 " << InventoryBitmapFileName << L" 已经被另一个应用进程打开并占用了！请先关闭那个应用进程！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				case WINCODEC_ERR_COMPONENTNOTFOUND:			// 找不到可解码的组件，说明这不是有效的图像文件
					output_str << L"文件 " << InventoryBitmapFileName << L" 不是有效的图像文件，无法解码！请检查文件是否为图像文件！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				default:			// 发生其他未知错误
					output_str << L"文件 " << InventoryBitmapFileName << L" 解码失败！发生了其他错误，错误码：" << hr << L" ，请查阅微软官方文档。";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;
			}


			// 读取一帧图片
			m_WICBitmapDecoder->GetFrame(0, &m_WICBitmapDecodeFrame);

			// 创建转换器
			m_WICFactory->CreateFormatConverter(&m_WICFormatConverter);

			// 将图片进行转换，注意 D2D 位图格式都必须是 GUID_WICPixelFormat32bppPBGRA
			// 进行转换后，位图数据会存储在 m_WICFormatConverter 上面
			m_WICFormatConverter->Initialize(m_WICBitmapDecodeFrame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
				nullptr, 0, WICBitmapPaletteTypeCustom);

			// D2D 设备上下文从 WIC 位图资源中创建 D2DBitmap，这个 m_WICFormatConverter 其实是 IWICBitmapSource 的子类
			// m_WICFormatConverter.Get() 相当于传递指向子类的指针，这样就能创建 D2D 位图了
			m_D2DDeviceContext->CreateBitmapFromWicBitmap(m_WICFormatConverter.Get(), &m_InventoryBitmap);
		}


		// 创建 m_HUDBitmap，道理和上面一样的
		{
			HRESULT hr = m_WICFactory->CreateDecoderFromFilename(HUDBitmapFileName.c_str(), nullptr, GENERIC_READ,
				WICDecodeMetadataCacheOnDemand, &m_WICBitmapDecoder);


			std::wostringstream output_str;		// 用于格式化字符串
			switch (hr)
			{
				case S_OK: break;	// 解码成功，直接 break 进入下一步即可

				case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):	// 文件找不到
					output_str << L"找不到文件 " << InventoryBitmapFileName << L" ！请检查文件路径是否有误！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				case HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT):	// 文件句柄正在被另一个应用进程占用
					output_str << L"文件 " << InventoryBitmapFileName << L" 已经被另一个应用进程打开并占用了！请先关闭那个应用进程！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				case WINCODEC_ERR_COMPONENTNOTFOUND:			// 找不到可解码的组件，说明这不是有效的图像文件
					output_str << L"文件 " << InventoryBitmapFileName << L" 不是有效的图像文件，无法解码！请检查文件是否为图像文件！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				default:			// 发生其他未知错误
					output_str << L"文件 " << InventoryBitmapFileName << L" 解码失败！发生了其他错误，错误码：" << hr << L" ，请查阅微软官方文档。";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;
			}


			m_WICBitmapDecoder->GetFrame(0, &m_WICBitmapDecodeFrame);

			m_WICFactory->CreateFormatConverter(&m_WICFormatConverter);

			m_WICFormatConverter->Initialize(m_WICBitmapDecodeFrame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
				nullptr, 0, WICBitmapPaletteTypeCustom);

			m_D2DDeviceContext->CreateBitmapFromWicBitmap(m_WICFormatConverter.Get(), &m_HUDBitmap);
		}


		// 创建 m_FlippedHUDBitmap
		{
			// WIC 工厂先创建翻转器
			m_WICFactory->CreateBitmapFlipRotator(&m_WICBitmapFlipRotator);

			// 翻转器初始化，将转换后的位图镜像翻转，WICBitmapTransformFlipHorizontal 表示水平镜像
			m_WICBitmapFlipRotator->Initialize(m_WICFormatConverter.Get(), WICBitmapTransformFlipHorizontal);

			// D2D 设备上下文从 WIC 位图资源中创建 D2DBitmap，这个 m_WICBitmapFlipRotator 也是 IWICBitmapSource 的子类
			m_D2DDeviceContext->CreateBitmapFromWicBitmap(m_WICBitmapFlipRotator.Get(), &m_FlippedHUDBitmap);
		}


		// 三个 D2DBitmap 都加载成功，返回 true
		return true;
	}



	// ---------------------------------------------------------------------------------------------------------------



	// D2D UI 渲染，此操作必须在命令队列 ExecuteCommandLists 之后，交换链 Present 之前
	void D2DUIRender(const UINT& CurrentFrameIndex, const UINT& WindowWidth, const UINT& WindowHeight)
	{
		// D3D11On12 设备告诉包装资源 (D3D11RenderTarget) 进入 InState (D3D12_RESOURCE_STATE_RENDER_TARGET) 渲染目标状态
		// 此操作不会插入一个多余的 Present -> RenderTarget 的资源状态转换屏障，它只会在 D3D11On12 这个包装层进行标记和同步
		// 所以必须要放在 ExecuteCommandLists 后面，而且前面必须携带 beg_barrier (Present -> RenderTarget) 的转换指令
		// 它会锁定包装资源的"内部所有权"和"状态转换独占权"，让后续的 D2D 命令可以安全地在处于 RENDER_TARGET 状态的资源上进行绘制
		m_D3D11On12Device->AcquireWrappedResources(m_D3D11WrappedRenderTarget[CurrentFrameIndex].GetAddressOf(), 1);

		// 经过 D3D11On12 的状态转换后，D2D 设置渲染目标，开始 2D 渲染
		m_D2DDeviceContext->SetTarget(m_D2DRenderTarget[CurrentFrameIndex].Get());


		// D2D 设备上下文开始 2D 渲染！2D 渲染的指令都要在 BeginDraw - EndDraw 之间完成 (和 GDI，GDI+，EasyX，EGE 这些一样)
		// 执行此句之后，D2DDeviceContext 会开始记录 D2D 渲染命令 (不会立即执行，只会记录命令)
		m_D2DDeviceContext->BeginDraw();

		// 不需要再次清理了，D2DRenderTarget = D3D12RenderTarget，这里清了会把已经绘制的 3D 对象清掉
		// m_D2DDeviceContext->Clear();

		// 绘制 HotBar 快捷物品栏
		{
			// 源图不要黑底，左上角从 (1, 1) 开始
			Source_HotBarInventory_Rect.left = 1;
			Source_HotBarInventory_Rect.top = 1;
			Source_HotBarInventory_Rect.right = Source_HotBarInventory_Rect.left + HotBar_Width;
			Source_HotBarInventory_Rect.bottom = Source_HotBarInventory_Rect.top + HotBar_Height;

			// 目标区域相对窗口水平居中，垂直方向在窗口底部
			Destination_HotBarInventory_Rect.left = (WindowWidth - HotBar_Width * Components_Scale_Rate) / 2;
			Destination_HotBarInventory_Rect.top = (WindowHeight - HotBar_Height * Components_Scale_Rate);
			Destination_HotBarInventory_Rect.right = Destination_HotBarInventory_Rect.left + HotBar_Width * Components_Scale_Rate;
			Destination_HotBarInventory_Rect.bottom = Destination_HotBarInventory_Rect.top + HotBar_Height * Components_Scale_Rate;


			// 绘制物品栏，对带透明通道的位图而言，这里会自动进行混合 (上面也指定预乘 alpha 了)，不用担心混合问题
			// 选 D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR 邻近点插值采样就行
			m_D2DDeviceContext->DrawBitmap(m_InventoryBitmap.Get(), Destination_HotBarInventory_Rect,
				1, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, Source_HotBarInventory_Rect);
		}


		// 绘制 Selected Slot 物品选择框
		{
			// 源图不要黑底，左上角从 (1, 23) 开始
			Source_Slot_Rect.left = 1;
			Source_Slot_Rect.top = 23;
			Source_Slot_Rect.right = Source_Slot_Rect.left + Slot_Width;
			Source_Slot_Rect.bottom = Source_Slot_Rect.top + Slot_Height;

			// 目标区域在物品栏左上角 -1，要和边框对齐，边框的这个 1 要单独乘放缩比例，不然会对不上槽位
			Destination_Slot_Rect.left = Destination_HotBarInventory_Rect.left - 1 * Components_Scale_Rate;
			Destination_Slot_Rect.top = Destination_HotBarInventory_Rect.top - 1 * Components_Scale_Rate;
			Destination_Slot_Rect.right = Destination_Slot_Rect.left + Slot_Width * Components_Scale_Rate;
			Destination_Slot_Rect.bottom = Destination_Slot_Rect.top + Slot_Height * Components_Scale_Rate;


			// 偏移到指定的物品栏槽位，一个槽位 20 x 20 (180 / 9 = 20)
			Destination_Slot_Rect.left += Selected_Slot_Index * 20 * Components_Scale_Rate;
			Destination_Slot_Rect.right += Selected_Slot_Index * 20 * Components_Scale_Rate;

			// 绘制物品选择框
			m_D2DDeviceContext->DrawBitmap(m_InventoryBitmap.Get(), Destination_Slot_Rect,
				1, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, Source_Slot_Rect);
		}


		// 绘制经验条
		{
			// 源图坐标从 (0, 64) 开始
			Source_XPBar_Rect.left = 0;
			Source_XPBar_Rect.top = 64;
			Source_XPBar_Rect.right = Source_XPBar_Rect.left + XPBar_Width;
			Source_XPBar_Rect.bottom = Source_XPBar_Rect.top + XPBar_Height;

			// 目标区域在物品栏上方 7 个像素，left 和 right 直接使用物品栏的 left 和 right
			Destination_XPBar_Rect.left = Destination_HotBarInventory_Rect.left;
			Destination_XPBar_Rect.top = Destination_HotBarInventory_Rect.top - 7 * Components_Scale_Rate;
			Destination_XPBar_Rect.right = Destination_HotBarInventory_Rect.right;
			Destination_XPBar_Rect.bottom = Destination_XPBar_Rect.top + XPBar_Height * Components_Scale_Rate;

			// 绘制经验条
			m_D2DDeviceContext->DrawBitmap(m_HUDBitmap.Get(), Destination_XPBar_Rect,
				1, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, Source_XPBar_Rect);
		}


		// 绘制生命值 (一共 10 颗心)
		{
			// 先画空心槽，源图坐标从 (16, 0) 开始
			Source_EmptyHeart_Rect.left = 16;
			Source_EmptyHeart_Rect.top = 0;
			Source_EmptyHeart_Rect.right = Source_EmptyHeart_Rect.left + Heart_Width;
			Source_EmptyHeart_Rect.bottom = Source_EmptyHeart_Rect.top + Heart_Width;

			// 再画实心，源图坐标从 (52, 0) 开始
			Source_FullHeart_Rect.left = 52;
			Source_FullHeart_Rect.top = 0;
			Source_FullHeart_Rect.right = Source_FullHeart_Rect.left + Heart_Width;
			Source_FullHeart_Rect.bottom = Source_FullHeart_Rect.top + Heart_Width;

			// 目标区域在经验条上方 10 个像素
			Destination_Heart_Rect.left = Destination_XPBar_Rect.left;
			Destination_Heart_Rect.top = Destination_XPBar_Rect.top - 10 * Components_Scale_Rate;
			Destination_Heart_Rect.right = Destination_Heart_Rect.left + Heart_Width * Components_Scale_Rate;
			Destination_Heart_Rect.bottom = Destination_Heart_Rect.top + Heart_Height * Components_Scale_Rate;


			// 用循环依次绘制 10 颗心
			for (UINT i = 0; i < 10; i++)
			{
				// 先画空心槽
				m_D2DDeviceContext->DrawBitmap(m_HUDBitmap.Get(), Destination_Heart_Rect,
					1, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, Source_EmptyHeart_Rect);

				// 再画实心
				m_D2DDeviceContext->DrawBitmap(m_HUDBitmap.Get(), Destination_Heart_Rect,
					1, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, Source_FullHeart_Rect);

				// 偏移 left 和 right，准备画下一颗心，-1 是为了让边缘重叠，不重叠不好看
				Destination_Heart_Rect.left += (Heart_Width - 1) * Components_Scale_Rate;
				Destination_Heart_Rect.right += (Heart_Width - 1) * Components_Scale_Rate;
			}

		}


		// 绘制饥饿值 (饱食度，一共 10 个鸡腿)
		{
			// 水平镜像翻转后，源图左上角是 (BitmapWidth - 16 - HurgerBar_Width, 36)
			Source_HurgerBar_Rect.left = 256 - 16 - HurgerBar_Width;
			Source_HurgerBar_Rect.top = 36;
			Source_HurgerBar_Rect.right = Source_HurgerBar_Rect.left + HurgerBar_Width;
			Source_HurgerBar_Rect.bottom = Source_HurgerBar_Rect.top + HurgerBar_Height;


			// 目标区域在经验条上方 10 个像素，left 和 right 倒着画
			Destination_HurgerBar_Rect.left = Destination_XPBar_Rect.right - HurgerBar_Width * Components_Scale_Rate;
			Destination_HurgerBar_Rect.top = Destination_XPBar_Rect.top - 10 * Components_Scale_Rate;
			Destination_HurgerBar_Rect.right = Destination_HurgerBar_Rect.left + HurgerBar_Width * Components_Scale_Rate;
			Destination_HurgerBar_Rect.bottom = Destination_HurgerBar_Rect.top + HurgerBar_Height * Components_Scale_Rate;


			// 用循环依次绘制 10 颗鸡腿
			for (UINT i = 0; i < 10; i++)
			{
				// 绘制图标
				m_D2DDeviceContext->DrawBitmap(m_FlippedHUDBitmap.Get(), Destination_HurgerBar_Rect,
					1, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, Source_HurgerBar_Rect);

				// 偏移 left 和 right，准备画下一颗鸡腿，-1 是为了让边缘重叠，不重叠不好看
				Destination_HurgerBar_Rect.left -= (HurgerBar_Width - 1) * Components_Scale_Rate;
				Destination_HurgerBar_Rect.right -= (HurgerBar_Width - 1) * Components_Scale_Rate;
			}
		}


		// 十字准心
		{
			// 源图坐标从 (3, 3) 开始
			Source_CrossHair_Rect.left = 3;
			Source_CrossHair_Rect.top = 3;
			Source_CrossHair_Rect.right = Source_CrossHair_Rect.left + CrossHair_Width;
			Source_CrossHair_Rect.bottom = Source_CrossHair_Rect.top + CrossHair_Height;

			// 十字准心在窗口中央
			Destination_CrossHair_Rect.left = (WindowWidth / 2.0) - (CrossHair_Width * Components_Scale_Rate) / 2.0;
			Destination_CrossHair_Rect.top = (WindowHeight / 2.0) - (CrossHair_Height * Components_Scale_Rate) / 2.0;
			Destination_CrossHair_Rect.right = Destination_CrossHair_Rect.left + CrossHair_Width * Components_Scale_Rate;
			Destination_CrossHair_Rect.bottom = Destination_CrossHair_Rect.top + CrossHair_Height * Components_Scale_Rate;

			// 绘制十字准心
			m_D2DDeviceContext->DrawBitmap(m_HUDBitmap.Get(), Destination_CrossHair_Rect,
				1, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, Source_CrossHair_Rect);
		}


		// D2D 设备上下文结束 2D 渲染！D2DDeviceContext 结束对渲染命令的记录，准备提交给 GPU
		m_D2DDeviceContext->EndDraw();


		// D3D11On12 设备告诉包装资源 (D3D11RenderTarget) 进入 OutState (D3D12_RESOURCE_STATE_PRESENT) 呈现状态
		// 说明 D2D 已经渲染完成了，接下来释放包装资源的"内部所有权"和"状态转换独占权"，将这些交还给 D3D12 层设备
		// 另外这个操作还会自动插入一个 RenderTarget -> Present 的资源屏障，所以 end_barrier 我们注释掉不用了，防止状态重复转换
		m_D3D11On12Device->ReleaseWrappedResources(m_D3D11WrappedRenderTarget[CurrentFrameIndex].GetAddressOf(), 1);

		// 接下来是最关键的一个指令！D3D11 设备上下文进行刷新指令操作，将 D2D 的绘图指令全部提交到 D3D12CommandQueue
		// (注意这里不是 D2DDeviceContext->Flush()，它的意思是立即执行所有挂起的绘图命令，但不会刷新与渲染目标关联的 D3D 设备上下文)
		// 没有它，D2D 绘制指令不会被执行，更不会显示在屏幕 (渲染目标) 上，它相当于 D2D 的 ExecuteCommandLists
		m_D3D11DeviceContext->Flush();
	}



	// ---------------------------------------------------------------------------------------------------------------


	// 获取 Selected_Slot_Index
	inline UINT Get_Selected_Slot_Index()
	{
		return Selected_Slot_Index;
	}

	// 设置 Selected_Slot_Index
	inline void Set_Selected_Slot_Index(const UINT& index)
	{
		Selected_Slot_Index = index;
	}

	// 获取 Components_Scale_Rate
	inline float Get_Components_Scale_Rate()
	{
		return Components_Scale_Rate;
	}

	// 设置 Components_Scale_Rate
	inline void Set_Components_Scale_Rate(float rate)
	{
		Components_Scale_Rate = rate;
	}
};



// ---------------------------------------------------------------------------------------------------------------



// DX12 引擎
class DX12Engine
{
private:

	int WindowWidth = 1280;		// 窗口宽度
	int WindowHeight = 720;		// 窗口高度
	HWND m_hwnd;				// 窗口句柄

	ComPtr<ID3D12Debug> m_D3D12DebugDevice;					// D3D12 调试层设备
	UINT m_DXGICreateFactoryFlag = NULL;					// 创建 DXGI 工厂时需要用到的标志

	ComPtr<IDXGIFactory5> m_DXGIFactory;					// DXGI 工厂
	ComPtr<IDXGIAdapter1> m_DXGIAdapter;					// 显示适配器 (显卡)
	ComPtr<ID3D12Device4> m_D3D12Device;					// D3D12 核心设备

	ComPtr<ID3D12CommandQueue> m_CommandQueue;				// 命令队列
	ComPtr<ID3D12CommandAllocator> m_CommandAllocator;		// 命令分配器
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;		// 命令列表

	ComPtr<IDXGISwapChain3> m_DXGISwapChain;				// DXGI 交换链
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;					// RTV 描述符堆
	ComPtr<ID3D12Resource> m_D3D12RenderTarget[3];			// 渲染目标数组，每一副渲染目标对应一个窗口缓冲区
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;					// RTV 描述符句柄
	UINT RTVDescriptorSize = 0;								// RTV 描述符的大小
	UINT FrameIndex = 0;									// 帧索引，表示当前渲染的第 i 帧 (第 i 个渲染目标)

	ComPtr<ID3D12Fence> m_Fence;							// 围栏
	UINT64 FenceValue = 0;									// 用于围栏等待的围栏值
	HANDLE RenderEvent = NULL;								// GPU 渲染事件
	D3D12_RESOURCE_BARRIER beg_barrier = {};				// 渲染开始的资源屏障，呈现 -> 渲染目标

	// 误区一：渲染结束的资源屏障，渲染目标 -> 呈现
	// 下面的 m_D3D11On12Device->ReleaseWrappedResources 会自动帮我们做 RenderTarget -> Present 的呈现
	// (因为 D2D，D3D11 经过 D3D11On12 Device 包装后的渲染目标接口，实际指向的都是 D3D12RenderTarget)
	// 下文 CommandQueue 开始执行 3D 绘制指令的时候，我们还需要进行 2D 渲染，仍然需要 RenderTarget 状态
	// 如果再进行一次 end_barrier 的转换会发生 D3D12 ERROR (资源状态转换问题)，所以我们只保留并使用 beg_barrier
	// D3D12_RESOURCE_BARRIER end_barrier = {};


	// 视口
	D3D12_VIEWPORT ViewPort = D3D12_VIEWPORT{ 0, 0, float(WindowWidth), float(WindowHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	// 裁剪矩形
	D3D12_RECT ScissorRect = D3D12_RECT{ 0, 0, WindowWidth, WindowHeight };


	// D2D 引擎对象，用于渲染 2D UI 界面
	D2DEngine m_D2DEngine;


	// ---------------------------------------------------------------------------------------------------------------

public:

	// 初始化窗口
	void STEP01_InitWindow(HINSTANCE hins)
	{
		WNDCLASS wc = {};					// 用于记录窗口类信息的结构体
		wc.hInstance = hins;				// 窗口类需要一个应用程序的实例句柄 hinstance

		// 绑定回调函数，利用 std::bind，将 DX12Engine::CallBackFunc 绑定到 CallBackWrapper 的函数包装器上
		CallBackWrapper::Broker_Func = std::bind(&DX12Engine::CallBackFunc, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		wc.lpfnWndProc = CallBackWrapper::CallBackFunc;		// 窗口类需要一个回调函数，用于处理窗口产生的消息，注意这里传递的是中间层的回调函数
		wc.lpszClassName = L"DX12 Game";					// 窗口类的名称

		RegisterClass(&wc);					// 注册窗口类，将窗口类录入到操作系统中

		// 使用上文的窗口类创建窗口
		m_hwnd = CreateWindow(wc.lpszClassName, L"Minecraft (当前放大比例：2.0 倍 UI)", WS_SYSMENU | WS_OVERLAPPED,
			10, 10, WindowWidth, WindowHeight,
			NULL, NULL, hins, NULL);

		// 因为指定了窗口大小不可变的 WS_SYSMENU 和 WS_OVERLAPPED，应用不会自动显示窗口，需要使用 ShowWindow 强制显示窗口
		ShowWindow(m_hwnd, SW_SHOW);
	}

	// 创建调试层
	void STEP02_CreateDebugDevice()
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
	bool STEP03_CreateDevice()
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
					OutputDebugStringW(L"当前使用的显卡：");
					OutputDebugStringW(adap.Description);
					OutputDebugStringW(L"\n");
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
	void STEP04_CreateCommandComponents()
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
	void STEP05_CreateRenderTarget()
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
			m_DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&m_D3D12RenderTarget[i]));

			// 创建 RTV 描述符，将渲染目标绑定到描述符上
			m_D3D12Device->CreateRenderTargetView(m_D3D12RenderTarget[i].Get(), nullptr, RTVHandle);

			// 偏移到下一个 RTV 句柄
			RTVHandle.ptr += RTVDescriptorSize;
		}
	}

	// 创建围栏和资源屏障，用于 CPU-GPU 的同步
	void STEP06_CreateFenceAndBarrier()
	{
		// 创建 CPU 上的等待事件，注意第二个参数填 false 表示自动重置事件 (每当经过一次 Wait 函数，自动重置无信号状态)
		// 第三个初始状态参数填 true 表示有信号状态，我们后面没有任何 copy 动作，直接进渲染循环即可
		RenderEvent = CreateEvent(nullptr, false, true, nullptr);

		// 创建围栏，设定初始值为 0
		m_D3D12Device->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));


		// 设置资源屏障
		// beg_barrier 起始屏障：Present 呈现状态 -> Render Target 渲染目标状态
		beg_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;					// 指定类型为转换屏障		
		beg_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		beg_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		/*
			// end_barrier 终止屏障：Render Target 渲染目标状态 -> Present 呈现状态
			// 这里被注释掉了，不需要用到，原因见上文对 end_barrier 的注释
			end_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			end_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			end_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		*/
	}



	// ---------------------------------------------------------------------------------------------------------------

	

	// 初始化 D2D 引擎，创建 D3D11On12 相关的设备，执行 D2D 引擎内部的四个必要的成员函数
	void STEP07_InitializeD2DEngine()
	{
		m_D2DEngine.D2D_STEP01_CreateD3D11Device(m_D3D12Device, m_CommandQueue);
		m_D2DEngine.D2D_STEP02_CreateD2DDevice();
		m_D2DEngine.D2D_STEP03_CreateD2DRenderTarget(m_hwnd, m_D3D12RenderTarget);

		m_D2DEngine.D2D_STEP04_LoadAtlasIntoD2DBitmaps();
	}



	// ---------------------------------------------------------------------------------------------------------------


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

		// 设置视口 (光栅化阶段)，用于光栅化里的屏幕映射
		m_CommandList->RSSetViewports(1, &ViewPort);
		// 设置裁剪矩形 (光栅化阶段)
		m_CommandList->RSSetScissorRects(1, &ScissorRect);

		// 将起始转换屏障的资源指定为当前渲染目标
		beg_barrier.Transition.pResource = m_D3D12RenderTarget[FrameIndex].Get();
		// 调用资源屏障，将渲染目标由 Present 呈现(只读) 转换到 RenderTarget 渲染目标(只写)
		m_CommandList->ResourceBarrier(1, &beg_barrier);


		// 误区二：清空当前渲染目标的背景为天蓝色，此操作会同时清理 3D 和 2D 的已渲染/绘制的对象 (清空整个后台窗口缓冲)
		// 注意这里！下面不需要用到 m_D2DDeviceContext->Clear 了，原因：D2DRenderTarget = D3D12RenderTarget
		m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);

		// 用 RTV 句柄设置渲染目标
		m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, nullptr);


		/*
			// 这里被注释掉了，不需要用到，原因见上文对 end_barrier 的注释

			// 将终止转换屏障的资源指定为当前渲染目标
			end_barrier.Transition.pResource = m_D3D12RenderTarget[FrameIndex].Get();
			// 再通过一次资源屏障，将渲染目标由 RenderTarget 渲染目标(只写) 转换到 Present 呈现(只读)
			m_CommandList->ResourceBarrier(1, &end_barrier);

		*/

		// 关闭命令列表，Record 录制状态 -> Close 关闭状态，命令列表只有关闭才可以提交
		m_CommandList->Close();

		// 用于传递命令用的临时 ID3D12CommandList 数组
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// 执行上文的渲染命令！
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);


		// 向 GPU 提交完 3D 渲染命令后，CPU 准备记录 2D 渲染指令
		m_D2DEngine.D2DUIRender(FrameIndex, WindowWidth, WindowHeight);


		// 向命令队列发出交换缓冲的命令，此命令会加入到命令队列中，命令队列执行到该命令时，会通知交换链交换缓冲
		// 3D 和 2D 绘制指令的记录与提交必须要在交换链 Present 之前，交换缓冲说明一帧已经画完了，开始绘制下一帧缓冲
		m_DXGISwapChain->Present(1, NULL);


		// 将围栏预定值设定为下一帧的任务完成值，说明 GPU 完成了一项任务
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		// 命令队列执行到这里会修改围栏值，表示渲染已完成，"击中"围栏，同时修改围栏的 Completed Value 任务完成值
		// 这里传入 FenceValue 是因为 CommandQueue 要用这个值标记预定事件，关联围栏
		m_CommandQueue->Signal(m_Fence.Get(), FenceValue);
		// 设置围栏的预定事件，当渲染完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		// 这里传入 FenceValue 是因为围栏要拿这个值开辟对应的 Event Slot 事件槽，并将 CPU 端事件句柄绑定到事件槽上
		m_Fence->SetEventOnCompletion(FenceValue, RenderEvent);
	}


	// 渲染循环
	void STEP08_RenderLoop()
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
				{
					Render();
				}
				break;


				case 1:				// ActiveEvent 是 1，说明渲染事件未完成，CPU 主线程同时处理窗口消息，防止界面假死
				{
					// 查看消息队列是否有消息，如果有就获取。 PM_REMOVE 表示获取完消息，就立刻将该消息从消息队列中移除
					while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						// 如果程序没有收到退出消息，就向操作系统发出派发消息的命令
						if (msg.message != WM_QUIT)
						{
							TranslateMessage(&msg);		// 翻译消息，当键盘按键发出信号 (WM_KEYDOWN)，将虚拟按键值转换为对应的 ASCII 码，同时产生 WM_CHAR 消息
							DispatchMessage(&msg);		// 派发消息，通知操作系统调用回调函数处理消息
						}
						else
						{
							isExit = true;							// 收到退出消息，就退出消息循环
						}
					}
				}
				break;


				case WAIT_TIMEOUT:	// 渲染超时
				{

				}
				break;

			}
		}
	}



	// 回调函数，处理窗口产生的消息
	// 1-9 数字键 —— 切换选中的物品槽
	// 滚轮 —— 切换选中的物品槽
	// Q/q —— 增大 UI 界面比例 (最大 3 倍)
	// E/e —— 减小 UI 界面比例 (最小 1 倍)
	// 关闭窗口 —— 窗口关闭，程序进程退出
	LRESULT CALLBACK CallBackFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
			case WM_DESTROY:			// 窗口被销毁 (当按下右上角 X 关闭窗口时)
			{
				PostQuitMessage(0);		// 向操作系统发出退出请求 (WM_QUIT)，结束消息循环
			}
			break;


			case WM_CHAR:	// 获取键盘产生的字符消息，TranslateMessage 会将虚拟键码翻译成字符码，同时产生 WM_CHAR 消息
			{
				switch (wParam)		// wParam 是按键对应的字符 ASCII 码
				{
					// 数字键，就设置 Selected_Slot_Index
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					{
						// 物品栏索引
						UINT Selected_Slot_Index = wParam - '1';
						m_D2DEngine.Set_Selected_Slot_Index(Selected_Slot_Index);
					}
					break;


					// 每按一次 Q/q 键，增加 0.1 倍 UI 界面，最多 3 倍
					case 'Q':
					case 'q':
					{
						// 获取当前缩放比例
						float Components_Scale_Rate = m_D2DEngine.Get_Components_Scale_Rate();

						// 检查是否超过 3，有就进行 Clamp 裁剪操作
						Components_Scale_Rate = Components_Scale_Rate + 0.1 >= 3 ? 3 : Components_Scale_Rate + 0.1;

						// 设置新的缩放比例
						m_D2DEngine.Set_Components_Scale_Rate(Components_Scale_Rate);

						std::wostringstream NewTitleName;	// 更新状态并改变标题

						// 拼接字符串， std::fixed 表示将浮点数输出设置为固定小数点表示法
						// std::setprecision(1) 表示精确到 (保留) 小数点后一位
						// std::fixed 和 std::setprecision 是一起用的，没了 std::fixed 就会变成 "保留一位有效数字"
						NewTitleName << L"Minecraft (当前放大比例：" <<
							std::fixed << std::setprecision(1) << Components_Scale_Rate << L" 倍 UI)";

						// 更新标题
						SetWindowText(hwnd, NewTitleName.str().c_str());
					}
					break;

					// 每按一次 E/e 键，减少 0.1 倍 UI 界面，最少 1 倍
					case 'E':
					case 'e':
					{
						// 获取当前缩放比例
						float Components_Scale_Rate = m_D2DEngine.Get_Components_Scale_Rate();

						// 检查是否低于 1，有就进行 Clamp 裁剪操作
						Components_Scale_Rate = Components_Scale_Rate - 0.1 <= 1 ? 1 : Components_Scale_Rate - 0.1;

						// 设置新的缩放比例
						m_D2DEngine.Set_Components_Scale_Rate(Components_Scale_Rate);

						std::wostringstream NewTitleName;	// 更新状态并改变标题

						NewTitleName << L"Minecraft (当前放大比例：" <<
							std::fixed << std::setprecision(1) << Components_Scale_Rate << L" 倍 UI)";

						// 更新标题
						SetWindowText(hwnd, NewTitleName.str().c_str());
					}
					break;
				}
			}
			break;


			case WM_MOUSEWHEEL:		// 鼠标滚轮消息，和数字键功能一样切换选中框
			{
				// 获取当前滑槽索引
				UINT Selected_Slot_Index = m_D2DEngine.Get_Selected_Slot_Index();

				// 获取滚轮旋转量
				int delta = GET_WHEEL_DELTA_WPARAM(wParam);

				// 向上滚动：切换到上一个槽
				if (delta < 0) Selected_Slot_Index--;
				// 向下滚动：切换到下一个槽
				if (delta > 0) Selected_Slot_Index++;

				// 无论如何滚动，Selected_Slot_Index 必须在 [0, 8] 之间，防止越界
				Selected_Slot_Index = (Selected_Slot_Index + 9) % 9;

				// 设置新索引
				m_D2DEngine.Set_Selected_Slot_Index(Selected_Slot_Index);
			}
			break;


			// 如果接收到其他消息，直接默认返回整个窗口
			default: return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return 0;
	}


	// 运行窗口
	static void Run(HINSTANCE hins)
	{
		DX12Engine engine;
		engine.STEP01_InitWindow(hins);
		engine.STEP02_CreateDebugDevice();
		engine.STEP03_CreateDevice();
		engine.STEP04_CreateCommandComponents();
		engine.STEP05_CreateRenderTarget();
		engine.STEP06_CreateFenceAndBarrier();

		engine.STEP07_InitializeD2DEngine();

		engine.STEP08_RenderLoop();
	}
};


// 主函数
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}


