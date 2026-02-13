
// (10) RenderGLTFModel: 使用 DirectX 12 + Assimp 渲染《合金装备崛起:复仇》中的 塞穆尔·罗德里格斯 gltf 模型 (该模型没有绑定骨骼)
// 鸣谢原作者大大: Mono_213 (https://sketchfab.com/Mono_213) 
// 模型项目地址: https://sketchfab.com/3d-models/metal-gear-rising-jetstream-sam-7256008fd1124ec589fdd98d4b5acf33


// windows.h 与标准库里的 min/max 函数重名导致冲突了，禁用 windows.h 里面的 min/max 函数
#define NOMINMAX
// C++ 17 开始把 std::codecvt_utf8 给取消了，直接使用会报错 (和 scanf 一样)，加这个宏可以绕过报错，继续使用
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include<Windows.h>			// Windows 窗口编程核心头文件
#include<d3d12.h>			// DX12 核心头文件
#include<dxgi1_6.h>			// DXGI 头文件，用于管理与 DX12 相关联的其他必要设备，如 DXGI 工厂和 交换链
#include<DirectXColors.h>	// DirectX 颜色库
#include<DirectXMath.h>		// DirectX 数学库
#include<d3dcompiler.h>		// DirectX Shader 着色器编译库
#include<wincodec.h>		// WIC 图像处理框架，用于解码编码转换图片文件


#include<wrl.h>				// COM 组件模板库，方便写 DX12 和 DXGI 相关的接口
#include<string>			// C++ 标准 string 库
#include<sstream>			// C++ 字符串流处理库
#include<functional>		// C++ 标准函数对象库，用于下文的 std::function 函数包装器与 std::bind 绑定回调函数
#include<fstream>			// C++ 文件流处理库
#include<vector>			// C++ STL vector 容器库
#include<codecvt>			// C++ 字符编码转换库，用于 string 转 wstring


#include<assimp/Importer.hpp>				// Assimp Importer 模型导入器，用于导入模型,读取模型数据
#include<assimp/postprocess.h>				// PostProcess 后处理，提供多种标志 (aiProcess_xxx)，用于改善模型的导入质量与性能
#include<assimp/scene.h>					// Scene 核心组件库，用于存储与管理导入的 3D 模型的所有数据


#pragma comment(lib,"d3d12.lib")			// 链接 DX12 核心 DLL
#pragma comment(lib,"dxgi.lib")				// 链接 DXGI DLL
#pragma comment(lib,"dxguid.lib")			// 链接 DXGI 必要的设备 GUID
#pragma comment(lib,"d3dcompiler.lib")		// 链接 DX12 需要的着色器编译 DLL
#pragma comment(lib,"windowscodecs.lib")	// 链接 WIC DLL
#pragma comment(lib,"assimp-vc143-mtd.lib")	// 链接 Assimp DLL

using namespace Microsoft;
using namespace Microsoft::WRL;		// 使用 wrl.h 里面的命名空间，我们需要用到里面的 Microsoft::WRL::ComPtr COM智能指针
using namespace DirectX;			// DirectX 命名空间


// 1.项目 -> 属性 -> VC++ 目录 -> 包含目录 -> 添加 Assimp/include (或者写 $(SolutionDir)Assimp/include )
// 2.项目 -> 属性 -> VC++ 目录 -> 库目录 -> 添加 Assimp/lib (或者写 $(SolutionDir)Assimp/lib )
// 3.将 Assimp/lib 里面的 assimp-vc143-mtd.lib 与 assimp-vc143-mtd.dll 复制到
//   exe 所在的目录下 (x64/Debug 文件夹)，生成的 exe 需要依赖这两个链接库



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


// 摄像机类
class Camera
{
private:

	XMVECTOR EyePosition = XMVectorSet(1, 1, 1, 1);			// 摄像机在世界空间下的位置
	XMVECTOR FocusPosition = XMVectorSet(0, 0, 0, 1);		// 摄像机在世界空间下观察的焦点位置
	XMVECTOR UpDirection = XMVectorSet(0, 1, 0, 0);			// 世界空间垂直向上的向量

	// 摄像机观察方向的单位向量，用于前后移动
	XMVECTOR ViewDirection = XMVector3Normalize(FocusPosition - EyePosition);

	// 焦距，摄像机原点与焦点的距离，XMVector3Length 表示对向量取模
	float FocalLength = XMVectorGetX(XMVector3Length(FocusPosition - EyePosition));

	// 摄像机向右方向的单位向量，用于左右移动，XMVector3Cross 求两向量叉乘
	// 注意叉乘不符合交换律，交换后结果方向相反，如果左右移动方向反了，可能需要检查一下叉乘
	XMVECTOR RightDirection = XMVector3Normalize(XMVector3Cross(UpDirection, ViewDirection));

	POINT LastCursorPoint = {};								// 上一次鼠标的位置

	float FovAngleY = XM_PIDIV4;							// 垂直视场角
	float AspectRatio = 16.0 / 9.0;							// 投影窗口宽高比
	float NearZ = 0.1;										// 近平面到原点的距离
	float FarZ = 1000;										// 远平面到原点的距离

	XMMATRIX ModelMatrix;									// 模型矩阵，模型空间 -> 世界空间
	XMMATRIX ViewMatrix;									// 观察矩阵，世界空间 -> 观察空间
	XMMATRIX ProjectionMatrix;								// 投影矩阵，观察空间 -> 齐次裁剪空间

	XMMATRIX MVPMatrix;										// MVP 矩阵，类外需要用公有方法 GetMVPMatrix 获取

public:

	Camera()	// 摄像机的构造函数
	{
		// 模型矩阵，这里设置绕 x 轴旋转 90°，是因为模型作者使用的建模软件不同，垂直向上的坐标轴有差异
		// 有些是 y 轴朝上建模的，有些是 z 轴朝上建模的 (例如这里是 Z 轴方向朝上，需要绕 x 轴旋转 90° 才行)
		ModelMatrix = XMMatrixRotationX(XM_PIDIV2);
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

	// 更新上一次的鼠标位置
	void UpdateLastCursorPos()
	{
		GetCursorPos(&LastCursorPoint);
	}

	// 当鼠标左键长按并移动时，旋转摄像机视角
	void CameraRotate()
	{
		POINT CurrentCursorPoint = {};
		GetCursorPos(&CurrentCursorPoint);	// 获取当前鼠标位置

		// 根据鼠标在屏幕坐标系的 x,y 轴的偏移量，计算摄像机旋转角
		float AngleX = XMConvertToRadians(0.25 * static_cast<float>(CurrentCursorPoint.x - LastCursorPoint.x));
		float AngleY = XMConvertToRadians(0.25 * static_cast<float>(CurrentCursorPoint.y - LastCursorPoint.y));

		// 旋转摄像机
		RotateByY(AngleY);
		RotateByX(AngleX);

		UpdateLastCursorPos();		// 旋转完毕，更新上一次的鼠标位置
	}

	// 更新 MVP 矩阵
	void UpdateMVPMatrix()
	{
		// 主要是更新观察矩阵
		ViewMatrix = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);
		MVPMatrix = ModelMatrix * ViewMatrix * ProjectionMatrix;
	}

	// 获取 MVP 矩阵
	inline XMMATRIX& GetMVPMatrix()
	{
		// 每次返回前，都更新一次
		UpdateMVPMatrix();
		return MVPMatrix;
	}

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
	ComPtr<ID3D12Resource> m_RenderTarget[3];				// 渲染目标数组，每一副渲染目标对应一个窗口缓冲区
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;					// RTV 描述符句柄
	UINT RTVDescriptorSize = 0;								// RTV 描述符的大小
	UINT FrameIndex = 0;									// 帧索引，表示当前渲染的第 i 帧 (第 i 个渲染目标)

	ComPtr<ID3D12Fence> m_Fence;							// 围栏
	UINT64 FenceValue = 0;									// 用于围栏等待的围栏值
	HANDLE RenderEvent = NULL;								// GPU 渲染事件
	D3D12_RESOURCE_BARRIER beg_barrier = {};				// 渲染开始的资源屏障，呈现 -> 渲染目标
	D3D12_RESOURCE_BARRIER end_barrier = {};				// 渲染结束的资源屏障，渲染目标 -> 呈现

	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;					// DSV 描述符堆
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle;					// DSV 描述符句柄
	ComPtr<ID3D12Resource> m_DepthStencilBuffer;			// DSV 深度模板缓冲资源

	// DSV 资源的格式
	// 深度模板缓冲只支持四种格式:
	// DXGI_FORMAT_D24_UNORM_S8_UINT	(每个像素占用四个字节 32 位，24 位无符号归一化浮点数留作深度值，8 位整数留作模板值)
	// DXGI_FORMAT_D32_FLOAT_S8X24_UINT	(每个像素占用八个字节 64 位，32 位浮点数留作深度值，8 位整数留作模板值，其余 24 位保留不使用)
	// DXGI_FORMAT_D16_UNORM			(每个像素占用两个字节 16 位，16 位无符号归一化浮点数留作深度值，范围 [0,1]，不使用模板)
	// DXGI_FORMAT_D32_FLOAT			(每个像素占用四个字节 32 位，32 位浮点数留作深度值，不使用模板)
	// 这里我们选择最常用的格式 DXGI_FORMAT_D24_UNORM_S8_UINT
	DXGI_FORMAT DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	

	// ---------------------------------------------------------------------------------------------------------------



	std::string ModelFileName = "sam/scene.gltf";					// 模型文件名
	std::string ModelTextureFilePath = "sam/";						// 模型所在文件夹
	Assimp::Importer* m_ModelImporter = new Assimp::Importer;		// 模型导入器
	const aiScene* m_ModelScene = nullptr;							// 模型/场景对象

