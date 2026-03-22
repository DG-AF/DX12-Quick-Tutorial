
// (22) ParticleSystem: 进一步学习计算着色器，学习粒子的动态生成与销毁，模拟 Minecraft 的粒子破坏效果


#include<Windows.h>				// Windows 窗口编程核心头文件
#include<d3d12.h>				// DX12 核心头文件
#include<dxgi1_6.h>				// DXGI 头文件，用于管理与 DX12 相关联的其他必要设备，如 DXGI 工厂和 交换链
#include<wincodec.h>			// WIC 图像处理框架，用于解码编码转换图片文件
#include<d3dcompiler.h>			// DirectX Shader 着色器编译库
#include<DirectXColors.h>		// DirectX 颜色库
#include<DirectXMath.h>			// DirectX 数学库
#include<DirectXCollision.h>	// DirectX 碰撞检测库，提供碰撞检测可用的数据结构与函数


#include<d3d11_4.h>				// 最新版本的 DX11 头文件，DX12 与 D2D 的互操作需要 DX11 搭桥引线，需要用到 D3D11Device
#include<d3d11on12.h>			// DX11On12 就是我们 "搭桥引线" 要用的过渡设备，它包含了过渡设备运行在 DX12 底层的所有必要声明
#include<d2d1_3.h>				// 最新版本的 Direct 2D 图形库，包含了很多新旧版本的组件和函数，我们需要拿它绘制 UI


#include<wrl.h>					// COM 组件模板库，方便写 DX12 和 DXGI 相关的接口
#include<string>				// C++ 标准 string 库
#include<sstream>				// C++ 字符串流处理库
#include<functional>			// C++ 标准函数对象库，用于下文的 std::function 函数包装器与 std::bind 绑定回调函数
#include<fstream>				// C++ 文件流处理库
#include<vector>				// C++ STL vector 容器库
#include<codecvt>				// C++ 字符编码转换库，用于 string 转 wstring
#include<random>				// C++ 高质量随机库，用于生成随机数


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



// 命名空间 DX12TextureHelper 包含了帮助我们转换纹理图片格式的结构体与函数
namespace DX12TextureHelper
{
	// 纹理转换用，不是 DX12 所支持的格式，DX12 没法用

	// Standard GUID -> DXGI 格式转换结构体
	struct WICTranslate
	{
		GUID wic;
		DXGI_FORMAT format;
	};

