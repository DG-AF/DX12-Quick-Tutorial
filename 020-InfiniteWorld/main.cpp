
// (20) InfiniteWorld: 进一步学习计算着色器，掌握动态资源的管理，认识 UAV 纹理数组与结构化缓冲区，模拟 MC 无限世界的生成


// windows.h 与标准库里的 min/max 函数重名导致冲突了，禁用 windows.h 里面的 min/max 函数
#define NOMINMAX


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
#include<cmath>					// C++ 数学标准库
#include<unordered_map>			// C++ STL unordered_map 哈希表库，用于新旧区块比较
#include<stack>					// C++ STL stack 栈库，用于下文的空闲栈


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



// D2D 引擎，这里相比第 18 章把 D2D 的内容删了，仅保留 WIC 部分功能，暂时不画 UI
// 目的是希望大家重点关注新东西：动态资源加载与卸载、UAV 纹理数组、UAV 结构体缓冲区
class D2DEngine
{
private:

	ComPtr<IWICImagingFactory> m_WICFactory;				// WIC 工厂
	ComPtr<IWICBitmapDecoder> m_WICBitmapDecoder;			// 位图解码器
	ComPtr<IWICBitmapFrameDecode> m_WICBitmapDecodeFrame;	// 由解码器得到的单个位图帧
	ComPtr<IWICFormatConverter> m_WICFormatConverter;		// 位图转换器



	// ---------------------------------------------------------------------------------------------------------------

public:

	// WIC 工厂初始化函数，仅初始化 WIC 工厂，一个进程重复释放创建 WIC 工厂会报错
	void D2D_STEP01_InitializeWICFactory()
	{
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_WICFactory));
	}


	// DX12Engine 传递要加载的纹理名，WIC 再次读取图片，并将它们转换成 DX12 可用的 WICBitmapSource
	// vector 是一个 inout 输入输出参数，外部 (DX12Engine) 提供 vector，此函数逐一创建 vector 中的元素
	bool D2D_STEP02_LoadTextureIntoWICBitmaps(
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

	XMVECTOR EyePosition = XMVectorSet(5, 26, 12, 1);		// 摄像机在世界空间下的位置
	XMVECTOR FocusPosition = XMVectorSet(0, 8, 0, 1);		// 摄像机在世界空间下观察的焦点位置
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

	ComPtr<ID3D12Fence> m_RenderFence;						// 专门用于 渲染 + 短期复制资源 的围栏
	UINT64 FenceValue = 0;									// 用于围栏等待的围栏值
	HANDLE RenderEvent = NULL;								// GPU 渲染事件
	D3D12_RESOURCE_BARRIER beg_barrier = {};				// 渲染开始的资源屏障，呈现 -> 渲染目标
	D3D12_RESOURCE_BARRIER end_barrier = {};				// 渲染结束的资源屏障，渲染目标 -> 呈现 (如果用了 D2D 这个要被移除)

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
		L"resource/dirt.png",			// 0.泥土
		L"resource/grass_side.png",		// 1.草方块侧面
		L"resource/grass_top.png",		// 2.草方块顶面
		L"resource/stone.png",			// 3.石头
		L"resource/bedrock.png",		// 4.基岩
	};


	// 3D 渲染使用的方块 WIC 纹理资源组
	// 当资源全部加载到上传堆，全部 WIC 位图资源 (DX12) 都会被释放，不再让它们占内存
	std::vector<ComPtr<IWICBitmapSource>> m_TextureGroup;



	// ---------------------------------------------------------------------------------------------------------------



	// 纹理数组所有纹理的 DXGI 格式
	DXGI_FORMAT TextureFormat = DXGI_FORMAT_UNKNOWN;

	// 方块纹理数组默认堆资源
	// Texture2DArray m_TextureArray : register(t1, space0);
	ComPtr<ID3D12Resource> m_SRVTextureArray_DefaultResource;
	// 方块纹理数组上传堆资源，用于中转
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


	// 用于 SRV 纹理数组 和 UAV 纹理数组 的描述符堆 (SRV + UAV)
	ComPtr<ID3D12DescriptorHeap> m_SRVUAVHeap;
	// 纹理数组的 SRV 描述符 CPU 句柄
	D3D12_CPU_DESCRIPTOR_HANDLE SRVTextureArray_CPUHandle;
	// 纹理数组的 SRV 描述符 GPU 句柄
	D3D12_GPU_DESCRIPTOR_HANDLE SRVTextureArray_GPUHandle;



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

		{0, 0, 0, 0, 0, 0},		// 0.泥土
		{1, 1, 1, 1, 2, 0},		// 1.草方块
		{3, 3, 3, 3, 3, 3},		// 2.石头
		{4, 4, 4, 4, 4, 4}		// 3.基岩
	};


	// SRV Structured Buffer 的上传堆资源
	ComPtr<ID3D12Resource> m_SRVStructuredBuffer_UploadResource;
	// SRV Structured Buffer 的默认堆资源
	// StructuredBuffer<CUBEFACE> BlockCubeTexture_IndexGroup : register(t0, space0);
	ComPtr<ID3D12Resource> m_SRVStructuredBuffer_DefaultResource;



	// ---------------------------------------------------------------------------------------------------------------



	// 最大区块加载半径是 4 (包括玩家所处区块本身)，进入加载半径的新区块就加载，加载范围 (5+5-1)^2 = 81
	const int MaxCreateRadius = 5;
	// 最大区块保持半径是 5 (包括玩家所处区块本身)，脱离保持半径的区块将被卸载，保持范围 (6+6-1)^2 = 121
	const int MaxUnloadRadius = 6;
	// 16x16 区块边长
	const int ChunkSideLength = 16;
	// 区块的最大高度，由于性能原因我们暂时设置成 24，后面我们会逐步扩充到 1.16 之前的 256 高度
	const int MaxHeightHeight = 24;
	// 一个区块在实例缓冲的最大实例数量，每个区块至少要占 16x16x24 的空间
	const UINT MaxChunkInstanceCount = ChunkSideLength * ChunkSideLength * MaxHeightHeight;
	// UAV 纹理数组和 SRV 待加载列表一次创建的最大区块数量 (9x9=81，最大加载范围)
	const UINT MaxCreateChunkCount = (MaxCreateRadius * 2 - 1) * (MaxCreateRadius * 2 - 1);
	// 实例缓冲所能容纳的最大已加载区块数量 (11x11=121，最大保持范围)
	const UINT MaxKeepChunkCount = (MaxUnloadRadius * 2 - 1) * (MaxUnloadRadius * 2 - 1);



	const UINT TerrianGridWidth = 16;		// 每个区块高度图的宽度 (单位：像素 -> 方块)
	const UINT TerrianGridHeight = 16;		// 每个区块高度图的高度 (单位：像素 -> 方块)


	// 确定噪声随机生成的世界种子，相同的种子，无论在何时何地生成，都会得到完全相同的地形
	// 这个就叫噪声一个特点: "哈希性"，它本质上是一个伪随机数产生器，内部由哈希函数实现
	const UINT WorldSeed = 626830893;


	D3D12_HEAP_PROPERTIES ReadbackHeapDesc = { D3D12_HEAP_TYPE_READBACK };	// 回读堆属性结构体



	// SRV 待生成区块列表上传堆资源，用于将新加载的区块左上角坐标信息
	// SRV 结构化缓冲可以用于上传堆资源，上传堆具有 Write-Combine 写入组合的 CPU 内存属性
	// 当你通过 Map 获取的指针向这片内存写入数据时，你的写入操作会直接被 CPU 的写入组合缓冲区合并，
	// 并通过内存总线高效地传输给 GPU，只要你的写入操作在 GPU 执行命令之前已经完成，即使不用 Unmap，
	// 那么 GPU 也能正确地读取到最新的数据，Unmap 只是解除 CPU 地址映射，释放内核资源，通知驱动做优化
	// StructuredBuffer<int2> m_ReadyCreateChunkBuffer : register(t0, space0);
	ComPtr<ID3D12Resource> m_SRVReadyCreateChunkBuffer_UploadResource;

	BYTE* ReadyCreateChunkPointer = nullptr;		// 指向待生成区块列表上传堆资源的指针


	// UAV 纹理数组默认堆资源，用于 GPU 在计算着色器上计算每个区块的高度图，每个纹理元素表示一个区块的高度图
	// RWTexture2DArray<float> m_HeightTextureArray : register(u0, space0);
	ComPtr<ID3D12Resource> m_UAVHeightTextureArray_DefaultResource;

	// UAV 纹理数组回读堆资源，用于 CPU 回读每个区块的高度图，并加载每个新区块的实例数据
	ComPtr<ID3D12Resource> m_UAVHeightTextureArray_ReadbackResource;



	// UAV 纹理数组单个元素每行对齐大小 (单位: 字节，需要 256 字节对齐)
	UINT UAVMapBytePerRowSize = 0;
	// UAV 纹理数组每个元素所需要的真实大小 (单位: 字节，除末行外需要 256 字节对齐)
	UINT UAVMapSubResourceSize = 0;
	// UAV 纹理数组单个元素占整个资源的对齐大小 (单位: 字节，需要 512 字节对齐)
	UINT UAVMapsElementSize = 0;
	// UAV 纹理数组在回读堆中最终所需要的总大小 (单位: 字节，除末行外需要 512 字节对齐)
	UINT UAVReadbackResourceSize = 0;

	UINT SRVUAVDescriptorSize = 0;			// SRV/UAV 这类描述符的大小

	// UAV 区块高度纹理数组的 CPU 句柄
	D3D12_CPU_DESCRIPTOR_HANDLE UAVHeightTextureArray_CPUHandle;
	// UAV 区块高度纹理数组的 GPU 句柄
	D3D12_GPU_DESCRIPTOR_HANDLE UAVHeightTextureArray_GPUHandle;



	// ---------------------------------------------------------------------------------------------------------------



	// NoiseShader 使用的根签名
	ComPtr<ID3D12RootSignature> m_NoiseRootSignature;
	// NoiseShader 使用的 PSO
	ComPtr<ID3D12PipelineState> m_NoisePSO;


	// 将 UAV 纹理数组 从 UNORDERED_ACCESS -> COPY_SOURCE 的资源屏障
	D3D12_RESOURCE_BARRIER UAVToCopySource_barrier = {};
	// 将 UAV 纹理数组 从 COPY_SOURCE -> UNORDERED_ACCESS 的资源屏障
	D3D12_RESOURCE_BARRIER CopySourceToUAV_barrier = {};



	// ---------------------------------------------------------------------------------------------------------------



	// 专门用于渲染方块的根签名
	ComPtr<ID3D12RootSignature> m_RenderRootSignature;
	// 专门用于渲染方块的渲染管线状态
	ComPtr<ID3D12PipelineState> m_RenderPSO;


	// 用于渲染方块的常量缓冲资源
	// cbuffer GlobalData : register(b0, space0)
	ComPtr<ID3D12Resource> m_CBVRenderBlock_UploadResource;

	// 常量缓冲结构体
	struct RenderCBuffer
	{
		// MVP 矩阵，用于将顶点数据从顶点空间变换到齐次裁剪空间
		XMFLOAT4X4 MVPMatrix;
	};

	// 常量缓冲结构体指针，下文 Map 后指针会指向 RenderCBVResource 的地址
	RenderCBuffer* RenderCBufferPointer = nullptr;

	Camera m_FirstCamera;		// 第一人称摄像机
	


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
	};



	// 上传堆顶点资源
	ComPtr<ID3D12Resource> m_BlockVertexResource;
	// 上传堆索引资源
	ComPtr<ID3D12Resource> m_BlockIndexResource;
	// 上传堆实例资源
	ComPtr<ID3D12Resource> m_BlockInstanceResource;


	// 指向 m_BlockInstanceResource 所有方块实例资源的指针，注意指针类型是 BLOCK_INSTANCE！
	BLOCK_INSTANCE* BlockInstanceMapPointer = nullptr;



	// ---------------------------------------------------------------------------------------------------------------



	// 区块状态枚举
	enum CHUNK_STATE
	{
		NONE,		// 无状态，表示该区块尚未使用的初始状态
		CREATE,		// 加载状态，代表该区块进入了玩家的可见范围，需要加载到缓冲中
		KEEP,		// 保持状态，代表该区块已加载，且仍在玩家的范围内，不进行任何操作
		UNLOAD		// 卸载状态，代表该区块已经远离了玩家一定范围，暂时不需要了，卸载节省缓冲资源
	};


	// 16x16x24 的区块信息结构体，存储区块 实例偏移、实例量、AABB 包围盒、区块状态 这些元信息
	struct CHUNK
	{
		XMINT2 ChunkXZ = {};		// 整个区块左上角的 xz 轴坐标
		UINT InstanceOffset = 0;	// 整个区块在实例缓冲的偏移量 (单位：方块实例)
		UINT InstanceCount = 0;		// 整个区块的实例数量 (单位：方块实例)
		CHUNK_STATE State = NONE;	// 整个区块的状态
	};


	// 用于放置区块实例元信息的 HiveBuffer 缓冲，它不是连续缓冲，里面有数据空位 (最大 121 个区块元素)
	std::vector<CHUNK> ChunkMetaDataHiveBuffer;


	// 用于区块遍历，快速查找，状态比较，存储所有已加载区块索引的哈希表
	// Key 是区块左上角坐标 ChunkXZ，value 是区块元素在 ChunkMetaDataBuffer 的索引
	// 删除元素后 unordered_map 的其他元素 (迭代器) 稳定，这一点不必担心
	std::unordered_map<UINT64, UINT> ChunkSearchMap;


	// 管理实例缓冲是否有 能容纳区块的空位 (空泡) 的空闲栈，这个空位是区块在实例缓冲的偏移量
	// 空闲栈借鉴了 C++26 即将到来的 std::hive (蜂巢) 中 "空泡" 和 "空闲链表" 的思想
	// std::hive 是一种比 vector 综合性能更强的无序数据结构，非常适合存储游戏实体数据
	// 大部分游戏引擎的 ECS 系统 (Entity-Component-System 实体组件系统) 都是基于它的思想实现的
	// 我们后续的章节也会涉及到简单 ECS 系统的实现，ECS 系统对我们的学习是很重要的
	// std::vector 是动态连续数组，如果有元素被 erase 移除，后面的元素也要移到前面
	// 这种操作常常使对象迭代器失效，所以不能在循环内移除元素，而且如果频繁有旧元素移除，新元素添加，
	// 久而久之，就会产生很大的元素移动开销，而且元素 (迭代器) 不稳定，对数据复用很不方便
	// 在 std::hive 中，元素被移除后，会原地产生一个 "数据空泡"，后面的元素不会进行移动
	// 新数据想写入 std::hive，直接找最近的空泡就行，非常方便，还能提高数据复用率
	// 因为 "空泡" 的存在，在 std::hive 里，插入元素、删除元素只做一次，非常高效
	// 而且还能循环插入/删除，元素 (迭代器) 不会移动，插入新元素后，其他元素不会有影响，非常稳定
	// 那如何找到最近的 "空泡" 呢？这个数据结构使用了 "空闲链表" 来存储这些空泡，查找 "空泡" 也只做一次
	// 我们这里用栈来模拟 "空闲链表"，栈顶就是距离最近的 "空泡" (数据空位)
	std::stack<UINT> FreeChunkSpaceStack;


	// 待加载区块连续缓冲 (最大 81 个区块元素)
	std::vector<CHUNK> ReadyCreateChunkBuffer;



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

		// end_barrier 终止屏障：Render Target 渲染目标状态 -> Present 呈现状态
		end_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		end_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		end_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
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



	// 使用 D2D 引擎的 WIC 功能加载并转换纹理图片
	void STEP11_LoadImageAndTransform()
	{
		m_D2DEngine.D2D_STEP01_InitializeWICFactory();
		m_D2DEngine.D2D_STEP02_LoadTextureIntoWICBitmaps(TextureNames, m_TextureGroup);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 获取纹理数组的各种属性，以第一个元素为准，后面的元素这些属性是一样的 (作者检查过了)
	void STEP12_GetTextureArrayElementsProperties()
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
	void STEP13_CreateTextureArrayResource()
	{
		// 用于中转纹理的上传堆资源结构体
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		UploadResourceDesc.Width = UploadResourceSize;
		UploadResourceDesc.Height = 1;
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		UploadResourceDesc.DepthOrArraySize = 1;
		UploadResourceDesc.MipLevels = 1;
		UploadResourceDesc.SampleDesc.Count = 1;


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
	void STEP14_CopyTextureArrayToDefaultResource()
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


		// 将围栏预定值设定为下一帧的任务完成值，说明 GPU 完成了一项任务
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		// 命令队列执行到这里会修改围栏值，表示渲染已完成，"击中"围栏，同时修改围栏的 Completed Value 任务完成值
		// 这里传入 FenceValue 是因为 CommandQueue 要用这个值标记预定事件，关联围栏
		m_CommandQueue->Signal(m_RenderFence.Get(), FenceValue);
		// 设置围栏的预定事件，当渲染完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		// 这里传入 FenceValue 是因为围栏要拿这个值开辟对应的 Event Slot 事件槽，并将 CPU 端事件句柄绑定到事件槽上
		m_RenderFence->SetEventOnCompletion(FenceValue, RenderEvent);


		// 让主线程强制等待复制完成，经过此函数后 RenderEvent 会自动重置到无信号状态 (CreateEvent 第二个参数)
		WaitForSingleObject(RenderEvent, INFINITE);
	}


	// 创建 SRV + UAV 两个描述符的着色器资源描述符堆
	void STEP15_CreateSRVUAVHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};					// SRV/UAV 描述符堆信息结构体
		HeapDesc.NumDescriptors = 2;								// SRV + 新的 UAV
		HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// 类型是 CBV/SRV/UAV 描述符都可以放
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// 描述符堆着色器可见

		// 创建 SRV/UAV 描述符堆
		m_D3D12Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&m_SRVUAVHeap));
	}


	// 用上文创建的 m_SRVTextureArray_DefaultResource 创建 SRV 纹理数组描述符
	void STEP16_CreateTextureArraySRV()
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

		// 获取 SRV 描述符的 CPU 句柄
		SRVTextureArray_CPUHandle = m_SRVUAVHeap->GetCPUDescriptorHandleForHeapStart();
		// 获取 SRV 描述符的 GPU 句柄
		SRVTextureArray_GPUHandle = m_SRVUAVHeap->GetGPUDescriptorHandleForHeapStart();

		// 创建 SRV 描述符
		m_D3D12Device->CreateShaderResourceView(m_SRVTextureArray_DefaultResource.Get(), &SRVTextureArrayDesc, SRVTextureArray_CPUHandle);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建 SRV Structured Buffer (结构化缓冲区)
	// 我们这里要传递立方体面纹理索引数据 (静态资源)，所以用 SRV Structured Buffer
	void STEP17_CreateStructuredBufferResource()
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
	void STEP18_CopyStructuredBufferToDefaultResource()
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


		// 下一个等待就是 RenderLoop 的 MsgWaitForMultipleObjects，不需要用 WaitForSingleObject 了
		// 这里再用一次 WaitForSingleObject 就会使事件变成无信号 (CreateEvent 第二个参数)
		// 导致在 MsgWaitForMultipleObjects 那里卡死，永远返回 1，窗口白屏，完全进不去 case 0 渲染函数
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建 NoiseShader 通过柏林噪声算法生成区块所需要的各种资源
	void STEP19_CreateNoiseRequireResource()
	{
		// 创建 m_SRVReadyCreateChunkBuffer_UploadResource，并进行持续化映射
		{
			// 上传堆资源结构体信息，注意资源宽度!
			D3D12_RESOURCE_DESC UploadResourceDesc = {};
			UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			UploadResourceDesc.Width = MaxCreateChunkCount * sizeof(XMINT2);
			UploadResourceDesc.Height = 1;
			UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			UploadResourceDesc.DepthOrArraySize = 1;
			UploadResourceDesc.MipLevels = 1;
			UploadResourceDesc.SampleDesc.Count = 1;


			// 创建区块待生成列表上传堆资源，注意状态！
			m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE,
				&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&m_SRVReadyCreateChunkBuffer_UploadResource));


			// 进行持续化映射，通过指针进行写入操作，在 GPU 进行读取之前都能完成
			m_SRVReadyCreateChunkBuffer_UploadResource->Map(0, nullptr,
				reinterpret_cast<void**>(&ReadyCreateChunkPointer));
		}


		// 创建 m_UAVHeightTextureArray_DefaultResource
		{
			// UAV 纹理数组默认堆资源信息，创建原理和 SRV 纹理数组 相同
			// 注意！要带上相应的 UAV Flag！
			D3D12_RESOURCE_DESC DefaultResourceDesc = {};
			DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			DefaultResourceDesc.Width = TerrianGridWidth;
			DefaultResourceDesc.Height = TerrianGridHeight;
			DefaultResourceDesc.Format = DXGI_FORMAT_R32_FLOAT;
			DefaultResourceDesc.DepthOrArraySize = MaxCreateChunkCount;
			DefaultResourceDesc.MipLevels = 1;
			DefaultResourceDesc.SampleDesc.Count = 1;
			DefaultResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;


			// 创建 UAV 纹理数组默认堆资源，注意资源状态！
			m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE,
				&DefaultResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
				IID_PPV_ARGS(&m_UAVHeightTextureArray_DefaultResource));
		}


		// 创建 m_UAVHeightTextureArray_ReadbackResource，需要计算对齐大小！
		{
			// UAV 纹理数组回读堆和 SRV 上传堆一样要计算对齐大小！
			UAVMapBytePerRowSize = Ceil(TerrianGridWidth * sizeof(float), 256) * 256;
			UAVMapSubResourceSize = UAVMapBytePerRowSize * (TerrianGridHeight - 1) + TerrianGridWidth * 4;

			UAVMapsElementSize = Ceil(UAVMapSubResourceSize, 512) * 512;
			UAVReadbackResourceSize = UAVMapsElementSize * (MaxCreateChunkCount - 1) + UAVMapSubResourceSize;


			// UAV 纹理数组回读堆信息
			D3D12_RESOURCE_DESC ReadbackResourceDesc = {};
			ReadbackResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			ReadbackResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			ReadbackResourceDesc.Width = UAVReadbackResourceSize;
			ReadbackResourceDesc.Height = 1;
			ReadbackResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			ReadbackResourceDesc.DepthOrArraySize = 1;
			ReadbackResourceDesc.MipLevels = 1;
			ReadbackResourceDesc.SampleDesc.Count = 1;


			// 创建 UAV 纹理数组回读堆资源，注意资源状态！
			m_D3D12Device->CreateCommittedResource(&ReadbackHeapDesc, D3D12_HEAP_FLAG_NONE,
				&ReadbackResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
				IID_PPV_ARGS(&m_UAVHeightTextureArray_ReadbackResource));
		}
	}


	// 在 STEP15_CreateSRVUAVHeap 的基础上创建 UAV 描述符，描述这个 UAV 高度图资源是 UAV Texture2DArray
	// 注意：用于纹理的 UAV 资源也不能做根描述符使用！必须使用根描述表！
	void STEP20_CreateHeightTextureArrayUAV()
	{
		// UAV 描述符信息结构体
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		// 类型是 Texture2DArray
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		// 格式必须和资源一样，R32_FLOAT
		UAVDesc.Format = DXGI_FORMAT_R32_FLOAT;
		// 首元素索引是 0
		UAVDesc.Texture2DArray.FirstArraySlice = 0;
		// 纹理数组的长度
		UAVDesc.Texture2DArray.ArraySize = MaxCreateChunkCount;
		// 不使用 Mipmap，填 0，注意这里和 SRV 纹理数组不一样！
		UAVDesc.Texture2DArray.MipSlice = 0;


		// 获取 SRV/UAV 这类描述符的大小
		SRVUAVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// UAV 是 SRV 的下一个描述堆元素 (第二个描述符)，在获取首元素 (SRVHandle) 的基础上进行偏移
		UAVHeightTextureArray_CPUHandle = SRVTextureArray_CPUHandle;
		UAVHeightTextureArray_CPUHandle.ptr += SRVUAVDescriptorSize;
		UAVHeightTextureArray_GPUHandle = SRVTextureArray_GPUHandle;
		UAVHeightTextureArray_GPUHandle.ptr += SRVUAVDescriptorSize;


		// 创建 HeightTextureArray 的 UAV 描述符
		// 第二个参数是计数器资源，我们不需要它，填 nullptr 就行
		m_D3D12Device->CreateUnorderedAccessView(m_UAVHeightTextureArray_DefaultResource.Get(),
			nullptr, &UAVDesc, UAVHeightTextureArray_CPUHandle);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建专门用于 NoiseShader 的根签名
	void STEP21_CreateNoiseRootSignature()
	{
		// NoiseRootSignature 的根参数 + 静态采样器列表
		// Para 0: (Type = Root Constants,	 1 DWORD)  (b0, space0) CBV 根常量，用于常量缓冲 (世界种子)
		// Para 1: (Type = Root Descriptor,	 2 DWORD)  (t0, space0) SRV 根描述符，用于区块待生成列表
		// Para 2: (Type = Descriptor Table, 1 DWORD)  (u0, space0) UAV 根描述表，用于高度图纹理数组
		// 
		// None Static Sampler	没有静态采样器，计算着色器现在不需要用到

		ComPtr<ID3DBlob> SignatureBlob;			// 根签名字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码

		D3D12_ROOT_PARAMETER RootParameters[3] = {};		// 根参数数组


		// 第一个根参数：32 位根常量 (WorldSeed)
		RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		RootParameters[0].Constants.Num32BitValues = 1;		// 只有一个根常量
		RootParameters[0].Constants.ShaderRegister = 0;		// b0
		RootParameters[0].Constants.RegisterSpace = 0;		// space0
		RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


		// 第二个根参数：SRV 根描述符 (m_ReadyUploadChunkBuffer)
		RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		RootParameters[1].Descriptor.ShaderRegister = 0;	// t0
		RootParameters[1].Descriptor.RegisterSpace = 0;		// space0
		RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


		// 第三个根参数：描述符表 (1 UAV，m_HeightTextureArray)
		D3D12_DESCRIPTOR_RANGE UAVRangeDesc = {};			// UAV 描述符在 Table 的范围
		UAVRangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		UAVRangeDesc.NumDescriptors = 1;					// Range 只有一个描述符
		UAVRangeDesc.BaseShaderRegister = 0;				// u0
		UAVRangeDesc.RegisterSpace = 0;						// space0
		UAVRangeDesc.OffsetInDescriptorsFromTableStart = 0;

		RootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		RootParameters[2].DescriptorTable.pDescriptorRanges = &UAVRangeDesc;
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
			IID_PPV_ARGS(&m_NoiseRootSignature));
	}


	// 创建专门用于 NoiseShader 的 PSO
	void STEP22_CreateNoisePSO()
	{
		// 计算着色器使用的 PSO 信息结构体，注意类型是 D3D12_COMPUTE_PIPELINE_STATE_DESC
		D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePSODesc = {};

		// 第一次绑定根签名，使用的是上面的 m_ComputeRootSignature
		// 本次设置是将根签名与 PSO 绑定，生成对应版本的根签名适配 PSO，设置渲染管线的输入参数状态
		ComputePSODesc.pRootSignature = m_NoiseRootSignature.Get();


		ComPtr<ID3DBlob> ComputeShaderBlob;		// 计算着色器二进制字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码

		// 编译 NoiseShader 的 NoiseCSMain
		D3DCompileFromFile(L"NoiseShader.hlsl", nullptr, nullptr, "NoiseCSMain", "cs_5_1", NULL, NULL, &ComputeShaderBlob, &ErrorBlob);
		if (ErrorBlob)
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		ComputePSODesc.CS.BytecodeLength = ComputeShaderBlob->GetBufferSize();		// CS 字节码数据长度
		ComputePSODesc.CS.pShaderBytecode = ComputeShaderBlob->GetBufferPointer();	// CS 字节码数据指针

		// 不使用特殊标志
		ComputePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		// 创建用于地形生成的 NoisePSO
		m_D3D12Device->CreateComputePipelineState(&ComputePSODesc, IID_PPV_ARGS(&m_NoisePSO));
	}


	// 创建用于 UAV 纹理数组 回读的两个资源屏障，这两个资源屏障用于回读高度图的资源转换
	void STEP23_CreateHeightMapBarrier()
	{
		// UAVToCopySource_barrier 屏障：Unordered Access 无序访问状态 -> Copy Source 复制源状态
		// 描述符只描述资源的用途与布局，CPU 回读 UAV 纹理数组 的数据，说明 UAV 资源用途发生了改变，需要进行屏障转换
		// 如果没有下面这两个资源屏障的话，默认堆 -> 回读堆 的复制操作会失败，或者发生数据错误，调试层报 D3D12 ERROR
		UAVToCopySource_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		UAVToCopySource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		UAVToCopySource_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		UAVToCopySource_barrier.Transition.pResource = m_UAVHeightTextureArray_DefaultResource.Get();
		// 注意这里！我们设置的是纹理数组，这个 Subresource 指的是要设置屏障的子资源索引！
		// 我们所有纹理子资源都要进行屏障转换，直接用 DX12 给的 ALL_SUBRESOURCES 宏定义
		UAVToCopySource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;


		// CopySourceToUAV_barrier 屏障：Copy Source 复制源状态 -> Unordered Access 无序访问状态
		// 当复制完成后，转换到无序访问状态，供下一次生成区块的计算着色器使用，如此循环往复
		CopySourceToUAV_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		CopySourceToUAV_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		CopySourceToUAV_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		CopySourceToUAV_barrier.Transition.pResource = m_UAVHeightTextureArray_DefaultResource.Get();
		// 所有纹理子资源都要进行屏障转换
		CopySourceToUAV_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建专门用于渲染方块的根签名
	void STEP24_CreateRenderRootSignature()
	{
		// RenderRootSignature 的根参数 + 静态采样器列表
		// Para 0: (Type = Root Descriptor,  2 DWORD)  (b0, space0) CBV 根描述符，用于常量缓冲
		// Para 1: (Type = Root Descriptor,  2 DWORD)  (t0, space0) SRV 根描述符，用于结构化缓冲区
		// Para 2: (Type = Descriptor Table, 1 DWORD)  (t1, space0) SRV 描述符表，用于纹理数组
		// 
		// Sampler 0: (Type = Static Sampler) (s0, space0) 静态采样器 (邻近点过滤)，用于纹理数组采样

		ComPtr<ID3DBlob> SignatureBlob;			// 根签名字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码

		D3D12_ROOT_PARAMETER RootParameters[3] = {};		// 根参数数组


		// 第一个根参数：CBV 根描述符 (常量缓冲)，根描述符是内联描述符
		// 所以下文绑定根参数时，只需要传递常量缓冲资源的地址即可
		RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		RootParameters[0].Descriptor.ShaderRegister = 0;	// b0
		RootParameters[0].Descriptor.RegisterSpace = 0;		// space0
		RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


		// 第二个根参数：SRV 根描述符 (结构化缓冲区)，注意！SRV 根描述符不能用于纹理！
		RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		RootParameters[1].Descriptor.ShaderRegister = 0;	// t0
		RootParameters[1].Descriptor.RegisterSpace = 0;		// space0
		RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


		// 第三个根参数：SRV 根描述表 (纹理数组)
		D3D12_DESCRIPTOR_RANGE SRVRangeDesc = {};	// SRV 描述符在 Table 的范围
		SRVRangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		SRVRangeDesc.NumDescriptors = 1;					// Range 只有一个 SRV 描述符
		SRVRangeDesc.BaseShaderRegister = 1;				// t1
		SRVRangeDesc.RegisterSpace = 0;						// space0
		SRVRangeDesc.OffsetInDescriptorsFromTableStart = 0;

		RootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParameters[2].DescriptorTable.pDescriptorRanges = &SRVRangeDesc;
		RootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
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


	// 创建专门用于渲染方块的 PSO
	void STEP25_CreateRenderBlockPSO()
	{
		// PSO 信息结构体
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

		// Input Assembler 输入装配阶段
		D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};			// 输入样式信息结构体
		D3D12_INPUT_ELEMENT_DESC InputElementDesc[5] = {};		// 输入元素信息结构体数组

		// Input Slot 0: Vertex Stream 顶点流，逐顶点输入

		// 顶点位置 float4 Position
		InputElementDesc[0].SemanticName = "POSITION";										// 要锚定的语义
		InputElementDesc[0].SemanticIndex = 0;												// 语义索引	
		InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;						// 输入格式
		InputElementDesc[0].InputSlot = 0;													// 输入槽编号	
		InputElementDesc[0].AlignedByteOffset = 0;											// 在输入槽中的偏移
		InputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;	// 输入流类型
		InputElementDesc[0].InstanceDataStepRate = 0;										// 实例数据步进率	


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



		InputLayoutDesc.NumElements = 5;						// 输入元素个数
		InputLayoutDesc.pInputElementDescs = InputElementDesc;	// 输入元素结构体数组指针
		PSODesc.InputLayout = InputLayoutDesc;					// 设置渲染管线 IA 阶段的输入样式



		ComPtr<ID3DBlob> VertexShaderBlob;		// 顶点着色器二进制字节码
		ComPtr<ID3DBlob> PixelShaderBlob;		// 像素着色器二进制字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码

		// 编译顶点着色器 Vertex Shader
		D3DCompileFromFile(L"RenderShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", NULL, NULL, &VertexShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// 编译像素着色器 Pixel Shader
		D3DCompileFromFile(L"RenderShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", NULL, NULL, &PixelShaderBlob, &ErrorBlob);
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
		m_D3D12Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_RenderPSO));
	}


	// 创建专门用于 RenderShader 的常量缓冲资源
	void STEP26_CreateRenderCBVResource()
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



	// ---------------------------------------------------------------------------------------------------------------



	// 创建顶点流的顶点缓冲和索引缓冲，用的是 VBV0 和 IBV
	void STEP27_CreatePerVertexAndIndexBuffer()
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


	// 创建实例流缓冲资源，最大能容纳 121 个 16x16x24 的区块，用的是 VBV1
	void STEP28_CreatePerInstanceBuffer()
	{
		// 上传堆实例资源结构体，注意资源大小！
		D3D12_RESOURCE_DESC InstanceResourceDesc = {};
		InstanceResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		InstanceResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		InstanceResourceDesc.Width = MaxKeepChunkCount * MaxChunkInstanceCount * sizeof(BLOCK_INSTANCE);
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
		VertexBufferView[1].SizeInBytes = MaxKeepChunkCount * MaxChunkInstanceCount * sizeof(BLOCK_INSTANCE);

		// 进行实例化资源的持续化映射
		m_BlockInstanceResource->Map(0, nullptr, reinterpret_cast<void**>(&BlockInstanceMapPointer));
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 以区块左上角 xz 坐标拼接变成的整数作为哈希表键
	inline UINT64 ChunkInMapKey(int ChunkX, int ChunkZ)
	{
		return ((UINT64)ChunkX << 32) | (UINT32)ChunkZ;
	}


	// 初始化区块相关联的容器 (数据结构)
	void STEP29_InitChunkContainer()
	{
		// 做占位用的空区块结构体
		CHUNK EmptyChuckSpace = {};
		// 区块缓冲先推入 121 个空泡，之后大小就固定了，不会再变了
		ChunkMetaDataHiveBuffer.resize(MaxKeepChunkCount, EmptyChuckSpace);


		// 最开始区块实例缓冲没有实例，全是空位，循环从大到小推入 "空泡" 索引到空闲栈中
		// 栈是先入先出的，栈顶 "空泡" 是缓冲第一个元素的索引 0
		for (int i = MaxKeepChunkCount - 1; i >= 0; i--)
		{
			FreeChunkSpaceStack.push(i);
		}
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 更新每帧数据，包括每帧的 MVP 矩阵数据，以及区块加载信息
	void UpdateFrameData()
	{
		// 将更新后的矩阵，存储到共享内存上的常量缓冲，这样 GPU 就可以访问到 MVP 矩阵了
		XMStoreFloat4x4(&RenderCBufferPointer->MVPMatrix, m_FirstCamera.GetMVPMatrix());



		// 获取玩家当前位置
		XMVECTOR CameraPosition = m_FirstCamera.GetEyePosition();

		// 获取玩家所处区块左上角 xz 坐标，一个区块 16x16
		int CurrentChunkX = floor(XMVectorGetX(CameraPosition) / ChunkSideLength) * ChunkSideLength;
		int CurrentChunkZ = floor(XMVectorGetZ(CameraPosition) / ChunkSideLength) * ChunkSideLength;



		// 遍历哈希表，先检查所有已加载区块，如果有区块已经脱离了保持范围，就进行卸载
		{
			// 临时存储哈希表中要卸载的区块键的 vector
			std::vector<UINT64> _temp_UnloadChunkHashKeyList;


			// 遍历哈希表，如果有区块脱离了最大保持范围，先记录需要卸载的键
			// 因为在任何容器进行自身对象的范围 for 遍历，同时还要删除自身迭代器，是相当危险的
			// 下面的范围 for 支持 [key, value] 这种键值对元素的简洁写法
			for (auto& [key, index] : ChunkSearchMap)
			{
				// 当前遍历区块左上角的 xz 坐标
				int SearchMapChunkX = ChunkMetaDataHiveBuffer[index].ChunkXZ.x;
				int SearchMapChunkZ = ChunkMetaDataHiveBuffer[index].ChunkXZ.y;

				// 原版游戏计算区块距离，采用的就是 Chebyshev Distance 切比雪夫距离
				// 它计算的是玩家与区块平面 xz 轴距离的绝对值，结果是两者中最大的绝对值: max(|dx|, |dz|)
				// 切比雪夫的优点是计算量极少，完美与平面正方形网格 (区块) 对齐
				int distance = std::max(std::abs(SearchMapChunkX - CurrentChunkX), std::abs(SearchMapChunkZ - CurrentChunkZ));


				// 如果某个区块在保持范围之外，暂时不需要加载它了，给这个区块加入卸载列表
				// 注意！最大保持半径 (5) 要乘以区块边长！因为上面相减得到的距离是以方块边长为粒度的！
				if (distance > (MaxUnloadRadius - 1) * ChunkSideLength)
				{
					// 加入卸载标志
					ChunkMetaDataHiveBuffer[index].State = UNLOAD;
					// 需要卸载的区块实例偏移清零
					ChunkMetaDataHiveBuffer[index].InstanceOffset = 0;
					// 需要卸载的区块实例数量清零
					ChunkMetaDataHiveBuffer[index].InstanceCount = 0;


					// 将哈希键加入到卸载列表
					_temp_UnloadChunkHashKeyList.push_back(key);
					// 区块元素被删除，原地变成 "空泡"，空闲栈推入新 "空泡" 索引，方便数据复用
					FreeChunkSpaceStack.push(index);
				}
			}


			// 哈希表根据卸载列表，逐一删除键值对
			for (const auto& key : _temp_UnloadChunkHashKeyList)
			{
				ChunkSearchMap.erase(key);
			}
		}



		// 以玩家自身所在的区块为中心，计算所需要加载的区块，如果在哈希表上找到了已存在的区块，就保持加载
		// 哈希表找不到，说明是新区块，加入待加载列表中，等会在 Render 进行真正的加载
		{
			// 遍历玩家周围所有的可加载区块 (9x9=81 正方体范围)
			// x/z 指的是相对玩家自身区块的 x/z 轴偏移索引，-4 <= ChunkIndex <= 4
			for (int x = 1 - MaxCreateRadius; x <= MaxCreateRadius - 1; x++)
			{
				for (int z = 1 - MaxCreateRadius; z <= MaxCreateRadius - 1; z++)
				{
					// 获取该区块左上角坐标
					int ChunkX = CurrentChunkX + x * ChunkSideLength;
					int ChunkZ = CurrentChunkZ + z * ChunkSideLength;

					// 获取区块的哈希键
					UINT64 HashKey = ChunkInMapKey(ChunkX, ChunkZ);

					// 如果在哈希表上没找到区块，说明是需要加载的新区块
					if (ChunkSearchMap.find(HashKey) == ChunkSearchMap.end())
					{
						// 新区块会占用一个空间，从空闲栈获取一个最近的"空泡"
						UINT NewChunkIndex = FreeChunkSpaceStack.top();
						// 新区块使用了"空泡"，栈顶弹出元素
						FreeChunkSpaceStack.pop();


						CHUNK NewChunk = {};		// 新区块信息结构体
						// 新区块处于待加载状态
						NewChunk.State = CREATE;
						// 新区块左上角坐标
						NewChunk.ChunkXZ = { ChunkX, ChunkZ };
						// 指定新区块新的实例偏移
						NewChunk.InstanceOffset = NewChunkIndex * MaxChunkInstanceCount;
						// 新区块还没加载，实例数量暂时先清零
						NewChunk.InstanceCount = 0;


						// 区块实例缓冲添加新区块
						ChunkMetaDataHiveBuffer[NewChunkIndex] = NewChunk;
						// 哈希表添加新键值对
						ChunkSearchMap[HashKey] = NewChunkIndex;
						// 待加载列表添加新区块
						ReadyCreateChunkBuffer.push_back(NewChunk);
					}
				}
			}
		}
	}



	// 开启 NoiseShader 计算区块高度图，并复制到回读堆资源的渲染分支
	void RenderBranch_GenerateNewChunkHeightMap()
	{
		// 更新 SRV 待加载生成区块缓冲，注意每个元素是 XMINT2
		// StructuredBuffer<int2> m_ReadyCreateChunkBuffer : register(t0, space0);
		for (UINT i = 0; i < ReadyCreateChunkBuffer.size(); i++)
		{
			memcpy(ReadyCreateChunkPointer + i * sizeof(XMINT2),
				&ReadyCreateChunkBuffer[i].ChunkXZ, sizeof(XMINT2));
		}



		// 先重置命令分配器
		m_CommandAllocator->Reset();
		// 再重置命令列表，Close 关闭状态 -> Record 录制状态
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);



		// 用于设置描述符堆用的临时 ID3D12DescriptorHeap 数组
		ID3D12DescriptorHeap* _temp_DescriptorHeaps[] = { m_SRVUAVHeap.Get() };
		// 设置描述符堆，描述符堆表示了根描述表第二次寻址，寻找描述符需要的基地址
		m_CommandList->SetDescriptorHeaps(1, _temp_DescriptorHeaps);

		// 设置 NoiseRootSignature
		// 第二次设置根签名，本次检测 PSO 根签名的合法性 (引用资源是否匹配)，检测成功会开启显存与寄存器的映射通道
		m_CommandList->SetComputeRootSignature(m_NoiseRootSignature.Get());

		// 设置 NoisePSO
		m_CommandList->SetPipelineState(m_NoisePSO.Get());

		// 设置第一个根参数：32 位根常量 (世界种子)
		m_CommandList->SetComputeRoot32BitConstant(0, WorldSeed, 0);

		// 设置第二个根参数：SRV 根描述符 (待加载区块列表)
		m_CommandList->SetComputeRootShaderResourceView(1,
			m_SRVReadyCreateChunkBuffer_UploadResource->GetGPUVirtualAddress());

		// 设置第三个根参数：根描述表 (UAV 高度图纹理数组)
		m_CommandList->SetComputeRootDescriptorTable(2, UAVHeightTextureArray_GPUHandle);



		// 根据需要加载的区块数量，调度相应的线程，开始计算
		// 需要加载的高度图，调度的线程组 = 需要加载区块的数量，每个线程组对应 UAV 纹理数组一块高度图
		m_CommandList->Dispatch(ReadyCreateChunkBuffer.size(), 1, 1);


		// 计算完成后，将 UAV 纹理数组转换成可以用于复制的状态
		m_CommandList->ResourceBarrier(1, &UAVToCopySource_barrier);

		// 资源脚本，描述要复制的资源，UAV 纹理数组每个元素都要复制一次
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> PlacedFootprints(ReadyCreateChunkBuffer.size());

		// UAV 默认堆纹理数组资源结构体
		D3D12_RESOURCE_DESC DefaultResourceDesc = m_UAVHeightTextureArray_DefaultResource->GetDesc();

		// 获取纹理复制脚本，用于下文的纹理复制
		m_D3D12Device->GetCopyableFootprints(&DefaultResourceDesc, 0, ReadyCreateChunkBuffer.size(), 0,
			&PlacedFootprints[0], nullptr, nullptr, nullptr);

		// 复制 UAV 纹理数组到回读堆，每个子资源 (纹理数组元素) 都要调用一次 CopyTextureRegion 指令
		for (UINT i = 0; i < ReadyCreateChunkBuffer.size(); i++)
		{
			// 复制目标位置 (回读堆资源) 结构体
			D3D12_TEXTURE_COPY_LOCATION DstLocation = {};
			// 指定资源的用途是一个使用复制脚本的缓冲区
			DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			// 选择第 i 个复制脚本
			DstLocation.PlacedFootprint = PlacedFootprints[i];
			// 要复制到的目标资源 (回读堆资源)
			DstLocation.pResource = m_UAVHeightTextureArray_ReadbackResource.Get();

			// 复制源位置 (默认堆资源) 结构体
			D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};
			// 指定资源的用途是一块纹理子资源
			SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			// 要复制的第 i 个纹理子资源
			SrcLocation.SubresourceIndex = i;
			// 被复制的资源 (默认堆资源)
			SrcLocation.pResource = m_UAVHeightTextureArray_DefaultResource.Get();

			// 记录复制第 i 个子资源 (纹理数组元素) 到回读堆的命令 (显存 -> 共享内存) 
			m_CommandList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);
		}

		// 复制完成后，将 UAV 纹理数组转换回无序访问状态，准备下一次地形加载
		m_CommandList->ResourceBarrier(1, &CopySourceToUAV_barrier);


		// 关闭命令列表
		m_CommandList->Close();

		// 用于传递命令用的临时 ID3D12CommandList 数组
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// 提交命令队列，执行上文的复制命令！
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);


		// 将围栏预定值设定为下一帧
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		m_CommandQueue->Signal(m_RenderFence.Get(), FenceValue);
		// 设置围栏的预定事件，当计算完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		m_RenderFence->SetEventOnCompletion(FenceValue, RenderEvent);


		// 强制让 CPU 主线程等待 GPU 执行完成，正在被 GPU 复制的资源，CPU 不可以进行 Map-Unmap 操作
		// 必须等待复制完成才可以进行读取，否则会发生资源冲突
		// 这种写法弊处多多，后面我们学到 多线程渲染、多命令列表、多命令队列、多围栏同步 会进行一次大改
		WaitForSingleObject(RenderEvent, INFINITE);
	}



	// 将回读堆的数据转换成新区块实例数据，并加入到缓冲的渲染分支
	void RenderBranch_AppendNewChunkInstance()
	{
		// 指定回读范围的 Range，用于消除 D3D12 Warning
		D3D12_RANGE HeightMapsRange = { 0, UAVReadbackResourceSize };
		// 用于传递高度数据的指针，注意类型是 float (四字节)
		float* HeightMapPointer = nullptr;
		// 获取回读堆资源地址
		m_UAVHeightTextureArray_ReadbackResource->Map(0, &HeightMapsRange,
			reinterpret_cast<void**>(&HeightMapPointer));


		// UAV 纹理数组子资源每行的对齐大小 (单位：float)
		UINT UAVMapRowPerFloat = UAVMapBytePerRowSize / sizeof(float);
		// UAV 纹理数组每个子资源的对齐大小 (单位：float)
		UINT UAVMapElementPerFloat = UAVMapsElementSize / sizeof(float);


		// 先保存实例资源原先的起始地址，等会需要进行指针偏移，注意类型是 BLOCK_INSTANCE
		// 类型后 const 表示指针指向的内存可修改，但指针本身的地址值不可更改
		BLOCK_INSTANCE* const OriginAddress = BlockInstanceMapPointer;


		// 逐一对新区块生成高度图，并放到指定的实例缓冲，变更区块 Hive Buffer 的状态
		for (UINT MapIndex = 0; MapIndex < ReadyCreateChunkBuffer.size(); MapIndex++)
		{
			// 新区块方块实例的数量
			UINT NewChunkInstanceCount = 0;

			// 偏移到方块实例缓冲下，新区块的地址
			BlockInstanceMapPointer = OriginAddress + ReadyCreateChunkBuffer[MapIndex].InstanceOffset;

			// 区块相对世界中心的 xz 轴偏移
			int WorldOffsetX = ReadyCreateChunkBuffer[MapIndex].ChunkXZ.x;
			int WorldOffsetZ = ReadyCreateChunkBuffer[MapIndex].ChunkXZ.y;


			// 遍历 UAV 纹理高度图，逐一生成地形网格方块柱，并记录实例数量
			for (int GridZ = 0; GridZ < TerrianGridHeight; GridZ++)
			{
				for (int GridX = 0; GridX < TerrianGridWidth; GridX++)
				{
					// 获取当前网格的最大高度的原始数据 (RWTexture2D<float>)
					const float RawHeight = *HeightMapPointer;

					// 对原始数据四舍五入，就能得到我们可以用的最大高度
					const int CurrentGridMaxHeight = round(RawHeight);


					// 从 0 开始，对该网格生成方块柱
					for (int GridY = 0; GridY <= CurrentGridMaxHeight; GridY++)
					{
						BLOCK_INSTANCE NewBlock = {};					// 新方块
						NewBlock.BlockOffset.x = WorldOffsetX + GridX;	// 当前网格 x 轴坐标
						NewBlock.BlockOffset.y = GridY;					// 当前网格 y 轴坐标
						NewBlock.BlockOffset.z = WorldOffsetZ + GridZ;	// 当前网格 z 轴坐标


						// 最底层方块是基岩
						if (GridY == 0)
						{
							NewBlock.BlockType = 3;
						}
						// 包括 y = 4 以下都是石头，暂时先这样定义
						else if (GridY <= 4)
						{
							NewBlock.BlockType = 2;
						}
						// 剩下的是泥土
						else
						{
							NewBlock.BlockType = 0;
						}


						// 方块实例缓冲添加新实例
						*BlockInstanceMapPointer = NewBlock;

						// 指向缓冲的指针指向下一个空位
						BlockInstanceMapPointer++;

						// 新区块实例数量 +1
						NewChunkInstanceCount++;
					}


					// 如果最高的方块的类型是泥土，就变成草方块
					if ((BlockInstanceMapPointer - 1)->BlockType == 0)
					{
						(BlockInstanceMapPointer - 1)->BlockType = 1;
					}


					// x 轴偏移到下一个纹理元素
					HeightMapPointer++;
				}


				// z 轴偏移到下一行第一个纹理元素
				HeightMapPointer += UAVMapRowPerFloat - TerrianGridWidth;
			}


			// 通过哈希表找到当前区块在 HiveBuffer 的真实位置，并修改相关属性
			const UINT64 ChunkIndex = ChunkSearchMap[ChunkInMapKey(WorldOffsetX, WorldOffsetZ)];
			// 加载完成，将该区块设置为 KEEP 保持状态
			ChunkMetaDataHiveBuffer[ChunkIndex].State = KEEP;
			// 赋值上面两个嵌套循环计算得到的总区块数量
			ChunkMetaDataHiveBuffer[ChunkIndex].InstanceCount = NewChunkInstanceCount;


			// 整个指针偏移到下一个纹理图起始位置
			HeightMapPointer += UAVMapElementPerFloat - UAVMapRowPerFloat * TerrianGridHeight;
		}


		// 清空待加载区块
		ReadyCreateChunkBuffer.clear();

		// 注意这里，指向方块实例缓冲的指针还原起始位置！
		BlockInstanceMapPointer = OriginAddress;

		// 结束映射，和上传堆 Write-Combine 写入结合属性不同，回读堆是 Cachable 可缓存的内存属性
		// 它的设计目标是让 GPU 写入的数据能被 CPU 高效地读取，必须要 Map-Unmap，否则报 D3D12 Error
		// 在回读堆上调用 Unmap 是一个关键信号，它会通知驱动程序：CPU 即将读取这块内存
		// 驱动程序 (硬件) 会确保任何可能包含这片区域旧数据的 CPU 缓存行被标记为无效
		// 这样，当你后续再次 Map 并读取时，CPU 会强制从真正的物理内存中加载数据，从而读取到 GPU 写入的最新值
		// Unmap 用于刷新 CPU 缓存，保证读取到最新数据，在回读堆中起到了隐式的同步作用
		m_UAVHeightTextureArray_ReadbackResource->Unmap(0, nullptr);
	}



	// 向 GPU 提交绘制区块命令的渲染分支
	void RenderBranch_RenderChunk()
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



		// 再进行渲染
		{
			// 将起始转换屏障的资源指定为当前渲染目标
			beg_barrier.Transition.pResource = m_D3D12RenderTarget[FrameIndex].Get();
			// 调用资源屏障，将渲染目标由 Present 呈现(只读) 转换到 RenderTarget 渲染目标(只写)
			m_CommandList->ResourceBarrier(1, &beg_barrier);


			// 设置视口 (光栅化阶段)，用于光栅化里的屏幕映射
			m_CommandList->RSSetViewports(1, &ViewPort);
			// 设置裁剪矩形 (光栅化阶段)
			m_CommandList->RSSetScissorRects(1, &ScissorRect);


			// 用 RTV 句柄设置渲染目标，同时用 DSV 句柄设置深度模板缓冲，开启深度测试
			m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, &DSVHandle);

			// 清空后台的深度模板缓冲，将深度重置为初始值 1
			m_CommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

			// 清空当前渲染目标的背景为天蓝色
			m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);



			// 用于设置描述符堆用的临时 ID3D12DescriptorHeap 数组
			ID3D12DescriptorHeap* _temp_DescriptorHeaps[] = { m_SRVUAVHeap.Get() };
			// 设置描述符堆，描述符堆表示了根描述表第二次寻址，寻找描述符需要的基地址
			m_CommandList->SetDescriptorHeaps(1, _temp_DescriptorHeaps);

			// 设置 RenderRootSignature
			m_CommandList->SetGraphicsRootSignature(m_RenderRootSignature.Get());

			// 设置 RenderBlockPSO
			m_CommandList->SetPipelineState(m_RenderPSO.Get());

			// 设置第一个根参数：CBV 根描述符 (MVP 缓冲)
			m_CommandList->SetGraphicsRootConstantBufferView(0,
				m_CBVRenderBlock_UploadResource->GetGPUVirtualAddress());

			// 设置第二个根参数：SRV 根描述符 (方块类型-纹理索引组)
			m_CommandList->SetGraphicsRootShaderResourceView(1,
				m_SRVStructuredBuffer_DefaultResource->GetGPUVirtualAddress());

			// 设置第三个根参数：根描述表 (SRV 方块纹理数组)
			m_CommandList->SetGraphicsRootDescriptorTable(2, SRVTextureArray_GPUHandle);



			// 设置图元拓扑：三角形列表
			m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// 设置 VBV 顶点缓冲描述符数组，两个 VBV 都会被设置
			m_CommandList->IASetVertexBuffers(0, 2, VertexBufferView);

			// 设置 IBV 索引缓冲描述符
			m_CommandList->IASetIndexBuffer(&IndexBufferView);


			
			// 遍历已加载区块哈希表，对已加载区块进行多实例渲染
			// 如果区块实例数量不为 0，就渲染
			for (const auto& [key, index] : ChunkSearchMap)
			{
				if (ChunkMetaDataHiveBuffer[index].InstanceCount != 0)
				{
					m_CommandList->DrawIndexedInstanced(36, ChunkMetaDataHiveBuffer[index].InstanceCount,
						0, 0, ChunkMetaDataHiveBuffer[index].InstanceOffset);
				}
			}
			
			

			// 将终止转换屏障的资源指定为当前渲染目标
			end_barrier.Transition.pResource = m_D3D12RenderTarget[FrameIndex].Get();
			// 再通过一次资源屏障，将渲染目标由 RenderTarget 渲染目标(只写) 转换到 Present 呈现(只读)
			m_CommandList->ResourceBarrier(1, &end_barrier);
		}



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
		m_CommandQueue->Signal(m_RenderFence.Get(), FenceValue);
		// 设置围栏的预定事件，当渲染完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		m_RenderFence->SetEventOnCompletion(FenceValue, RenderEvent);
	}



	// 渲染
	void Render()
	{
		// 更新每帧都需要的数据
		UpdateFrameData();


		// 如果有待加载的新区块，就开启并调度 NoiseShader 计算高度图，之后回读高度纹理数组创建区块实例
		if (ReadyCreateChunkBuffer.size() != 0)
		{
			// 先调度 GPU 生成高度图
			RenderBranch_GenerateNewChunkHeightMap();
			// 回读高度图，生成新区块实例
			RenderBranch_AppendNewChunkInstance();
		}


		// 每帧都进行区块渲染
		RenderBranch_RenderChunk();
	}



	// 渲染循环
	void STEP30_RenderLoop()
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



	// 在原版游戏中移动鼠标就可以旋转摄像机视角，鼠标光标移动后会自动重置到窗口中央，这个叫 "鼠标捕获"
	// 检测 WM_MOUSEMOVE 是不是光标自动重置到窗口中央的时候发送的，如果是，利用这个标志忽略一次消息
	bool isAutoResetCursorMsg = true;


	// 回调函数，处理窗口产生的消息
	// WASD 键 —— 摄像机前后左右移动
	// 鼠标移动 —— 摄像机视角旋转
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
						m_FirstCamera.Walk(1);
						break;

					case 's':
					case 'S':	// 向后移动
						m_FirstCamera.Walk(-1);
						break;

					case 'a':
					case 'A':	// 向左移动
						m_FirstCamera.Strafe(-1);
						break;

					case 'd':
					case 'D':	// 向右移动
						m_FirstCamera.Strafe(1);
						break;


					case '/':	// 退出，和 Esc 一样的功能，后面会演化成原版的打指令
					{
						SendMessage(hwnd, WM_DESTROY, wParam, lParam);
					}
					break;
				}

				// 获取玩家坐标，并更新窗口标题栏
				{
					auto pos = m_FirstCamera.GetEyePosition();
					int x = XMVectorGetX(pos);
					int y = XMVectorGetY(pos);
					int z = XMVectorGetZ(pos);

					std::wstring wstr = L"Minecraft: 玩家当前坐标 (" + std::to_wstring(x) + L"," +
						std::to_wstring(y) + L"," + std::to_wstring(z) + L")";

					SetWindowText(hwnd, wstr.c_str());
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


			// 如果接收到其他消息，直接默认返回整个窗口
			default: return DefWindowProc(hwnd, msg, wParam, lParam);
		}
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


		engine.STEP11_LoadImageAndTransform();


		engine.STEP12_GetTextureArrayElementsProperties();
		engine.STEP13_CreateTextureArrayResource();
		engine.STEP14_CopyTextureArrayToDefaultResource();
		engine.STEP15_CreateSRVUAVHeap();
		engine.STEP16_CreateTextureArraySRV();


		engine.STEP17_CreateStructuredBufferResource();
		engine.STEP18_CopyStructuredBufferToDefaultResource();


		engine.STEP19_CreateNoiseRequireResource();
		engine.STEP20_CreateHeightTextureArrayUAV();


		engine.STEP21_CreateNoiseRootSignature();
		engine.STEP22_CreateNoisePSO();
		engine.STEP23_CreateHeightMapBarrier();


		engine.STEP24_CreateRenderRootSignature();
		engine.STEP25_CreateRenderBlockPSO();
		engine.STEP26_CreateRenderCBVResource();


		engine.STEP27_CreatePerVertexAndIndexBuffer();
		engine.STEP28_CreatePerInstanceBuffer();


		engine.STEP29_InitChunkContainer();


		engine.STEP30_RenderLoop();
	}
};



// 主函数
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}