	// 导入模型使用的标志
	// aiProcess_ConvertToLeftHanded: Assimp 导入的模型是以 OpenGL 的右手坐标系为基础的，将模型转换成 DirectX 的左手坐标系
	// aiProcess_Triangulate：模型设计师可能使用多边形对模型进行建模的，对于用多边形建模的模型，将它们都转换成基于三角形建模
	// aiProcess_FixInfacingNormals：建模软件都是双面显示的，所以设计师不会在意顶点绕序方向，部分面会被剔除无法正常显示，需要翻转过来
	// aiProcess_LimitBoneWeights: 限制网格的骨骼权重最多为 4 个，其余权重无需处理
	// aiProcess_GenBoundBoxes: 对每个网格，都生成一个 AABB 体积盒
	// aiProcess_JoinIdenticalVertices: 将位置相同的顶点合并为一个顶点，从而减少模型的顶点数量，优化内存使用和提升渲染效率。
	UINT ModelImportFlag = aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_FixInfacingNormals |
		aiProcess_LimitBoneWeights | aiProcess_GenBoundingBoxes | aiProcess_JoinIdenticalVertices;


	// 材质/纹理结构体
	struct Material
	{
		std::string FilePath;						// 材质文件路径
		aiTextureType Type;							// 材质类型
		ComPtr<ID3D12Resource> UploadTexture;		// 位于上传堆的纹理
		ComPtr<ID3D12Resource> DefaultTexture;		// 位于默认堆的纹理

		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;		// CPU 句柄
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;		// GPU 句柄
	};

	std::vector<Material> MaterialGroup;			// 模型用到的材质组


	struct VERTEX									// 顶点数据结构体
	{
		XMFLOAT4 position;							// 顶点位置
		XMFLOAT2 texcoordUV;						// 顶点纹理坐标
	};

	struct MESH										// 网格数据结构体
	{
		UINT MaterialIndex;							// Mesh 用到的材质索引

		UINT VertexGroupOffset;						// 顶点信息在 VertexGroup 的偏移
		UINT IndexGroupOffset;						// 索引信息在 IndexGroup 的偏移
		UINT VertexCount;							// 顶点数量
		UINT IndexCount;							// 索引数量
	};