	// WIC 格式与 DXGI 像素格式的对应表，该表中的格式为被支持的格式
	static WICTranslate g_WICFormats[] =
	{
		{ GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },
		{ GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
		{ GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },
		{ GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
		{ GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM },
		{ GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM },
		{ GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM },
		{ GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },
		{ GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
		{ GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },
		{ GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
		{ GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
		{ GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
		{ GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },
		{ GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM }
	};

	// GUID -> Standard GUID 格式转换结构体
	struct WICConvert
	{
		GUID source;
		GUID target;
	};

	// WIC 像素格式转换表
	static WICConvert g_WICConvert[] =
	{
		// 目标格式一定是最接近的被支持的格式
		{ GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray },			// DXGI_FORMAT_R8_UNORM
		{ GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray },			// DXGI_FORMAT_R8_UNORM
		{ GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray },			// DXGI_FORMAT_R8_UNORM
		{ GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf },		// DXGI_FORMAT_R16_FLOAT
		{ GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat },	// DXGI_FORMAT_R32_FLOAT
		{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 },		// DXGI_FORMAT_B5G5R5A1_UNORM
		{ GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 },	// DXGI_FORMAT_R10G10B10A2_UNORM
		{ GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT

		{ GUID_WICPixelFormat128bppRGBAFloat,       GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat64bppRGBAHalf,         GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppRGBA,             GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat32bppRGBA,             GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat32bppBGRA,             GUID_WICPixelFormat32bppBGRA },			// DXGI_FORMAT_B8G8R8A8_UNORM
		{ GUID_WICPixelFormat32bppBGR,              GUID_WICPixelFormat32bppBGR },			// DXGI_FORMAT_B8G8R8X8_UNORM
		{ GUID_WICPixelFormat32bppRGBA1010102XR,    GUID_WICPixelFormat32bppRGBA1010102XR },// DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM
		{ GUID_WICPixelFormat32bppRGBA1010102,      GUID_WICPixelFormat32bppRGBA1010102 },	// DXGI_FORMAT_R10G10B10A2_UNORM
		{ GUID_WICPixelFormat16bppBGRA5551,         GUID_WICPixelFormat16bppBGRA5551 },		// DXGI_FORMAT_B5G5R5A1_UNORM
		{ GUID_WICPixelFormat16bppBGR565,           GUID_WICPixelFormat16bppBGR565 },		// DXGI_FORMAT_B5G6R5_UNORM
		{ GUID_WICPixelFormat32bppGrayFloat,        GUID_WICPixelFormat32bppGrayFloat },	// DXGI_FORMAT_R32_FLOAT
		{ GUID_WICPixelFormat16bppGrayHalf,         GUID_WICPixelFormat16bppGrayHalf },		// DXGI_FORMAT_R16_FLOAT
		{ GUID_WICPixelFormat16bppGray,             GUID_WICPixelFormat16bppGray },			// DXGI_FORMAT_R16_UNORM
		{ GUID_WICPixelFormat8bppGray,              GUID_WICPixelFormat8bppGray },			// DXGI_FORMAT_R8_UNORM
		{ GUID_WICPixelFormat8bppAlpha,             GUID_WICPixelFormat8bppAlpha }			// DXGI_FORMAT_A8_UNORM
	};


	// 查表确定兼容的最接近格式是哪个
	bool GetTargetPixelFormat(const GUID* pSourceFormat, GUID* pTargetFormat)
	{
		*pTargetFormat = *pSourceFormat;
		for (size_t i = 0; i < _countof(g_WICConvert); ++i)
		{
			if (InlineIsEqualGUID(g_WICConvert[i].source, *pSourceFormat))
			{
				*pTargetFormat = g_WICConvert[i].target;
				return true;
			}
		}
		return false;		// 找不到，就返回 false
	}

	// 查表确定最终对应的 DXGI 格式是哪一个
	DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID* pPixelFormat)
	{
		for (size_t i = 0; i < _countof(g_WICFormats); ++i)
		{
			if (InlineIsEqualGUID(g_WICFormats[i].wic, *pPixelFormat))
			{
				return g_WICFormats[i].format;
			}
		}
		return DXGI_FORMAT_UNKNOWN;		// 找不到，就返回 UNKNOWN
	}
}



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



// D2D 引擎，用于 2D UI 绘制
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
	// D2D 内部并不拥有这个资源的真实显存，只是得到了 D3D12 资源翻译后的使用权，资源还是 D3D12 的，一点都没变过
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
	// HUD 界面元素所属的位图，包含生命值、护甲值、饥饿值、经验槽等
	ComPtr<ID2D1Bitmap> m_HUDBitmap;
	// 水平镜像翻转的 HUD 界面位图，用于绘制右侧饱食度
	ComPtr<ID2D1Bitmap> m_FlippedHUDBitmap;


	// 物品栏展示方块的位图
	std::vector<ComPtr<ID2D1Bitmap>> m_InventoryBlockBitmaps;


	// m_InventoryBitmap 的图片文件名 (相对路径)
	std::wstring InventoryBitmapFileName = L"UIresource/widgets.png";
	// m_HUDBitmap 和 m_FlippedHUDBitmap 的图片文件名 (相对路径)
	std::wstring HUDBitmapFileName = L"UIresource/icons.png";


	// 物品栏使用的纹理
	std::vector<std::wstring> InventoryBlockNames =
	{
		// 0.熔炉
		L"resource/furnace_front_off.png",
		L"resource/furnace_side.png",
		L"resource/furnace_top.png",
		// 1.工作台
		L"resource/crafting_table_front.png",
		L"resource/crafting_table_side.png",
		L"resource/crafting_table_top.png",
		// 2.TNT
		L"resource/tnt_side.png",
		L"resource/tnt_top.png",
		// 3.橡木原木
		L"resource/log_oak.png",
		L"resource/log_oak_top.png",
		// 4.橡木木板
		L"resource/planks_oak.png",
		// 5.书架
		L"resource/bookshelf.png",
		// 6.泥土
		L"resource/dirt.png",
		// 7.草方块
		L"resource/grass_top.png",
		L"resource/grass_side.png",
		// 8.萤石
		L"resource/glowstone.png"
	};



	// ---------------------------------------------------------------------------------------------------------------



	// 我们要渲染物品栏内的方块，之所以在 2D 平面上也能呈现立体感，是因为它们使用轴侧视图
	// 物品栏中的方块以固定的等轴测视角显示，这样能使 正面 (+X)，后面 (-Z)，上面 (+Y) 同时扁平化呈现在平面上
	// 我们需要提供正方体数据，对这个正方体进行等轴侧变换，再往这个扁平化的正方体贴纹理 (D2D 位图)

	// 等轴变换矩阵 (模型空间 -> 屏幕空间)
	XMMATRIX IsometricMatrix;


	// 方块三个面 { 正面 (+X)，后面 (-Z)，上面 (+Y) } NDC 空间下的顶点位置数据
	// 顺序遵循 左上角 -> 右上角 -> 右下角 -> 左下角
	std::vector<XMVECTOR> BlockItemsVertex =
	{
		// 正面 (+X)
		XMVectorSet(1, 1, -1, 1),
		XMVectorSet(1, 1, 1, 1),
		XMVectorSet(1, -1, 1, 1),
		XMVectorSet(1, -1, -1, 1),

		// 后面 (-Z)
		XMVectorSet(-1, 1, -1, 1),
		XMVectorSet(1, 1, -1, 1),
		XMVectorSet(1, -1, -1, 1),
		XMVectorSet(-1, -1, -1, 1),

		// 上面 (+Y)
		XMVectorSet(-1, 1, -1, 1),
		XMVectorSet(-1, 1, 1, 1),
		XMVectorSet(1, 1, 1, 1),
		XMVectorSet(1, 1, -1, 1)
	};


	// 物品栏立方体面结构体，只有一个长度为 3 的 UINT 数组成员
	// 描述一个方块三个面在 m_InventoryBlockBitmaps 的索引
	struct ITEMCUBEFACE
	{
		// 三个立方体面对应的 D2D 位图在 m_InventoryBlockBitmaps 中的位置
		// 数组索引 0-2 分别对应右面 (+X)，后面 (-Z)，上面 (+Y)
		UINT FaceBitmapIndex[3];
	};


	// 物品栏 9 个方块物品的方块类型-位图索引组
	std::vector<ITEMCUBEFACE> BlockBitmap_IndexGroup =
	{
		// 右面 (+X) -> 后面 (-Z) -> 上面 (+Y)

		{0, 1, 2},		// 0.熔炉
		{3, 4, 5},		// 1.工作台
		{6, 6, 7},		// 2.TNT
		{8, 8, 9},		// 3.橡木原木
		{10, 10, 10},	// 4.橡木木板
		{11, 11, 10},	// 5.书架
		{12, 12, 12},	// 6.泥土
		{14, 14, 13},	// 7.草方块
		{15, 15, 15},	// 8.萤石
	};


	// 物品栏 9 个方块物品的预渲染等轴立体图标
	std::vector<ComPtr<ID2D1Bitmap>> m_InventoryBlockIcons;

	// 位图渲染目标，用于生成并渲染方块图标，位图渲染目标也拥有 m_D2DDeviceContext 相似的能力
	ComPtr<ID2D1BitmapRenderTarget> m_BitmapRenderTarget;


	const float Slot_InnerSpace_Width = 15.0f;	// 物品栏空槽的宽度
	const float Slot_InnerSpace_Height = 15.0f;	// 物品栏空槽的高度

	// 物品栏 (位图渲染目标) 的方框大小
	D2D1_SIZE_F Slot_InnerSpace_Size = { Slot_InnerSpace_Width , Slot_InnerSpace_Height };


	const float BlockBitmap_Width = 16.0f;		// 方块位图的宽度
	const float BlockBitmap_Height = 16.0f;		// 方块位图的宽度

	// 方块纹理源图范围
	D2D1_RECT_F Source_BlockItems_Rect = { 0, 0, BlockBitmap_Width, BlockBitmap_Height };



	// ---------------------------------------------------------------------------------------------------------------



	float Components_Scale_Rate = 2.5;					// 组件放大倍数
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


	D2D_RECT_F Source_InnerSpace_Rect = {};				// 物品栏空槽的源图范围
	D2D_RECT_F Destination_InnerSpace_Rect = {};		// 物品栏空槽的目标范围



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



	// 利用 D2D_STEP01_CreateD3D11Device 创建的 D3D11On12 设备，创建 D2D 相关设备
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
	void D2D_STEP03_CreateD2DRenderTarget(HWND MainWindowHwnd, ComPtr<ID3D12Resource>(&m_D3D12RenderTarget)[3])
	{
		// 利用 GetDpiForWindow 获取窗口的 DPI
		DPI = GetDpiForWindow(MainWindowHwnd);


		// D2D 位图属性，渲染目标其实就是一个特殊纹理 (2D 位图)，这一点在 D3D11 和 D2D 道理也是一样的
		D2D1_BITMAP_PROPERTIES1 BitmapProperties = {};
		// 设置位图选项 (标志)，指定这两个标志，表示创建一个专用渲染目标，作为最终输出的画板，而不是作为中间资源被反复利用
		BitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

		// 渲染目标的 DXGI_FORMAT 指定 DXGI_FORMAT_UNKNOWN，会自动匹配表面格式
		BitmapProperties.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
		// 指定像素颜色值采用预乘 alpha 格式存储，可以减少着色器中的计算
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

			// 用于临时转化和翻译用的 DXGISurface 接口，对于上面那几个渲染目标资源，D2D 只认 DXGISurface
			ComPtr<IDXGISurface> _temp_DXGISurface;

			// 将 D3D11WrappedResource 的数据继承到 DXGISurface
			m_D3D11WrappedRenderTarget[i].As(&_temp_DXGISurface);

			// 利用 D2DDeviceContext 将 DXGISurface 接口转化成 D2DRenderTarget，这样我们就完成了渲染目标从 D3D12 到 D2D 的翻译
			m_D2DDeviceContext->CreateBitmapFromDxgiSurface(_temp_DXGISurface.Get(),
				BitmapProperties, &m_D2DRenderTarget[i]);
		}
	}



	// ---------------------------------------------------------------------------------------------------------------



	// WIC 工厂初始化函数，仅初始化 WIC 工厂，一个进程重复释放创建 WIC 工厂会报错
	void D2D_STEP04_InitializeWICFactory()
	{
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_WICFactory));
	}



	// WIC 从 resource 读取 UI 图集，然后通过 D2DDeviceContext 的成员方法创建并转化成 D2DBitmap
	// Atlas 图集，相当于包含所有界面小元素的大图，是纹理图片的一种形式
	bool D2D_STEP05_LoadUIAtlasIntoD2DBitmaps()
	{
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


		// 创建 m_FlippedHUDBitmap，将上面已经创建好的 m_HUDBitmap 进行镜像翻转，并存储到新的 D2DBitmap 接口
		{
			// WIC 工厂先创建翻转器
			m_WICFactory->CreateBitmapFlipRotator(&m_WICBitmapFlipRotator);

			// 翻转器初始化，将转换后的位图镜像翻转，WICBitmapTransformFlipHorizontal 表示水平镜像翻转
			m_WICBitmapFlipRotator->Initialize(m_WICFormatConverter.Get(), WICBitmapTransformFlipHorizontal);

			// D2D 设备上下文从 WIC 位图资源中创建 D2DBitmap，这个 m_WICBitmapFlipRotator 也是 IWICBitmapSource 的子类
			m_D2DDeviceContext->CreateBitmapFromWicBitmap(m_WICBitmapFlipRotator.Get(), &m_FlippedHUDBitmap);
		}


		// 创建 m_InventoryBlockBitmaps，用于物品栏方块物品
		{
			// m_InventoryBlockBitmaps 先重置大小
			m_InventoryBlockBitmaps.resize(InventoryBlockNames.size());

			// 遍历并逐一加载
			for (UINT i = 0; i < InventoryBlockNames.size(); i++)
			{
				HRESULT hr = m_WICFactory->CreateDecoderFromFilename(InventoryBlockNames[i].c_str(), nullptr, GENERIC_READ,
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

				m_D2DDeviceContext->CreateBitmapFromWicBitmap(m_WICFormatConverter.Get(), &m_InventoryBlockBitmaps[i]);
			}
		}


		// D2DBitmap 都加载成功，返回 true
		return true;
	}



	// DX12Engine 传递要加载的纹理名，WIC 再次读取图片，并将它们转换成 DX12 可用的 WICBitmapSource
	// vector 是一个 inout 输入输出参数，外部 (DX12Engine) 提供 vector，此函数逐一创建 vector 中的元素
	bool D2D_STEP06_LoadTextureIntoWICBitmaps(
		const std::vector<std::wstring>& TextureNames,
		std::vector<ComPtr<IWICBitmapSource>>& TextureGroup)
	{
		// 先获取原先 TextureGroup 拥有元素的数量
		size_t OriginalTextureGroupSize = TextureGroup.size();

		// TextureGroup 先重置大小
		TextureGroup.resize(OriginalTextureGroupSize + TextureNames.size());

		// 循环遍历加载
		for (UINT i = 0; i < TextureNames.size(); i++)
		{
			// 先创建图片解码器，并将图片文件加载到内存
			HRESULT hr = m_WICFactory->CreateDecoderFromFilename(TextureNames[i].c_str(), nullptr, GENERIC_READ,
				WICDecodeMetadataCacheOnDemand, &m_WICBitmapDecoder);

			// 用于格式化字符串
			std::wostringstream output_str;
			// 如果创建失败，就检查 HRESULT 返回值并提示信息
			switch (hr)
			{
				case S_OK: break;	// 解码成功，直接 break 进入下一步即可

				case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):	// 文件找不到
					output_str << L"找不到文件 " << TextureNames[i].c_str() << L" ！请检查文件路径是否有误！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				case HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT):	// 文件句柄正在被另一个应用进程占用
					output_str << L"文件 " << TextureNames[i].c_str() << L" 已经被另一个应用进程打开并占用了！请先关闭那个应用进程！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				case WINCODEC_ERR_COMPONENTNOTFOUND:			// 找不到可解码的组件，说明这不是有效的图像文件
					output_str << L"文件 " << TextureNames[i].c_str() << L" 不是有效的图像文件，无法解码！请检查文件是否为图像文件！";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;

				default:			// 发生其他未知错误
					output_str << L"文件 " << TextureNames[i].c_str() << L" 解码失败！发生了其他错误，错误码：" << hr << L" ，请查阅微软官方文档。";
					MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
					return false;
			}

			// 从解码器中获取一帧图片
			m_WICBitmapDecoder->GetFrame(0, &m_WICBitmapDecodeFrame);


			// 获取图片格式，并将它转化为 DX12 能接受的纹理格式
			WICPixelFormatGUID SourceFormat = {};				// 源图格式
			GUID TargetFormat = {};								// 目标格式

			m_WICBitmapDecodeFrame->GetPixelFormat(&SourceFormat);						// 获取源图格式

			// 获取目标格式，如果没有可支持的目标格式，就返回 false 并提示信息
			if (DX12TextureHelper::GetTargetPixelFormat(&SourceFormat, &TargetFormat) == false)
			{
				::MessageBox(NULL, L"此纹理不受支持!", L"提示", MB_OK);
				return false;
			}


			// 获取目标格式后，将纹理转换为目标格式，使其能被 DX12 使用
			m_WICFactory->CreateFormatConverter(&m_WICFormatConverter);
			// 初始化转换器，实际上是把位图进行了转换
			m_WICFormatConverter->Initialize(m_WICBitmapDecodeFrame.Get(), TargetFormat, WICBitmapDitherTypeNone,
				nullptr, 0.0f, WICBitmapPaletteTypeCustom);
			// 将位图数据继承到 WIC 位图资源，我们等会要在 WIC 位图资源上获取信息
			m_WICFormatConverter.As(&TextureGroup[OriginalTextureGroupSize + i]);
		}

		// WICBitmapSource 都加载成功，返回 true
		return true;
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 计算等轴变换矩阵 (模型空间 -> 屏幕空间)，并对方块物品顶点数据进行等轴变换
	void D2D_STEP07_CalcIsometricMatrixAndTransform()
	{
		// 先绕 y 轴旋转 45°，XM_PIDIV4 = 45°，让正面和后面可见
		XMMATRIX RotateY_Matrix = XMMatrixRotationY(XM_PIDIV4);
		// 再绕 x 轴旋转 -30°，XM_PIDIV2 = 90°，让顶面可见
		XMMATRIX RotateX_Matrix = XMMatrixRotationX(-XM_PIDIV2 / 3.0);
		// 构建旋转矩阵
		XMMATRIX RotationMatrix = RotateY_Matrix * RotateX_Matrix;

		// 构建缩放矩阵 (x,y 轴缩放系数相同，y 轴是负数是因为 D2D 坐标系 y 轴朝下，需要指定负数翻转位图)
		// 这个缩放系数 5 的计算方法:
		// 模型空间下方块的边长是 2 (详情见上文 BlockItemsVertex)，y 轴旋转 45° 后顶面边长 2 * sqrt(2)
		// 2 * sqrt(2) * 5 = 10 * sqrt(2) = sqrt(200) < sqrt(225) = 15
		// 恰好留了 (sqrt(225) - sqrt(200)) / 2 的空隙，差不多等于 0.5
		XMMATRIX ScalingMatrix = XMMatrixScaling(5, -5, 1);

		// 构建平移矩阵 (一个物品槽内部空位 15x15，从物品槽左上角移到正中心偏左 0.5 像素)
		XMMATRIX TranslationMatrix = XMMatrixTranslation(7, 7, 0);

		// 最终构建等轴变换矩阵 (旋转 -> 缩放 -> 平移，顺序不满足乘法交换律)
		// 注意！均匀缩放可以和旋转矩阵交换位置，结果不变；非均匀缩放不可以交换！
		IsometricMatrix = RotationMatrix * ScalingMatrix * TranslationMatrix;


		// 对每个顶点进行等轴变换
		for (UINT i = 0; i < BlockItemsVertex.size(); i++)
		{
			BlockItemsVertex[i] = XMVector3TransformCoord(BlockItemsVertex[i], IsometricMatrix);
		}
	}



	// 对每个方块物品的顶点数据进行等轴变换，并绘制相应面的 D2D 位图，生成预渲染立体图标
	void D2D_STEP08_GenerateBlockItemIcons()
	{
		// m_InventoryBlockIcons 重置大小为 9，等会要进行位图创建
		m_InventoryBlockIcons.resize(BlockBitmap_IndexGroup.size());


		// 循环遍历每个方块，生成 9 个图标
		// BlockIndex 是每个方块 (m_InventoryBlockIcons/BlockBitmap_IndexGroup 每个元素) 的索引
		for (UINT BlockIndex = 0; BlockIndex < BlockBitmap_IndexGroup.size(); BlockIndex++)
		{
			// 为每个不同的方块图标，创建一个 D2D 可兼容的新位图渲染目标 
			// (Compatible 可兼容的，早些时候 D3D10、D3D11 的渲染目标资源绑定，渲染到纹理也是它做的)
			m_D2DDeviceContext->CreateCompatibleRenderTarget(
				D2D1::SizeF(Slot_InnerSpace_Width, Slot_InnerSpace_Height),	// 位图渲染目标的大小 (15x15)
				D2D1::SizeU(DPI, DPI),										// 位图渲染目标 DPI 大小
				m_InventoryBlockBitmaps[0]->GetPixelFormat(),				// 位图渲染目标的 D2D 格式，要和原资源一致									
				&m_BitmapRenderTarget										// 要创建的位图渲染目标接口
			);

			// 位图渲染目标开启渲染
			m_BitmapRenderTarget->BeginDraw();

			// 位图渲染目标清空背景为透明黑色 (不填参数默认透明黑色)，这样我们就得到了背景透明的位图，后续方便混合
			m_BitmapRenderTarget->Clear();


			// 循环遍历每个面，绘制三个面到位图渲染目标中 { 正面 (+X)，后面 (-Z)，上面 (+Y) }
			// 到下一个面的步长是 4，所以不同面顶点计算式 VertexIndex = FaceIndex * 4 + ConnerPointIndex
			for (UINT FaceIndex = 0; FaceIndex < 3; FaceIndex++)
			{
				// 提取每个面左上角 P0 (0, 0)，右上角 P1 (w, 0)，左下角 P2 (0, h) 的坐标
				XMFLOAT2 P0, P1, P2;
				XMStoreFloat2(&P0, BlockItemsVertex[FaceIndex * 4 + 0]);	// 左上角
				XMStoreFloat2(&P1, BlockItemsVertex[FaceIndex * 4 + 1]);	// 右上角
				XMStoreFloat2(&P2, BlockItemsVertex[FaceIndex * 4 + 3]);	// 左下角


				// 预渲染的绘制需要 3x2 仿射矩阵，由于 2D 变换最后一列是固定的 [0, 0, 1]，D2D 索性把它简化了
				// 这个二维仿射变换包含缩放，旋转，错切与平移，它表示将 纹理位图 (矩形) 映射到 方块面平行四边形
				// 你会疑惑方块面明明有四个点，为什么只用了其中三个，因为三个点就可以唯一确定一个仿射变换了
				// 关于矩阵中每一项的推导过程，可以问 AI
				D2D1_MATRIX_3X2_F BitmapAffineMatrix = {
					(P1.x - P0.x) / BlockBitmap_Width, (P1.y - P0.y) / BlockBitmap_Width,
					(P2.x - P0.x) / BlockBitmap_Height, (P2.y - P0.y) / BlockBitmap_Height,
					P0.x, P0.y
				};


				// 位图渲染目标设置并更新仿射矩阵，接下来会影响到下面的渲染操作
				m_BitmapRenderTarget->SetTransform(BitmapAffineMatrix);

				// 获得对应方块下某个面对应的纹理 (位图) 索引
				UINT FaceTexture_InBitmapsArrayIndex = BlockBitmap_IndexGroup[BlockIndex].FaceBitmapIndex[FaceIndex];

				// 渲染目标绘制位图，将位图渲染到对应的方块面上
				m_BitmapRenderTarget->DrawBitmap(
					m_InventoryBlockBitmaps[FaceTexture_InBitmapsArrayIndex].Get(),
					Source_BlockItems_Rect
				);
			}


			// 三个面都绘制完成，结束渲染
			m_BitmapRenderTarget->EndDraw();
			// 将绘制好的位图导出到对应的 D2Dbitmap，生成预渲染位图
			m_BitmapRenderTarget->GetBitmap(&m_InventoryBlockIcons[BlockIndex]);
		}
	}



	// ---------------------------------------------------------------------------------------------------------------



	// D2D 渲染函数之一，在 BeginDraw - EndDraw 之间，绘制 2D UI 元素
	void D2DRenderBranch_UIElements(const UINT& WindowWidth, const UINT& WindowHeight)
	{
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
	}


	// D2D 渲染函数之一，在 BeginDraw - EndDraw 之间，绘制物品栏上的方块物品
	void D2DRenderBranch_InventoryItems()
	{
		// 源图大小 15x15
		Source_InnerSpace_Rect.left = 0;
		Source_InnerSpace_Rect.top = 0;
		Source_InnerSpace_Rect.right = Source_InnerSpace_Rect.left + Slot_InnerSpace_Width;
		Source_InnerSpace_Rect.bottom = Source_InnerSpace_Rect.top + Slot_InnerSpace_Height;

		// 目标区域 (物品栏空槽) 在左上角 +3，注意这个 +3 也要与缩放系数相乘
		Destination_InnerSpace_Rect.left = Destination_HotBarInventory_Rect.left + 3 * Components_Scale_Rate;
		Destination_InnerSpace_Rect.top = Destination_HotBarInventory_Rect.top + 3 * Components_Scale_Rate;
		Destination_InnerSpace_Rect.right = Destination_InnerSpace_Rect.left + Slot_InnerSpace_Width * Components_Scale_Rate;
		Destination_InnerSpace_Rect.bottom = Destination_InnerSpace_Rect.top + Slot_InnerSpace_Height * Components_Scale_Rate;

		// 逐一绘制每个方块物品立体图标
		for (UINT i = 0; i < m_InventoryBlockIcons.size(); i++)
		{
			// 绘制物品立体图标
			m_D2DDeviceContext->DrawBitmap(m_InventoryBlockIcons[i].Get(), Destination_InnerSpace_Rect,
				1, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, Source_InnerSpace_Rect);

			// 偏移到对应的空槽，一个槽位 20 x 20 (180 / 9 = 20)
			Destination_InnerSpace_Rect.left += 20 * Components_Scale_Rate;
			Destination_InnerSpace_Rect.right += 20 * Components_Scale_Rate;
		}
	}


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


		// 先绘制 2D UI 元素
		D2DRenderBranch_UIElements(WindowWidth, WindowHeight);


		// 再绘制物品栏上的方块物品
		D2DRenderBranch_InventoryItems();


		// D2D 设备上下文结束 2D 渲染！D2DDeviceContext 结束对渲染命令的记录，准备提交给 GPU
		m_D2DDeviceContext->EndDraw();


		// D3D11On12 设备告诉包装资源 (D3D11RenderTarget) 进入 OutState (D3D12_RESOURCE_STATE_PRESENT) 呈现状态
		// 说明 D2D 已经渲染完成了，接下来释放包装资源的"内部所有权"和"状态转换独占权"，将这些交还给 D3D12 层设备
		// 另外这个操作还会自动插入一个 RenderTarget -> Present 的资源屏障，所以 end_barrier 我们不用了，防止状态重复转换
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

	// 获取 WIC 纹理资源的 BitsPerPixel 图像深度
	inline UINT Get_WICTexture_BitsPerPixel(const ComPtr<IWICBitmapSource>& TextureWICResource)
	{
		UINT BitsPerPixel = 0;		// 图像深度

		// 获取纹理的 WIC 纹理格式
		WICPixelFormatGUID _temp_WICPixelFormat = {};
		TextureWICResource->GetPixelFormat(&_temp_WICPixelFormat);

		ComPtr<IWICComponentInfo> _temp_WICComponentInfo = {};			// 用于获取 BitsPerPixel 纹理图像深度
		ComPtr<IWICPixelFormatInfo> _temp_WICPixelInfo = {};			// 用于获取 BitsPerPixel 纹理图像深度
		m_WICFactory->CreateComponentInfo(_temp_WICPixelFormat, &_temp_WICComponentInfo);
		_temp_WICComponentInfo.As(&_temp_WICPixelInfo);
		_temp_WICPixelInfo->GetBitsPerPixel(&BitsPerPixel);				// 获取 BitsPerPixel 图像深度

		return BitsPerPixel;
	}
};



// ---------------------------------------------------------------------------------------------------------------



// 摄像机类
class Camera
{
private:

	XMVECTOR EyePosition = XMVectorSet(4, 4, 2, 1);			// 摄像机在世界空间下的位置
	XMVECTOR FocusPosition = XMVectorSet(0, 0, 0, 1);		// 摄像机在世界空间下观察的焦点位置
	XMVECTOR UpDirection = XMVectorSet(0, 1, 0, 0);			// 世界空间垂直向上的向量

	// 摄像机观察方向的单位向量，用于前后移动
	XMVECTOR ViewDirection = XMVector3Normalize(FocusPosition - EyePosition);

	// 焦距，摄像机原点与焦点的距离，焦距会影响摄像机旋转视角的速度
	float FocalLength = XMVectorGetX(XMVector3Length(FocusPosition - EyePosition));

	// 摄像机向右方向的单位向量，用于左右移动，XMVector3Cross 求两向量叉乘
	// 注意叉乘不符合交换律，交换后结果方向相反，如果左右移动方向反了，可能需要检查一下叉乘
	XMVECTOR RightDirection = XMVector3Normalize(XMVector3Cross(UpDirection, ViewDirection));

	POINT LastCursorPoint = {};								// 上一次鼠标的位置
	POINT CurrentCursorPoint = {};							// 当前鼠标的位置

	float FovAngleY = XM_PIDIV4;							// 垂直视场角
	float AspectRatio = 16.0 / 9.0;							// 投影窗口宽高比
	float NearZ = 0.1;										// 近平面到原点的距离
	float FarZ = 1000;										// 远平面到原点的距离

	XMMATRIX ModelMatrix;									// 模型矩阵，模型空间 -> 世界空间
	XMMATRIX ViewMatrix;									// 观察矩阵，世界空间 -> 观察空间
	XMMATRIX ProjectionMatrix;								// 投影矩阵，观察空间 -> 齐次裁剪空间

	XMMATRIX MVPMatrix;										// MVP 矩阵，类外需要用公有方法 GetMVPMatrix 获取


	// 视锥体包围盒
	BoundingFrustum CameraFrustum;
	// 围成视锥体 6 个平面的平面方程
	std::vector<XMVECTOR> FrustumPlanes = std::vector<XMVECTOR>(6);



	// ---------------------------------------------------------------------------------------------------------------

public:

	Camera()	// 摄像机的构造函数
	{
		// 模型矩阵，这里设置成单位矩阵，是因为模型导入的时候已经是 y 轴朝上的了，无需再进行旋转
		ModelMatrix = XMMatrixIdentity();
		// 观察矩阵，注意前两个参数是点，第三个参数才是向量
		ViewMatrix = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);
		// 投影矩阵 (注意近平面和远平面距离不能 <= 0!)
		ProjectionMatrix = XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ);
	}

	// 摄像机前后移动，参数 Stride 是移动速度 (步长)，正数向前移动，负数向后移动
	void Walk(float Stride)
	{
		EyePosition += Stride * ViewDirection;
		FocusPosition += Stride * ViewDirection;
	}

	// 摄像机左右移动，参数 Stride 是移动速度 (步长)，正数向右移动，负数向左移动
	void Strafe(float Stride)
	{
		EyePosition += Stride * RightDirection;
		FocusPosition += Stride * RightDirection;
	}

	// 鼠标在屏幕空间 y 轴上移动，相当于摄像机以向右的向量 RightDirection 向上向下旋转，人眼往上下看
	void RotateByY(float angleY)
	{
		// 以向右向量为轴构建旋转矩阵，旋转 ViewDirection 和 UpDirection
		XMMATRIX R = XMMatrixRotationAxis(RightDirection, angleY);

		UpDirection = XMVector3TransformNormal(UpDirection, R);
		ViewDirection = XMVector3TransformNormal(ViewDirection, R);

		// 利用 ViewDirection 观察向量、FocalLength 焦距，更新焦点位置
		FocusPosition = EyePosition + ViewDirection * FocalLength;
	}

	// 鼠标在屏幕空间 x 轴上移动，相当于摄像机绕世界空间的 y 轴向左向右旋转，人眼往左右看
	void RotateByX(float angleX)
	{
		// 以世界坐标系下的 y 轴 (0,1,0,0) 构建旋转矩阵，三个向量 ViewDirection, UpDirection, RightDirection 都要旋转
		XMMATRIX R = XMMatrixRotationY(angleX);

		UpDirection = XMVector3TransformNormal(UpDirection, R);
		ViewDirection = XMVector3TransformNormal(ViewDirection, R);
		RightDirection = XMVector3TransformNormal(RightDirection, R);

		// 利用 ViewDirection 观察向量、FocalLength 焦距，更新焦点位置
		FocusPosition = EyePosition + ViewDirection * FocalLength;
	}

	// 更新上一次的鼠标位置，此函数只负责 GetCursorPos，不负责 SetCursorPos
	inline void UpdateLastCursorPos()
	{
		GetCursorPos(&LastCursorPoint);
	}

	// 更新当前鼠标的位置，此函数只负责 GetCursorPos，不负责 SetCursorPos
	inline void UpdateCurrentCursorPos()
	{
		GetCursorPos(&CurrentCursorPoint);
	}

	// 当鼠标移动时，旋转摄像机视角，此函数不负责任何 GetCursorPos / SetCursorPos
	void CameraRotate()
	{
		// 根据鼠标在屏幕坐标系的 x,y 轴的偏移量，计算摄像机旋转角
		float AngleX = XMConvertToRadians(0.25 * static_cast<float>(CurrentCursorPoint.x - LastCursorPoint.x));
		float AngleY = XMConvertToRadians(0.25 * static_cast<float>(CurrentCursorPoint.y - LastCursorPoint.y));

		// 旋转摄像机
		RotateByY(AngleY);
		RotateByX(AngleX);
	}

	// 更新 MVP 矩阵
	void UpdateMVPMatrix()
	{
		// 主要是更新观察矩阵
		ViewMatrix = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);
		MVPMatrix = ModelMatrix * ViewMatrix * ProjectionMatrix;
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 获取 MVP 矩阵
	inline XMMATRIX& GetMVPMatrix()
	{
		// 每次返回前，都更新一次
		UpdateMVPMatrix();
		return MVPMatrix;
	}

	// 获取观察矩阵的逆矩阵 (观察空间 -> 世界空间)
	inline XMMATRIX GetInverseViewMatrix()
	{
		UpdateMVPMatrix();
		// 矩阵求逆，这个函数还可以顺带算行列式，第一个参数是原矩阵的行列式，如果需要的话可以用个变量接着
		return XMMatrixInverse(nullptr, ViewMatrix);
	}

	// 获取投影矩阵的逆矩阵 (齐次裁剪空间 -> 观察空间)
	inline XMMATRIX GetInverseProjectionMatrix()
	{
		UpdateMVPMatrix();
		return XMMatrixInverse(nullptr, ProjectionMatrix);
	}

	// 获取摄像机位置
	inline XMVECTOR GetEyePosition()
	{
		return EyePosition;
	}

	// 获取摄像机焦点
	inline XMVECTOR GetFocusPosition()
	{
		return FocusPosition;
	}

	// 获取观察向量，这里需要再次进行单位化，否则可能会有匪夷所思的运行时错误
	inline XMVECTOR GetViewDirection()
	{
		return XMVector3Normalize(ViewDirection);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 设置摄像机位置
	inline void SetEyePosition(XMVECTOR pos)
	{
		EyePosition = pos;

		// 改变位置后，观察向量、焦距、右方向向量也要改变，否则会发生视角瞬移
		ViewDirection = XMVector3Normalize(FocusPosition - EyePosition);
		FocalLength = XMVectorGetX(XMVector3Length(FocusPosition - EyePosition));
		RightDirection = XMVector3Normalize(XMVector3Cross(UpDirection, ViewDirection));
	}

	// 设置摄像机焦点
	inline void SetFocusPosition(XMVECTOR pos)
	{
		FocusPosition = pos;

		// 改变位置后，观察向量、焦距、右方向向量也要改变，否则会发生视角瞬移
		ViewDirection = XMVector3Normalize(FocusPosition - EyePosition);
		FocalLength = XMVectorGetX(XMVector3Length(FocusPosition - EyePosition));
		RightDirection = XMVector3Normalize(XMVector3Cross(UpDirection, ViewDirection));
	}

	// 设置摄像机的模型矩阵
	inline void SetModelMatrix(XMMATRIX ModelMatrix)
	{
		this->ModelMatrix = ModelMatrix;
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 更新视锥体包围盒
	void UpdateCameraFrustum()
	{
		// 先更新 MVP 矩阵
		UpdateMVPMatrix();

		// 用投影矩阵重新创建一个新的视锥体包围盒
		// 视锥形状 (FOV、near、far、宽高比) 由投影决定，ProjectionMatrix 定义了视锥体的形状
		// 注意！构建出来的视锥体在 View Space 观察空间！不要被 ProjectionMatrix 迷惑了！
		CameraFrustum = BoundingFrustum(ProjectionMatrix);

		// 将视锥体从 ViewSpace 变换到 WorldSpace
		// 这样就能和区块的 AABB 包围盒在同一个世界空间，进行相交检测和视锥剔除了
		// ViewMatrix 的逆矩阵决定视锥体在世界中的位置和朝向
		CameraFrustum.Transform(CameraFrustum, XMMatrixInverse(nullptr, ViewMatrix));
	}


	// 获取视锥体的 6 个平面方程 (平面顺序：近、远、右、左、上、下)
	inline std::vector<XMVECTOR> GetFrustumPlanes()
	{
		// 先更新视锥体
		UpdateCameraFrustum();

		// 通过 BoundingFrustum::GetPlanes 更新并获取近、远、右、左、上、下平面的方程 (系数)
		CameraFrustum.GetPlanes(&FrustumPlanes[0], &FrustumPlanes[1],
			&FrustumPlanes[2], &FrustumPlanes[3], &FrustumPlanes[4], &FrustumPlanes[5]);


		// 注意这里！还要对 6 个平面方程进行 单位化 + 朝向统一，否则可见区块会被全部剔除
		// DirectXMath 不会保证得到的视锥体平面方程 (法线) 全部朝内
		// 有时候可能因为一些微小抖动导致朝向颠倒，CullingShader 反向剔除，我们需要手动保证法线全部朝内
		for (auto& Plane : FrustumPlanes)
		{
			// 对平面方程进行单位化
			Plane = XMPlaneNormalize(Plane);
			// 获取平面与摄像机焦点的点积 (将焦点代入平面方程，看看焦点是否在平面内侧)
			// 如果点积 < 0，说明平面方向反了，朝向向外，需要反转
			if (XMVectorGetX(XMPlaneDotCoord(Plane, FocusPosition)) < 0)
			{
				// 将平面方程 (法线) 进行反转，让平面朝向视锥体内部
				Plane = XMVectorNegate(Plane);
			}
		}

		return FrustumPlanes;
	}
};



// ---------------------------------------------------------------------------------------------------------------



// DX12 引擎，主引擎，用于 3D 物体渲染
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

	ComPtr<ID3D12Fence> m_RenderFence;						// 用于 渲染 + 复制资源 的围栏
	UINT64 FenceValue = 0;									// 用于围栏等待的围栏值
	HANDLE RenderEvent = NULL;								// GPU 渲染事件
	D3D12_RESOURCE_BARRIER beg_barrier = {};				// 渲染开始的资源屏障，呈现 -> 渲染目标

	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;					// DSV 描述符堆
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle;					// DSV 描述符句柄
	ComPtr<ID3D12Resource> m_DepthStencilBuffer;			// DSV 深度模板缓冲资源

	DXGI_FORMAT DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;	// DSV 资源的格式


	// 视口
	D3D12_VIEWPORT ViewPort = D3D12_VIEWPORT{ 0, 0, float(WindowWidth), float(WindowHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	// 裁剪矩形
	D3D12_RECT ScissorRect = D3D12_RECT{ 0, 0, WindowWidth, WindowHeight };



	// ---------------------------------------------------------------------------------------------------------------



	// D2D 引擎对象
	D2DEngine m_D2DEngine;


	// 纹理资源名 (路径) 组，存储需要加载的 3D 渲染纹理名称 (相对路径)
	std::vector<std::wstring> TextureNames =
	{
		L"resource/bedrock.png",				// 0.基岩
		L"resource/furnace_front_off.png",		// 1.熔炉正面
		L"resource/furnace_side.png",			// 2.熔炉侧面
		L"resource/furnace_top.png",			// 3.熔炉顶面
		L"resource/crafting_table_front.png",	// 4.工作台正面
		L"resource/crafting_table_side.png",	// 5.工作台侧面
		L"resource/crafting_table_top.png",		// 6.工作台顶面
		L"resource/tnt_side.png",				// 7.TNT 正面
		L"resource/tnt_top.png",				// 8.TNT 顶面
		L"resource/tnt_bottom.png",				// 9.TNT 底面
		L"resource/log_oak.png",				// 10.橡木原木侧面
		L"resource/log_oak_top.png",			// 11.橡木原木顶面
		L"resource/planks_oak.png",				// 12.橡木木板
		L"resource/bookshelf.png",				// 13.书架
		L"resource/dirt.png",					// 14.泥土
		L"resource/grass_top.png",				// 15.草方块顶部
		L"resource/grass_side.png",				// 16.草方块侧面
		L"resource/glowstone.png",				// 17.萤石

		L"resource/destroy_stage_0.png",		// 18.破坏阶段 (0)
		L"resource/destroy_stage_1.png",		// 19.破坏阶段 (1)
		L"resource/destroy_stage_2.png",		// 20.破坏阶段 (2)
		L"resource/destroy_stage_3.png",		// 21.破坏阶段 (3)
		L"resource/destroy_stage_4.png",		// 22.破坏阶段 (4)
		L"resource/destroy_stage_5.png",		// 23.破坏阶段 (5)
		L"resource/destroy_stage_6.png",		// 24.破坏阶段 (6)
		L"resource/destroy_stage_7.png",		// 25.破坏阶段 (7)
		L"resource/destroy_stage_8.png",		// 26.破坏阶段 (8)
		L"resource/destroy_stage_9.png"			// 27.破坏阶段 (9)
	};


	// 3D 渲染使用的方块 WIC 纹理资源组
	// 当资源全部加载到上传堆，全部 WIC 位图资源 (DX12) 都会被释放，不再让它们占内存
	std::vector<ComPtr<IWICBitmapSource>> m_TextureGroup;



	// ---------------------------------------------------------------------------------------------------------------



	// 纹理数组所有纹理的 DXGI 格式
	DXGI_FORMAT TextureFormat = DXGI_FORMAT_UNKNOWN;

	// 用于 全遮挡完整方块 + 破坏纹理 的纹理数组默认堆资源
	// Texture2DArray m_TextureArray : register(t1, space0);
	ComPtr<ID3D12Resource> m_SRVTextureArray_DefaultResource;
	// 用于 全遮挡完整方块 + 破坏纹理 的纹理数组上传堆资源，用于中转
	ComPtr<ID3D12Resource> m_SRVTextureArray_UploadResource;


	UINT BitsPerPixel = 0;				// 纹理数组所有纹理的图像深度 (单位：比特)
	UINT TextureWidth = 0;				// 纹理数组所有纹理的宽度 (单位：像素)
	UINT TextureHeight = 0;				// 纹理数组所有纹理的高度 (单位：像素)

	UINT64 BytePerRowSize = 0;			// 纹理数组单个纹理每行所占的字节数，用于纹理复制 (单位：字节)
	UINT64 TextureSize = 0;				// 纹理数组单个纹理的真实大小 (单位：字节)
	UINT64 UploadResourceRowSize = 0;	// 对于单个纹理，上传堆资源每行对齐需要的大小 (单位：字节，需要 256 字节对齐)
	UINT64 UploadSubResourceSize = 0;	// 对于单个纹理，上传堆资源所需要分配的总大小 (单位：字节)

	UINT64 UploadArrayElementSize = 0;	// 硬件偏移寻址纹理数组每个元素，上传堆资源分配对齐需要的大小 (单位：字节，需要 512 字节对齐)
	UINT64 UploadResourceSize = 0;		// 对于整个纹理数组，上传堆资源最终要分配的总大小 (单位：字节)


	D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };		// 上传堆属性结构体
	D3D12_HEAP_PROPERTIES DefaultHeapDesc = { D3D12_HEAP_TYPE_DEFAULT };	// 默认堆属性结构体


	// 用于渲染 纹理数组 和 粒子缓冲区 的 SRV 描述符堆 (2 SRV)
	ComPtr<ID3D12DescriptorHeap> m_RenderSRVHeap;

	// 纹理数组的 SRV 描述符 CPU 句柄
	D3D12_CPU_DESCRIPTOR_HANDLE SRVTextureArray_CPUHandle;
	// 纹理数组的 SRV 描述符 GPU 句柄
	D3D12_GPU_DESCRIPTOR_HANDLE SRVTextureArray_GPUHandle;

	// m_ParticlesHiveBuffer 粒子缓冲区在 m_RenderSRVHeap 的 CPU 句柄
	D3D12_CPU_DESCRIPTOR_HANDLE SRVParticlesBuffer_CPUHandle;
	// m_ParticlesHiveBuffer 粒子缓冲区在 m_RenderSRVHeap 的 GPU 句柄
	D3D12_GPU_DESCRIPTOR_HANDLE SRVParticlesBuffer_GPUHandle;



	// ---------------------------------------------------------------------------------------------------------------



	// 立方体面结构体，只有一个 UINT 数组成员
	// 数组索引表示对应的立方体面索引，数组元素值表示对应立方体面的纹理在 Texture Array 的位置
	struct CUBEFACE
	{
		// 六个立方体面对应的纹理在 Texture Array 中的位置
		// 数组索引 0-5 分别对应右面 (+X)，左面 (-X)，前面 (+Z)，后面 (-Z)，上面 (+Y)，下面 (-Y)
		UINT FaceTexture_InArrayIndex[6];
	};

	// 方块类型-纹理索引组，每个 vector 索引表示不同的方块类型，每个 vector 元素值表示对应方块六个面的纹理数据索引数据
	// 在 shader 会根据 逐实例数据 (方块类型) 和 逐顶点数据 (方块每个面对应的纹理索引) 来索引对应的纹理，这样就不用反复换绑 SRV 了
	std::vector<CUBEFACE> BlockCubeTexture_IndexGroup =
	{
		// 一个完整方块有六个面，右面 (+X)，左面 (-X)，前面 (+Z)，后面 (-Z)，上面 (+Y)，下面 (-Y)，我们以右面是方块正面为准

		{0, 0, 0, 0, 0, 0},			// 0.基岩
		{1, 2, 2, 2, 3, 3},			// 1.熔炉
		{4, 4, 5, 5, 6, 6},			// 2.工作台
		{7, 7, 7, 7, 8, 9},			// 3.TNT
		{10, 10, 10, 10, 11, 11},	// 4.橡木原木
		{12, 12, 12, 12, 12, 12},	// 5.橡木木板
		{13, 13, 13, 13, 12, 12},	// 6.书架
		{14, 14, 14, 14, 14, 14},	// 7.泥土
		{16, 16, 16, 16, 15, 14},	// 8.草方块
		{17, 17, 17, 17, 17, 17}	// 9.萤石
	};


	// SRV Structured Buffer 的上传堆资源
	ComPtr<ID3D12Resource> m_SRVStructuredBuffer_UploadResource;
	// SRV Structured Buffer 的默认堆资源
	// StructuredBuffer<CUBEFACE> BlockCubeTexture_IndexGroup : register(t0, space0);
	ComPtr<ID3D12Resource> m_SRVStructuredBuffer_DefaultResource;



	// ---------------------------------------------------------------------------------------------------------------



	// 粒子实例
	struct Particle
	{
		XMFLOAT3 Position;		// 粒子的世界坐标
		XMFLOAT3 Velocity;		// 粒子的速度
		float Life;				// 剩余生命 (单位：秒)，大于 0 表示存活
		UINT BlockType;			// 所属方块类型
	};


	// 管理所有粒子的粒子缓冲区默认堆资源
	// RWStructuredBuffer<Particle> m_ParticlesHiveBuffer : register(u0, space0);	[ParticleShader]
	// StructuredBuffer<Particle> m_ParticlesHiveBuffer : register(t2, space0);		[RenderParticleShader]
	ComPtr<ID3D12Resource> m_SRVUAVParticlesHiveBuffer_DefaultResource;


	// 记录 "空泡" (数据空位) 的空闲栈 (数组栈) 默认堆资源
	// RWStructuredBuffer<uint> m_ParticlesFreeStack : register(u1, space0);
	ComPtr<ID3D12Resource> m_UAVParticlesFreeStack_DefaultResource;


	// 记录空闲栈最近可用 "空泡" 索引的栈顶指针默认堆资源
	// RWByteAddressBuffer m_StackTopPointer : register(u2, space0);
	ComPtr<ID3D12Resource> m_UAVStackTopPointer_DefaultResource;


	// 粒子待生成列表上传堆资源，用于 CPU 持续向计算着色器输入粒子实例数据
	// StructuredBuffer<Particle> m_SpawnParticlesList : register(t0, space0);
	ComPtr<ID3D12Resource> m_SRVSpawnParticlesList_UploadResource;


	// 待生成列表中，每帧新粒子的总数量
	// cbuffer SpawnParticlesData : register(b0, space0);
	UINT SpawnParticlesCount = 0;


	// cbuffer UpdateParticlesData : register(b1, space0)
	// 相比上一帧，当前帧过去的时间，用于计算速度与位移
	float PerFrameDataTime = 0;
	// 粒子的重力数据
	const float GravityData = 1.5f;

	// 粒子缓冲最多可以存储 128 个粒子
	const UINT MaxAllocParticlesNums = 128;
	// SpawnCSMain 一个线程组包含的线程数量
	const UINT SpawnCSMainThreadNums = 16;
	// UpdateCSMain 一个线程组包含的线程数量
	const UINT UpdateCSMainThreadNums = 16;
	// InitCSMain 一个线程组包含的线程数量
	const UINT InitCSMainThreadNums = 16;
	

	// 指向 m_SRVSpawnParticlesList_UploadResource 的上传堆数据指针，表示准备要上传到计算着色器的目标缓冲地址
	BYTE* ReadySpawnParticlesListPointer = nullptr;



	// ---------------------------------------------------------------------------------------------------------------



	// 用于 ParticleShader 的根签名
	ComPtr<ID3D12RootSignature> m_ParticleRootSignature;
	// 用于 SpawnCSMain 每帧生成新粒子的 PSO
	ComPtr<ID3D12PipelineState> m_ParticleSpawnPSO;
	// 用于 UpdateCSMain 每帧更新粒子的 PSO
	ComPtr<ID3D12PipelineState> m_ParticleUpdatePSO;
	// 用于 InitCSMain 开始时初始资源的 PSO
	ComPtr<ID3D12PipelineState> m_ParticleInitPSO;


	// CBV/SRV/UAV 描述符的大小，这三个描述符大小是相同的
	UINT CBVSRVUAVDescriptorSize = 0;

	// 用于 ParticleShader 的 SRV/UAV 描述符堆 (3 UAV + 1 SRV)
	ComPtr<ID3D12DescriptorHeap> m_ParticleUAVSRVHeap;
	// m_ParticleUAVSRVHeap 的起始偏移 CPU 句柄，用于创建描述符
	D3D12_CPU_DESCRIPTOR_HANDLE ParticleUAVSRVHeap_CPUBaseHandle;
	// m_ParticleUAVSRVHeap 的起始偏移 GPU 句柄，用于快速绑定描述符到渲染管线
	D3D12_GPU_DESCRIPTOR_HANDLE ParticleUAVSRVHeap_GPUBaseHandle;


	// 将 m_ParticlesHiveBuffer 从 UNORDERED_ACCESS -> NON_PIXEL_SHADER_RESOURCE 的资源屏障
	D3D12_RESOURCE_BARRIER UAVToSRV_barrier;
	// 将 m_ParticlesHiveBuffer 从 NON_PIXEL_SHADER_RESOURCE -> UNORDERED_ACCESS 的资源屏障
	D3D12_RESOURCE_BARRIER SRVToUAV_barrier;



	// ---------------------------------------------------------------------------------------------------------------



	// 用于渲染方块的常量缓冲资源
	// cbuffer GlobalData : register(b0, space0)
	ComPtr<ID3D12Resource> m_CBVRenderBlock_UploadResource;

	// 常量缓冲结构体
	struct RenderCBuffer
	{
		// MVP 矩阵，用于将顶点数据从顶点空间变换到齐次裁剪空间
		XMFLOAT4X4 MVPMatrix;
		// 方块朝向 (0-5 分别对应 右左前后上下，6-8 用于特殊方块)，存储的旋转到对应朝向的旋转矩阵
		XMFLOAT4X4 BlockFaceForwardMatrix[9];
		// 方块破坏阶段使用纹理的索引
		UINT DestroyStage;
	};

	// 常量缓冲结构体指针，下文 Map 后指针会指向 RenderCBVResource 的地址
	RenderCBuffer* RenderCBufferPointer = nullptr;

	Camera m_FirstCamera;		// 第一人称摄像机



	// ---------------------------------------------------------------------------------------------------------------



	// 渲染方块，粒子，破坏纹理使用的根签名
	ComPtr<ID3D12RootSignature> m_RenderRootSignature;
	// 渲染方块的 PSO
	ComPtr<ID3D12PipelineState> m_RenderBlockPSO;
	// 渲染粒子的 PSO
	ComPtr<ID3D12PipelineState> m_RenderParticlePSO;
	// 渲染破坏纹理的 PSO
	ComPtr<ID3D12PipelineState> m_DestroyStagePSO;



	// ---------------------------------------------------------------------------------------------------------------



	// Vertex Buffer View (VBV) 顶点缓冲描述符数组, VBV0 是逐顶点流，VBV1 是逐实例流
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView[2] = {};
	// Index Buffer View (IBV) 索引缓冲描述符
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};


	// 方块顶点结构体
	struct VERTEX
	{
		XMFLOAT4 Position;		// 顶点在方块自身的模型空间的位置
		XMFLOAT2 TexcoordUV;	// 顶点纹理 UV
		UINT FaceIndex;			// 顶点所属的立方体面索引
	};


	// 每个方块实例共用的顶点数据 (逐顶点流)
	std::vector<VERTEX> PreBlockVertexData =
	{
		// 一个完整方块有六个面，右面 (+X)，左面 (-X)，前面 (+Z)，后面 (-Z)，上面 (+Y)，下面 (-Y)，我们以右面是方块正面为准
		// 顺序遵循 左上角 -> 右上角 -> 右下角 -> 左下角

		// 右面 (+X, FaceIndex = 0)
		{ XMFLOAT4(0.5, 0.5, -0.5, 1), XMFLOAT2(0, 0), 0 },
		{ XMFLOAT4(0.5, 0.5, 0.5, 1), XMFLOAT2(1, 0), 0 },
		{ XMFLOAT4(0.5, -0.5, 0.5, 1), XMFLOAT2(1, 1), 0 },
		{ XMFLOAT4(0.5, -0.5, -0.5, 1), XMFLOAT2(0, 1), 0 },

		// 左面 (-X, FaceIndex = 0.5)
		{ XMFLOAT4(-0.5, 0.5, 0.5, 1), XMFLOAT2(0, 0), 1 },
		{ XMFLOAT4(-0.5, 0.5, -0.5, 1), XMFLOAT2(1, 0), 1 },
		{ XMFLOAT4(-0.5, -0.5, -0.5, 1), XMFLOAT2(1, 1), 1 },
		{ XMFLOAT4(-0.5, -0.5, 0.5, 1), XMFLOAT2(0, 1), 1 },

		// 前面 (+Z, FaceIndex = 2)
		{ XMFLOAT4(0.5, 0.5, 0.5, 1), XMFLOAT2(0, 0), 2 },
		{ XMFLOAT4(-0.5, 0.5, 0.5, 1), XMFLOAT2(1, 0), 2 },
		{ XMFLOAT4(-0.5, -0.5, 0.5, 1), XMFLOAT2(1, 1), 2 },
		{ XMFLOAT4(0.5, -0.5, 0.5, 1), XMFLOAT2(0, 1), 2 },

		// 后面 (-Z, FaceIndex = 3)
		{ XMFLOAT4(-0.5, 0.5, -0.5, 1), XMFLOAT2(0, 0), 3 },
		{ XMFLOAT4(0.5, 0.5, -0.5, 1), XMFLOAT2(1, 0), 3 },
		{ XMFLOAT4(0.5, -0.5, -0.5, 1), XMFLOAT2(1, 1), 3 },
		{ XMFLOAT4(-0.5, -0.5, -0.5, 1), XMFLOAT2(0, 1), 3 },

		// 上面 (+Y, FaceIndex = 4)
		{ XMFLOAT4(-0.5, 0.5, -0.5, 1), XMFLOAT2(0, 0), 4 },
		{ XMFLOAT4(-0.5, 0.5, 0.5, 1), XMFLOAT2(1, 0), 4 },
		{ XMFLOAT4(0.5, 0.5, 0.5, 1), XMFLOAT2(1, 1), 4 },
		{ XMFLOAT4(0.5, 0.5, -0.5, 1), XMFLOAT2(0, 1), 4 },

		// 下面 (-Y, FaceIndex = 5)
		{ XMFLOAT4(0.5, -0.5, -0.5, 1), XMFLOAT2(0, 0), 5 },
		{ XMFLOAT4(0.5, -0.5, 0.5, 1), XMFLOAT2(1, 0), 5 },
		{ XMFLOAT4(-0.5, -0.5, 0.5, 1), XMFLOAT2(1, 1), 5 },
		{ XMFLOAT4(-0.5, -0.5, -0.5, 1), XMFLOAT2(0, 1), 5 }
	};


	// 每个方块实例共用的索引数据
	std::vector<UINT> PreBlockIndexData =
	{
		// 右面
		0, 1, 2, 0, 2, 3,
		// 左面
		4, 5, 6, 4, 6, 7,
		// 前面
		8, 9, 10, 8, 10, 11,
		// 后面
		12, 13, 14, 12, 14, 15,
		// 上面
		16, 17, 18, 16, 18, 19,
		// 下面
		20, 21, 22, 20, 22, 23
	};


	// 方块实例结构体
	struct BLOCK_INSTANCE
	{
		XMFLOAT3 BlockOffset;	// 每个方块实例距离世界中心 (0, 0, 0) 的位移
		UINT BlockType;			// 方块类型
		UINT RotateIndex;		// 方块的面朝向 (旋转矩阵在 BlockFaceForwardMatrix 的索引)
	};


	// 方块实例组，存储每一个方块实例
	std::vector<BLOCK_INSTANCE> BlockGroup;


	// 上传堆顶点资源
	ComPtr<ID3D12Resource> m_BlockVertexResource;
	// 上传堆索引资源
	ComPtr<ID3D12Resource> m_BlockIndexResource;
	// 上传堆实例资源
	ComPtr<ID3D12Resource> m_BlockInstanceResource;


	// 上传堆实例资源最大能存储的方块数量 (这里设置最多能放 10000 个方块)
	const size_t MaxBlockInstanceCount = 10000;


	// 指向 m_BlockInstanceResource 所有方块实例资源的指针，注意指针类型是 BLOCK_INSTANCE！
	BLOCK_INSTANCE* BlockInstanceMapPointer = nullptr;



	// ---------------------------------------------------------------------------------------------------------------



	// 高分辨率计时器的频率，注意！在 WM_CREATE 创建窗口时需要获取一次频率
	// 这个频率是由硬件决定的，基本上不会变，获取一次就行，如果忘记获取，破坏的方块就会瞬间变成基岩，请注意！
	LARGE_INTEGER CounterFrequency = {};
	// 起始破坏时间刻，用于计算方块破坏时间
	LARGE_INTEGER BeginDestroyRecordedTick = {};
	// 当前破坏时间刻，用于计算方块破坏时间
	LARGE_INTEGER CurrentDestroyTick = {};
	// 上一帧记录的时间刻，用于计算粒子
	LARGE_INTEGER LastFrameTick = {};
	// 当前帧的时间刻，用于计算粒子
	LARGE_INTEGER CurrentFrameTick = {};


	// 当前破坏阶段纹理索引 (-1 表示无方块破坏)
	UINT DestroyStageIndex = -1;

	// 要破坏方块实例的索引 (-1 表示无方块破坏)
	UINT DestroyInstanceIndex = -1;


	// 判断数据是否被更新的脏数据标志，如果为真就触发下面 UpdateConstantBuffer 的数据全量更新
	bool isDirtyData = true;


	// 在原版游戏中移动鼠标就可以旋转摄像机视角，鼠标光标移动后会自动重置到窗口中央，这个叫 "鼠标捕获"
	// 光标重置的 SetCursorPos 会额外发送一条 WM_MOUSEMOVE 消息，这个额外的消息会把光标移回去，导致视角卡住动不了
	// 如果是正常的鼠标移动，就利用这个标志旋转摄像机，同时计算并重置光标位置
	// 如果是 SetCursorPos 产生的消息，就利用这个标志丢弃不处理
	bool isAutoResetCursorMsg = true;


	// 判断鼠标左键是否按下/松开，破坏方块的一切前提是鼠标左键处于按下状态，这个标志很重要，能决定方块的持续破坏
	bool isLeftButtonPressDown = false;


	// 物品快捷栏方块类型在结构化缓冲的索引，如果有一种数据结构可以很方便记录索引，还能动态扩容/绑定就好了...
	const std::vector<UINT> InventoryBlockTypeGroup = {
		1,	// 0.熔炉
		2,	// 1.工作台
		3,	// 2.TNT
		4,	// 3.橡木原木
		5,	// 4.橡木木板
		6,	// 5.书架
		7,	// 6.泥土
		8,	// 7.草方块
		9	// 8.萤石
	};


	// CPU 端新粒子待生成列表，预分配 128 个粒子的内存
	std::vector<Particle> SpawnParticleList = std::vector<Particle>(MaxAllocParticlesNums);


	// 破坏类型
	enum DESTROY_TYPE
	{
		BLOCK_DESTROYING,			// 方块正在被破坏
		BLOCK_COMPLETE_DESTROY		// 方块已经完成破坏
	};



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
		m_hwnd = CreateWindow(wc.lpszClassName, L"Minecraft (按 Esc 键或 / 键退出)", WS_SYSMENU | WS_OVERLAPPED,
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


	// 屏蔽 MissingClearValue 带来的调试层警告刷屏，这些警告无关紧要
	void STEP04_IgnoreClearValueWarning()
	{
#if defined(_DEBUG)

		// 临时调试层消息队列，用于获取并屏蔽 D3D12 WARNING
		ComPtr<ID3D12InfoQueue> _temp_DebugInfoQueue;

		// 将 D3D12 设备的数据继承到新的消息队列接口，创建调试层消息队列
		m_D3D12Device.As(&_temp_DebugInfoQueue);

		// 定义要抑制的警告 ID
		D3D12_MESSAGE_ID hideMessages[1] =
		{
			// 没设置 ClearValue 的警告 ID
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
		};

		// 调试层消息过滤结构体
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = 1;					// 要屏蔽的消息 ID 数
		filter.DenyList.pIDList = hideMessages;		// 指向消息数据的指针

		// 向消息过滤器添加要屏蔽的 Warning/Error，这样就不用见到 Warning 在下面的调试窗口刷屏了
		// 慎用消息过滤，除非迫不得已 (就像现在这样)，否则不要使用，会错过很多一击致命的问题根源
		_temp_DebugInfoQueue->AddStorageFilterEntries(&filter);

#endif
	}


	// 创建命令三件套
	void STEP05_CreateCommandComponents()
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
	void STEP06_CreateRenderTarget()
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
	void STEP07_CreateFenceAndBarrier()
	{
		// 创建 CPU 上的等待事件，注意第二个参数填 false 表示自动重置事件 (每当经过一次 Wait 函数，自动重置无信号状态)
		// 第三个初始状态参数填 false 表示无信号状态，后面有 copy 动作，防止资源冲突
		RenderEvent = CreateEvent(nullptr, false, false, nullptr);

		// 创建围栏，设定初始值为 0
		m_D3D12Device->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_RenderFence));


		// 设置资源屏障
		// beg_barrier 起始屏障：Present 呈现状态 -> Render Target 渲染目标状态
		beg_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		beg_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		beg_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}


	// 创建 DSV 深度模板描述符堆 (Non-Shader Visible)
	void STEP08_CreateDSVHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc = {};		// DSV 描述符堆结构体
		DSVHeapDesc.NumDescriptors = 1;						// 描述符只有 1 个，因为我们只有一个渲染目标
		DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// 描述符堆类型

		// 创建 DSV 描述符堆 (Depth Stencil View，深度模板描述符)，用于深度测试与模板测试
		m_D3D12Device->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(&m_DSVHeap));

		// 获取 DSV 的 CPU 句柄
		DSVHandle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
	}


	// 创建深度与模板缓冲，用于开启深度测试，渲染物体正确的深度与遮挡关系
	void STEP09_CreateDepthStencilBuffer()
	{
		D3D12_RESOURCE_DESC DSVResourceDesc = {};							// 深度模板缓冲资源信息结构体
		DSVResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		// 深度缓冲其实也是一块纹理
		DSVResourceDesc.Format = DSVFormat;									// 资源纹理格式
		DSVResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;				// 深度缓冲的布局也是 UNKNOWN
		DSVResourceDesc.Width = WindowWidth;								// 宽度和渲染目标一致
		DSVResourceDesc.Height = WindowHeight;								// 高度和渲染目标一致
		DSVResourceDesc.MipLevels = 1;										// Mipmap 层级，设置为 1 就行
		DSVResourceDesc.DepthOrArraySize = 1;								// 纹理数组大小 (3D 纹理深度),设置为 1 就行
		DSVResourceDesc.SampleDesc.Count = 1;								// 采样次数，设置为 1 就行
		DSVResourceDesc.SampleDesc.Quality = 0;								// 采样质量，设置为 0 就行
		DSVResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// 资源标志

		D3D12_CLEAR_VALUE DepthStencilBufferClearValue = {};				// 用于清空深度缓冲的信息结构体，DX12 能对这个进行优化
		DepthStencilBufferClearValue.DepthStencil.Depth = 1.0f;				// 要清空到的深度值，清空后会重置到该值
		DepthStencilBufferClearValue.DepthStencil.Stencil = 0;				// 要清空到的模板值，清空后会重置到该值
		DepthStencilBufferClearValue.Format = DSVFormat;					// 要清空缓冲的格式，要和上文一致

		// 默认堆属性，深度缓冲也是一块纹理，所以用默认堆
		D3D12_HEAP_PROPERTIES DefaultProperties = { D3D12_HEAP_TYPE_DEFAULT };

		// 创建资源，深度缓冲的显存占用与渲染目标尺寸相关，属于常规开销
		// 所以直接 CreateCommittedResource 隐式堆创建即可，让操作系统帮我们管理
		m_D3D12Device->CreateCommittedResource(&DefaultProperties, D3D12_HEAP_FLAG_NONE, &DSVResourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &DepthStencilBufferClearValue, IID_PPV_ARGS(&m_DepthStencilBuffer));
	}


	// 创建 DSV 描述符，DSV 描述符用于描述深度模板缓冲区，这个描述符才是渲染管线要设置的对象
	void STEP10_CreateDSV()
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVViewDesc = {};
		DSVViewDesc.Format = DSVFormat;								// DSV 描述符格式要和资源一致
		DSVViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 深度缓冲本质也是一块 2D 纹理
		DSVViewDesc.Flags = D3D12_DSV_FLAG_NONE;					// 这个 Flag 是用来设置读写权限的，深度值和模板值均可以读写

		// 创建 DSV 描述符 (Depth Stencil View，深度模板描述符)
		m_D3D12Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &DSVViewDesc, DSVHandle);
	}


	// 上取整算法，对 A 向上取整，判断至少要多少个长度为 B 的空间才能容纳 A，用于内存对齐
	inline UINT Ceil(UINT A, UINT B)
	{
		return (A + B - 1) / B;
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 初始化 D2D 引擎，创建 D3D11On12 相关的设备并进行初始化，执行 D2D 引擎内部的三个必要的成员函数
	void STEP11_InitializeD2DEngine()
	{
		m_D2DEngine.D2D_STEP01_CreateD3D11Device(m_D3D12Device, m_CommandQueue);
		m_D2DEngine.D2D_STEP02_CreateD2DDevice();
		m_D2DEngine.D2D_STEP03_CreateD2DRenderTarget(m_hwnd, m_D3D12RenderTarget);
	}


	// 在初始化 D2D 引擎的基础上，从外部文件加载位图
	// 部分用于 UI 并转换到 D2D 位图，部分用于方块纹理并转换到 DX12 可用的 WIC 位图
	void STEP12_LoadImageAndTransform()
	{
		m_D2DEngine.D2D_STEP04_InitializeWICFactory();
		m_D2DEngine.D2D_STEP05_LoadUIAtlasIntoD2DBitmaps();
		m_D2DEngine.D2D_STEP06_LoadTextureIntoWICBitmaps(TextureNames, m_TextureGroup);
	}


	// D2D 引擎创建物品栏方块的预渲染图，准备物品栏方块物品的渲染
	void STEP13_LoadAndGenerateBlockIcons()
	{
		m_D2DEngine.D2D_STEP07_CalcIsometricMatrixAndTransform();
		m_D2DEngine.D2D_STEP08_GenerateBlockItemIcons();
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 获取纹理数组的各种属性，以第一个元素为准，后面的元素这些属性是一样的 (作者检查过了)
	void STEP14_GetTextureArrayElementsProperties()
	{
		// 获取第一个纹理的 DXGI 格式
		WICPixelFormatGUID WICPixelFormat = {};
		m_TextureGroup[0]->GetPixelFormat(&WICPixelFormat);
		TextureFormat = DX12TextureHelper::GetDXGIFormatFromPixelFormat(&WICPixelFormat);

		// 获取第一个纹理的图像深度
		BitsPerPixel = m_D2DEngine.Get_WICTexture_BitsPerPixel(m_TextureGroup[0]);

		// 获取纹理宽高
		m_TextureGroup[0]->GetSize(&TextureWidth, &TextureHeight);


		// 获取纹理每行所占的真实字节数，1 Byte = 8 Bits
		BytePerRowSize = TextureWidth * BitsPerPixel / 8;
		// 获取纹理真实总大小
		TextureSize = BytePerRowSize * TextureHeight;

		// DX12 API 要求在上传堆的纹理资源每行必须 256 字节对齐，这样能方便硬件批量复制
		// D3D12_TEXTURE_DATA_PITCH_ALIGNMENT = 256
		UploadResourceRowSize = Ceil(BytePerRowSize, 256) * 256;
		// 计算纹理数组单个元素实际需要的上传堆资源大小，最后一行无需对齐，直接复制
		UploadSubResourceSize = UploadResourceRowSize * (TextureHeight - 1) + BytePerRowSize;


		// 我们要在算出 UploadSubResourceSize 的基础上，再进行一次 512 对齐，算出纹理数组每个元素在上传堆所占的真实大小
		// 硬件正确偏移到每个元素的起始点。仍然是最后一个元素无需对齐，直接复制
		// D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT = 512
		UploadArrayElementSize = Ceil(UploadSubResourceSize, 512) * 512;

		// 最后计算上传堆资源所需要的总大小，公式和上面的 UploadSubResourceSize 计算是一样的
		UploadResourceSize = UploadArrayElementSize * (m_TextureGroup.size() - 1) + UploadSubResourceSize;
	}


	// 创建纹理数组需要的上传堆资源与默认堆资源
	void STEP15_CreateTextureArrayResource()
	{
		// 用于中转纹理的上传堆资源结构体
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 资源类型，上传堆的资源类型都是 buffer 缓冲
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// 资源布局，指定资源的存储方式，上传堆的资源都是 row major 按行线性存储
		UploadResourceDesc.Width = UploadResourceSize;						// 资源宽度，上传堆的资源宽度是资源的总大小，注意资源大小必须只多不少
		UploadResourceDesc.Height = 1;										// 资源高度，上传堆仅仅是传递线性资源的，所以高度必须为 1
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 资源格式，上传堆资源的格式必须为 UNKNOWN
		UploadResourceDesc.DepthOrArraySize = 1;							// 资源深度，上传堆资源必须为 1
		UploadResourceDesc.MipLevels = 1;									// Mipmap 等级，上传堆资源必须为 1
		UploadResourceDesc.SampleDesc.Count = 1;							// 资源采样次数，上传堆资源都是填 1


		// 创建上传堆资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &UploadResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_SRVTextureArray_UploadResource));


		// 默认堆资源结构体
		D3D12_RESOURCE_DESC DefaultResourceDesc = {};
		DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 资源类型选 Texture 2D (下文的描述符会描述它是一个纹理数组)
		DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;			// 纹理资源的布局都是 UNKNOWN
		DefaultResourceDesc.DepthOrArraySize = m_TextureGroup.size();		// 资源深度 = 纹理数组长度
		DefaultResourceDesc.Width = TextureWidth;							// 资源宽度，这里填单个纹理的宽度 (单位：像素)
		DefaultResourceDesc.Height = TextureHeight;							// 资源高度，这里填单个纹理的高度 (单位：像素)
		DefaultResourceDesc.Format = TextureFormat;							// 资源格式，这里填纹理格式，要和纹理数组一样
		DefaultResourceDesc.MipLevels = 1;									// Mipmap 等级，我们暂时不使用 Mipmap (只有一层 Mipmap)，所以填 1
		DefaultResourceDesc.SampleDesc.Count = 1;							// 资源采样次数，这里我们填 1 就行


		// 创建默认堆资源
		m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE, &DefaultResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_SRVTextureArray_DefaultResource));
	}


	// 将纹理数组资源逐步复制到默认堆资源中
	void STEP16_CopyTextureArrayToDefaultResource()
	{
		// 用于暂时存储纹理数据的指针，这里要用 malloc 分配空间
		BYTE* TextureData = (BYTE*)malloc(TextureSize);

		// 用于传递资源的指针
		BYTE* TransferPointer = nullptr;

		// Map 开始映射，Map 方法会得到上传堆资源的地址 (在共享内存上)，传递给指针，这样我们就能通过 memcpy 操作复制数据了
		m_SRVTextureArray_UploadResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));


		// 循环复制 TextureGroup 每个 WIC 资源到上传堆，然后逐一释放，i 是纹理数组元素索引
		for (UINT i = 0; i < m_TextureGroup.size(); i++)
		{
			// 对于每个纹理元素，将整块纹理数据读到 TextureData 中，方便下面的 memcpy 复制操作
			m_TextureGroup[i]->CopyPixels(nullptr, BytePerRowSize, TextureSize, TextureData);

			// 向上传堆资源逐行复制纹理数据 (CPU 高速缓存 -> 共享内存)，j 是复制的行数
			for (UINT j = 0; j < TextureHeight; j++)
			{
				// 复制一行
				memcpy(TransferPointer, TextureData, BytePerRowSize);
				// 纹理指针偏移到下一行
				TextureData += BytePerRowSize;
				// 上传堆资源指针偏移到下一行，注意偏移长度不同！
				TransferPointer += UploadResourceRowSize;
			}

			// 单个元素复制完毕，纹理 WIC 资源指针复用，恢复到最开始的位置，准备下一次 CopyPixels
			TextureData -= TextureSize;

			// 上传堆资源指针回到本数组元素的起点
			TransferPointer -= UploadResourceRowSize * TextureHeight;

			// 上传堆资源指针偏移到下一个数组元素的位置
			// 请大家认真想一想下面的等式成立吗？ (反正作者被下面的大小偏移坑爆了，渲染不出来盯了三小时 + 一遍遍问 deepseek 才改出来)
			// UploadResourceRowSize * TextureHeight == UploadSubResourceSize == UploadArrayElementSize
			TransferPointer += UploadArrayElementSize;


			// 每个元素复制完，重置并释放 WIC 位图资源，防止它占内存
			m_TextureGroup[i].Reset();
		}

		// Unmap 结束映射，让上传堆处于只读状态
		m_SRVTextureArray_UploadResource->Unmap(0, nullptr);
		// 释放上文 malloc 分配的空间，后面我们用不到它，做一个干净的程序员
		free(TextureData);




		// 资源脚本，用来描述要复制的资源。如果复制目标是纹理数组，每个子资源 (纹理数组元素) 各复制一次，各需要一个资源脚本
		// 如果复制纹理数组只用一个脚本，下文 GPU 执行 CopyTextureRegion 会寻址出界，报 Stack Corrupted，调试层不会提示这个信息
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> PlacedFootprints(m_TextureGroup.size());

		D3D12_RESOURCE_DESC DefaultResourceDesc = m_SRVTextureArray_DefaultResource->GetDesc();	// 默认堆资源结构体

		// 获取纹理复制脚本，用于下文的纹理复制，注意第三个参数！第三个参数是目标资源的子资源数量！我们复制的是纹理数组，要填数组长度！
		// 当你填了 DefaultResourceDesc 和 TextureGroup.size()，这个函数会自动填充每个资源脚本的各种参数
		m_D3D12Device->GetCopyableFootprints(&DefaultResourceDesc, 0, m_TextureGroup.size(), 0,
			&PlacedFootprints[0], nullptr, nullptr, nullptr);


		// 复制资源需要使用 GPU 的 CopyEngine 复制引擎，所以需要向命令队列发出复制命令
		m_CommandAllocator->Reset();								// 先重置命令分配器
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);	// 再重置命令列表，复制命令不需要 PSO 状态，所以第二个参数填 nullptr


		// 注意！复制纹理数组到默认堆，每个子资源 (纹理数组元素) 都要调用一次 CopyTextureRegion 指令
		// DstLocation.SubresourceIndex 和 SrcLocation.PlacedFootprint 的参数也要跟着变！这样才能正确复制
		for (UINT i = 0; i < m_TextureGroup.size(); i++)
		{
			D3D12_TEXTURE_COPY_LOCATION DstLocation = {};						// 复制目标位置 (默认堆资源) 结构体
			DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		// 纹理复制类型，这里必须指向纹理
			DstLocation.SubresourceIndex = i;									// 指定要复制的子资源索引 (第 i 个元素)
			DstLocation.pResource = m_SRVTextureArray_DefaultResource.Get();	// 要复制到的资源 (默认堆资源)

			D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};						// 复制源位置 (上传堆资源) 结构体
			SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// 纹理复制类型，这里必须指向缓冲区
			SrcLocation.PlacedFootprint = PlacedFootprints[i];					// 指定要复制的资源脚本信息 (用第 i 个资源脚本)
			SrcLocation.pResource = m_SRVTextureArray_UploadResource.Get();		// 被复制数据的缓冲 (上传堆资源)

			// 记录复制第 i 个子资源 (纹理数组元素) 到默认堆的命令 (共享内存 -> 显存) 
			m_CommandList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);
		}



		// 关闭命令列表
		m_CommandList->Close();

		// 用于传递命令用的临时 ID3D12CommandList 数组
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// 提交复制命令！GPU 开始复制！
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);


		// 将围栏预定值设定为下一帧，注意复制资源也需要围栏等待，否则会发生资源冲突！
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		// 围栏对象会关联命令队列，围栏对象内部会根据这个围栏值开辟 Event Slot 事件槽，并将围栏值填进去
		// 当 Command Queue 执行完成，会修改相关联的围栏对象的 CompletedValue 任务值，然后通知围栏对象
		m_CommandQueue->Signal(m_RenderFence.Get(), FenceValue);
		// 设置围栏的预定事件，当复制完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		// 实际上是将 CPU 端的信号事件句柄，通过围栏值寻址到对应的 Event Slot 事件槽，然后将其绑定
		// 当 GPU 端的 CommandQueue 的任务执行完成，自身会修改与其相关联所有围栏对象内部的 CompletedValue 任务值，然后激发相关联的围栏对象
		m_RenderFence->SetEventOnCompletion(FenceValue, RenderEvent);


		// 让主线程强制等待复制完成，经过此函数后 RenderEvent 会自动重置到无信号状态 (CreateEvent 第二个参数)
		WaitForSingleObject(RenderEvent, INFINITE);
	}


	// 创建用于 纹理数组 和 粒子缓冲区 的 SRV 着色器资源描述符堆 (2 SRV)
	void STEP17_CreateRenderSRVHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC SRVHeapDesc = {};					// SRV 描述符堆信息结构体
		SRVHeapDesc.NumDescriptors = 2;									// 纹理数组 + 粒子缓冲区 SRV
		SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// 类型是 CBV/SRV/UAV 描述符都可以放
		SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// 着色器需要引用 SRV 资源，就必须设置着色器可见标志

		// 创建 SRV 描述符堆
		m_D3D12Device->CreateDescriptorHeap(&SRVHeapDesc, IID_PPV_ARGS(&m_RenderSRVHeap));
	}


	// 用上文创建的 m_TextureArrayDefaultResource 创建第一个 SRV 描述符
	void STEP18_CreateTextureArraySRV()
	{
		// Texture Array 的 SRV 信息结构体，我们要通过 SRV 告知 GPU 这个资源的类型与用法
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVTextureArrayDesc = {};
		// SRV 描述符的维度 (类型)，我们这里选 TEXTURE2DARRAY (2D 纹理数组)
		SRVTextureArrayDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		// 格式要填纹理数组的纹理格式
		SRVTextureArrayDesc.Format = TextureFormat;
		// RGBA 4 分量顺序不改变
		SRVTextureArrayDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		// 纹理数组的起始索引，在 2D 纹理数组中，Slice 切片表示一个数组元素 (一个 2D 纹理)
		SRVTextureArrayDesc.Texture2DArray.FirstArraySlice = 0;
		// 纹理数组的长度 (纹理的数量)
		SRVTextureArrayDesc.Texture2DArray.ArraySize = m_TextureGroup.size();
		// 只有一层 Mipmap，填 1
		SRVTextureArrayDesc.Texture2DArray.MipLevels = 1;

		// 获取 CPU 句柄
		SRVTextureArray_CPUHandle = m_RenderSRVHeap->GetCPUDescriptorHandleForHeapStart();
		// 获取 GPU 句柄
		SRVTextureArray_GPUHandle = m_RenderSRVHeap->GetGPUDescriptorHandleForHeapStart();

		// 创建 SRV 描述符
		m_D3D12Device->CreateShaderResourceView(m_SRVTextureArray_DefaultResource.Get(), &SRVTextureArrayDesc, SRVTextureArray_CPUHandle);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建 SRV Structured Buffer (结构化缓冲区)
	// 我们这里要传递立方体面纹理索引数据 (静态资源)，所以用 SRV Structured Buffer
	void STEP19_CreateStructuredBufferResource()
	{
		// Structured Buffer 中转资源的上传堆信息结构体，填法和顶点/索引缓冲一样
		D3D12_RESOURCE_DESC StructuredBufferUploadDesc = {};
		StructuredBufferUploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		StructuredBufferUploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		StructuredBufferUploadDesc.Width = BlockCubeTexture_IndexGroup.size() * sizeof(CUBEFACE);	// 宽度是整个结构化缓冲的大小
		StructuredBufferUploadDesc.Height = 1;
		StructuredBufferUploadDesc.Format = DXGI_FORMAT_UNKNOWN;
		StructuredBufferUploadDesc.DepthOrArraySize = 1;
		StructuredBufferUploadDesc.MipLevels = 1;
		StructuredBufferUploadDesc.SampleDesc.Count = 1;

		// 创建上传堆资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &StructuredBufferUploadDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_SRVStructuredBuffer_UploadResource));


		// Structured Buffer 中转资源的默认堆信息结构体
		D3D12_RESOURCE_DESC StructuredBufferDefaultDesc = {};
		StructuredBufferDefaultDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;					// 注意这里类型是缓冲
		StructuredBufferDefaultDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;						// 线性资源
		StructuredBufferDefaultDesc.Width = BlockCubeTexture_IndexGroup.size() * sizeof(CUBEFACE);	// 宽度是整个结构化缓冲的大小
		StructuredBufferDefaultDesc.Height = 1;
		StructuredBufferDefaultDesc.Format = DXGI_FORMAT_UNKNOWN;
		StructuredBufferDefaultDesc.DepthOrArraySize = 1;
		StructuredBufferDefaultDesc.MipLevels = 1;
		StructuredBufferDefaultDesc.SampleDesc.Count = 1;

		// 创建默认堆资源
		m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE, &StructuredBufferDefaultDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_SRVStructuredBuffer_DefaultResource));
	}



	// 将 SRV Structured Buffer Resource 逐步复制到默认堆资源中，注意 SRV Structured Buffer 不需要 SRVHeap
	// 和 CBVResource 一样，直接使用 SRV RootDescriptor
	void STEP20_CopyStructuredBufferToDefaultResource()
	{
		// 用于传递资源的指针
		BYTE* TransferPointer = nullptr;

		// Map 映射，获取上传堆资源的地址并传递到 TransferPointer
		m_SRVStructuredBuffer_UploadResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));

		// 直接 memcpy 复制 (CPU 高速缓存 -> 共享内存)
		memcpy(TransferPointer, &BlockCubeTexture_IndexGroup[0], BlockCubeTexture_IndexGroup.size() * sizeof(CUBEFACE));

		// UnMap 结束映射，下一步就要复制到默认堆
		m_SRVStructuredBuffer_UploadResource->Unmap(0, nullptr);


		// 复制资源需要使用 GPU 的 CopyEngine 复制引擎，所以需要向命令队列发出复制命令
		m_CommandAllocator->Reset();								// 先重置命令分配器
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);	// 再重置命令列表，复制命令不需要 PSO 状态，所以第二个参数填 nullptr


		// 发送复制到默认堆的指令，注意这里用的是 CopyBufferRegion 复制缓冲指令，不用填麻烦的结构体，直接填参数上传 (共享内存 -> GPU 显存)
		m_CommandList->CopyBufferRegion(m_SRVStructuredBuffer_DefaultResource.Get(), 0,
			m_SRVStructuredBuffer_UploadResource.Get(), 0, BlockCubeTexture_IndexGroup.size() * sizeof(CUBEFACE));


		// 关闭命令列表
		m_CommandList->Close();

		// 用于传递命令用的临时 ID3D12CommandList 数组
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// 提交复制命令！GPU 开始复制！
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);


		// 将围栏预定值设定为下一帧，注意复制资源也需要围栏等待，否则会发生资源冲突！
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		m_CommandQueue->Signal(m_RenderFence.Get(), FenceValue);
		// 设置围栏的预定事件，当复制完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		m_RenderFence->SetEventOnCompletion(FenceValue, RenderEvent);


		// 让主线程强制等待复制完成，经过此函数后 RenderEvent 会自动重置到无信号状态
		WaitForSingleObject(RenderEvent, INFINITE);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 在 Minecraft 中，玩家走路、破坏方块、TNT 爆炸摧毁方块、下水或钓鱼，岩浆迸射、地狱门等等都会产生粒子
	// 粒子是一种非常小的对象，可以用非常小的几何体表示 (原版是 2D 小正方体，我们这里用非常小的小方块)
	// 可以使用粒子呈现更直观的视觉反馈，本章我们会模拟原版游戏中破坏方块产生粒子的效果
	

	// 依次创建 ParticleShader 所需要的资源
	void STEP21_CreateParticleRequireResources()
	{
		// 创建 m_SRVUAVParticlesHiveBuffer_DefaultResource
		{
			// 用于创建 SRV/UAV 默认堆资源的结构体信息，最大容纳 128 个粒子
			D3D12_RESOURCE_DESC DefaultResourceDesc = {};
			DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			DefaultResourceDesc.Width = MaxAllocParticlesNums * sizeof(Particle);
			DefaultResourceDesc.Height = 1;
			DefaultResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			DefaultResourceDesc.DepthOrArraySize = 1;
			DefaultResourceDesc.MipLevels = 1;
			DefaultResourceDesc.SampleDesc.Count = 1;
			DefaultResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;


			// 创建 SRV/UAV 粒子缓冲区默认堆资源，注意资源状态！
			m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE,
				&DefaultResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
				IID_PPV_ARGS(&m_SRVUAVParticlesHiveBuffer_DefaultResource));
		}


		// 创建 m_UAVParticlesFreeStack_DefaultResource
		{
			// 用于创建 UAV 默认堆资源的结构体信息，最大容纳 128 个粒子
			D3D12_RESOURCE_DESC DefaultResourceDesc = {};
			DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			DefaultResourceDesc.Width = MaxAllocParticlesNums * sizeof(Particle);
			DefaultResourceDesc.Height = 1;
			DefaultResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			DefaultResourceDesc.DepthOrArraySize = 1;
			DefaultResourceDesc.MipLevels = 1;
			DefaultResourceDesc.SampleDesc.Count = 1;
			DefaultResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;


			// 创建 UAV 空闲栈默认堆资源，注意资源状态！
			m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE,
				&DefaultResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
				IID_PPV_ARGS(&m_UAVParticlesFreeStack_DefaultResource));
		}


		// 创建 m_UAVStackTopPointer_DefaultResource
		{
			// 用于创建 UAV 默认堆资源的结构体信息，这个原始缓冲区只占 4 字节
			D3D12_RESOURCE_DESC DefaultResourceDesc = {};
			DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			DefaultResourceDesc.Width = 4;
			DefaultResourceDesc.Height = 1;
			DefaultResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			DefaultResourceDesc.DepthOrArraySize = 1;
			DefaultResourceDesc.MipLevels = 1;
			DefaultResourceDesc.SampleDesc.Count = 1;
			DefaultResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;


			// 创建 UAV 栈顶指针默认堆资源，注意资源状态！
			m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE,
				&DefaultResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
				IID_PPV_ARGS(&m_UAVStackTopPointer_DefaultResource));
		}


		// 创建 m_SRVSpawnParticlesList_UploadResource
		{
			// 用于创建 SRV 上传堆资源的结构体信息
			D3D12_RESOURCE_DESC UploadResourceDesc = {};
			UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			UploadResourceDesc.Width = MaxAllocParticlesNums * sizeof(Particle);
			UploadResourceDesc.Height = 1;
			UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			UploadResourceDesc.DepthOrArraySize = 1;
			UploadResourceDesc.MipLevels = 1;
			UploadResourceDesc.SampleDesc.Count = 1;


			// 创建 SRV 栈顶指针上传堆资源，注意资源状态！
			m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE,
				&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&m_SRVSpawnParticlesList_UploadResource));


			// 我们每帧都要上传新粒子数据给计算着色器，进行持续化映射，不用再 Unmap 了
			m_SRVSpawnParticlesList_UploadResource->Map(0, nullptr,
				reinterpret_cast<void**>(&ReadySpawnParticlesListPointer));
		}
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建 m_ParticleUAVSRVHeap，在描述符堆上创建 3 UAV + 1 SRV 描述符，并绑定相应的资源到相应的描述符，
	// 此函数同时还会创建 m_RenderSRVHeap 的第二个描述符：粒子缓冲区 SRV 描述符
	void STEP22_CreateUAVSRVHeapAndDescriptor()
	{
		// 创建 m_ParticleUAVSRVHeap
		{
			// m_ParticleUAVSRVHeap 的描述符堆信息结构体
			D3D12_DESCRIPTOR_HEAP_DESC UAVSRVHeapDesc = {};
			// 3 UAV + 1 SRV
			UAVSRVHeapDesc.NumDescriptors = 4;
			// CBV/SRV/UAV 类型
			UAVSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			// 着色器可见标志，这样描述符堆才能绑定到渲染管线供着色器使用
			UAVSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			// 创建 m_ParticleUAVSRVHeap
			m_D3D12Device->CreateDescriptorHeap(&UAVSRVHeapDesc, IID_PPV_ARGS(&m_ParticleUAVSRVHeap));

			// 获取 CBV/SRV/UAV 描述符的大小，准备偏移句柄
			CBVSRVUAVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}


		// 创建 m_ParticleUAVSRVHeap 第一个 UAV 描述符：m_ParticlesHiveBuffer
		{
			// UAV 描述符信息结构体
			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDescriptorDesc = {};
			// 普通缓冲格式都是 DXGI_FORMAT_UNKNOWN
			UAVDescriptorDesc.Format = DXGI_FORMAT_UNKNOWN;
			// 缓冲类型
			UAVDescriptorDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			// 首元素索引都是 0
			UAVDescriptorDesc.Buffer.FirstElement = 0;
			// RWStructuredBuffer 的 Flag 都是 NONE
			UAVDescriptorDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			// m_ParticlesBuffer 的步长 (每个元素的大小)
			UAVDescriptorDesc.Buffer.StructureByteStride = sizeof(Particle);
			// m_ParticlesBuffer 分配的元素数量 (最大能容纳的粒子数量)
			UAVDescriptorDesc.Buffer.NumElements = MaxAllocParticlesNums;


			// 获取 m_ParticleUAVSRVHeap 第一个 CPU 描述符
			ParticleUAVSRVHeap_CPUBaseHandle = m_ParticleUAVSRVHeap->GetCPUDescriptorHandleForHeapStart();
			// 创建并绑定 m_ParticlesHiveBuffer UAV 描述符
			m_D3D12Device->CreateUnorderedAccessView(m_SRVUAVParticlesHiveBuffer_DefaultResource.Get(),
				nullptr, &UAVDescriptorDesc, ParticleUAVSRVHeap_CPUBaseHandle);
		}


		// 创建 m_ParticleUAVSRVHeap 第二个 UAV 描述符：m_ParticlesFreeStack
		{
			// UAV 描述符信息结构体
			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDescriptorDesc = {};
			// 普通缓冲格式都是 DXGI_FORMAT_UNKNOWN
			UAVDescriptorDesc.Format = DXGI_FORMAT_UNKNOWN;
			// 缓冲类型
			UAVDescriptorDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			// 首元素索引都是 0
			UAVDescriptorDesc.Buffer.FirstElement = 0;
			// RWStructuredBuffer 的 Flag 都是 NONE
			UAVDescriptorDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			// m_ParticlesFreeStack 的步长 (每个元素的大小)
			UAVDescriptorDesc.Buffer.StructureByteStride = sizeof(Particle);
			// m_ParticlesFreeStack 分配的元素数量 (最大能容纳的粒子数量)
			UAVDescriptorDesc.Buffer.NumElements = MaxAllocParticlesNums;


// 获取 m_ParticleUAVSRVHeap 第二个 CPU 描述符
ParticleUAVSRVHeap_CPUBaseHandle.ptr += CBVSRVUAVDescriptorSize;
// 创建并绑定 m_ParticlesFreeStack UAV 描述符
m_D3D12Device->CreateUnorderedAccessView(m_UAVParticlesFreeStack_DefaultResource.Get(),
	nullptr, &UAVDescriptorDesc, ParticleUAVSRVHeap_CPUBaseHandle);
		}


		// 创建 m_ParticleUAVSRVHeap 第三个 UAV 描述符：m_StackTopPointer
		{
			// UAV 描述符信息结构体
			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDescriptorDesc = {};
			// 注意这里！原始缓冲区必须是 DXGI_FORMAT_R32_TYPELESS (32 位无类型格式)！
			UAVDescriptorDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			// 缓冲类型
			UAVDescriptorDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			// 首元素索引都是 0
			UAVDescriptorDesc.Buffer.FirstElement = 0;
			// 注意这里！RWByteAddressBuffer 作为原始缓冲区，需要 RAW Flag！
			UAVDescriptorDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			// 注意这里！RWByteAddressBuffer 作为原始缓冲区，步长必须为 0！
			UAVDescriptorDesc.Buffer.StructureByteStride = 0;
			// m_StackTopPointer 只有一个 32 位 uint 元素
			UAVDescriptorDesc.Buffer.NumElements = 1;


			// 获取 m_ParticleUAVSRVHeap 第三个 CPU 描述符
			ParticleUAVSRVHeap_CPUBaseHandle.ptr += CBVSRVUAVDescriptorSize;
			// 创建并绑定 m_StackTopPointer UAV 描述符
			m_D3D12Device->CreateUnorderedAccessView(m_UAVStackTopPointer_DefaultResource.Get(),
				nullptr, &UAVDescriptorDesc, ParticleUAVSRVHeap_CPUBaseHandle);
		}


		// 创建 m_ParticleUAVSRVHeap 第四个 SRV 描述符：m_SpawnParticlesList
		{
			// SRV 描述符信息结构体
			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptorDesc = {};
			// 用于缓冲的描述符都是 DXGI_FORMAT_UNKNOWN
			SRVDescriptorDesc.Format = DXGI_FORMAT_UNKNOWN;
			// 缓冲类型
			SRVDescriptorDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			// 缓冲区也仍然要指定 RGBA 映射顺序，使用和纹理一样的配置就行
			SRVDescriptorDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			// 首元素索引为 0
			SRVDescriptorDesc.Buffer.FirstElement = 0;
			// StructuredBuffer 的 Flag 都是 NONE
			SRVDescriptorDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			// m_SpawnParticlesList 分配的元素数量 (最大能容纳的粒子数量)
			SRVDescriptorDesc.Buffer.NumElements = MaxAllocParticlesNums;
			// m_SpawnParticlesList 的步长 (每个元素的大小)
			SRVDescriptorDesc.Buffer.StructureByteStride = sizeof(Particle);


			// 获取 m_ParticleUAVSRVHeap 第四个 CPU 描述符
			ParticleUAVSRVHeap_CPUBaseHandle.ptr += CBVSRVUAVDescriptorSize;
			// 创建 m_SpawnParticlesList SRV 描述符
			m_D3D12Device->CreateShaderResourceView(m_SRVSpawnParticlesList_UploadResource.Get(),
				&SRVDescriptorDesc, ParticleUAVSRVHeap_CPUBaseHandle);
		}


		// 创建 m_RenderSRVHeap 第二个 SRV 描述符：m_ParticlesHiveBuffer
		{
			// 创建 m_RenderSRVHeap 的第二个描述符：m_ParticlesHiveBuffer 粒子缓冲区 SRV
			// 在 D3D12 中，允许将两种不同类型的描述符绑定到同一资源，实现资源不同用途的复用，因为描述符只是表示了资源的用途
			// 但是，在渲染管线中使用不同类型的描述符，需要进行资源屏障转换，告诉图形驱动 "现在我要换一个姿势使用这个资源"
			// 而且同一时间内，只能有一个绑定到该资源的描述符生效，否则就会发生资源竞争


			// SRV 描述符信息结构体
			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptorDesc = {};
			// 用于缓冲的描述符都是 DXGI_FORMAT_UNKNOWN
			SRVDescriptorDesc.Format = DXGI_FORMAT_UNKNOWN;
			// 缓冲类型
			SRVDescriptorDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			// 缓冲区也仍然要指定 RGBA 映射顺序，使用和纹理一样的配置就行
			SRVDescriptorDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			// 首元素索引为 0
			SRVDescriptorDesc.Buffer.FirstElement = 0;
			// StructuredBuffer 的 Flag 都是 NONE
			SRVDescriptorDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			// m_ParticlesBuffer 分配的元素数量 (最大能容纳的粒子数量)
			SRVDescriptorDesc.Buffer.NumElements = MaxAllocParticlesNums;
			// m_ParticlesBuffer 的步长 (每个元素的大小)
			SRVDescriptorDesc.Buffer.StructureByteStride = sizeof(Particle);


			// 获取 m_RenderSRVHeap 第二个 CPU 描述符
			SRVParticlesBuffer_CPUHandle.ptr = SRVTextureArray_CPUHandle.ptr + CBVSRVUAVDescriptorSize;
			// 创建 m_ParticlesHiveBuffer SRV 描述符
			m_D3D12Device->CreateShaderResourceView(m_SRVUAVParticlesHiveBuffer_DefaultResource.Get(),
				&SRVDescriptorDesc, SRVParticlesBuffer_CPUHandle);
		}


		// 获取 m_ParticleUAVSRVHeap 的 GPU 基址偏移句柄，在渲染管线中方便我们连续绑定
		ParticleUAVSRVHeap_GPUBaseHandle = m_ParticleUAVSRVHeap->GetGPUDescriptorHandleForHeapStart();

		// 获取 m_RenderSRVHeap 第二个 SRV 的 GPU 句柄
		SRVParticlesBuffer_GPUHandle.ptr = SRVTextureArray_GPUHandle.ptr + CBVSRVUAVDescriptorSize;
	}


	// 创建用于粒子效果的计算着色器根签名
	void STEP23_CreateParticleRootSignature()
	{
		// ParticleRootSignature 的根参数 + 静态采样器列表
		// Para 0: (Type = Root Constants,	1 DWORD)  (b0, space0) CBV 根常量，用于 SpawnCSMain 的常量缓冲
		// Para 1: (Type = Root Constants,	2 DWORD)  (b1, space0) CBV 根常量，用于 UpdateCSMain 的常量缓冲
		// Para 2: (Type = Descriptor Table 1 DWORD)  (u0 - u2, t0, space0) 根描述表，用于着色器用到的资源
		// 
		// None Static Sampler	没有静态采样器，计算着色器现在不需要用到

		ComPtr<ID3DBlob> SignatureBlob;			// 根签名字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码

		D3D12_ROOT_PARAMETER RootParameters[3] = {};		// 根参数数组


		// 第一个根参数：32 位根常量 (SpawnParticlesData)
		RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		RootParameters[0].Constants.Num32BitValues = 1;		// 只有一个根常量
		RootParameters[0].Constants.ShaderRegister = 0;		// b0
		RootParameters[0].Constants.RegisterSpace = 0;		// space0
		RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


		// 第二个根参数：三个 32 位根常量 (UpdateParticlesData)
		RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		RootParameters[1].Constants.Num32BitValues = 2;		// 两个根常量
		RootParameters[1].Constants.ShaderRegister = 1;		// b1
		RootParameters[1].Constants.RegisterSpace = 0;		// space0
		RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


		// 第三个根参数：根描述表 (3 UAV Range + 1 SRV Range)，根描述表本质上就是个排插，能快速绑定多个描述符
		D3D12_DESCRIPTOR_RANGE UAVSRVRange[2] = {};					// 3 UAV Range + 1 SRV Range

		// 第一个 Range：UAV Range (m_ParticlesHiveBuffer、m_ParticlesFreeStack、m_StackTopPointer)
		UAVSRVRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;	// 描述符 Range 在这里分流
		UAVSRVRange[0].NumDescriptors = 3;							// UAV Range 有三个描述符
		UAVSRVRange[0].BaseShaderRegister = 0;						// u0 - u2
		UAVSRVRange[0].RegisterSpace = 0;							// space0
		UAVSRVRange[0].OffsetInDescriptorsFromTableStart = 0;		// UAV Range 前面有 0 个描述符

		// 第二个 Range：SRV Range (m_SpawnParticlesList)
		UAVSRVRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		UAVSRVRange[1].NumDescriptors = 1;							// SRV Range 只有一个描述符
		UAVSRVRange[1].BaseShaderRegister = 0;						// t0
		UAVSRVRange[1].RegisterSpace = 0;							// space0
		UAVSRVRange[1].OffsetInDescriptorsFromTableStart = 3;		// SRV Range 前面有 3 个描述符

		RootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParameters[2].DescriptorTable.NumDescriptorRanges = 2;	// 两个 Range
		RootParameters[2].DescriptorTable.pDescriptorRanges = UAVSRVRange;
		RootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;



		// 根签名信息结构体，上限 64 DWORD，静态采样器不占用根签名
		D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
		RootSignatureDesc.NumParameters = 3;				// 三个根参数
		RootSignatureDesc.pParameters = RootParameters;		// 根参数数组指针
		RootSignatureDesc.NumStaticSamplers = 0;
		RootSignatureDesc.pStaticSamplers = nullptr;
		RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;	// 没有特殊标志


		// 编译根签名，让根签名先编译成 GPU 可读的二进制字节码
		D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &SignatureBlob, &ErrorBlob);
		if (ErrorBlob)
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// 用这个二进制字节码创建根签名对象
		m_D3D12Device->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&m_ParticleRootSignature));
	}


	// 创建用于粒子效果的三个 PSO: m_ParticleSpawnPSO、m_ParticleUpdatePSO、m_ParticleInitPSO
	void STEP24_CreateParticlePSO()
	{
		// 计算着色器使用的 PSO 信息结构体，注意类型是 D3D12_COMPUTE_PIPELINE_STATE_DESC
		D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePSODesc = {};

		// 第一次绑定根签名，使用的是上面的 m_ParticleRootSignature
		// 本次设置是将根签名与 PSO 绑定，生成对应版本的根签名适配 PSO，设置渲染管线的输入参数状态
		ComputePSODesc.pRootSignature = m_ParticleRootSignature.Get();

		// 不使用特殊标志
		ComputePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;


		ComPtr<ID3DBlob> ComputeShaderBlob;		// 计算着色器二进制字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码


		// 创建 m_ParticleSpawnPSO
		{
			// 编译计算着色器的 SpawnCSMain 函数
			D3DCompileFromFile(L"ParticleShader.hlsl", nullptr, nullptr, "SpawnCSMain", "cs_5_1", NULL, NULL, &ComputeShaderBlob, &ErrorBlob);
			if (ErrorBlob)
			{
				OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
				OutputDebugStringA("\n");
			}

			// 设置 CS 字节码
			ComputePSODesc.CS.BytecodeLength = ComputeShaderBlob->GetBufferSize();
			ComputePSODesc.CS.pShaderBytecode = ComputeShaderBlob->GetBufferPointer();

			// 创建用于 SpawnCSMain 添加并生成新粒子的 PSO
			m_D3D12Device->CreateComputePipelineState(&ComputePSODesc, IID_PPV_ARGS(&m_ParticleSpawnPSO));
		}


		// 创建 m_ParticleUpdatePSO
		{
			// 编译计算着色器的 SpawnCSMain 函数
			D3DCompileFromFile(L"ParticleShader.hlsl", nullptr, nullptr, "UpdateCSMain", "cs_5_1", NULL, NULL, &ComputeShaderBlob, &ErrorBlob);
			if (ErrorBlob)
			{
				OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
				OutputDebugStringA("\n");
			}

			// 设置 CS 字节码
			ComputePSODesc.CS.BytecodeLength = ComputeShaderBlob->GetBufferSize();
			ComputePSODesc.CS.pShaderBytecode = ComputeShaderBlob->GetBufferPointer();

			// 创建用于 SpawnCSMain 添加并生成新粒子的 PSO
			m_D3D12Device->CreateComputePipelineState(&ComputePSODesc, IID_PPV_ARGS(&m_ParticleUpdatePSO));
		}


		// 创建 m_ParticleInitPSO
		{
			// 编译计算着色器的 SpawnCSMain 函数
			D3DCompileFromFile(L"ParticleShader.hlsl", nullptr, nullptr, "InitCSMain", "cs_5_1", NULL, NULL, &ComputeShaderBlob, &ErrorBlob);
			if (ErrorBlob)
			{
				OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
				OutputDebugStringA("\n");
			}

			// 设置 CS 字节码
			ComputePSODesc.CS.BytecodeLength = ComputeShaderBlob->GetBufferSize();
			ComputePSODesc.CS.pShaderBytecode = ComputeShaderBlob->GetBufferPointer();

			// 创建用于 SpawnCSMain 添加并生成新粒子的 PSO
			m_D3D12Device->CreateComputePipelineState(&ComputePSODesc, IID_PPV_ARGS(&m_ParticleInitPSO));
		}
	}


	// 创建两个用于 UAV 资源相关的资源屏障
	// 上文 STEP22_CreateUAVSRVHeapAndDescriptor 已经说明原因了，描述符只描述资源的用途与布局
	// 不同类型的描述符可以绑定同一资源，但同时只能有一个描述符生效，而且必须经过资源屏障转换
	void STEP25_CreateParticlesBufferBarrier()
	{
		// UAVToSRV_barrier 屏障：Unordered Access 无序访问状态 -> Non Pixel Shader Resource 常规着色器资源状态
		// NON_PIXEL_SHADER_RESOURCE 的意思是 除了像素着色器，其他着色器都可以使用这个资源
		// 这个屏障用于 UpdateCSMain 已经执行完成，准备将粒子缓冲区交给顶点着色器进行渲染
		UAVToSRV_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		UAVToSRV_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		UAVToSRV_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		UAVToSRV_barrier.Transition.pResource = m_SRVUAVParticlesHiveBuffer_DefaultResource.Get();

		// SRVToUAV_barrier 屏障：Non Pixel Shader Resource 常规着色器资源状态 -> Unordered Access 无序访问状态
		// 这个屏障用于粒子缓冲区已经全部渲染完成，交给下一帧计算着色器计算并更新粒子缓冲
		SRVToUAV_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		SRVToUAV_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		SRVToUAV_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		SRVToUAV_barrier.Transition.pResource = m_SRVUAVParticlesHiveBuffer_DefaultResource.Get();
	}


	// 启动计算着色器，执行 InitCSMain，初始化粒子着色器所需要的资源与数据结构
	void STEP26_InitParticlesBuffer()
	{
		m_CommandAllocator->Reset();								// 先重置命令分配器
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);	// 再重置命令列表


		// 用于设置描述符堆用的临时 ID3D12DescriptorHeap 数组
		ID3D12DescriptorHeap* _temp_DescriptorHeaps[] = { m_ParticleUAVSRVHeap.Get() };
		// 设置描述符堆，描述符堆表示了根描述表第二次寻址，寻找描述符需要的基地址
		m_CommandList->SetDescriptorHeaps(1, _temp_DescriptorHeaps);

		// 第二次设置根签名，本次检测 PSO 根签名的合法性 (引用资源是否匹配)，检测成功会开启显存与寄存器的映射通道
		// 注意！这里是 SetComputeRootSignature 版本的！下文这一整个代码块涉及到资源绑定都是 Compute 版本！
		m_CommandList->SetComputeRootSignature(m_ParticleRootSignature.Get());

		// 设置 InitCSMain 的渲染管线状态
		m_CommandList->SetPipelineState(m_ParticleInitPSO.Get());
		
		// 设置第三个根参数：粒子缓冲、空闲栈、栈顶指针、待生成列表
		// 借助根描述表 + 描述符堆按顺序排列描述符，就可以一次性快速绑定多个描述符到管线中
		m_CommandList->SetComputeRootDescriptorTable(2, ParticleUAVSRVHeap_GPUBaseHandle);


		// InitCSMain 需要的线程组数量
		UINT ThreadGroupNums = Ceil(MaxAllocParticlesNums, InitCSMainThreadNums);

		// 调度线程组，开始初始化缓冲资源
		m_CommandList->Dispatch(ThreadGroupNums, 1, 1);


		// 关闭命令列表，准备上传到命令队列
		m_CommandList->Close();

		// 用于传递命令用的临时 ID3D12CommandList 数组
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// 执行上文的计算命令！
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);


		// 将围栏预定值设定为下一帧，注意复制资源也需要围栏等待，否则会发生资源冲突！
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		m_CommandQueue->Signal(m_RenderFence.Get(), FenceValue);
		// 设置围栏的预定事件，当复制完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		m_RenderFence->SetEventOnCompletion(FenceValue, RenderEvent);


		// 下一个等待就是 RenderLoop 的 MsgWaitForMultipleObjects，不需要用 WaitForSingleObject 了
		// 这里再用一次 WaitForSingleObject 就会使事件变成无信号 (CreateEvent 第二个参数)
		// 导致在 MsgWaitForMultipleObjects 那里卡死，永远返回 1，窗口白屏，完全进不去 case 0 渲染函数
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建用于 RenderShader/RenderParticleShader/DestroyStageShader 的常量缓冲资源
	void STEP27_CreateRenderCBVResource()
	{
		// 常量资源宽度，这里填整个结构体的大小，注意 256 字节对齐！
		// D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT = 256
		UINT CBufferWidth = Ceil(sizeof(RenderCBuffer), 256) * 256;

		// 常量缓冲 (上传堆) 资源结构体
		D3D12_RESOURCE_DESC CBVResourceDesc = {};
		CBVResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		CBVResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		CBVResourceDesc.Width = CBufferWidth;
		CBVResourceDesc.Height = 1;
		CBVResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		CBVResourceDesc.DepthOrArraySize = 1;
		CBVResourceDesc.MipLevels = 1;
		CBVResourceDesc.SampleDesc.Count = 1;

		// 上传堆属性的结构体，上传堆位于 CPU 和 GPU 的共享内存
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建常量缓冲资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &CBVResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_CBVRenderBlock_UploadResource));

		// 常量缓冲直接 Map 映射到结构体指针就行即可，不需要再 Unmap，小数据下常量缓冲传递效率很高
		m_CBVRenderBlock_UploadResource->Map(0, nullptr, reinterpret_cast<void**>(&RenderCBufferPointer));
	}


	// 将 9 个旋转矩阵复制到常量缓冲的 BlockFaceForwardMatrix 里，后面这个成员不会再变化了
	// shader 中会将实例数据与对应矩阵相乘，会得到不同朝向的方块，这样就能改变方块的朝向行为
	void STEP28_CopyRotateMatrixToCBuffer()
	{
		// 面向右面 (正面)，不用旋转，直接乘单位矩阵
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[0], XMMatrixIdentity());
		// 面向左面，根据左手定则，向 y 轴旋转 180°
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[1], XMMatrixRotationY(XM_PI));
		// 面向前面，向 y 轴旋转 90°
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[2], XMMatrixRotationY(XM_PIDIV2));
		// 面向后面，向 y 轴旋转 -90°
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[3], XMMatrixRotationY(-XM_PIDIV2));
		// 面向上面，向 z 轴旋转 -90° (对于正面朝上的特殊方块，相当于旋转到左面)
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[4], XMMatrixRotationZ(-XM_PIDIV2));
		// 面向下面，向 z 轴旋转 90° (对于正面朝上的特殊方块，相当于旋转到右面)
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[5], XMMatrixRotationZ(XM_PIDIV2));
		// 对于正面朝上的特殊方块，向 x 轴旋转 -90° 面向前面
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[6], XMMatrixRotationX(-XM_PIDIV2));
		// 对于正面朝上的特殊方块，向 x 轴旋转 90° 面向后面
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[7], XMMatrixRotationX(XM_PIDIV2));
		// 对于正面朝上的特殊方块，向 y 轴旋转 180° 翻转到下面
		XMStoreFloat4x4(&RenderCBufferPointer->BlockFaceForwardMatrix[8], XMMatrixRotationX(XM_PI));
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建用于渲染的根签名
	void STEP29_CreateRenderRootSignature()
	{
		// RenderRootSignature 根参数 + 静态采样器列表
		// Para 0: (Type = Root Descriptor,  2 DWORD)  (b0, space0) CBV 根描述符，用于常量缓冲
		// Para 1: (Type = Root Descriptor,  2 DWORD)  (t0, space0) SRV 根描述符，用于结构化缓冲区
		// Para 2: (Type = Descriptor Table, 1 DWORD)  (t1 - t2, space0) SRV 描述符表，用于纹理数组与粒子缓冲区
		// 
		// Sampler 0: (Type = Static Sampler) (s0, space0) 静态采样器 (邻近点过滤)，用于纹理数组采样

		ComPtr<ID3DBlob> SignatureBlob;			// 根签名字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码

		D3D12_ROOT_PARAMETER RootParameters[3] = {};		// 根参数数组


		// 第一个根参数：CBV 根描述符 (GlobalData)
		RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		RootParameters[0].Descriptor.ShaderRegister = 0;	// b0
		RootParameters[0].Descriptor.RegisterSpace = 0;		// space0
		RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


		// 第二个根参数：SRV 根描述符 (BlockCubeTexture_IndexGroup)
		RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		RootParameters[1].Descriptor.ShaderRegister = 0;	// t0
		RootParameters[1].Descriptor.RegisterSpace = 0;		// space0
		RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


		// 第三个根参数：根参数表 (2 SRV Range: m_TextureArray、m_ParticlesBuffer)
		D3D12_DESCRIPTOR_RANGE SRVRange = {};
		SRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		SRVRange.NumDescriptors = 2;		// 两个描述符 (纹理数组 + 粒子缓冲区)
		SRVRange.BaseShaderRegister = 1;	// t1 - t2
		SRVRange.RegisterSpace = 0;			// space0
		SRVRange.OffsetInDescriptorsFromTableStart = 0;

		RootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParameters[2].DescriptorTable.NumDescriptorRanges = 1;	// 一个 Range
		RootParameters[2].DescriptorTable.pDescriptorRanges = &SRVRange;
		RootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;



		D3D12_STATIC_SAMPLER_DESC StaticSamplerDesc = {};						// 静态采样器结构体，静态采样器不会占用根签名
		StaticSamplerDesc.ShaderRegister = 0;									// 要绑定的寄存器槽，对应 s0
		StaticSamplerDesc.RegisterSpace = 0;									// 要绑定的寄存器空间，对应 space0
		StaticSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;	// 纹理过滤类型，这里我们直接选 邻近点采样 就行
		StaticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// 在 U 方向上的纹理寻址方式
		StaticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// 在 V 方向上的纹理寻址方式
		StaticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// 在 W 方向上的纹理寻址方式 (3D 纹理会用到)
		StaticSamplerDesc.MinLOD = 0;											// 最小 LOD 细节层次，这里我们默认填 0 就行
		StaticSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;							// 最大 LOD 细节层次，这里我们默认填 D3D12_FLOAT32_MAX (没有 LOD 上限)
		StaticSamplerDesc.MipLODBias = 0;										// 基础 Mipmap 采样偏移量，我们这里我们直接填 0 就行
		StaticSamplerDesc.MaxAnisotropy = 1;									// 各向异性过滤等级，我们不使用各向异性过滤，默认填 1
		StaticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;			// 这个是用于阴影贴图的，我们不需要用它，所以填 D3D12_COMPARISON_FUNC_NEVER
		StaticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;		// 静态采样器在着色器中的可见性，这里指定仅在像素着色器可见



		// 根签名信息结构体，上限 64 DWORD，静态采样器不占用根签名
		D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
		RootSignatureDesc.NumParameters = 3;					// 三个根参数
		RootSignatureDesc.pParameters = RootParameters;			// 根参数数组指针
		RootSignatureDesc.NumStaticSamplers = 1;				// 一个静态采样器
		RootSignatureDesc.pStaticSamplers = &StaticSamplerDesc;	// 静态采样器指针

		// 允许顶点/实例数据从 IA 阶段输入
		RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


		// 编译根签名，让根签名先编译成 GPU 可读的二进制字节码
		D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &SignatureBlob, &ErrorBlob);
		if (ErrorBlob)
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// 用这个二进制字节码创建根签名对象
		m_D3D12Device->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&m_RenderRootSignature));
	}



	// 创建用于渲染的三个 PSO：m_RenderBlockPSO、m_RenderParticlePSO、m_DestroyStagePSO
	void STEP30_CreateRenderPSO()
	{
		// PSO 信息结构体
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

		// Input Assembler 输入装配阶段，这里复用了输入布局
		D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};			// 输入样式信息结构体
		D3D12_INPUT_ELEMENT_DESC InputElementDesc[6] = {};		// 输入元素信息结构体数组

		// Input Slot 0: Vertex Stream 顶点流，逐顶点输入

		// 顶点位置 float4 Position
		InputElementDesc[0].SemanticName = "POSITION";
		InputElementDesc[0].SemanticIndex = 0;	
		InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		InputElementDesc[0].InputSlot = 0;
		InputElementDesc[0].AlignedByteOffset = 0;
		InputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[0].InstanceDataStepRate = 0;


		// 纹理 UV 坐标 float2 texcoordUV
		InputElementDesc[1].SemanticName = "TEXCOORD";
		InputElementDesc[1].SemanticIndex = 0;
		InputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		InputElementDesc[1].InputSlot = 0;
		InputElementDesc[1].AlignedByteOffset = 16;
		InputElementDesc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[1].InstanceDataStepRate = 0;


		// 顶点所属立方体索引 uint FaceIndex
		InputElementDesc[2].SemanticName = "FACEINDEX";
		InputElementDesc[2].SemanticIndex = 0;
		InputElementDesc[2].Format = DXGI_FORMAT_R32_UINT;
		InputElementDesc[2].InputSlot = 0;
		InputElementDesc[2].AlignedByteOffset = 24;
		InputElementDesc[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[2].InstanceDataStepRate = 0;



		// Input Slot 1: Instance Stream 实例流，逐实例输入

		// 方块实例相对世界空间的偏移 float3 BlockOffset
		InputElementDesc[3].SemanticName = "BLOCKOFFSET";
		InputElementDesc[3].SemanticIndex = 0;
		InputElementDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		InputElementDesc[3].InputSlot = 1;
		InputElementDesc[3].AlignedByteOffset = 0;
		InputElementDesc[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		InputElementDesc[3].InstanceDataStepRate = 1;


		// 方块实例类型 uint BlockType
		InputElementDesc[4].SemanticName = "BLOCKTYPE";
		InputElementDesc[4].SemanticIndex = 0;
		InputElementDesc[4].Format = DXGI_FORMAT_R32_UINT;
		InputElementDesc[4].InputSlot = 1;
		InputElementDesc[4].AlignedByteOffset = 12;
		InputElementDesc[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		InputElementDesc[4].InstanceDataStepRate = 1;


		// 方块实例面朝向旋转矩阵索引 uint RotateIndex
		InputElementDesc[5].SemanticName = "ROTATEINDEX";
		InputElementDesc[5].SemanticIndex = 0;
		InputElementDesc[5].Format = DXGI_FORMAT_R32_UINT;
		InputElementDesc[5].InputSlot = 1;
		InputElementDesc[5].AlignedByteOffset = 16;
		InputElementDesc[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		InputElementDesc[5].InstanceDataStepRate = 1;



		InputLayoutDesc.NumElements = 6;						// 输入元素个数
		InputLayoutDesc.pInputElementDescs = InputElementDesc;	// 输入元素结构体数组指针
		PSODesc.InputLayout = InputLayoutDesc;					// 设置渲染管线 IA 阶段的输入样式



		ComPtr<ID3DBlob> VertexShaderBlob;		// 顶点着色器二进制字节码
		ComPtr<ID3DBlob> PixelShaderBlob;		// 像素着色器二进制字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码


		// 创建用于渲染方块的 m_RenderBlockPSO
		{
			// 编译顶点着色器 Vertex Shader
			D3DCompileFromFile(L"RenderBlockShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", NULL, NULL, &VertexShaderBlob, &ErrorBlob);
			if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
			{
				OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
				OutputDebugStringA("\n");
			}

			// 编译像素着色器 Pixel Shader
			D3DCompileFromFile(L"RenderBlockShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", NULL, NULL, &PixelShaderBlob, &ErrorBlob);
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
			PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;		// 进行背面剔除，无透明方块，加快渲染效率
			PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;		// 纯色填充

			// 第一次设置根签名！本次设置是将根签名与 PSO 绑定，生成对应版本的根签名适配 PSO，设置渲染管线的输入参数状态
			PSODesc.pRootSignature = m_RenderRootSignature.Get();

			// 设置深度测试状态
			PSODesc.DSVFormat = DSVFormat;											// 设置深度缓冲的格式
			PSODesc.DepthStencilState.DepthEnable = true;							// 开启深度缓冲
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// 深度缓冲的比较方式
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// 深度缓冲的读写权限

			// 开启混合
			PSODesc.BlendState.RenderTarget[0].BlendEnable = true;

			// 让上层色彩乘上 SrcA，Src * SrcA
			PSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			// 让下层色彩乘上 1 - SrcA，Dest * (1 - SrcA)
			PSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			// 两种色彩相加，ResultRGB = Src * SrcA + Dest * (1 - SrcA)
			PSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

			// 下面的三个选项控制 Alpha 通道的混合，Alpha 通道与 RGB 通道的混合是分开的，这一点请留意！
			// ResultA = SrcA * 1 + DstA * 1

			// 让上层色彩透明度乘 1，表示使用 SrcA
			PSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			// 让下层色彩透明度乘 0，表示不使用 DstA
			PSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
			// 最终要混合的色彩 alpha 是 ResultA
			PSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;



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


			// 创建用于渲染方块的 m_RenderBlockPSO 对象
			m_D3D12Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_RenderBlockPSO));
		}
		

		// 创建用于渲染粒子的 m_RenderParticlePSO
		{
			// 除了 shader，其他都共用，创建 m_RenderParticlePSO
			D3DCompileFromFile(L"RenderParticleShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", NULL, NULL, &VertexShaderBlob, &ErrorBlob);
			if (ErrorBlob)
			{
				OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
				OutputDebugStringA("\n");
			}

			D3DCompileFromFile(L"RenderParticleShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", NULL, NULL, &PixelShaderBlob, &ErrorBlob);
			if (ErrorBlob)
			{
				OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
				OutputDebugStringA("\n");
			}

			PSODesc.VS.pShaderBytecode = VertexShaderBlob->GetBufferPointer();
			PSODesc.VS.BytecodeLength = VertexShaderBlob->GetBufferSize();
			PSODesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();
			PSODesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();


			// 创建用于渲染粒子的 m_RenderParticlePSO
			m_D3D12Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_RenderParticlePSO));
		}

		
		// 创建用于渲染破坏纹理的 m_DestroyStagePSO
		{
			// 除了 shader、深度缓冲、光栅化深度状态，其他都共用，创建 m_DestroyStagePSO
			D3DCompileFromFile(L"DestroyStageShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", NULL, NULL, &VertexShaderBlob, &ErrorBlob);
			if (ErrorBlob)
			{
				OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
				OutputDebugStringA("\n");
			}

			D3DCompileFromFile(L"DestroyStageShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", NULL, NULL, &PixelShaderBlob, &ErrorBlob);
			if (ErrorBlob)
			{
				OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
				OutputDebugStringA("\n");
			}

			PSODesc.VS.pShaderBytecode = VertexShaderBlob->GetBufferPointer();
			PSODesc.VS.BytecodeLength = VertexShaderBlob->GetBufferSize();
			PSODesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();
			PSODesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();


			// 深度比较方式 改为 深度小于或等于原有像素 就通过测试 (NewPixel.Depth <= CurrentPixel.Depth)
			PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			// 禁止深度写入，但允许读取已有的深度进行比较
			PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;


			// 常量偏移，这是一个整数，它会乘以一个与深度缓冲区格式相关的系数 (由硬件指定)，得到一个固定深度偏移量
			// 设为 -1，意味着给所有像素的深度值加上一个微小的负常数，让它们整体上更靠近相机一点
			PSODesc.RasterizerState.DepthBias = -1;
			// 斜率相关偏移，这是一个浮点数，这个值专门用来处理那些倾斜的、几乎平行于视线方向的面
			// 这样的面在深度上变化剧烈，只用常量偏移不够，视角移动方块侧面还是会闪烁，或者正面纹理错位
			// SlopeScaledDepthBias 让偏移量能随着面的倾斜程度自动调整，倾斜越厉害 (斜率越大)，加的偏移也越大
			PSODesc.RasterizerState.SlopeScaledDepthBias = -1.0f;
			// 偏移钳位，这是一个浮点数，用来限制最终偏移量的最大值 (或最小值)，设 0 表示不进行钳位
			PSODesc.RasterizerState.DepthBiasClamp = 0;


			// 创建用于渲染方块表面破坏纹理的 m_DestroyStagePSO 对象
			m_D3D12Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_DestroyStagePSO));
		}
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建顶点流的顶点缓冲和索引缓冲，用的是 VBV0 和 IBV
	void STEP31_CreatePerVertexAndIndexBuffer()
	{
		// 上传堆顶点资源结构体
		D3D12_RESOURCE_DESC VertexResourceDesc = {};
		VertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		VertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		VertexResourceDesc.Width = PreBlockVertexData.size() * sizeof(VERTEX);
		VertexResourceDesc.Height = 1;
		VertexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		VertexResourceDesc.DepthOrArraySize = 1;
		VertexResourceDesc.MipLevels = 1;
		VertexResourceDesc.SampleDesc.Count = 1;

		// 创建顶点资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &VertexResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_BlockVertexResource));


		// 上传堆索引资源结构体
		D3D12_RESOURCE_DESC IndexResourceDesc = {};
		IndexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		IndexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		IndexResourceDesc.Width = PreBlockIndexData.size() * sizeof(UINT);
		IndexResourceDesc.Height = 1;
		IndexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		IndexResourceDesc.DepthOrArraySize = 1;
		IndexResourceDesc.MipLevels = 1;
		IndexResourceDesc.SampleDesc.Count = 1;

		// 创建索引资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &IndexResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_BlockIndexResource));


		// 将数据复制到上传堆
		BYTE* TransferPointer = nullptr;

		m_BlockVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		memcpy(TransferPointer, &PreBlockVertexData[0], PreBlockVertexData.size() * sizeof(VERTEX));
		m_BlockVertexResource->Unmap(0, nullptr);

		m_BlockIndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		memcpy(TransferPointer, &PreBlockIndexData[0], PreBlockIndexData.size() * sizeof(UINT));
		m_BlockIndexResource->Unmap(0, nullptr);


		// 填写 VBV0，IBV 结构体
		VertexBufferView[0].BufferLocation = m_BlockVertexResource->GetGPUVirtualAddress();
		VertexBufferView[0].StrideInBytes = sizeof(VERTEX);
		VertexBufferView[0].SizeInBytes = PreBlockVertexData.size() * sizeof(VERTEX);

		IndexBufferView.BufferLocation = m_BlockIndexResource->GetGPUVirtualAddress();
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		IndexBufferView.SizeInBytes = PreBlockIndexData.size() * sizeof(UINT);
	}


	// 创建实例流缓冲资源，最大放 10000 个方块，用的是 VBV1
	void STEP32_CreatePerInstanceBuffer()
	{
		BLOCK_INSTANCE blockstance = {};		// 方块实例结构体

		// 在世界中心上面一个方块设置一个不可破坏的基岩
		blockstance.BlockOffset = XMFLOAT3(0, 1, 0);
		blockstance.BlockType = 0;
		blockstance.RotateIndex = 0;
		// 新增方块
		BlockGroup.push_back(blockstance);


		// 上传堆实例资源结构体，注意资源大小！
		D3D12_RESOURCE_DESC InstanceResourceDesc = {};
		InstanceResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		InstanceResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		InstanceResourceDesc.Width = MaxBlockInstanceCount * sizeof(BLOCK_INSTANCE);
		InstanceResourceDesc.Height = 1;
		InstanceResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		InstanceResourceDesc.DepthOrArraySize = 1;
		InstanceResourceDesc.MipLevels = 1;
		InstanceResourceDesc.SampleDesc.Count = 1;

		// 创建实例资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &InstanceResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_BlockInstanceResource));


		// 填写 VBV1 结构体，注意资源大小！
		VertexBufferView[1].BufferLocation = m_BlockInstanceResource->GetGPUVirtualAddress();
		VertexBufferView[1].StrideInBytes = sizeof(BLOCK_INSTANCE);
		VertexBufferView[1].SizeInBytes = MaxBlockInstanceCount * sizeof(BLOCK_INSTANCE);

		// 进行实例化资源的持续化映射
		m_BlockInstanceResource->Map(0, nullptr, reinterpret_cast<void**>(&BlockInstanceMapPointer));
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 第 13、14、18 章的 GetTickCount64 精度太低了，精度只有毫秒级
	// 我们需要换上新的 QueryPerformanceCounter 高分辨率计时器进行微秒级计数
	// 用于计算并转换 高分辨率计时器下 两个时间刻相隔的时间 (单位：秒)
	inline double GetPerformaceCounterDiffTime(LARGE_INTEGER beg, LARGE_INTEGER end)
	{
		return static_cast<double>(end.QuadPart - beg.QuadPart) / CounterFrequency.QuadPart;
	}



	// 生成范围内随机整数 (int)
	inline int GenerateRandomInt(int minNum, int maxNum)
	{
		// 随机数设备
		std::random_device RandomDevice;
		// 32 位随机数梅森旋转种子
		std::mt19937 RandomSeed(RandomDevice());
		// 随机整数均匀分布列表
		std::uniform_int_distribution<int> RandomIntList(minNum, maxNum);
		// 返回一个随机整数
		return RandomIntList(RandomSeed);
	}



	// 生成范围内随机浮点数 (float)
	inline float GenerateRandomFloat(float minNum, float maxNum)
	{
		// 随机数设备
		std::random_device RandomDevice;
		// 32 位随机数梅森旋转种子
		std::mt19937 RandomSeed(RandomDevice());
		// 随机实数均匀分布列表
		std::uniform_real_distribution<float> RandomFloatList(minNum, maxNum);
		// 返回一个随机整数
		return RandomFloatList(RandomSeed);
	}



	// 创建破坏方块产生的粒子效果，并将粒子效果上传到 m_SpawnParticlesList
	void DestroyParticlesEffect(DESTROY_TYPE DestroyType)
	{
		// 根据 DestroyType 破坏类型确定具体的粒子效果
		switch (DestroyType)
		{
			// 目标方块正在破坏，随机在方块表面附近生成一个新粒子，模拟原版的破坏飞溅效果
			case BLOCK_DESTROYING:
			{
				// 目标破坏方块中心的坐标
				XMFLOAT3 BlockPosition = BlockGroup[DestroyInstanceIndex].BlockOffset;
				// 目标破坏方块的类型
				UINT BlockType = BlockGroup[DestroyInstanceIndex].BlockType;


				// 先清空 SpawnParticlesList
				SpawnParticleList.clear();


				// 新粒子，随机速度，随机方块内部位置，随机生命，生命周期比下面短
				Particle NewParticle = {};
				NewParticle.BlockType = BlockType;
				NewParticle.Life = GenerateRandomFloat(0.2, 0.5);
				NewParticle.Position = BlockPosition;

				// 随机决定粒子在哪个表面生成
				UINT FaceIndex = GenerateRandomInt(0, 5);

				switch (FaceIndex)
				{
					case 0:		// 右面 (+X)
					{
						NewParticle.Position.x += GenerateRandomFloat(0.4, 0.6);
						NewParticle.Velocity.x += GenerateRandomFloat(0.8, 1.5);
						NewParticle.Position.y += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.y += GenerateRandomFloat(-0.4, 1.0);
						NewParticle.Position.z += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.z += GenerateRandomFloat(-1.5, 1.5);
					}
					break;

					case 1:		// 左面 (-X)
					{
						NewParticle.Position.x -= GenerateRandomFloat(0.4, 0.6);
						NewParticle.Velocity.x -= GenerateRandomFloat(0.8, 1.5);
						NewParticle.Position.y += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.y += GenerateRandomFloat(-0.4, 1.0);
						NewParticle.Position.z += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.z += GenerateRandomFloat(-1.5, 1.5);
					}
					break;

					case 2:		// 前面 (+Z)
					{
						NewParticle.Position.z += GenerateRandomFloat(0.4, 0.6);
						NewParticle.Velocity.z += GenerateRandomFloat(0.8, 1.5);
						NewParticle.Position.y += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.y += GenerateRandomFloat(-0.4, 1.0);
						NewParticle.Position.x += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.x += GenerateRandomFloat(-1.5, 1.5);
					}
					break;

					case 3:		// 后面 (-Z)
					{
						NewParticle.Position.z -= GenerateRandomFloat(0.4, 0.6);
						NewParticle.Velocity.z -= GenerateRandomFloat(0.8, 1.5);
						NewParticle.Position.y += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.y += GenerateRandomFloat(-0.4, 1.0);
						NewParticle.Position.x += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.x += GenerateRandomFloat(-1.5, 1.5);
					}
					break;

					case 4:		// 上面 (+Y)
					{
						NewParticle.Position.y += GenerateRandomFloat(0.4, 0.6);
						NewParticle.Velocity.y += GenerateRandomFloat(0.4, 1.0);
						NewParticle.Position.x += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.x += GenerateRandomFloat(-1.5, 1.5);
						NewParticle.Position.z += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.z += GenerateRandomFloat(-1.5, 1.5);
					}
					break;

					case 5:		// 下面 (-Y)
					{
						NewParticle.Position.y -= GenerateRandomFloat(0.4, 0.6);
						NewParticle.Velocity.y -= GenerateRandomFloat(0.1, 0.4);
						NewParticle.Position.x += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.x += GenerateRandomFloat(-1.5, 1.5);
						NewParticle.Position.z += GenerateRandomFloat(-0.5, 0.5);
						NewParticle.Velocity.z += GenerateRandomFloat(-1.5, 1.5);
					}
					break;
				}


				// 添加新粒子
				SpawnParticleList.push_back(NewParticle);


				// 更新 SpawnParticlesList 中新粒子的数量
				SpawnParticlesCount = SpawnParticleList.size();
				// 将 vector 的数据复制到上传堆资源中
				memcpy(ReadySpawnParticlesListPointer, &SpawnParticleList[0],
					SpawnParticlesCount * sizeof(Particle));
			}
			break;


			// 目标方块破坏完成，在方块中心创建并上传大量粒子
			case BLOCK_COMPLETE_DESTROY:
			{
				// 目标破坏方块中心的坐标
				XMFLOAT3 BlockPosition = BlockGroup[DestroyInstanceIndex].BlockOffset;
				// 目标破坏方块的类型
				UINT BlockType = BlockGroup[DestroyInstanceIndex].BlockType;


				// 随机粒子数 (整数)，范围 [36, 48]
				int RandomParticleNums = GenerateRandomInt(36, 48);
				// 先清空 SpawnParticlesList
				SpawnParticleList.clear();


				// 在方块中心附近生成随机粒子
				for (int i = 0; i < RandomParticleNums; i++)
				{
					// 新粒子，随机速度，随机方块内部位置，随机生命，生命周期比上面长
					Particle NewParticle = {};
					NewParticle.BlockType = BlockType;
					NewParticle.Life = GenerateRandomFloat(0.5, 1.2);
					NewParticle.Position = BlockPosition;
					NewParticle.Position.x += GenerateRandomFloat(-0.5, 0.5);
					NewParticle.Position.y += GenerateRandomFloat(-0.5, 0.5);
					NewParticle.Position.z += GenerateRandomFloat(-0.5, 0.5);
					NewParticle.Velocity.x += GenerateRandomFloat(-1.5, 1.5);
					NewParticle.Velocity.y += GenerateRandomFloat(-0.8, 1.0);
					NewParticle.Velocity.z += GenerateRandomFloat(-1.5, 1.5);

					// 添加新粒子
					SpawnParticleList.push_back(NewParticle);
				}


				// 更新 SpawnParticlesList 中新粒子的数量
				SpawnParticlesCount = SpawnParticleList.size();
				// 将 vector 的数据复制到上传堆资源中
				memcpy(ReadySpawnParticlesListPointer, &SpawnParticleList[0],
					SpawnParticlesCount * sizeof(Particle));
			}
			break;
		}
	}
	


	// ---------------------------------------------------------------------------------------------------------------



	// 对 BlockGroup 的所有方块进行屏幕射线碰撞检测，如果测试通过，说明有方块正被放置/破坏
	// 测试通过，返回最近的目标方块索引；测试不通过，返回 -1
	// 破坏方向跟随十字准星，十字准心在窗口中央，NDC 空间坐标为 (0, 0)，摄像机向准星方向在世界空间发射射线
	// FaceIndex 是一个额外的输出参数，输出射线碰到最近一个面的索引 (0-5 右左前后上下)
	UINT ScreenRaycast(UINT* FaceIndex = nullptr)
	{
		// 由于十字准星永远固定在屏幕中心，玩家永远向着准星的方向进行破坏/放置，所以摄像机的观察向量就是屏幕射线
		XMVECTOR ScreenRay = m_FirstCamera.GetViewDirection();
		// 屏幕射线的起始点 (摄像机原点)
		XMVECTOR RayOrigin = m_FirstCamera.GetEyePosition();


		// 方块的 AABB 包围盒，完整固定方块的 AABB 包围盒 = 它本身
		// BoundingBox 是 DirectX 碰撞检测库的一个非常重要的数据结构，它表示一个 AABB 包围盒
		// 这个 BoundingBox 里面还有一些辅助函数 (包括类的公有辅助函数)，可以去微软文档了解一下
		BoundingBox BlockAABBbox = {};

		UINT NearestBlockIndex = -1;				// 距离摄像机原点最近方块的索引
		float NearestDistance = D3D12_FLOAT32_MAX;	// 最近方块的距离

		// 最大交互距离 = 7，方块边长是 1，java 版手长是 4.5 个方块，因为视角原因，我们让手长适当加长一下
		const float MaxInterativeDistance = 7;


		// 遍历方块组的全部方块
		for (UINT i = 0; i < BlockGroup.size(); i++)
		{
			// 当前方块的距离
			float CurrentBlockDistance = D3D12_FLOAT32_MAX;

			// 更新包围盒
			BlockAABBbox.Center = BlockGroup[i].BlockOffset;	// 方块位移就是包围盒中心
			BlockAABBbox.Extents = XMFLOAT3(0.5, 0.5, 0.5);		// 包围盒的 xyz 半边长

			// 如果起始点 (摄像机原点) 就在某一个方块内部，就忽略该方块 (无法从方块内部破坏/放置)
			if (BlockAABBbox.Contains(RayOrigin))
			{
				return -1;
			}

			// BoundingBox::Intersects 会进行屏幕射线碰撞检测，如果射线穿过了这个方块，会返回 true 和一些输出参数
			if (BlockAABBbox.Intersects(RayOrigin, ScreenRay, CurrentBlockDistance))
			{
				// 如果在最大交互距离之内，而且比已经记录的更近，就更新
				if (CurrentBlockDistance <= MaxInterativeDistance && CurrentBlockDistance < NearestDistance)
				{
					NearestBlockIndex = i;
					NearestDistance = CurrentBlockDistance;
				}
			}

			// 如果需要用到面索引，并且找到了方块，就提供该方块的最近表面索引
			if (FaceIndex && NearestBlockIndex != -1)
			{
				// 先求屏幕射线到 AABB 最近面的交点坐标，这个 NearestDistance 已经是最小距离了
				// 所以这个交点就是最近交点。交点就在射线上，直接用 Origin + dis * Direction 这种向量方法求坐标
				XMVECTOR HitPoint = RayOrigin + NearestDistance * ScreenRay;
				// 方块的中心点，XMLoadFloat3 表示将 XMFLOAT3 的数据加载到 XMVECTOR
				XMVECTOR BlockCenter = XMLoadFloat3(&BlockGroup[NearestBlockIndex].BlockOffset);
				// 交点相对中心的偏移
				XMVECTOR HitPointOffset = HitPoint - BlockCenter;

				float OffsetX = XMVectorGetX(HitPointOffset);	// 获取 x 轴偏移
				float OffsetY = XMVectorGetY(HitPointOffset);	// 获取 y 轴偏移
				float OffsetZ = XMVectorGetZ(HitPointOffset);	// 获取 z 轴偏移

				// 比较 x, y, z 轴偏移的绝对值，其中一个轴的绝对值最大，说明方块最近面只能是这个方向上的两个面
				// 因为偏移最大值只能是方块半边长 1，大多数情况都只有一个轴的绝对值最大，判断正负就可以确定最终面了
				// 为什么不直接判断哪个方向的偏移是 1 呢? 因为有浮点数误差，近似判断是否为 1，不如直接判断最大值
				// 即使屏幕射线射中的是方块的棱边或棱角，这种方法也能正确选择是哪一个面
				if (abs(OffsetX) >= abs(OffsetY) && abs(OffsetX) >= abs(OffsetZ))
				{
					*FaceIndex = OffsetX > 0 ? 0 : 1;	// 0: 右面 (+X)、1: 左面 (-X)
				}
				else if (abs(OffsetZ) >= abs(OffsetX) && abs(OffsetZ) >= abs(OffsetY))
				{
					*FaceIndex = OffsetZ > 0 ? 2 : 3;	// 2: 前面 (+Z)、3: 后面 (-Z)
				}
				else
				{
					*FaceIndex = OffsetY > 0 ? 4 : 5;	// 4: 上面 (+Y)、5: 左边 (-Y)
				}
			}
		}

		return NearestBlockIndex;	// 最终返回目标实例索引
	}



	// 如果鼠标左键按下，就进行屏幕射线相交检测，检测到目标就破坏方块，并在目标方块周围产生粒子效果
	void DestroyBlock()
	{
		// 先进行屏幕射线相交检测，查看射线是否远离方块，如果索引相同就更新破坏阶段，不同就立即重置

		UINT FaceIndex = -1;									// 射线击中的最近表面
		UINT CurrentHitBlockIndex = ScreenRaycast(&FaceIndex);	// 当前破坏的方块索引

		// 如果射线没有击中任何方块，或者击中的是基岩，立即重置破坏状态
		if (CurrentHitBlockIndex == -1 || BlockGroup[CurrentHitBlockIndex].BlockType == 0)
		{
			// 重置方块索引
			DestroyInstanceIndex = -1;
			// 重置破坏阶段纹理索引
			DestroyStageIndex = -1;
		}

		// 如果射线击中了方块，但不是当前实例，立即重置破坏状态，新方块从阶段 0 开始
		else if (CurrentHitBlockIndex != DestroyInstanceIndex)
		{
			// 重置方块索引为新方块
			DestroyInstanceIndex = CurrentHitBlockIndex;
			// 重置计时器
			QueryPerformanceCounter(&BeginDestroyRecordedTick);
			QueryPerformanceCounter(&CurrentDestroyTick);
			// 重置破坏阶段纹理索引至阶段 0
			DestroyStageIndex = 18;
			// 更新常量缓冲
			RenderCBufferPointer->DestroyStage = DestroyStageIndex;

			// 产生少量破坏粒子效果
			DestroyParticlesEffect(BLOCK_DESTROYING);
		}

		// 如果破坏的仍然是当前方块实例，更新方块破坏状态
		else
		{
			// 更新当前计时
			QueryPerformanceCounter(&CurrentDestroyTick);
			// 计算破坏阶段的纹理索引，一共 10 副纹理，每幅纹理 0.1 秒 = 100 毫秒
			// 这里我们固定除了基岩，破坏方块都是 1s，读者们掌握后可以查 MC wiki 自由拓展
			// 18 是破坏纹理在 TextureGroup 的基础索引
			DestroyStageIndex = 18 +
				static_cast<UINT>(GetPerformaceCounterDiffTime(BeginDestroyRecordedTick, CurrentDestroyTick) * 10);
			// 更新当前破坏阶段索引到常量缓冲，我们需要在 DestroyStageShader.hlsl 上采样破坏纹理
			RenderCBufferPointer->DestroyStage = DestroyStageIndex;


			// 如果计算后索引 > 27，说明已经达到 1s，目标方块破坏完成
			// 移除这个方块，重置破坏状态，更新脏数据标志，下面将进行新一轮数据更新
			if (DestroyStageIndex > 27)
			{
				// 先在方块中心产生大量破坏粒子效果
				DestroyParticlesEffect(BLOCK_COMPLETE_DESTROY);

				// 再删除 vector 迭代器指代的实例对象 (目标方块)
				BlockGroup.erase(BlockGroup.begin() + DestroyInstanceIndex);

				DestroyInstanceIndex = -1;	// 重置破坏实例索引
				DestroyStageIndex = -1;		// 重置破坏阶段索引
				isDirtyData = true;			// 数据更新标志设 true
			}
			else
			{
				// 产生少量破坏粒子效果
				DestroyParticlesEffect(BLOCK_DESTROYING);
			}
		}
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 更新每帧的数据，将每帧新的常量数据传递到常量缓冲区中，这样就能看到动态的 3D 画面了
	// 在这里处理方块破坏逻辑，WM_LBUTTONDOWN 仅负责更新标志，不负责任何破坏逻辑，因为它的事都被 UpdateConstantBuffer 做了
	// WM_RBUTTONDOWN 不仅要更新标志，还需要立即重置破坏相关的变量，这样可以减少 UpdateConstantBuffer 的判断与命令执行次数
	void UpdateFrameBuffer()
	{
		// 将更新后的矩阵，存储到共享内存上的常量缓冲，这样 GPU 就可以访问到 MVP 矩阵了
		XMStoreFloat4x4(&RenderCBufferPointer->MVPMatrix, m_FirstCamera.GetMVPMatrix());



		// 如果鼠标左键按下，就执行 DestroyBlock
		if (isLeftButtonPressDown)
		{
			DestroyBlock();
		}


		// 如果放置/破坏方块触发更新，isDirtyData = true，就进行全量更新
		if (isDirtyData)
		{
			// 全量更新
			memcpy(BlockInstanceMapPointer, &BlockGroup[0], BlockGroup.size() * sizeof(BLOCK_INSTANCE));
			// 重置标志，只在数据 (BlockGroup) 被修改的时候才进行全量更新，这样就不用反复更新导致帧率下降了
			isDirtyData = false;
		}
	}



	// 渲染
	void Render()
	{
		// 每帧渲染开始前，调用 UpdateFrameBuffer() 更新每帧数据
		UpdateFrameBuffer();


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


		// 计算着色器新增并更新粒子
		{
			// 用于设置 m_ComputeUAVSRVHeap 用的临时 ID3D12DescriptorHeap 数组
			ID3D12DescriptorHeap* _temp_ComputeHeaps[] = { m_ParticleUAVSRVHeap.Get() };
			// 设置计算着色器需要用到的 m_ComputeUAVSRVHeap，这个指令会向 GPU 传递并绑定描述符堆的基地址
			// 这个指令是一个开销比较大的指令，官方建议一帧只调用一次或两次，所以鼓励资源与描述符复用
			// 并且同类型 Heap 一次最多只能设置一个：CBV/SRV/UAV 类型一个，Sampler 类型一个
			m_CommandList->SetDescriptorHeaps(1, _temp_ComputeHeaps);

			// 第二次设置根签名，本次检测 PSO 根签名的合法性 (引用资源是否匹配)，检测成功会开启显存与寄存器的映射通道
			// 注意！这里是 SetComputeRootSignature 版本的！下文这一整个代码块涉及到资源绑定都是 Compute 版本！
			m_CommandList->SetComputeRootSignature(m_ParticleRootSignature.Get());


			// 如果 m_SpawnParticlesList 有新粒子，记录计算着色器生成新粒子的命令，准备上传到 SpawnCSMain
			if (SpawnParticlesCount != 0)
			{
				// 设置 SpawnCSMain 的渲染管线状态
				m_CommandList->SetPipelineState(m_ParticleSpawnPSO.Get());

				// 设置第一个根参数：SpawnParticlesCount 待生成列表的粒子数量
				m_CommandList->SetComputeRoot32BitConstant(0, SpawnParticlesCount, 0);

				// 设置第三个根参数：粒子缓冲、空闲栈、栈顶指针、待生成列表
				// 借助根描述表 + 描述符堆按顺序排列描述符，就可以一次性快速绑定多个描述符到管线中
				m_CommandList->SetComputeRootDescriptorTable(2, ParticleUAVSRVHeap_GPUBaseHandle);

				// 计算新粒子总共需要的线程组数
				UINT NeedDispatchThreadGroupNum = Ceil(SpawnParticlesCount, SpawnCSMainThreadNums);

				// 调度线程组，开始添加粒子！
				m_CommandList->Dispatch(NeedDispatchThreadGroupNum, 1, 1);

				// 上传完新粒子后，将 SpawnParticlesCount 清 0
				SpawnParticlesCount = 0;
			}


			// 每帧记录计算着色器更新粒子的命令，不管 m_ParticlesHiveBuffer 是否还有活跃粒子，微小开销可以忽略
			{
				// 设置 UpdateCSMain 的渲染管线状态
				m_CommandList->SetPipelineState(m_ParticleUpdatePSO.Get());

				// 先更新当前帧时间
				QueryPerformanceCounter(&CurrentFrameTick);
				// 计算每帧过去的时间 (当前帧 - 上一帧)
				float ElapsedTime =
					static_cast<float>(GetPerformaceCounterDiffTime(LastFrameTick, CurrentFrameTick));
				// 再更新上一帧时间
				QueryPerformanceCounter(&LastFrameTick);


				// 设置第二个根参数：每帧过去的时间，粒子重力，粒子缓冲最大分配容量
				// 注意第三个参数是要设置的常量值所在根参数的位置 (索引)
				// 小心！第二个参数是 UINT，直接把 float 传进去会发生隐式转换，丢失小数部分！
				// 我们需要使用 reinterpret_cast 进行二进制位的重新解释 (强制转换)，这样就不会发生小数截断了
				m_CommandList->SetComputeRoot32BitConstant(1,
					*reinterpret_cast<UINT*>(&ElapsedTime), 0);

				// 设置粒子重力，也要 reinterpret_cast 强制转换
				m_CommandList->SetComputeRoot32BitConstant(1,
					*reinterpret_cast<const UINT*>(&GravityData), 1);

				// 设置第三个根参数：粒子缓冲、空闲栈、栈顶指针、待生成列表
				// 借助根描述表 + 描述符堆按顺序排列描述符，就可以一次性快速绑定多个描述符到管线中
				m_CommandList->SetComputeRootDescriptorTable(2, ParticleUAVSRVHeap_GPUBaseHandle);


				// 计算新粒子总共需要的线程组数
				UINT NeedDispatchThreadGroupNum = Ceil(MaxAllocParticlesNums, UpdateCSMainThreadNums);

				// 调度线程组，开始添加粒子！
				m_CommandList->Dispatch(NeedDispatchThreadGroupNum, 1, 1);


				// 注意这里！当计算完成后，我们等会就要将粒子缓冲区挪入渲染管线做 SRV Resource
				// 顶点着色器直接访问 UAV 资源是未定义行为，大部分硬件普遍不支持的
				// 所以还需要将 UAV 资源转换到 SRV 资源，配合上面在 m_RenderSRVHeap 创建的第二个描述符来渲染
				m_CommandList->ResourceBarrier(1, &UAVToSRV_barrier);
			}
		}


		// 渲染方块、粒子以及破坏纹理
		{
			// 将起始转换屏障的资源指定为当前渲染目标
			beg_barrier.Transition.pResource = m_D3D12RenderTarget[FrameIndex].Get();
			// 先对渲染目标设置 Present -> Render Target 的资源屏障
			m_CommandList->ResourceBarrier(1, &beg_barrier);


			// 设置视口 (光栅化阶段)，用于光栅化里的屏幕映射
			m_CommandList->RSSetViewports(1, &ViewPort);
			// 设置裁剪矩形 (光栅化阶段)
			m_CommandList->RSSetScissorRects(1, &ScissorRect);


			// 用 RTV 句柄设置渲染目标，同时用 DSV 句柄设置深度模板缓冲，开启深度测试
			m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, &DSVHandle);

			// 清空当前渲染目标的背景为天蓝色，此操作会同时清理 3D 和 2D 的已渲染/绘制的对象 (清空整个后台窗口缓冲)
			// 注意这里！不需要用到 D2DUIRender 里面的 m_D2DDeviceContext->Clear 了，原因：D2DRenderTarget = D3D12RenderTarget
			m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);

			// 清空后台的深度模板缓冲，将深度重置为初始值 1
			m_CommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);


			// 用于设置 m_RenderSRVHeap 用的临时 ID3D12DescriptorHeap 数组
			ID3D12DescriptorHeap* _temp_RenderHeaps[] = { m_RenderSRVHeap.Get() };
			// 设置渲染需要用到的 m_RenderSRVHeap
			m_CommandList->SetDescriptorHeaps(1, _temp_RenderHeaps);

			// 第二次设置根签名，本次检测 PSO 根签名的合法性 (引用资源是否匹配)，检测成功会开启显存与寄存器的映射通道
			// 注意！这里是 SetGraphicsRootSignature 版本的！下文这一整个代码块涉及到资源绑定都是 Graphics 版本！
			m_CommandList->SetGraphicsRootSignature(m_RenderRootSignature.Get());

			// 设置第一个根参数：CBV 常量缓冲 (GlobalData)
			m_CommandList->SetGraphicsRootConstantBufferView(0, m_CBVRenderBlock_UploadResource->GetGPUVirtualAddress());
			// 设置第二个根参数：SRV 结构化缓冲区 (BlockCubeTexture_IndexGroup)
			m_CommandList->SetGraphicsRootShaderResourceView(1, m_SRVStructuredBuffer_DefaultResource->GetGPUVirtualAddress());
			// 设置第三个根参数：SRV 纹理数组 + 粒子缓冲区，通过根描述表与描述符堆上的排列，提供基地址就能连续绑定
			m_CommandList->SetGraphicsRootDescriptorTable(2, SRVTextureArray_GPUHandle);


			// 渲染方块
			{
				// 设置渲染方块的 PSO
				m_CommandList->SetPipelineState(m_RenderBlockPSO.Get());

				// 设置图元拓扑 (输入装配阶段)，我们这里设置三角形列表
				m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				// 设置 VBV 顶点缓冲描述符数组，两个 VBV 都会被设置 (输入装配阶段) 
				m_CommandList->IASetVertexBuffers(0, 2, VertexBufferView);

				// 设置 IBV 索引缓冲描述符 (输入装配阶段) 
				m_CommandList->IASetIndexBuffer(&IndexBufferView);

				// Draw Call 渲染所有目标实例！
				m_CommandList->DrawIndexedInstanced(PreBlockIndexData.size(), BlockGroup.size(), 0, 0, 0);
			}


			// 每帧渲染粒子
			{
				// 设置渲染粒子的 PSO
				m_CommandList->SetPipelineState(m_RenderParticlePSO.Get());

				// 渲染粒子缓冲区下的所有粒子 (不管粒子是否存活、死亡或未使用，对应的 shader 会处理的)
				m_CommandList->DrawIndexedInstanced(PreBlockIndexData.size(), MaxAllocParticlesNums, 0, 0, 0);


				// 注意这里！渲染完成后需要再对粒子缓冲区资源进行一次 SRVToUAV 的屏障转换！这样下一帧计算着色器才能继续使用！
				m_CommandList->ResourceBarrier(1, &SRVToUAV_barrier);
			}


			// 渲染目标方块的破坏纹理
			{
				// 如果某个方块正在被破坏 (DestroyInstanceIndex != -1)，就对这个方块绘制相应的破坏纹理
				if (DestroyInstanceIndex != -1)
				{
					// 设置渲染破坏纹理的 PSO
					m_CommandList->SetPipelineState(m_DestroyStagePSO.Get());

					// 绘制破坏纹理 (实际上是又套了一层表面是破坏纹理的方块)
					m_CommandList->DrawIndexedInstanced(PreBlockIndexData.size(), 1, 0, 0, DestroyInstanceIndex);
				}
			}
		}


		// 关闭命令列表，Record 录制状态 -> Close 关闭状态，命令列表只有关闭才可以提交
		m_CommandList->Close();

		// 用于传递命令用的临时 ID3D12CommandList 数组
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// 执行上文的渲染命令！
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);


		// 向 GPU 提交完 3D 渲染命令后，CPU 准备记录 2D 渲染指令
		// 注意 D2D 归还渲染目标的使用权，还会自动插入一道 Render Target -> Present 的屏障
		m_D2DEngine.D2DUIRender(FrameIndex, WindowWidth, WindowHeight);


		// 向命令队列发出交换缓冲的命令，此命令会加入到命令队列中，命令队列执行到该命令时，会通知交换链交换缓冲
		// 3D 和 2D 绘制指令的记录与提交必须要在交换链 Present 之前，交换缓冲说明一帧已经画完了，开始绘制下一帧缓冲
		m_DXGISwapChain->Present(1, NULL);


		// 将围栏预定值设定为下一帧的任务完成值，说明 GPU 完成了一项任务
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		// 命令队列执行到这里会修改围栏值，表示渲染已完成，"击中"围栏，同时修改围栏的 Completed Value 任务完成值
		// 这里传入 FenceValue 是因为 CommandQueue 要用这个值标记预定事件，关联围栏
		m_CommandQueue->Signal(m_RenderFence.Get(), FenceValue);
		// 设置围栏的预定事件，当渲染完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		// 这里传入 FenceValue 是因为围栏要拿这个值开辟对应的 Event Slot 事件槽，并将 CPU 端事件句柄绑定到事件槽上
		m_RenderFence->SetEventOnCompletion(FenceValue, RenderEvent);
	}



	// 渲染循环
	void STEP33_RenderLoop()
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
							isExit = true;				// 收到退出消息，就退出消息循环
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



	// ---------------------------------------------------------------------------------------------------------------



	// 根据不同的选择槽索引，在选中方块表面上放置不同的方块，这个操作是瞬间发生的
	// 放置的方块正面需要面向玩家，部分方块需要特殊处理
	// NearestPlacedBlockIndex 屏幕射线选中的方块实例索引
	// FaceIndex 选中方块的最近面索引
	void PlacedBlock(UINT NearestPlacedBlockIndex, UINT FaceIndex)
	{
		// 新方块实例
		BLOCK_INSTANCE NewBlockInstance = {};
		// 新方块类型
		UINT NewBlockType = InventoryBlockTypeGroup[m_D2DEngine.Get_Selected_Slot_Index()];
		// 摄像机原点
		XMVECTOR RayOrigin = m_FirstCamera.GetEyePosition();

		switch (FaceIndex)
		{
			case 0:		// 右面
			{
				NewBlockInstance.BlockOffset = BlockGroup[NearestPlacedBlockIndex].BlockOffset;
				NewBlockInstance.BlockOffset.x += 1;
				NewBlockInstance.BlockType = NewBlockType;

				// 根据方块类型，进行不同的处理
				switch (NewBlockType)
				{
					case 3:		// TNT
					{
						// TNT 要统一到正面，不然太难看了...
						NewBlockInstance.RotateIndex = 0;
					}
					break;

					// 不是上面的种种特殊方块，就是常规方块，直接赋值
					default:
					{
						NewBlockInstance.RotateIndex = 0;
					}
				}

				BlockGroup.push_back(NewBlockInstance);
			}
			break;

			case 1:		// 左面
			{
				NewBlockInstance.BlockOffset = BlockGroup[NearestPlacedBlockIndex].BlockOffset;
				NewBlockInstance.BlockOffset.x -= 1;
				NewBlockInstance.BlockType = NewBlockType;

				// 根据方块类型，进行不同的处理
				switch (NewBlockType)
				{
					case 3:		// TNT
					{
						// TNT 要统一到正面，不然太难看了...
						NewBlockInstance.RotateIndex = 0;
					}
					break;

					// 不是上面的种种特殊方块，就是常规方块，直接赋值
					default:
					{
						NewBlockInstance.RotateIndex = 1;
					}
				}

				BlockGroup.push_back(NewBlockInstance);
			}
			break;

			case 2:		// 前面
			{
				NewBlockInstance.BlockOffset = BlockGroup[NearestPlacedBlockIndex].BlockOffset;
				NewBlockInstance.BlockOffset.z += 1;
				NewBlockInstance.BlockType = NewBlockType;

				// 根据方块类型，进行不同的处理
				switch (NewBlockType)
				{
					case 3:		// TNT
					{
						// TNT 要统一到正面，不然太难看了...
						NewBlockInstance.RotateIndex = 0;
					}
					break;

					// 不是上面的种种特殊方块，就是常规方块，直接赋值
					default:
					{
						NewBlockInstance.RotateIndex = 2;
					}
				}

				BlockGroup.push_back(NewBlockInstance);
			}
			break;

			case 3:		// 后面
			{
				NewBlockInstance.BlockOffset = BlockGroup[NearestPlacedBlockIndex].BlockOffset;
				NewBlockInstance.BlockOffset.z -= 1;
				NewBlockInstance.BlockType = NewBlockType;

				// 根据方块类型，进行不同的处理
				switch (NewBlockType)
				{
					case 3:		// TNT
					{
						// TNT 要统一到正面，不然太难看了...
						NewBlockInstance.RotateIndex = 0;
					}
					break;

					// 不是上面的种种特殊方块，就是常规方块，直接赋值
					default:
					{
						NewBlockInstance.RotateIndex = 3;
					}
				}

				BlockGroup.push_back(NewBlockInstance);
			}
			break;

			case 4:		// 上面
			{
				NewBlockInstance.BlockOffset = BlockGroup[NearestPlacedBlockIndex].BlockOffset;
				NewBlockInstance.BlockOffset.y += 1;
				NewBlockInstance.BlockType = NewBlockType;

				// 根据方块类型，进行不同的处理
				switch (NewBlockType)
				{
					case 3:		// TNT
					{
						// TNT 要统一到正面，不然太难看了...
						NewBlockInstance.RotateIndex = 0;
					}
					break;

					// 不是上面的种种特殊方块，就是常规方块
					// 常规方块面方向是左右前后，计算 xz 两个坐标相对方块中心的绝对值，朝向绝对值更大的
					default:
					{
						XMVECTOR Offset = RayOrigin - XMLoadFloat3(&NewBlockInstance.BlockOffset);
						float OffsetX = XMVectorGetX(Offset);
						float OffsetZ = XMVectorGetZ(Offset);

						if (abs(OffsetX) >= abs(OffsetZ))
						{
							NewBlockInstance.RotateIndex = OffsetX > 0 ? 0 : 1;
						}
						else
						{
							NewBlockInstance.RotateIndex = OffsetZ > 0 ? 2 : 3;
						}
					}
				}

				BlockGroup.push_back(NewBlockInstance);
			}
			break;

			case 5:		// 下面
			{
				NewBlockInstance.BlockOffset = BlockGroup[NearestPlacedBlockIndex].BlockOffset;
				NewBlockInstance.BlockOffset.y -= 1;
				NewBlockInstance.BlockType = NewBlockType;

				// 根据方块类型，进行不同的处理
				switch (NewBlockType)
				{
					case 3:		// TNT
					{
						// TNT 要统一到正面，不然太难看了...
						NewBlockInstance.RotateIndex = 0;
					}
					break;

					// 不是上面的种种特殊方块，就是常规方块
					// 常规方块面方向是左右前后，计算 xz 两个坐标相对方块中心的绝对值，朝向绝对值更大的
					default:
					{
						XMVECTOR Offset = RayOrigin - XMLoadFloat3(&NewBlockInstance.BlockOffset);
						float OffsetX = XMVectorGetX(Offset);
						float OffsetZ = XMVectorGetZ(Offset);

						if (abs(OffsetX) >= abs(OffsetZ))
						{
							NewBlockInstance.RotateIndex = OffsetX > 0 ? 0 : 1;
						}
						else
						{
							NewBlockInstance.RotateIndex = OffsetZ > 0 ? 2 : 3;
						}
					}
				}

				BlockGroup.push_back(NewBlockInstance);
			}
			break;
		}

		// 更新脏数据标志为 true，表示下一次渲染前需要在 UpdateConstantBuffer 更新
		isDirtyData = true;
	}



	// 回调函数，处理窗口产生的消息
	// 1-9 数字键 —— 切换选中的物品槽
	// 滚轮 —— 切换选中的物品槽
	// WASD 键 —— 摄像机前后左右移动
	// 鼠标移动 —— 摄像机视角旋转
	// 鼠标左键长按 —— 破坏方块
	// 鼠标右键单击 —— 放置方块
	// Esc 键或 / 键 —— 发送 WM_DESTROY 消息，窗口关闭，程序进程退出 (加这个 / 键是防没有 Esc 退不出去的)
	LRESULT CALLBACK CallBackFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
			case WM_CREATE:		// 窗口被创建，此消息只会产生一次
			{
				// 创建窗口时隐藏光标，程序退出时会自动恢复光标显示
				ShowCursor(false);
				// 先设置光标到窗口中心，防止偏移过大导致视角瞬移
				SetCursorPos(WindowWidth / 2, WindowHeight / 2);
				// 更新当前与上一次鼠标光标位置，防止视角瞬移
				m_FirstCamera.UpdateCurrentCursorPos();
				m_FirstCamera.UpdateLastCursorPos();
				// 设置高分辨率计时器的频率
				QueryPerformanceFrequency(&CounterFrequency);
			}
			break;


			case WM_DESTROY:			// 窗口被销毁 (当按下右上角 X 关闭窗口时)
			{
				SetWindowText(hwnd, L"正在等待 CPU 与 GPU 同步中，同步完成会自动关闭窗口...");
				PostQuitMessage(0);		// 向操作系统发出退出请求 (WM_QUIT)，结束消息循环
				Sleep(1000);			// 发送退出消息后让主线程等待一秒，先让 CPU 与 GPU 做完同步再安全释放资源
			}
			break;


			case WM_CHAR:	// 获取键盘产生的字符消息，TranslateMessage 会将虚拟键码翻译成字符码，同时产生 WM_CHAR 消息
			{
				switch (wParam)		// wParam 是按键对应的字符 ASCII 码
				{
					case 'w':
					case 'W':	// 向前移动
						m_FirstCamera.Walk(0.4);
						break;

					case 's':
					case 'S':	// 向后移动
						m_FirstCamera.Walk(-0.4);
						break;

					case 'a':
					case 'A':	// 向左移动
						m_FirstCamera.Strafe(-0.4);
						break;

					case 'd':
					case 'D':	// 向右移动
						m_FirstCamera.Strafe(0.4);
						break;


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

					case '/':	// 退出，和 Esc 一样的功能，后面会演化成原版的打指令
					{
						SendMessage(hwnd, WM_DESTROY, wParam, lParam);
					}
					break;
				}
			}
			break;


			case WM_KEYDOWN:	// 按下按键就会触发的消息，和 WM_CHAR 不冲突，可以处理非字符键产生的事件
			{
				switch (wParam)		// wParam 是按键对应的虚拟键码
				{
					case VK_ESCAPE:	// Esc 键属于控制键 (非字符键)，不能在 WM_CHAR 上进行处理
					{
						SendMessage(hwnd, WM_DESTROY, wParam, lParam);
					}
					break;
				}
			}
			break;


			case WM_MOUSEMOVE:	// 获取鼠标移动消息，鼠标光标一次有效移动，就旋转摄像机视角
			{
				if (isAutoResetCursorMsg)	// 如果是 SetCursorPos 发送的消息，直接忽略
				{
					isAutoResetCursorMsg = false;	// 重置标志，准备接收下一次有效移动
					break;							// 直接结束本次 switch，不用再处理了
				}

				// 更新当前光标位置
				m_FirstCamera.UpdateCurrentCursorPos();
				// 旋转摄像机视角，旋转视角 = (当前光标位置 - 上一次光标位置) * 旋转系数
				m_FirstCamera.CameraRotate();
				// 光标移动后，设置光标在窗口中心，此函数会额外发送一个 WM_MOUSEMOVE，需要进行忽略
				SetCursorPos(WindowWidth / 2, WindowHeight / 2);
				// 激活重置标志，下一次接收到 WM_MOUSEMOVE 就忽略
				isAutoResetCursorMsg = true;
				// 更新上一次光标位置，将当前光标位置 (窗口中心) 设置成上一次光标位置
				// 防止窗口卡住一个视角不动，或者偏移导致差值过大，视角瞬移
				m_FirstCamera.UpdateLastCursorPos();
			}
			break;


			case WM_MOUSEWHEEL:		// 鼠标滚轮消息，和数字键功能一样切换选中框
			{
				// 获取当前滑槽索引
				UINT Selected_Slot_Index = m_D2DEngine.Get_Selected_Slot_Index();

				// 获取滚轮旋转量
				int delta = GET_WHEEL_DELTA_WPARAM(wParam);

				// 向上滚动：切换到上一个槽
				if (delta > 0) Selected_Slot_Index--;
				// 向下滚动：切换到下一个槽
				if (delta < 0) Selected_Slot_Index++;

				// 无论如何滚动，Selected_Slot_Index 必须在 [0, 8] 之间，防止越界
				Selected_Slot_Index = (Selected_Slot_Index + 9) % 9;

				// 设置新索引
				m_D2DEngine.Set_Selected_Slot_Index(Selected_Slot_Index);
			}
			break;


			case WM_LBUTTONDOWN:	// 左键按下，按下状态更新，UpdateConstantBuffer 开始处理破坏逻辑
			{
				// 按下一次左键才会发送一个 WM_LBUTTONDOWN 消息，长按不会持续发送
				isLeftButtonPressDown = true;
			}
			break;


			case WM_LBUTTONUP:		// 左键松开，按下状态重置，立即重置相关的破坏状态变量
			{
				// 松开一次左键才会发送一个 WM_LBUTTONUP 消息
				isLeftButtonPressDown = false;
				// 重置方块索引
				DestroyInstanceIndex = -1;
				// 重置破坏阶段纹理索引
				DestroyStageIndex = -1;
			}
			break;


			case WM_RBUTTONDOWN:	// 右键按下，如果屏幕射线与方块相交，在最近的方块前放置新方块
			{
				// 按下一次右键才会发送一个 WM_RBUTTONDOWN 消息，长按不会持续发送

				UINT NearestPlacedBlockIndex = -1;	// 选中的最近方块实例的索引
				UINT FaceIndex = -1;				// 选中方块的最近面索引

				// 进行碰撞检测
				NearestPlacedBlockIndex = ScreenRaycast(&FaceIndex);

				// 如果找到最近方块，就放置
				if (NearestPlacedBlockIndex != -1)
				{
					PlacedBlock(NearestPlacedBlockIndex, FaceIndex);
				}
			}
			break;


			// 如果接收到其他消息，直接默认返回整个窗口
			default: return DefWindowProc(hwnd, msg, wParam, lParam);
		}

		return 0;
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 运行窗口
	static void Run(HINSTANCE hins)
	{
		DX12Engine engine;
		engine.STEP01_InitWindow(hins);
		engine.STEP02_CreateDebugDevice();
		engine.STEP03_CreateDevice();
		engine.STEP04_IgnoreClearValueWarning();
		engine.STEP05_CreateCommandComponents();
		engine.STEP06_CreateRenderTarget();
		engine.STEP07_CreateFenceAndBarrier();
		engine.STEP08_CreateDSVHeap();
		engine.STEP09_CreateDepthStencilBuffer();
		engine.STEP10_CreateDSV();


		engine.STEP11_InitializeD2DEngine();
		engine.STEP12_LoadImageAndTransform();
		engine.STEP13_LoadAndGenerateBlockIcons();


		engine.STEP14_GetTextureArrayElementsProperties();
		engine.STEP15_CreateTextureArrayResource();
		engine.STEP16_CopyTextureArrayToDefaultResource();
		engine.STEP17_CreateRenderSRVHeap();
		engine.STEP18_CreateTextureArraySRV();


		engine.STEP19_CreateStructuredBufferResource();
		engine.STEP20_CopyStructuredBufferToDefaultResource();


		engine.STEP21_CreateParticleRequireResources();


		engine.STEP22_CreateUAVSRVHeapAndDescriptor();
		engine.STEP23_CreateParticleRootSignature();
		engine.STEP24_CreateParticlePSO();
		engine.STEP25_CreateParticlesBufferBarrier();
		engine.STEP26_InitParticlesBuffer();


		engine.STEP27_CreateRenderCBVResource();
		engine.STEP28_CopyRotateMatrixToCBuffer();


		engine.STEP29_CreateRenderRootSignature();
		engine.STEP30_CreateRenderPSO();


		engine.STEP31_CreatePerVertexAndIndexBuffer();
		engine.STEP32_CreatePerInstanceBuffer();


		engine.STEP33_RenderLoop();
	}
};



// 主函数
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}