	ComPtr<ID3D12Resource> UploadVertexResource;	// 上传堆顶点资源
	ComPtr<ID3D12Resource> UploadIndexResource;		// 上传堆索引资源
	ComPtr<ID3D12Resource> DefaultVertexResource;	// 默认堆顶点资源
	ComPtr<ID3D12Resource> DefaultIndexResource;	// 默认堆索引资源

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};	// 顶点信息描述符
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};	// 索引信息描述符


	std::vector<VERTEX> VertexGroup;		// 顶点组，存储整个模型的顶点信息
	std::vector<UINT> IndexGroup;			// 索引组，存储整个模型的索引信息

	std::vector<MESH> MeshGroup;			// 网格信息组


	struct AABB				// AABB 盒结构体
	{
		float minBoundsX;	// 最小坐标点 X 值
		float minBoundsY;	// 最小坐标点 Y 值
		float minBoundsZ;	// 最小坐标点 Z 值

		float maxBoundsX;	// 最大坐标点 X 值
		float maxBoundsY;	// 最大坐标点 Y 值
		float maxBoundsZ;	// 最大坐标点 Z 值
	};

	AABB ModelBoundingBox;	// 模型 AABB 盒，用于调整摄像机视野，防止模型在摄像机视野外飞出去



	// ---------------------------------------------------------------------------------------------------------------



	ComPtr<IWICImagingFactory> m_WICFactory;				// WIC 工厂
	ComPtr<IWICBitmapDecoder> m_WICBitmapDecoder;			// 位图解码器
	ComPtr<IWICBitmapFrameDecode> m_WICBitmapDecodeFrame;	// 由解码器得到的单个位图帧
	ComPtr<IWICFormatConverter> m_WICFormatConverter;		// 位图转换器
	ComPtr<IWICBitmapSource> m_WICBitmapSource;				// WIC 位图资源，用于获取位图数据
	UINT TextureWidth = 0;									// 纹理宽度
	UINT TextureHeight = 0;									// 纹理高度
	UINT BitsPerPixel = 0;									// 图像深度，图片每个像素占用的比特数
	UINT BytePerRowSize = 0;								// 纹理每行数据的真实字节大小，用于读取纹理数据、上传纹理资源
	DXGI_FORMAT TextureFormat = DXGI_FORMAT_UNKNOWN;		// 纹理格式

	ComPtr<ID3D12DescriptorHeap> m_SRVHeap;					// SRV 描述符堆

	UINT TextureSize = 0;									// 纹理的真实大小 (单位：字节)
	UINT UploadResourceRowSize = 0;							// 上传堆资源每行的大小 (单位：字节)
	UINT UploadResourceSize = 0;							// 上传堆资源的总大小 (单位：字节)

	UINT SRVDescriptorSize;									// SRV 描述符的大小


	ComPtr<ID3D12Resource> m_CBVResource;		// 常量缓冲资源，用于存放 MVP 矩阵，MVP 矩阵每帧都要更新，所以需要存储在常量缓冲区中
	struct CBuffer								// 常量缓冲结构体
	{
		XMFLOAT4X4 MVPMatrix;					// MVP 矩阵，用于将顶点数据从顶点空间变换到齐次裁剪空间
	};
	CBuffer* MVPBuffer = nullptr;	// 常量缓冲结构体指针，里面存储的是 MVP 矩阵信息，下文 Map 后指针会指向 CBVResource 的地址

	Camera m_FirstCamera;			// 第一人称摄像机

	ComPtr<ID3D12RootSignature> m_RootSignature;			// 根签名
	ComPtr<ID3D12PipelineState> m_PipelineStateObject;		// 渲染管线状态


	// 视口
	D3D12_VIEWPORT viewPort = D3D12_VIEWPORT{ 0, 0, float(WindowWidth), float(WindowHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	// 裁剪矩形
	D3D12_RECT ScissorRect = D3D12_RECT{ 0, 0, WindowWidth, WindowHeight };



public:

	// 初始化窗口
	void STEP1_InitWindow(HINSTANCE hins)
	{
		WNDCLASS wc = {};					// 用于记录窗口类信息的结构体
		wc.hInstance = hins;				// 窗口类需要一个应用程序的实例句柄 hinstance

		// 绑定回调函数，利用 std::bind，将 DX12Engine::CallBackFunc 绑定到 CallBackWrapper 的函数包装器上
		// std::bind 的作用是将 CallBackFunc 带参数绑定到一个对象上，并生成对应的函数包装器
		// 第一个参数表示对象的成员函数的指针，因为编译器不会将对象的成员函数隐式转换成函数指针，所以必须在前面添加 &
		// 第二个参数表示对象的地址，这个参数就表示类成员函数需要传递的 this 指针
		// 之后的表示将要绑定的参数，std::placeholders 表示占位符，用户调用函数，传递实参时，这个占位符会将实参一一对应
		CallBackWrapper::Broker_Func = std::bind(&DX12Engine::CallBackFunc, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		wc.lpfnWndProc = CallBackWrapper::CallBackFunc;		// 窗口类需要一个回调函数，用于处理窗口产生的消息，注意这里传递的是中间层的回调函数
		wc.lpszClassName = L"DX12 Game";					// 窗口类的名称

		RegisterClass(&wc);					// 注册窗口类，将窗口类录入到操作系统中

		// 使用上文的窗口类创建窗口
		m_hwnd = CreateWindow(wc.lpszClassName, L"Samuel Rodrigues", WS_SYSMENU | WS_OVERLAPPED,
			10, 10, WindowWidth, WindowHeight,
			NULL, NULL, hins, NULL);

		// 因为指定了窗口大小不可变的 WS_SYSMENU 和 WS_OVERLAPPED，应用不会自动显示窗口，需要使用 ShowWindow 强制显示窗口
		ShowWindow(m_hwnd, SW_SHOW);
	}

	// 创建调试层
	void STEP2_CreateDebugDevice()
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
	bool STEP3_CreateDevice()
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
	void STEP4_CreateCommandComponents()
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
	void STEP5_CreateRenderTarget()
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
	void STEP6_CreateFenceAndBarrier()
	{
		// 创建 CPU 上的等待事件
		RenderEvent = CreateEvent(nullptr, false, false, nullptr);

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

	// 创建 DSV 深度模板描述符堆 (Non-Shader Visible)
	void STEP7_CreateDSVHeap()
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
	void STEP8_CreateDepthStencilBuffer()
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
		// 资源标志，创建深度模板缓冲，必须要填 D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL，否则会创建失败
		DSVResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE DepthStencilBufferClearValue = {};				// 用于清空深度缓冲的信息结构体，DX12 能对这个进行优化
		DepthStencilBufferClearValue.DepthStencil.Depth = 1.0f;				// 要清空到的深度值，清空后会重置到该值
		DepthStencilBufferClearValue.DepthStencil.Stencil = 0;				// 要清空到的模板值，清空后会重置到该值
		DepthStencilBufferClearValue.Format = DSVFormat;					// 要清空缓冲的格式，要和上文一致

		// 默认堆属性，深度缓冲也是一块纹理，所以用默认堆
		D3D12_HEAP_PROPERTIES DefaultProperties = { D3D12_HEAP_TYPE_DEFAULT };

		// 创建资源，深度缓冲只会占用很少资源，所以直接 CreateCommittedResource 隐式堆创建即可，让操作系统帮我们管理
		m_D3D12Device->CreateCommittedResource(&DefaultProperties, D3D12_HEAP_FLAG_NONE, &DSVResourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &DepthStencilBufferClearValue, IID_PPV_ARGS(&m_DepthStencilBuffer));
	}

	// 创建 DSV 描述符，DSV 描述符用于描述深度模板缓冲区，这个描述符才是渲染管线要设置的对象
	void STEP9_CreateDSV()
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVViewDesc = {};
		DSVViewDesc.Format = DSVFormat;								// DSV 描述符格式要和资源一致
		DSVViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 深度缓冲本质也是一块 2D 纹理
		DSVViewDesc.Flags = D3D12_DSV_FLAG_NONE;					// 这个 Flag 是用来设置读写权限的，深度值和模板值均可以读写

		// 创建 DSV 描述符 (Depth Stencil View，深度模板描述符)
		m_D3D12Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &DSVViewDesc, DSVHandle);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 读取模型数据，数据会存储在 aiScene 对象
	bool STEP10_OpenModelFile()
	{
		// 使用 ReadFile 函数直接传递文件路径打开文件，路径可以有中文文字这些 utf-8 字符 (Assimp 最近修复了这个 bug)
		m_ModelScene = m_ModelImporter->ReadFile(ModelFileName, ModelImportFlag);

		// 如果模型没有成功载入 (无法载入，载入未完成，载入后无根节点)
		if (!m_ModelScene || m_ModelScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_ModelScene->mRootNode)
		{
			// Assimp 载入模型的错误信息
			std::string Assimp_error_msg = m_ModelImporter->GetErrorString();

			std::string errorMsg = "载入文件 ";
			errorMsg += ModelFileName;
			errorMsg += " 失败！错误原因：";
			errorMsg += Assimp_error_msg;
			MessageBoxA(NULL, errorMsg.c_str(), "错误", MB_ICONERROR | MB_OK);
			return false;
		}

		return true;
	}


	// 添加模型材质
	void STEP11_AddModelMaterials()
	{
		// 遍历模型中的所有材质
		for (UINT i = 0; i < m_ModelScene->mNumMaterials; i++)
		{
			// Assimp 解析出来的模型材质
			aiMaterial* material = m_ModelScene->mMaterials[i];

			// 模型文件携带的材质/纹理文件路径
			aiString materialPath;

			
			// 检测并加载 Diffuse Material 漫反射材质贴图，漫反射贴图是表示物体表面颜色的基本纹理，大多数都是漫反射材质
			// 一般而言会带有 _diffuse，_base_color，_albedo 这些后缀，一般情况下这三个是等同的
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &materialPath) == aiReturn_SUCCESS)
			{
				Material mt = {};						// 新材质
				mt.FilePath = ModelTextureFilePath;		// 新材质所在的文件夹
				mt.FilePath += materialPath.C_Str();	// 新材质的文件名
				mt.Type = aiTextureType_DIFFUSE;		// 新材质的类型
				MaterialGroup.push_back(mt);			// 添加新材质
			}
			// 如果不是 DIFFUSE 贴图，就设置默认贴图，有些网格对应的确实是默认贴图，默认贴图不显示颜色
			// 一般是图形引擎内置，发布的模型文件无法找到相关纹理
			else
			{
				Material mt = {};						// 新材质
				mt.Type = aiTextureType_NONE;			// 新材质的类型
				MaterialGroup.push_back(mt);			// 添加新材质
			}
		}
	}


	// 添加模型网格数据
	// Mesh 网格相当于模型的皮肤，它存储了模型要渲染的顶点信息
	void STEP12_AddModelData()
	{
		// 当前 Mesh 在 VertexGroup 的索引
		UINT CurrentMeshVertexGroupOffset = 0;
		// 当前 Mesh 在 IndexGroup 的索引
		UINT CurrentMeshIndexGroupOffset = 0;

		// 遍历模型所有的 Mesh 信息，在 Assimp 中，Mesh 与 Bone 是分开存储的，一个 Mesh 会有多个顶点
		for (UINT i = 0; i < m_ModelScene->mNumMeshes; i++)
		{
			// 当前网格
			const aiMesh* mesh = m_ModelScene->mMeshes[i];

			// 如果 Mesh 顶点数量是 0，直接跳过
			if (mesh->mNumVertices == 0) continue;

			// 要渲染的新 Mesh，注意这里用大括号初始化，把所有成员置 0 了
			MESH new_mesh = {};

			
			// 遍历模型所有的 Mesh 信息，一个 Mesh 会有多个顶点
			for (UINT j = 0; j < mesh->mNumVertices; j++)
			{
				// 新顶点
				VERTEX new_vertex = {};

				// 新顶点位置 (相对于当前 Mesh)
				new_vertex.position = XMFLOAT4(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z, 1);


				// 新节点纹理 UV，如果有就添加，没有就默认 (-1, -1)
				// 注意这个 0 指的是第 0 号 UV 通道 (详情请见 UE5 文档: UV 通道)
				// 对于同一个顶点，不同的 UV 通道可以有不同的 UV 坐标，常用于光照，但我们这里不涉及，直接获取第 0 号纹理 UV 即可
				if (mesh->HasTextureCoords(0))
				{
					new_vertex.texcoordUV = XMFLOAT2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
				}
				else
				{
					new_vertex.texcoordUV = XMFLOAT2(-1, -1);	// 默认纹理 UV 坐标，Pixel Shader 会进行处理
				}

				// 添加新节点
				VertexGroup.push_back(new_vertex);
			}


			// 添加 Mesh 的所有顶点索引信息，在 Assimp 中，索引通常是以 Face 为单位存储的
			// 因为模型设计师可能会用多边形进行建模，上文指定 aiProcess_Triangulate 就是为了把可能的多边形全部分割成三角形
			// 所以我们这里直接指定 mesh->mFaces[j].mIndices 添加节点即可
			for (UINT j = 0; j < mesh->mNumFaces; j++)
			{
				// 对每个 Face，都添加三个索引 (aiProcess_Triangulate 已经保证过每个面有且只有三个索引了)
				IndexGroup.push_back(mesh->mFaces[j].mIndices[0]);
				IndexGroup.push_back(mesh->mFaces[j].mIndices[1]);
				IndexGroup.push_back(mesh->mFaces[j].mIndices[2]);
			}


			// Mesh 对应的材质索引，Assimp 保证每个 Mesh 最多对应一个材质，如果对应了多个材质，导入模型时会自动将其分割
			new_mesh.MaterialIndex = mesh->mMaterialIndex;


			// 设置 Mesh 的顶点索引数量与偏移
			new_mesh.VertexGroupOffset = CurrentMeshVertexGroupOffset;		// 顶点偏移
			new_mesh.IndexGroupOffset = CurrentMeshIndexGroupOffset;		// 索引偏移
			new_mesh.VertexCount = mesh->mNumVertices;						// 顶点数量
			new_mesh.IndexCount = mesh->mNumFaces * 3;						// 索引数量

			// 更新当前顶点和索引的偏移
			CurrentMeshVertexGroupOffset = VertexGroup.size();
			CurrentMeshIndexGroupOffset = IndexGroup.size();
			

			// 添加新 Mesh
			MeshGroup.push_back(new_mesh);
		}
	}


	// 计算模型的 AABB 包围盒，并调整摄像机，防止模型在摄像机视野外飞出去
	void STEP13_CalcModelBoundingBox()
	{
		// 设置初始值
		ModelBoundingBox =
		{
			m_ModelScene->mMeshes[0]->mAABB.mMin.x,
			m_ModelScene->mMeshes[0]->mAABB.mMin.y,
			m_ModelScene->mMeshes[0]->mAABB.mMin.z,

			m_ModelScene->mMeshes[0]->mAABB.mMax.x,
			m_ModelScene->mMeshes[0]->mAABB.mMax.y,
			m_ModelScene->mMeshes[0]->mAABB.mMax.z
		};

		// 逐网格遍历，计算整个模型的 AABB 包围盒，请注意导入模型时要指定 aiProcess_GenBoundingBoxes，否则 mAABB 成员会没有数据
		for (UINT i = 1; i < m_ModelScene->mNumMeshes; i++)
		{
			// 当前网格
			const aiMesh* mesh = m_ModelScene->mMeshes[i];

			// 更新总包围盒
			ModelBoundingBox.minBoundsX = std::min(mesh->mAABB.mMin.x, ModelBoundingBox.minBoundsX);
			ModelBoundingBox.minBoundsY = std::min(mesh->mAABB.mMin.y, ModelBoundingBox.minBoundsY);
			ModelBoundingBox.minBoundsZ = std::min(mesh->mAABB.mMin.z, ModelBoundingBox.minBoundsZ);

			ModelBoundingBox.maxBoundsX = std::max(mesh->mAABB.mMax.x, ModelBoundingBox.maxBoundsX);
			ModelBoundingBox.maxBoundsY = std::max(mesh->mAABB.mMax.y, ModelBoundingBox.maxBoundsY);
			ModelBoundingBox.maxBoundsZ = std::max(mesh->mAABB.mMax.z, ModelBoundingBox.maxBoundsZ);
		}


		// 计算模型中心点
		XMFLOAT3 CenterPoint = {};
		CenterPoint.x = (ModelBoundingBox.maxBoundsX + ModelBoundingBox.minBoundsX) / 2.0;
		CenterPoint.y = (ModelBoundingBox.maxBoundsY + ModelBoundingBox.minBoundsY) / 2.0;
		CenterPoint.z = (ModelBoundingBox.maxBoundsZ + ModelBoundingBox.minBoundsZ) / 2.0;

		
		// 计算模型 AABB 盒半径
		float RadiusX = (ModelBoundingBox.maxBoundsX - ModelBoundingBox.minBoundsX) / 2.0;	// AABB 盒半径 x 分量
		float RadiusY = (ModelBoundingBox.maxBoundsY - ModelBoundingBox.minBoundsY) / 2.0;	// AABB 盒半径 y 分量
		float RadiusZ = (ModelBoundingBox.maxBoundsZ - ModelBoundingBox.minBoundsZ) / 2.0;	// AABB 盒半径 z 分量

		// 模型 AABB 盒半径 (外接球半径)
		float Radius = std::sqrt(RadiusX * RadiusX + RadiusY * RadiusY + RadiusZ * RadiusZ) / 2.0;


		// 设置摄像机焦点
		m_FirstCamera.SetFocusPosition(XMVectorSet(CenterPoint.x, CenterPoint.y, CenterPoint.z, 1.0));

		// 设置摄像机位置
		m_FirstCamera.SetEyePosition(XMVectorSet(CenterPoint.x, CenterPoint.y, CenterPoint.z - Radius, 1.0));
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建 SRV Descriptor Heap 着色器资源描述符堆 (Shader Visible)
	void CreateSRVHeap()
	{
		// 创建 SRV 描述符堆 (Shader Resource View，着色器资源描述符)
		D3D12_DESCRIPTOR_HEAP_DESC SRVHeapDesc = {};
		SRVHeapDesc.NumDescriptors = MaterialGroup.size();					// 描述符堆的容量
		SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;			// 描述符堆类型，CBV、SRV、UAV 这三种描述符可以放在同一种描述符堆上
		SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;		// 描述符堆标志，Shader-Visible 表示对着色器可见

		// 创建 SRV 描述符堆
		m_D3D12Device->CreateDescriptorHeap(&SRVHeapDesc, IID_PPV_ARGS(&m_SRVHeap));
	}

	// 启动命令列表，准备录制复制命令
	void StartCommandRecord()
	{
		// 复制资源需要使用 GPU 的 CopyEngine 复制引擎，所以需要向命令队列发出复制命令

		m_CommandAllocator->Reset();								// 先重置命令分配器
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);	// 再重置命令列表，复制命令不需要 PSO 状态，所以第二个参数填 nullptr
	}

	// 上取整算法，对 A 向上取整，判断至少要多少个长度为 B 的空间才能容纳 A，用于内存对齐
	inline UINT Ceil(UINT A, UINT B)
	{
		return (A + B - 1) / B;
	}

	// 创建默认纹理资源，有些 Mesh 会引用源模型文件中并不存在的 Default Teature 默认纹理，简单创建即可
	void CreateDefaultTexture(const UINT index)
	{
		// 用于中转纹理的上传堆资源结构体
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 资源类型，上传堆的资源类型都是 buffer 缓冲
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// 资源布局，指定资源的存储方式，上传堆的资源都是 row major 按行线性存储
		UploadResourceDesc.Width = 512;										// 资源宽度，上传堆的资源宽度是资源的总大小，注意资源大小必须只多不少
		UploadResourceDesc.Height = 1;										// 资源高度，上传堆仅仅是传递线性资源的，所以高度必须为 1
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 资源格式，上传堆资源的格式必须为 UNKNOWN
		UploadResourceDesc.DepthOrArraySize = 1;							// 资源深度，这个是用于纹理数组和 3D 纹理的，上传堆资源必须为 1
		UploadResourceDesc.MipLevels = 1;									// Mipmap 等级，这个是用于纹理的，上传堆资源必须为 1
		UploadResourceDesc.SampleDesc.Count = 1;							// 资源采样次数，上传堆资源都是填 1

		// 上传堆属性的结构体，上传堆位于 CPU 和 GPU 的共享内存
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建上传堆资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &UploadResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&MaterialGroup[index].UploadTexture));


		// 注意！默认纹理的纹理格式要选 DXGI_FORMAT_R8G8B8A8_UNORM！
		TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		// 用于放纹理的默认堆资源结构体
		D3D12_RESOURCE_DESC DefaultResourceDesc = {};
		DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 资源类型，这里指定为 Texture2D 2D纹理
		DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;			// 纹理资源的布局都是 UNKNOWN
		DefaultResourceDesc.Width = 2;										// 资源宽度，这里填纹理宽度
		DefaultResourceDesc.Height = 2;										// 资源高度，这里填纹理高度
		DefaultResourceDesc.Format = TextureFormat;							// 资源格式，默认纹理选 DXGI_FORMAT_R8G8B8A8_UNORM 就行
		DefaultResourceDesc.DepthOrArraySize = 1;							// 资源深度，我们只有一副纹理，所以填 1
		DefaultResourceDesc.MipLevels = 1;									// Mipmap 等级，我们暂时不使用 Mipmap，所以填 1
		DefaultResourceDesc.SampleDesc.Count = 1;							// 资源采样次数，这里我们填 1 就行

		// 默认堆属性的结构体，默认堆位于显存
		D3D12_HEAP_PROPERTIES DefaultHeapDesc = { D3D12_HEAP_TYPE_DEFAULT };

		// 创建默认堆资源
		m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE, &DefaultResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&MaterialGroup[index].DefaultTexture));
	}

	// CommandList 录制命令，录制将默认纹理数据复制到默认堆资源的命令
	void CopyDefaultTextureToDefaultResource(const UINT index)
	{
		// 创建默认贴图数据，一共 2 行，每行两个 DXGI_R8G8B8A8_UNROM 数据，每个数据占 8 Byte (Byte = UINT8)
		UINT8 DefaultTextureData[2 * 2 * 4];

		// 设置颜色数据
		for (UINT i = 0; i < 2; i++)
		{
			DefaultTextureData[i * 4 + 0] = 255;	// R (红色)
			DefaultTextureData[i * 4 + 1] = 255;	// G (绿色)
			DefaultTextureData[i * 4 + 2] = 255;	// B (蓝色)
			DefaultTextureData[i * 4 + 3] = 128;	// A (透明度)
		}

		// 传递数据的指针
		BYTE* TransferPointer = nullptr;

		// Map 开始映射，Map 方法会得到上传堆资源的地址 (在共享内存上)，传递给指针，这样我们就能通过 memcpy 操作复制数据了
		MaterialGroup[index].UploadTexture->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));

		// 逐行复制数据，默认纹理上传堆一行 256 Byte
		for (UINT i = 0; i < 2; i++)
		{
			// 向上传堆资源逐行复制纹理数据 (CPU 高速缓存 -> 共享内存)
			memcpy(TransferPointer, DefaultTextureData + i * 8, 8);
			// 上传堆资源指针偏移到下一行
			TransferPointer += 256;
		}

		// Unmap 结束映射，因为我们无法直接读写默认堆资源，需要上传堆复制到那里，在复制之前，我们需要先结束映射，让上传堆处于只读状态
		MaterialGroup[index].UploadTexture->Unmap(0, nullptr);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint = {};									// 资源脚本，用来描述要复制的资源
		D3D12_RESOURCE_DESC DefaultResourceDesc = MaterialGroup[index].DefaultTexture->GetDesc();	// 默认堆资源结构体

		// 获取纹理复制脚本，用于下文的纹理复制
		m_D3D12Device->GetCopyableFootprints(&DefaultResourceDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION DstLocation = {};						// 复制目标位置 (默认堆资源) 结构体
		DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		// 纹理复制类型，这里必须指向纹理
		DstLocation.SubresourceIndex = 0;									// 指定要复制的子资源索引
		DstLocation.pResource = MaterialGroup[index].DefaultTexture.Get();	// 要复制到的资源

		D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};						// 复制源位置 (上传堆资源) 结构体
		SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// 纹理复制类型，这里必须指向缓冲区
		SrcLocation.PlacedFootprint = PlacedFootprint;						// 指定要复制的资源脚本信息
		SrcLocation.pResource = MaterialGroup[index].UploadTexture.Get();	// 被复制数据的缓冲



		// 记录复制资源到默认堆的命令 (共享内存 -> 显存) 
		m_CommandList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);
	}

	// 加载纹理到内存中
	bool LoadTextureFromFile(const std::wstring TextureFilename)
	{
		// 如果还没创建 WIC 工厂，就新建一个 WIC 工厂实例。注意！WIC 工厂不可以重复释放与创建！
		if (m_WICFactory == nullptr) CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_WICFactory));

		// 创建图片解码器，并将图片读入到解码器中
		HRESULT hr = m_WICFactory->CreateDecoderFromFilename(TextureFilename.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &m_WICBitmapDecoder);

		std::wostringstream output_str;		// 用于格式化字符串
		switch (hr)
		{
		case S_OK: break;	// 解码成功，直接 break 进入下一步即可

		case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):	// 文件找不到
			output_str << L"找不到文件 " << TextureFilename << L" ！请检查文件路径是否有误！";
			MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
			return false;

		case HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT):	// 文件句柄正在被另一个应用进程占用
			output_str << L"文件 " << TextureFilename << L" 已经被另一个应用进程打开并占用了！请先关闭那个应用进程！";
			MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
			return false;

		case WINCODEC_ERR_COMPONENTNOTFOUND:			// 找不到可解码的组件，说明这不是有效的图像文件
			output_str << L"文件 " << TextureFilename << L" 不是有效的图像文件，无法解码！请检查文件是否为图像文件！";
			MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
			return false;

		default:			// 发生其他未知错误
			output_str << L"文件 " << TextureFilename << L" 解码失败！发生了其他错误，错误码：" << hr << L" ，请查阅微软官方文档。";
			MessageBox(NULL, output_str.str().c_str(), L"错误", MB_OK | MB_ICONERROR);
			return false;
		}

		// 获取图片数据的第一帧，这个 GetFrame 可以用于 gif 这种多帧动图
		m_WICBitmapDecoder->GetFrame(0, &m_WICBitmapDecodeFrame);


		// 获取图片格式，并将它转化为 DX12 能接受的纹理格式
		// 如果碰到格式无法支持的错误，可以用微软提供的 画图3D 来转换，强力推荐!
		WICPixelFormatGUID SourceFormat = {};				// 源图格式
		GUID TargetFormat = {};								// 目标格式

		m_WICBitmapDecodeFrame->GetPixelFormat(&SourceFormat);						// 获取源图格式

		if (DX12TextureHelper::GetTargetPixelFormat(&SourceFormat, &TargetFormat))	// 获取目标格式
		{
			TextureFormat = DX12TextureHelper::GetDXGIFormatFromPixelFormat(&TargetFormat);	// 获取 DX12 支持的格式
		}
		else	// 如果没有可支持的目标格式
		{
			::MessageBox(NULL, L"此纹理不受支持!", L"提示", MB_OK);
			return false;
		}


		// 获取目标格式后，将纹理转换为目标格式，使其能被 DX12 使用
		m_WICFactory->CreateFormatConverter(&m_WICFormatConverter);		// 创建图片转换器
		// 初始化转换器，实际上是把位图进行了转换
		m_WICFormatConverter->Initialize(m_WICBitmapDecodeFrame.Get(), TargetFormat, WICBitmapDitherTypeNone,
			nullptr, 0.0f, WICBitmapPaletteTypeCustom);
		// 将位图数据继承到 WIC 位图资源，我们要在这个 WIC 位图资源上获取信息
		m_WICFormatConverter.As(&m_WICBitmapSource);



		m_WICBitmapSource->GetSize(&TextureWidth, &TextureHeight);		// 获取纹理宽高

		ComPtr<IWICComponentInfo> _temp_WICComponentInfo = {};			// 用于获取 BitsPerPixel 纹理图像深度
		ComPtr<IWICPixelFormatInfo> _temp_WICPixelInfo = {};			// 用于获取 BitsPerPixel 纹理图像深度
		m_WICFactory->CreateComponentInfo(TargetFormat, &_temp_WICComponentInfo);
		_temp_WICComponentInfo.As(&_temp_WICPixelInfo);
		_temp_WICPixelInfo->GetBitsPerPixel(&BitsPerPixel);				// 获取 BitsPerPixel 图像深度

		return true;
	}

	// 创建用于上传的 UploadResource 与用于放纹理的 DefaultResource
	void CreateUploadAndDefaultResource(const UINT index)
	{
		// 计算纹理每行数据的真实数据大小 (单位：Byte 字节)，因为纹理图片在内存中是线性存储的
		// 想获取纹理的真实大小、正确读取纹理数据、上传到 GPU，必须先获取纹理的 BitsPerPixel 图像深度，因为不同位图深度可能不同
		// 然后再计算每行像素占用的字节，除以 8 是因为 1 Byte = 8 bits
		BytePerRowSize = TextureWidth * BitsPerPixel / 8;

		// 纹理的真实大小 (单位：字节)
		TextureSize = BytePerRowSize * TextureHeight;

		// 上传堆资源每行的大小 (单位：字节)，注意这里要进行 256 字节对齐！
		// 因为 GPU 与 CPU 架构不同，GPU 注重并行计算，注重结构化数据的快速读取，读取数据都是以 256 字节为一组来读的
		// 因此要先要对 BytePerRowSize 进行对齐，判断需要有多少组才能容纳纹理每行像素，不对齐的话数据会读错的。
		UploadResourceRowSize = Ceil(BytePerRowSize, 256) * 256;

		// 上传堆资源的总大小 (单位：字节)，分配空间必须只多不少，否则会报 D3D12 MinimumAlloc Error 资源内存创建错误
		// 注意最后一行不用内存对齐 (因为后面没其他行了，不用内存对齐也能正确读取)，所以要 (TextureHeight - 1) 再加 BytePerRowSize
		UploadResourceSize = UploadResourceRowSize * (TextureHeight - 1) + BytePerRowSize;


		// 用于中转纹理的上传堆资源结构体
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 资源类型，上传堆的资源类型都是 buffer 缓冲
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// 资源布局，指定资源的存储方式，上传堆的资源都是 row major 按行线性存储
		UploadResourceDesc.Width = UploadResourceSize;						// 资源宽度，上传堆的资源宽度是资源的总大小，注意资源大小必须只多不少
		UploadResourceDesc.Height = 1;										// 资源高度，上传堆仅仅是传递线性资源的，所以高度必须为 1
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 资源格式，上传堆资源的格式必须为 UNKNOWN
		UploadResourceDesc.DepthOrArraySize = 1;							// 资源深度，这个是用于纹理数组和 3D 纹理的，上传堆资源必须为 1
		UploadResourceDesc.MipLevels = 1;									// Mipmap 等级，这个是用于纹理的，上传堆资源必须为 1
		UploadResourceDesc.SampleDesc.Count = 1;							// 资源采样次数，上传堆资源都是填 1

		// 上传堆属性的结构体，上传堆位于 CPU 和 GPU 的共享内存
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建上传堆资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &UploadResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&MaterialGroup[index].UploadTexture));



		// 用于放纹理的默认堆资源结构体
		D3D12_RESOURCE_DESC DefaultResourceDesc = {};
		DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 资源类型，这里指定为 Texture2D 2D纹理
		DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;			// 纹理资源的布局都是 UNKNOWN
		DefaultResourceDesc.Width = TextureWidth;							// 资源宽度，这里填纹理宽度
		DefaultResourceDesc.Height = TextureHeight;							// 资源高度，这里填纹理高度
		DefaultResourceDesc.Format = TextureFormat;							// 资源格式，这里填纹理格式，要和纹理一样
		DefaultResourceDesc.DepthOrArraySize = 1;							// 资源深度，我们只有一副纹理，所以填 1
		DefaultResourceDesc.MipLevels = 1;									// Mipmap 等级，我们暂时不使用 Mipmap，所以填 1
		DefaultResourceDesc.SampleDesc.Count = 1;							// 资源采样次数，这里我们填 1 就行

		// 默认堆属性的结构体，默认堆位于显存
		D3D12_HEAP_PROPERTIES DefaultHeapDesc = { D3D12_HEAP_TYPE_DEFAULT };

		// 创建默认堆资源
		m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE, &DefaultResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&MaterialGroup[index].DefaultTexture));
	}

	// CommandList 录制命令，录制将纹理数据复制到默认堆资源的命令
	void CopyTextureDataToDefaultResource(const UINT index)
	{
		// 用于暂时存储纹理数据的指针，这里要用 malloc 分配空间
		BYTE* TextureData = (BYTE*)malloc(TextureSize);

		// 将整块纹理数据读到 TextureData 中，方便后文的 memcpy 复制操作
		m_WICBitmapSource->CopyPixels(nullptr, BytePerRowSize, TextureSize, TextureData);

		// 用于传递资源的指针
		BYTE* TransferPointer = nullptr;

		// Map 开始映射，Map 方法会得到上传堆资源的地址 (在共享内存上)，传递给指针，这样我们就能通过 memcpy 操作复制数据了
		MaterialGroup[index].UploadTexture->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));

		// 这里我们要逐行复制数据！注意两个指针偏移的长度不同！
		for (UINT i = 0; i < TextureHeight; i++)
		{
			// 向上传堆资源逐行复制纹理数据 (CPU 高速缓存 -> 共享内存)
			memcpy(TransferPointer, TextureData, BytePerRowSize);
			// 纹理指针偏移到下一行
			TextureData += BytePerRowSize;
			// 上传堆资源指针偏移到下一行，注意偏移长度不同！
			TransferPointer += UploadResourceRowSize;
		}

		// Unmap 结束映射，因为我们无法直接读写默认堆资源，需要上传堆复制到那里，在复制之前，我们需要先结束映射，让上传堆处于只读状态
		MaterialGroup[index].UploadTexture->Unmap(0, nullptr);

		TextureData -= TextureSize;		// 纹理资源指针偏移回初始位置
		free(TextureData);				// 释放上文 malloc 分配的空间，后面我们用不到它，不要让它占内存

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint = {};									// 资源脚本，用来描述要复制的资源
		D3D12_RESOURCE_DESC DefaultResourceDesc = MaterialGroup[index].DefaultTexture->GetDesc();	// 默认堆资源结构体

		// 获取纹理复制脚本，用于下文的纹理复制
		m_D3D12Device->GetCopyableFootprints(&DefaultResourceDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION DstLocation = {};						// 复制目标位置 (默认堆资源) 结构体
		DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		// 纹理复制类型，这里必须指向纹理
		DstLocation.SubresourceIndex = 0;									// 指定要复制的子资源索引
		DstLocation.pResource = MaterialGroup[index].DefaultTexture.Get();	// 要复制到的资源

		D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};						// 复制源位置 (上传堆资源) 结构体
		SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// 纹理复制类型，这里必须指向缓冲区
		SrcLocation.PlacedFootprint = PlacedFootprint;						// 指定要复制的资源脚本信息
		SrcLocation.pResource = MaterialGroup[index].UploadTexture.Get();	// 被复制数据的缓冲


		// 记录复制资源到默认堆的命令 (共享内存 -> 显存) 
		m_CommandList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);
	}

	// 关闭命令列表，启动命令队列，正式开始将纹理复制到默认堆资源中
	void StartCommandExecute()
	{
		// 关闭命令列表
		m_CommandList->Close();

		// 用于传递命令用的临时 ID3D12CommandList 数组
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// 提交复制命令！GPU 开始复制！
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);


		// 将围栏预定值设定为下一帧，注意复制资源也需要围栏等待，否则会发生资源冲突
		FenceValue++;
		// 在命令队列 (命令队列在 GPU 端) 设置围栏预定值，此命令会加入到命令队列中
		// 命令队列执行到这里会修改围栏值，表示复制已完成，"击中"围栏
		m_CommandQueue->Signal(m_Fence.Get(), FenceValue);
		// 设置围栏的预定事件，当复制完成时，围栏被"击中"，激发预定事件，将事件由无信号状态转换成有信号状态
		m_Fence->SetEventOnCompletion(FenceValue, RenderEvent);
	}

	// 最终创建 SRV 着色器资源描述符，用于描述默认堆资源为一块纹理，创建完 SRV 描述符，会将描述符句柄存储到纹理映射表中
	void CreateSRV(const UINT index, D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
	{
		// SRV 描述符信息结构体
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptorDesc = {};
		// SRV 描述符类型，这里我们指定 Texture2D 2D纹理
		SRVDescriptorDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		// SRV 描述符的格式也要填纹理格式
		SRVDescriptorDesc.Format = TextureFormat;
		// 纹理采样后每个纹理像素 RGBA 分量的顺序，D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING 表示纹理采样后分量顺序不改变
		SRVDescriptorDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		// 这里我们不使用 Mipmap，所以填 1
		SRVDescriptorDesc.Texture2D.MipLevels = 1;

		// 创建 SRV 描述符，注意这里要用参数中的 CPUHandle
		m_D3D12Device->CreateShaderResourceView(MaterialGroup[index].DefaultTexture.Get(), &SRVDescriptorDesc, CPUHandle);

		// 将当前 SRV 描述符句柄存储到 ModelManager 的纹理映射表中，注意我们传的是引用参数，可以直接对参数进行修改
		MaterialGroup[index].CPUHandle = CPUHandle;
		MaterialGroup[index].GPUHandle = GPUHandle;
	}


	// 将 std::string 转换成 std::wstring
	const std::wstring StringConvertToWString(const std::string str)
	{
		// 创建 wstring_convert 对象，用于 UTF-8 到 wchar 宽字符的转换
		static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		// 将 str 转换成宽字符版本 (wstring)
		return converter.from_bytes(str);
	}


	// 读取并创建纹理资源
	void STEP14_CreateModelTextureResource()
	{
		CreateSRVHeap();	// 创建 SRV 描述符堆，创建时就会确定描述符的 CPU 和 GPU 地址，无需担心

		// 当前元素的 CPU 句柄
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();
		// 当前元素的 GPU 句柄
		D3D12_GPU_DESCRIPTOR_HANDLE CurrentGPUHandle = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();
		// SRV 描述符的大小
		SRVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		StartCommandRecord();	// 启动命令列表，开始录制命令

		// 对材质组进行遍历
		for (UINT i = 0; i < MaterialGroup.size(); i++)
		{
			// 如果不是默认纹理
			if (MaterialGroup[i].Type != aiTextureType_NONE)
			{
				// 从纹理文件中加载数据
				LoadTextureFromFile(StringConvertToWString(MaterialGroup[i].FilePath));
				// 创建上传堆和默认堆资源
				CreateUploadAndDefaultResource(i);
				// 将纹理数据复制到上传堆，并记录一条上传堆复制到默认堆的命令
				CopyTextureDataToDefaultResource(i);
			}
			else // 否则就进行特殊处理
			{
				// 创建资源
				CreateDefaultTexture(i);
				// 复制数据
				CopyDefaultTextureToDefaultResource(i);
			}

			// 最终创建 SRV 描述符
			CreateSRV(i, CurrentCPUHandle, CurrentGPUHandle);

			// CPU 和 GPU 句柄偏移，准备下一个纹理
			CurrentCPUHandle.ptr += SRVDescriptorSize;
			CurrentGPUHandle.ptr += SRVDescriptorSize;
		}

		StartCommandExecute();	// 关闭命令列表，交给命令队列执行


		// 让主线程强制等待命令队列的完成，WaitForSingleObject 会阻塞当前线程，直到 event 有信号，或达到指定时间
		// 因为我们复制完资源，还要执行复制顶点到默认堆的命令，这个需要重置命令分配器
		// 重置命令分配器的要求是：命令队列必须执行完分配器的命令，否则会发生资源竞争
		// 而 CPU，GPU 两者恰好是异步执行的，也就是说 CommandQueue->ExecuteCommandLists() 后 CPU 端仍然会继续执行
		// 所以我们在这里要阻塞 Main 函数所在的主线程，同步 CPU 与 GPU 的执行
		// 然而这里用 WaitForSingleObject 阻塞主线程并不是一个好的选择，第二个参数 INFINITE 设定等待时间无限，更是一种不好的做法
		// 如果要执行的命令数量多，执行耗时长，主线程一阻塞，而绑定窗口的消息回调恰好是在主线程上执行的，那么程序就会不幸的 gg 了
		// 后面的教程我们会将程序进行多线程优化，避免使用 WaitForSingleObject 同步，而是改用 Render 里的 MsgWaitForMultipleObjects
		WaitForSingleObject(RenderEvent, INFINITE);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建顶点与索引资源，包括上传堆与默认堆，同时创建描述符
	void STEP15_CreateMeshResourceAndDescriptor()
	{
		// 如果两边 (上传堆和默认堆) 的用途都是 buffer 线性缓冲，不需要再进行 256 字节对齐了

		// 上传堆顶点资源结构体
		D3D12_RESOURCE_DESC UploadVertexResourceDesc = {};
		UploadVertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// 上传堆资源类型都是 buffer
		UploadVertexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// buffer 的格式都是 DXGI_FORMAT_UNKNOWN
		UploadVertexResourceDesc.Width = sizeof(VERTEX) * VertexGroup.size();	// 总大小是顶点数 * 顶点结构体大小
		UploadVertexResourceDesc.Height = 1;									// buffer 高度都是 1
		UploadVertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// buffer 的布局都是 row_major 行主列
		UploadVertexResourceDesc.DepthOrArraySize = 1;							// 填 1
		UploadVertexResourceDesc.MipLevels = 1;									// 填 1
		UploadVertexResourceDesc.SampleDesc.Count = 1;							// 填 1
		UploadVertexResourceDesc.SampleDesc.Quality = 0;						// 填 0

		// 默认堆顶点资源结构体
		D3D12_RESOURCE_DESC DefaultVertexResourceDesc = {};
		DefaultVertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// 这里选 buffer
		DefaultVertexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// buffer 的格式都是 DXGI_FORMAT_UNKNOWN
		DefaultVertexResourceDesc.Width = sizeof(VERTEX) * VertexGroup.size();	// 总大小是顶点数 * 顶点结构体大小
		DefaultVertexResourceDesc.Height = 1;									// buffer 高度都是 1
		DefaultVertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// buffer 的布局都是 row_major 行主列
		DefaultVertexResourceDesc.DepthOrArraySize = 1;							// 填 1
		DefaultVertexResourceDesc.MipLevels = 1;								// 填 1
		DefaultVertexResourceDesc.SampleDesc.Count = 1;							// 填 1
		DefaultVertexResourceDesc.SampleDesc.Quality = 0;						// 填 0

		// 上传堆索引资源结构体
		D3D12_RESOURCE_DESC UploadIndexResourceDesc = {};
		UploadIndexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// 这里选 buffer
		UploadIndexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// buffer 的格式都是 DXGI_FORMAT_UNKNOWN
		UploadIndexResourceDesc.Width = sizeof(UINT) * IndexGroup.size();		// 总大小是索引数 * 一个 32 位整数的大小
		UploadIndexResourceDesc.Height = 1;										// buffer 高度都是 1
		UploadIndexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// buffer 的布局都是 row_major 行主列
		UploadIndexResourceDesc.DepthOrArraySize = 1;							// 填 1
		UploadIndexResourceDesc.MipLevels = 1;									// 填 1
		UploadIndexResourceDesc.SampleDesc.Count = 1;							// 填 1
		UploadIndexResourceDesc.SampleDesc.Quality = 0;							// 填 0

		// 默认堆索引资源结构体
		D3D12_RESOURCE_DESC DefaultIndexResourceDesc = {};
		DefaultIndexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// 这里选 buffer
		DefaultIndexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// buffer 的格式都是 DXGI_FORMAT_UNKNOWN
		DefaultIndexResourceDesc.Width = sizeof(UINT) * IndexGroup.size();		// 总大小是索引数 * 一个 32 位整数的大小
		DefaultIndexResourceDesc.Height = 1;									// buffer 高度都是 1
		DefaultIndexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// buffer 的布局都是 row_major 行主列
		DefaultIndexResourceDesc.DepthOrArraySize = 1;							// 填 1
		DefaultIndexResourceDesc.MipLevels = 1;									// 填 1
		DefaultIndexResourceDesc.SampleDesc.Count = 1;							// 填 1
		DefaultIndexResourceDesc.SampleDesc.Quality = 0;						// 填 0

		// 上传堆属性
		D3D12_HEAP_PROPERTIES UploadHeapProperties = { D3D12_HEAP_TYPE_UPLOAD };
		// 默认堆属性
		D3D12_HEAP_PROPERTIES DefaultHeapProperties = { D3D12_HEAP_TYPE_DEFAULT };

		

		// 创建资源，注意默认堆资源的状态是 D3D12_RESOURCE_STATE_COPY_DEST
		m_D3D12Device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &UploadVertexResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadVertexResource));

		m_D3D12Device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &DefaultVertexResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&DefaultVertexResource));

		m_D3D12Device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &UploadIndexResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadIndexResource));

		m_D3D12Device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &DefaultIndexResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&DefaultIndexResource));


		// 填写 VertexBufferView 顶点信息描述符 和 IndexBufferView 索引信息描述符
		// 注意：这里填写的是 DefaultResource 的地址，因为大量顶点数据需要经常访问的话，用默认堆资源效果更好，渲染速度更快

		VertexBufferView.BufferLocation = DefaultVertexResource->GetGPUVirtualAddress();	// 资源地址
		VertexBufferView.SizeInBytes = DefaultVertexResourceDesc.Width;						// 资源大小
		VertexBufferView.StrideInBytes = sizeof(VERTEX);									// 资源步长

		IndexBufferView.BufferLocation = DefaultIndexResource->GetGPUVirtualAddress();		// 资源地址
		IndexBufferView.SizeInBytes = DefaultIndexResourceDesc.Width;						// 资源大小
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;										// 资源格式 (步长)
	}


	// 将 Mesh 里面的顶点与索引数据复制到上传堆资源，因为碰到大量数据，GPU 访问上传堆的速度将会相当慢
	// 相反，访问默认堆的速度就快得多，如果涉及到大量静态数据，都可以通过上传堆移动到默认堆上，只是在 Copy Engine 复制缓冲的时候会耗一点时间
	void STEP16_CopyMeshToUploadResource()
	{
		// 用于传递数据的指针
		BYTE* TransferPointer = nullptr;

		// 获取上传堆顶点资源地址
		UploadVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		// 将 Mesh 顶点数据复制到上传堆资源上
		memcpy(TransferPointer, &VertexGroup[0], sizeof(VERTEX) * VertexGroup.size());
		// 关闭映射
		UploadVertexResource->Unmap(0, nullptr);


		// 获取上传堆索引资源地址
		UploadIndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		// 将 Mesh 顶点数据复制到上传堆资源上
		memcpy(TransferPointer, &IndexGroup[0], sizeof(UINT) * IndexGroup.size());
		// 关闭映射
		UploadIndexResource->Unmap(0, nullptr);
	}


	// 命令列表添加复制命令，并送到命令队列执行，将上传堆上的顶点索引资源复制到默认堆中
	void STEP17_CopyMeshToDefaultResource()
	{
		StartCommandRecord();	// 启动命令列表，开始录制命令

		// CopyBufferRegion 复制缓冲资源，常用于不同堆，CPU 不能直接访问的缓冲复制

		// 命令列表添加复制命令，复制上传堆顶点资源到默认堆中
		m_CommandList->CopyBufferRegion(DefaultVertexResource.Get(), 0,
			UploadVertexResource.Get(), 0, sizeof(VERTEX) * VertexGroup.size());
		// 命令列表添加复制命令，复制上传堆顶点资源到默认堆中
		m_CommandList->CopyBufferRegion(DefaultIndexResource.Get(), 0,
			UploadIndexResource.Get(), 0, sizeof(UINT) * IndexGroup.size());

		StartCommandExecute();	// 关闭命令列表，交给命令队列执行


		// 下一个等待就是 RenderLoop 的 MsgWaitForMultipleObjects，不需要用 WaitForSingleObject 了
	}



	// ---------------------------------------------------------------------------------------------------------------



	// 创建 Constant Buffer Resource 常量缓冲资源，常量缓冲是一块预先分配的高速显存，用于存储每一帧都要变换的资源，这里我们要存储 MVP 矩阵
	void STEP18_CreateCBVResource()
	{
		// 常量资源宽度，这里填整个结构体的大小。注意！硬件要求，常量缓冲需要 256 字节对齐！所以这里要进行 Ceil 向上取整，进行内存对齐！
		UINT CBufferWidth = Ceil(sizeof(CBuffer), 256) * 256;

		D3D12_RESOURCE_DESC CBVResourceDesc = {};						// 常量缓冲资源信息结构体
		CBVResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// 上传堆资源都是缓冲
		CBVResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// 上传堆资源都是按行存储数据的 (一维线性存储)
		CBVResourceDesc.Width = CBufferWidth;							// 常量缓冲区资源宽度 (要分配显存的总大小)
		CBVResourceDesc.Height = 1;										// 上传堆资源都是存储一维线性资源，所以高度必须为 1
		CBVResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 上传堆资源的格式必须为 DXGI_FORMAT_UNKNOWN
		CBVResourceDesc.DepthOrArraySize = 1;							// 资源深度，这个是用于纹理数组和 3D 纹理的，上传堆资源必须为 1
		CBVResourceDesc.MipLevels = 1;									// Mipmap 等级，这个是用于纹理的，上传堆资源必须为 1
		CBVResourceDesc.SampleDesc.Count = 1;							// 资源采样次数，上传堆资源都是填 1

		// 上传堆属性的结构体，上传堆位于 CPU 和 GPU 的共享内存
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建常量缓冲资源
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &CBVResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_CBVResource));



		// 常量缓冲直接 Map 映射到结构体指针就行即可，无需 Unmap 关闭映射，Map-Unmap 是耗时操作，对于动态数据我们都只需要 Map 一次就行，然后直接对指针修改数据，这样就实现了常量缓冲数据的修改
		// 因为我们每帧都要变换 MVP 矩阵，每帧都要对 MVPBuffer 进行修改，所以我们直接将上传堆资源地址映射到结构体指针
		// 每帧直接对指针进行修改操作，不用再进行 Unmap 了，这样着色器每帧都能读取到修改后的数据了
		m_CBVResource->Map(0, nullptr, reinterpret_cast<void**>(&MVPBuffer));

	}


	// 创建根签名
	void STEP19_CreateRootSignature()
	{
		ComPtr<ID3DBlob> SignatureBlob;			// 根签名字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码，根签名创建失败时用 OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); 可以获取报错信息

		D3D12_ROOT_PARAMETER RootParameters[2] = {};							// 根参数数组

		// 把更新频率高的根参数放前面，低的放后面，可以优化性能 (微软官方文档建议)
		// 因为 DirectX API 能对根签名进行 Version Control 版本控制，在根签名越前面的根参数，访问速度更快

		// 第一个根参数：CBV 根描述符，根描述符是内联描述符，所以下文绑定根参数时，只需要传递常量缓冲资源的地址即可

		D3D12_ROOT_DESCRIPTOR CBVRootDescriptorDesc = {};					// 常量缓冲根描述符信息结构体
		CBVRootDescriptorDesc.ShaderRegister = 0;							// 要绑定的寄存器编号，这里对应 HLSL 的 b0 寄存器
		CBVRootDescriptorDesc.RegisterSpace = 0;							// 要绑定的命名空间，这里对应 HLSL 的 space0

		RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// 常量缓冲对整个渲染管线都可见
		RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// 根参数的类型：CBV 根描述符
		RootParameters[0].Descriptor = CBVRootDescriptorDesc;				// 填上文的结构体


		// 第二个根参数：根描述表 (Range: SRV)

		D3D12_DESCRIPTOR_RANGE SRVDescriptorRangeDesc = {};						// Range 描述符范围结构体，一块 Range 表示一堆连续的同类型描述符
		SRVDescriptorRangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;		// Range 类型，这里指定 SRV 类型，CBV_SRV_UAV 在这里分流
		SRVDescriptorRangeDesc.NumDescriptors = 1;								// Range 里面的描述符数量 N，一次可以绑定多个描述符到多个寄存器槽上
		SRVDescriptorRangeDesc.BaseShaderRegister = 0;							// Range 要绑定的起始寄存器槽编号 i，绑定范围是 [t(i),t(i+N)]，我们绑定 t0
		SRVDescriptorRangeDesc.RegisterSpace = 0;								// Range 要绑定的寄存器空间，整个 Range 都会绑定到同一寄存器空间上，我们绑定 space0
		SRVDescriptorRangeDesc.OffsetInDescriptorsFromTableStart = 0;			// Range 到根描述表开头的偏移量 (单位：描述符)，根签名需要用它来寻找 Range 的地址，我们这填 0 就行

		D3D12_ROOT_DESCRIPTOR_TABLE RootDescriptorTableDesc = {};				// RootDescriptorTable 根描述表信息结构体，一个 Table 可以有多个 Range
		RootDescriptorTableDesc.pDescriptorRanges = &SRVDescriptorRangeDesc;	// Range 描述符范围指针
		RootDescriptorTableDesc.NumDescriptorRanges = 1;						// 根描述表中 Range 的数量

		RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;				// 根参数在着色器中的可见性，这里指定仅在像素着色器可见 (只有像素着色器用到了纹理)
		RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// 根参数类型，这里我们选 Table 根描述表，一个根描述表占用 1 DWORD
		RootParameters[1].DescriptorTable = RootDescriptorTableDesc;					// 根参数指针



		D3D12_STATIC_SAMPLER_DESC StaticSamplerDesc = {};						// 静态采样器结构体，静态采样器不会占用根签名
		StaticSamplerDesc.ShaderRegister = 0;									// 要绑定的寄存器槽，对应 s0
		StaticSamplerDesc.RegisterSpace = 0;									// 要绑定的寄存器空间，对应 space0
		StaticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;		// 静态采样器在着色器中的可见性，这里指定仅在像素着色器可见 (只有像素着色器用到了纹理采样)
		StaticSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;	// 纹理过滤类型，这里我们直接选 邻近点采样 就行
		StaticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// 在 U 方向上的纹理寻址方式
		StaticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// 在 V 方向上的纹理寻址方式
		StaticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// 在 W 方向上的纹理寻址方式 (3D 纹理会用到)
		StaticSamplerDesc.MinLOD = 0;											// 最小 LOD 细节层次，这里我们默认填 0 就行
		StaticSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;							// 最大 LOD 细节层次，这里我们默认填 D3D12_FLOAT32_MAX (没有 LOD 上限)
		StaticSamplerDesc.MipLODBias = 0;										// 基础 Mipmap 采样偏移量，我们这里我们直接填 0 就行
		StaticSamplerDesc.MaxAnisotropy = 1;									// 各向异性过滤等级，我们不使用各向异性过滤，需要默认填 1
		StaticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;			// 这个是用于阴影贴图的，我们不需要用它，所以填 D3D12_COMPARISON_FUNC_NEVER


		D3D12_ROOT_SIGNATURE_DESC rootsignatureDesc = {};			// 根签名信息结构体，上限 64 DWORD，静态采样器不占用根签名
		rootsignatureDesc.NumParameters = 2;						// 根参数数量
		rootsignatureDesc.pParameters = RootParameters;				// 根参数指针
		rootsignatureDesc.NumStaticSamplers = 1;					// 静态采样器数量
		rootsignatureDesc.pStaticSamplers = &StaticSamplerDesc;		// 静态采样器指针
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
	void STEP20_CreatePSO()
	{
		// PSO 信息结构体
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

		// Input Assembler 输入装配阶段
		D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};			// 输入样式信息结构体
		D3D12_INPUT_ELEMENT_DESC InputElementDesc[2] = {};		// 输入元素信息结构体数组


		// 顶点位置 float4 position
		InputElementDesc[0].SemanticName = "POSITION";										// 要锚定的语义
		InputElementDesc[0].SemanticIndex = 0;												// 语义索引	
		InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;						// 输入格式
		InputElementDesc[0].InputSlot = 0;													// 输入槽编号	
		InputElementDesc[0].AlignedByteOffset = 0;											// 在输入槽中的偏移
		InputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;	// 输入流类型
		InputElementDesc[0].InstanceDataStepRate = 0;										// 实例数据步进率	


		// 纹理 UV 坐标 float2 texcoordUV
		InputElementDesc[1].SemanticName = "TEXCOORD";										// 要锚定的语义
		InputElementDesc[1].SemanticIndex = 0;												// 语义索引
		InputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;								// 输入格式
		InputElementDesc[1].InputSlot = 0;													// 输入槽编号
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

		// 设置深度测试状态
		PSODesc.DSVFormat = DSVFormat;											// 设置深度缓冲的格式
		PSODesc.DepthStencilState.DepthEnable = true;							// 开启深度缓冲
		PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// 深度缓冲的比较方式
		PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// 深度缓冲的读写权限

		// D3D12_DEPTH_WRITE_MASK_ALL	允许通过深度测试的像素写入深度缓冲			(深度缓冲可读写)
		// D3D12_DEPTH_WRITE_MASK_ZERO	禁止对深度缓冲进行写操作，但仍可进行深度测试	(深度缓冲只读)
		// 两者只能选一个，不可共存。指定 DepthWriteMask 可以控制深度数据的读写，实现某些特效

		/*
			深度测试比较像素深度的伪代码，符合条件就覆盖新像素，不符合就丢弃
			NewPixel:			要写入的新像素
			CurrentPixel:		当前在缓冲区的像素
			DepthFunc:			比较方式 (实际上就是 C/C++ 的二元操作运算符)

			if (NewPixel.Depth  DepthFunc  CurrentPixel.Depth)
			{
				Accept(NewPixel)			// 新像素通过深度测试，下一步可以进行混合
				WriteDepth(NewPixel.Depth)	// 将新像素深度写入深度缓冲中
			}
			else
			{
				Reject(NewPixel)			// 丢弃新像素
			}


			D3D12_COMPARISON_FUNC_LESS 相当于小于号 <

			if (NewPixel.Depth  <  CurrentPixel.Depth)
			{
				Accept(NewPixel)			// 如果新像素深度更小，说明距离摄像机更靠前，通过深度测试
				WriteDepth(NewPixel.Depth)	// 将新像素深度写入深度缓冲中
			}
			else
			{
				Reject(NewPixel)			// 否则，这个新像素更靠后，被当前像素遮住了，丢弃新像素
			}
		*/


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



	// ---------------------------------------------------------------------------------------------------------------



	// 更新常量缓冲区，将每帧新的 MVP 矩阵传递到常量缓冲区中，这样就能看到动态的 3D 画面了
	void UpdateConstantBuffer()
	{
		// 将更新后的矩阵，存储到共享内存上的常量缓冲，这样 GPU 就可以访问到 MVP 矩阵了
		XMStoreFloat4x4(&MVPBuffer->MVPMatrix, m_FirstCamera.GetMVPMatrix());
	}


	// 渲染
	void Render()
	{
		// 每帧渲染开始前，调用 UpdateConstantBuffer() 更新常量缓冲区
		UpdateConstantBuffer();

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

		// 用 RTV 句柄设置渲染目标，同时用 DSV 句柄设置深度模板缓冲，开启深度测试
		m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, &DSVHandle);

		// 清空后台的深度模板缓冲，将深度重置为初始值 1，记住上文创建深度缓冲资源的时候，要填 ClearValue
		// 否则会报 D3D12 WARNING: The application did not pass any clear value to resource creation.
		m_CommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

		// 清空当前渲染目标的背景为天蓝色
		m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);

		// 用于设置描述符堆用的临时 ID3D12DescriptorHeap 数组
		ID3D12DescriptorHeap* _temp_DescriptorHeaps[] = { m_SRVHeap.Get() };
		// 设置描述符堆
		m_CommandList->SetDescriptorHeaps(1, _temp_DescriptorHeaps);

		// 设置常量缓冲 (第一个根参数)，我们复制完数据到 CBVResource 后，就可以让着色器读取、对顶点进行 MVP 变换了
		m_CommandList->SetGraphicsRootConstantBufferView(0, m_CBVResource->GetGPUVirtualAddress());

		// 设置图元拓扑 (输入装配阶段)，我们这里设置三角形列表
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		// 设置 VBV，IBV 描述符
		m_CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		m_CommandList->IASetIndexBuffer(&IndexBufferView);


		// 渲染全部网格
		for (const auto& Mesh : MeshGroup)
		{
			// 设置纹理 SRV
			m_CommandList->SetGraphicsRootDescriptorTable(1, MaterialGroup[Mesh.MaterialIndex].GPUHandle);

			// 绘制网格
			m_CommandList->DrawIndexedInstanced(Mesh.IndexCount, 1, Mesh.IndexGroupOffset, Mesh.VertexGroupOffset, 0);
		}


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
	void STEP21_RenderLoop()
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



	// ---------------------------------------------------------------------------------------------------------------



	// 回调函数，处理窗口产生的消息
	// WASD 键 —— WM_CHAR 字符消息 —— 摄像机前后左右移动
	// 鼠标长按左键移动 —— WM_MOUSEMOVE 鼠标移动消息 —— 摄像机视角旋转
	// 关闭窗口 —— WM_DESTROY 窗口销毁消息 —— 窗口关闭，程序进程退出
	LRESULT CALLBACK CallBackFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// 用 switch 将第二个参数分流，每个 case 分别对应一个窗口消息
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
			case 'w':
			case 'W':	// 向前移动
				m_FirstCamera.Walk(0.2);
				break;

			case 's':
			case 'S':	// 向后移动
				m_FirstCamera.Walk(-0.2);
				break;

			case 'a':
			case 'A':	// 向左移动
				m_FirstCamera.Strafe(-0.2);
				break;

			case 'd':
			case 'D':	// 向右移动
				m_FirstCamera.Strafe(0.2);
				break;
			}
		}
		break;


		case WM_MOUSEMOVE:	// 获取鼠标移动消息
		{
			switch (wParam)	// wParam 是鼠标按键的状态
			{
			case MK_LBUTTON:	// 当用户长按鼠标左键的同时移动鼠标，摄像机旋转
				m_FirstCamera.CameraRotate();
				break;

			// 按键没按，鼠标只是移动也要更新，否则就会发生摄像机视角瞬移
			default: m_FirstCamera.UpdateLastCursorPos();
			}
		}
		break;


		// 如果接收到其他消息，直接默认返回整个窗口
		default: return DefWindowProc(hwnd, msg, wParam, lParam);

		}

		return 0;	// 注意这里！default 除外的分支都会运行到这里，因此需要 return 0，否则就会返回系统随机值，导致窗口无法正常显示
	}


	// 运行窗口
	static void Run(HINSTANCE hins)
	{
		DX12Engine engine;
		engine.STEP1_InitWindow(hins);
		engine.STEP2_CreateDebugDevice();
		engine.STEP3_CreateDevice();
		engine.STEP4_CreateCommandComponents();
		engine.STEP5_CreateRenderTarget();
		engine.STEP6_CreateFenceAndBarrier();
		engine.STEP7_CreateDSVHeap();
		engine.STEP8_CreateDepthStencilBuffer();
		engine.STEP9_CreateDSV();

		engine.STEP10_OpenModelFile();
		engine.STEP11_AddModelMaterials();
		engine.STEP12_AddModelData();
		engine.STEP13_CalcModelBoundingBox();

		engine.STEP14_CreateModelTextureResource();

		engine.STEP15_CreateMeshResourceAndDescriptor();
		engine.STEP16_CopyMeshToUploadResource();
		engine.STEP17_CopyMeshToDefaultResource();

		engine.STEP18_CreateCBVResource();
		engine.STEP19_CreateRootSignature();
		engine.STEP20_CreatePSO();

		engine.STEP21_RenderLoop();
	}
};


// 主函数
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}
