
// (8) AlphaBlend：用 DirectX 12 绘制玻璃等有透明像素的物体，初步了解透明测试/混合与渲染顺序的关系

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

#pragma comment(lib,"d3d12.lib")			// 链接 DX12 核心 DLL
#pragma comment(lib,"dxgi.lib")				// 链接 DXGI DLL
#pragma comment(lib,"dxguid.lib")			// 链接 DXGI 必要的设备 GUID
#pragma comment(lib,"d3dcompiler.lib")		// 链接 DX12 需要的着色器编译 DLL
#pragma comment(lib,"windowscodecs.lib")	// 链接 WIC DLL

using namespace Microsoft;
using namespace Microsoft::WRL;		// 使用 wrl.h 里面的命名空间，我们需要用到里面的 Microsoft::WRL::ComPtr COM智能指针
using namespace DirectX;			// DirectX 命名空间


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
	// 前 5 章我们一直将类里面的回调函数设置成 static 静态函数，是因为 WIN32 API 是用纯 C 风格写的
	// WNDCLASS 的 lpfnWndProc 是 C-Style 的函数指针，而 DX12Engine::CallBackFunc 是类成员函数，还需要传递一个 this 指针
	// 这个 this 指针还包含了类实例的额外信息 (类成员,虚函数表,类继承关系)，但是 lpfnWndProc 传不了这个 this 指针
	// 函数声明不兼容，所以没法直接将 DX12Engine::CallBackFunc 赋值给 lpfnWndProc，常规的强制转换都不行 (reinterpret_cast 也不行)
	// 我们可以利用 C++11 的函数包装器 std::function ，利用它来保存 DX12Engine::CallBackFunc
	// [利用的是模板+仿函数 (函数对象) 闭包的特性，感兴趣可以查查资料，简单了解一下。如果想深究可以看看源码，这玩意内部实现非常神奇]
	// 然后再通过下文的静态函数 CallBackWrapper::CallBackFunc 间接调用，将这个 CallBackWrapper::CallBackFunc 传给 lpfnWndProc
	// 这样就实现了 类回调函数 -> C-Style 普通回调函数 的转化
	// 用 C++17 的 inline static 原因是 static 静态非常量成员要求类内声明，类外定义，不做类外定义的话就会报函数链接错误
	// 类内的静态成员变量仅仅是一个声明，要定义的时候才分配内存，在定义之前都是不可访问的，所以静态非常量成员不能在类内初始化
	// [详情可以看：https://www.cnblogs.com/lixuejian/p/13215271]
	// 如果去掉 inline，就要在后面加上定义。而且如果遇到多文件编译，每个文件都要定义这玩意的时候，就会发生链接错误，非常麻烦
	// inline static 允许静态成员变量可以直接类内初始化，可以完美规避上面这两种麻烦情况，所以我们才使用它
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

	XMVECTOR EyePosition = XMVectorSet(4, 5, -4, 1);		// 摄像机在世界空间下的位置
	XMVECTOR FocusPosition = XMVectorSet(4, 3, 4, 1);		// 摄像机在世界空间下观察的焦点位置
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
	float AspectRatio = 4.0 / 3.0;							// 投影窗口宽高比
	float NearZ = 0.1;										// 近平面到原点的距离
	float FarZ = 1000;										// 远平面到原点的距离

	XMMATRIX ViewMatrix;									// 观察矩阵，世界空间 -> 观察空间
	XMMATRIX ProjectionMatrix;								// 投影矩阵，观察空间 -> 齐次裁剪空间

	XMMATRIX MVPMatrix;										// MVP 矩阵，类外需要用公有方法 GetMVPMatrix 获取

public:

	Camera()	// 摄像机的构造函数
	{
		// 注意！我们这里移除了模型矩阵！每个模型会指定具体的模型矩阵！

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
		MVPMatrix = ViewMatrix * ProjectionMatrix;
	}

	// 获取 MVP 矩阵
	inline XMMATRIX& GetMVPMatrix()
	{
		// 每次返回前，都更新一次
		UpdateMVPMatrix();
		return MVPMatrix;
	}
};



// 顶点
struct VERTEX
{
	XMFLOAT4 position;			// 顶点在模型坐标系的坐标
	XMFLOAT2 texcoordUV;		// 顶点纹理 UV 坐标
};


// 模型类，这是个抽象类，有两个纯虚函数，派生类需要实现下面两个纯虚函数才能创建实例
class Model
{
protected:	// 保护项，仅在自身和派生类可见

	XMMATRIX ModelMatrix = XMMatrixIdentity();			// 模型矩阵，模型空间 -> 世界空间

	ComPtr<ID3D12Resource> m_VertexResource;			// D3D12 顶点资源
	ComPtr<ID3D12Resource> m_ModelMatrixResource;		// D3D12 模型矩阵资源
	ComPtr<ID3D12Resource> m_IndexResource;				// D3D12 索引资源

	// 每个模型的 VBV 顶点信息描述符数组，数组每个元素占用一个输入槽，多槽输入可以加速 CPU-GPU 的传递
	// VertexBufferView[0] 描述每个顶点的顶点信息 (position 位置，texcoordUV 纹理 UV 坐标)
	// VertexBufferView[1] 描述每个顶点对应的模型矩阵，模型矩阵会在 IA 阶段拆分成四个行向量进行输入，之后在 VS 阶段重新组装成矩阵
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView[2] = {};

	// 每个模型的 IBV 顶点索引描述符，一个模型只有一个索引描述符
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};

	// 纹理名 - GPU 句柄映射表，用于索引纹理，设置根参数
	std::unordered_map<std::string, D3D12_GPU_DESCRIPTOR_HANDLE> Texture_GPUHandle_Map;

	// 添加新纹理 (key) 到映射表，对应的 GPUHandle 先设置为 nullptr，仅限派生类可用，对外不公开
	void AppendTextureKey(std::string&& TextureName)
	{
		Texture_GPUHandle_Map[TextureName] = {};
	}

public:		// 公共项，全局均可见

	// 类外获取模型需要的纹理，返回映射表的只读引用
	const std::unordered_map<std::string, D3D12_GPU_DESCRIPTOR_HANDLE>& RequestForTextureMap()
	{
		return Texture_GPUHandle_Map;
	}

	// 模型获取类外已经创建纹理 SRV 描述符的 SRVHandle
	void SetTextureGPUHandle(std::string TextureName, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
	{
		Texture_GPUHandle_Map[TextureName] = GPUHandle;
	}

	// 获取模型矩阵
	XMMATRIX GetModelMatrix()
	{
		return ModelMatrix;
	}

	// 设置模型矩阵
	void SetModelMatrix(XMMATRIX Matrix)
	{
		ModelMatrix = Matrix;
	}

	// 创建资源与描述符，这个是纯虚函数，实例类需要实现
	virtual void CreateResourceAndDescriptor(ComPtr<ID3D12Device4>& pD3D12Device) = 0;

	// 绘制模型，这个也是纯虚函数，实例类需要实现
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) = 0;

};


// 全遮挡固体方块类 (抽象类)，继承自模型类，只定义 CreateResourceAndDescriptor 这个函数，DrawModel 仍然需要派生类实现
class SoildBlock : public Model
{
protected:	// 保护项，仅在自身和派生类可见

	// CPU 高速缓存上的顶点信息数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意 DirectX 使用的是左手坐标系，写顶点信息时，请比一比你的左手！
	inline static VERTEX VertexArray[24] =
	{
		// 正面
		{{0,1,0,1},{0,0}},
		{{1,1,0,1},{1,0}},
		{{1,0,0,1},{1,1}},
		{{0,0,0,1},{0,1}},

		// 背面
		{{1,1,1,1},{0,0}},
		{{0,1,1,1},{1,0}},
		{{0,0,1,1},{1,1}},
		{{1,0,1,1},{0,1}},

		// 左面
		{{0,1,1,1},{0,0}},
		{{0,1,0,1},{1,0}},
		{{0,0,0,1},{1,1}},
		{{0,0,1,1},{0,1}},

		// 右面
		{{1,1,0,1},{0,0}},
		{{1,1,1,1},{1,0}},
		{{1,0,1,1},{1,1}},
		{{1,0,0,1},{0,1}},

		// 上面
		{{0,1,1,1},{0,0}},
		{{1,1,1,1},{1,0}},
		{{1,1,0,1},{1,1}},
		{{0,1,0,1},{0,1}},

		// 下面
		{{0,0,0,1},{0,0}},
		{{1,0,0,1},{1,0}},
		{{1,0,1,1},{1,1}},
		{{0,0,1,1},{0,1}}
	};

	// 顶点索引数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意这里的 UINT == UINT32，后面填的格式 (步长) 必须是 DXGI_FORMAT_R32_UINT，否则会出错
	inline static UINT IndexArray[36] =
	{
		// 正面
		0,1,2,0,2,3,
		// 背面
		4,5,6,4,6,7,
		// 左面
		8,9,10,8,10,11,
		// 右面
		12,13,14,12,14,15,
		// 上面
		16,17,18,16,18,19,
		// 下面
		20,21,22,20,22,23
	};


public:		// 公共项，全局均可见

	// 创建资源与描述符，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void CreateResourceAndDescriptor(ComPtr<ID3D12Device4>& pD3D12Device) override
	{
		// 临时设置 XMFLOAT4X4 类型的模型矩阵，XMFLOAT4X4 擅长存储与传递，XMMATRIX 擅长并行运算
		XMFLOAT4X4 _temp_ModelMatrix = {};
		XMStoreFloat4x4(&_temp_ModelMatrix, ModelMatrix);

		// 用于批量复制模型矩阵的 vector，vector 的底层是一块连续内存，memcpy 复制连续内存有 CPU 优化，能快很多
		std::vector<XMFLOAT4X4> _temp_ModelMatrixGroup;
		// 批量填充 ModelMatrix 到 ModelMatrixGroup
		_temp_ModelMatrixGroup.assign(24, _temp_ModelMatrix);


		// 用于创建上传堆资源的 D3D12Resource 信息结构体，这个结构体可复用
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 上传堆资源类型都是 BUFFER 缓冲
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// row_major 行主序，上传堆资源都是按行存储的
		UploadResourceDesc.Height = 1;										// 上传堆资源高度都是 1 (线性存储)
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 上传堆资源都是 DXGI_FORMAT_UNKNOWN
		UploadResourceDesc.MipLevels = 1;									// 上传堆资源没有 Mipmap，所以都设 1
		UploadResourceDesc.DepthOrArraySize = 1;							// 上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Count = 1;							// 采样次数，上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Quality = 0;							// 采样质量，上传堆资源都设 0

		// 上传堆属性
		D3D12_HEAP_PROPERTIES HeapProperties = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建 VertexResource
		UploadResourceDesc.Width = 24 * sizeof(VERTEX);		// 宽度就是 VertexGroup 总元素大小

		// 以隐式堆方式创建资源，好处是简单方便，坏处是隐式堆由操作系统全权管理，开发者无法手动管理隐式堆的属性和生命周期
		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexResource));

		// 创建 ModelMatrix
		UploadResourceDesc.Width = 24 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ModelMatrixResource));

		// 创建 IndexResource
		UploadResourceDesc.Width = 36 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexResource));


		// 将数据进行转移: CPU 高速缓存 -> CPU 共享内存

		BYTE* TransmitPointer = nullptr;	// 用于传递数据的指针

		// 映射资源，获取 D3D12 资源的地址，同时 D3D12 资源开放写权限
		m_VertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		// 用 memcpy 将数据复制到 D3D12 资源
		memcpy(TransmitPointer, VertexArray, 24 * sizeof(VERTEX));
		// 关闭映射，资源只写，这样静态资源读取效率会快很多，动态资源无需关闭映射
		m_VertexResource->Unmap(0, nullptr);


		m_ModelMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, &_temp_ModelMatrixGroup[0], 24 * sizeof(XMFLOAT4X4));
		m_ModelMatrixResource->Unmap(0, nullptr);


		m_IndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, IndexArray, 36 * sizeof(UINT));
		m_IndexResource->Unmap(0, nullptr);


		// 创建完所有资源，就可以填写 VBV 和 IBV 描述符了

		// VBV[0]: 描述顶点位置与纹理 UV 坐标，占据第 0 号输入槽
		VertexBufferView[0].BufferLocation = m_VertexResource->GetGPUVirtualAddress();		// D3D12 资源地址
		VertexBufferView[0].SizeInBytes = 24 * sizeof(VERTEX);								// D3D12 资源总大小
		VertexBufferView[0].StrideInBytes = sizeof(VERTEX);									// D3D12 资源单个元素的大小 (步长)

		// VBV[1]: 描述顶点的模型矩阵，占据第 1 号输入槽
		VertexBufferView[1].BufferLocation = m_ModelMatrixResource->GetGPUVirtualAddress();	// D3D12 资源地址
		VertexBufferView[1].SizeInBytes = 24 * sizeof(XMFLOAT4X4);							// D3D12 资源总大小
		VertexBufferView[1].StrideInBytes = sizeof(XMFLOAT4X4);								// D3D12 资源单个元素的大小 (步长)

		// IBV: 描述顶点的索引
		IndexBufferView.BufferLocation = m_IndexResource->GetGPUVirtualAddress();			// D3D12 资源地址
		IndexBufferView.SizeInBytes = 36 * sizeof(UINT);									// D3D12 资源总大小
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;										// D3D12 资源单个元素的大小 (步长)
	}
};


// 台阶方块类 (抽象类)，继承自模型类，只定义 CreateResourceAndDescriptor 这个函数，DrawModel 仍然需要派生类实现
class SoildStair : public Model
{
protected:	// 保护项，仅在自身和派生类可见

	// CPU 高速缓存上的顶点信息数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意 DirectX 使用的是左手坐标系，写顶点信息时，请比一比你的左手！
	inline static VERTEX VertexArray[40] =
	{
		// 台阶底面
		{{0,0,0,1},{0,0}},
		{{1,0,0,1},{1,0}},
		{{1,0,1,1},{1,1}},
		{{0,0,1,1},{0,1}},

		// 台阶背面
		{{1,1,1,1},{0,0}},
		{{0,1,1,1},{1,0}},
		{{0,0,1,1},{1,1}},
		{{1,0,1,1},{0,1}},

		// 台阶正面
		{{0,0.5,0,1},{0,0.5}},
		{{1,0.5,0,1},{1,0.5}},
		{{1,0,0,1},{1,1}},
		{{0,0,0,1},{0,1}},

		{{0,1,0.5,1},{0,0}},
		{{1,1,0.5,1},{1,0}},
		{{1,0.5,0.5,1},{1,0.5}},
		{{0,0.5,0.5,1},{0,0.5}},

		// 台阶顶面
		{{0,0.5,0.5,1},{0,0.5}},
		{{1,0.5,0.5,1},{1,0.5}},
		{{1,0.5,0,1},{1,1}},
		{{0,0.5,0,1},{0,1}},

		{{0,1,1,1},{0,0}},
		{{1,1,1,1},{1,0}},
		{{1,1,0.5,1},{1,0.5}},
		{{0,1,0.5,1},{0,0.5}},

		// 台阶左面
		{{0,1,1,1},{0,0}},
		{{0,1,0.5,1},{0.5,0}},
		{{0,0,0.5,1},{0.5,1}},
		{{0,0,1,1},{0,1}},

		{{0,0.5,0.5,1},{0.5,0.5}},
		{{0,0.5,0,1},{1,0.5}},
		{{0,0,0,1},{1,1}},
		{{0,0,0.5,1},{0.5,1}},

		// 台阶右面
		{{1,1,0.5,1},{0.5,0}},
		{{1,1,1,1},{1,0}},
		{{1,0,1,1},{1,1}},
		{{1,0,0.5,1},{0.5,1}},

		{{1,0.5,0,1},{0,0.5}},
		{{1,0.5,0.5,1},{0.5,0.5}},
		{{1,0,0.5,1},{0.5,1}},
		{{1,0,0,1},{0,1}}
	};

	// 顶点索引数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意这里的 UINT == UINT32，后面填的格式 (步长) 必须是 DXGI_FORMAT_R32_UINT，否则会出错
	inline static UINT IndexArray[60] =
	{
		// 台阶底面
		0,1,2,0,2,3,
		// 台阶背面
		4,5,6,4,6,7,
		// 台阶正面
		8,9,10,8,10,11,
		12,13,14,12,14,15,
		// 台阶顶面
		16,17,18,16,18,19,
		20,21,22,20,22,23,
		// 台阶左面
		24,25,26,24,26,27,
		28,29,30,28,30,31,
		// 台阶右面
		32,33,34,32,34,35,
		36,37,38,36,38,39
	};


public:		// 公共项，全局均可见

	// 创建资源与描述符，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void CreateResourceAndDescriptor(ComPtr<ID3D12Device4>& pD3D12Device) override
	{
		// 临时设置 XMFLOAT4X4 类型的模型矩阵，XMFLOAT4X4 擅长存储与传递，XMMATRIX 擅长并行运算
		XMFLOAT4X4 _temp_ModelMatrix = {};
		XMStoreFloat4x4(&_temp_ModelMatrix, ModelMatrix);

		// 用于批量复制模型矩阵的 vector，vector 的底层是一块连续内存，memcpy 复制连续内存有 CPU 优化，能快很多
		std::vector<XMFLOAT4X4> _temp_ModelMatrixGroup;
		// 批量填充 ModelMatrix 到 ModelMatrixGroup
		_temp_ModelMatrixGroup.assign(40, _temp_ModelMatrix);


		// 用于创建上传堆资源的 D3D12Resource 信息结构体，这个结构体可复用
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 上传堆资源类型都是 BUFFER 缓冲
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// row_major 行主序，上传堆资源都是按行存储的
		UploadResourceDesc.Height = 1;										// 上传堆资源高度都是 1 (线性存储)
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 上传堆资源都是 DXGI_FORMAT_UNKNOWN
		UploadResourceDesc.MipLevels = 1;									// 上传堆资源没有 Mipmap，所以都设 1
		UploadResourceDesc.DepthOrArraySize = 1;							// 上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Count = 1;							// 采样次数，上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Quality = 0;							// 采样质量，上传堆资源都设 0

		// 上传堆属性
		D3D12_HEAP_PROPERTIES HeapProperties = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建 VertexResource
		UploadResourceDesc.Width = 40 * sizeof(VERTEX);		// 宽度就是 VertexGroup 总元素大小

		// 以隐式堆方式创建资源，好处是简单方便，坏处是隐式堆由操作系统全权管理，开发者无法手动管理隐式堆的属性和生命周期
		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexResource));

		// 创建 ModelMatrix
		UploadResourceDesc.Width = 40 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ModelMatrixResource));

		// 创建 IndexResource
		UploadResourceDesc.Width = 60 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexResource));


		// 将数据进行转移: CPU 高速缓存 -> CPU 共享内存

		BYTE* TransmitPointer = nullptr;	// 用于传递数据的指针

		// 映射资源，获取 D3D12 资源的地址，同时 D3D12 资源开放写权限
		m_VertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		// 用 memcpy 将数据复制到 D3D12 资源
		memcpy(TransmitPointer, VertexArray, 40 * sizeof(VERTEX));
		// 关闭映射，资源只写，这样静态资源读取效率会快很多，动态资源无需关闭映射
		m_VertexResource->Unmap(0, nullptr);


		m_ModelMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, &_temp_ModelMatrixGroup[0], 40 * sizeof(XMFLOAT4X4));
		m_ModelMatrixResource->Unmap(0, nullptr);


		m_IndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, IndexArray, 60 * sizeof(UINT));
		m_IndexResource->Unmap(0, nullptr);


		// 创建完所有资源，就可以填写 VBV 和 IBV 描述符了

		// VBV[0]: 描述顶点位置与纹理 UV 坐标，占据第 0 号输入槽
		VertexBufferView[0].BufferLocation = m_VertexResource->GetGPUVirtualAddress();		// D3D12 资源地址
		VertexBufferView[0].SizeInBytes = 40 * sizeof(VERTEX);								// D3D12 资源总大小
		VertexBufferView[0].StrideInBytes = sizeof(VERTEX);									// D3D12 资源单个元素的大小 (步长)

		// VBV[1]: 描述顶点的模型矩阵，占据第 1 号输入槽
		VertexBufferView[1].BufferLocation = m_ModelMatrixResource->GetGPUVirtualAddress();	// D3D12 资源地址
		VertexBufferView[1].SizeInBytes = 40 * sizeof(XMFLOAT4X4);							// D3D12 资源总大小
		VertexBufferView[1].StrideInBytes = sizeof(XMFLOAT4X4);								// D3D12 资源单个元素的大小 (步长)

		// IBV: 描述顶点的索引
		IndexBufferView.BufferLocation = m_IndexResource->GetGPUVirtualAddress();			// D3D12 资源地址
		IndexBufferView.SizeInBytes = 60 * sizeof(UINT);									// D3D12 资源总大小
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;										// D3D12 资源单个元素的大小 (步长)
	}
};


// 泥土 (实例类)，继承自全遮挡固体方块
class Dirt : public SoildBlock
{
public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Dirt()
	{
		this->AppendTextureKey("dirt");
	}

	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["dirt"]);

		// Draw Call 渲染！
		pCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
	}
};


// 橡木木板 (实例类)，继承自全遮挡固体方块
class Planks_Oak : public SoildBlock
{
public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Planks_Oak()
	{
		this->AppendTextureKey("planks_oak");
	}

	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["planks_oak"]);

		// Draw Call 渲染！
		pCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
	}
};


// 熔炉 (实例类)，继承自全遮挡固体方块
class Furnace : public SoildBlock
{
public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Furnace()
	{
		this->AppendTextureKey("furnace_front_off");
		this->AppendTextureKey("furnace_side");
		this->AppendTextureKey("furnace_top");
	}

	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		// 要更换纹理，可以通过 SetGraphicsRootDescriptorTable 改变根参数绑定的 GPUHandle

		// 渲染上下面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["furnace_top"]);
		pCommandList->DrawIndexedInstanced(12, 1, 24, 0, 0);

		// 渲染左右背面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["furnace_side"]);
		pCommandList->DrawIndexedInstanced(18, 1, 6, 0, 0);

		// 渲染正面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["furnace_front_off"]);
		pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}
};


// 工作台 (实例类)，继承自全遮挡固体方块
class Crafting_Table : public SoildBlock
{
public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Crafting_Table()
	{
		this->AppendTextureKey("crafting_table_front");
		this->AppendTextureKey("crafting_table_side");
		this->AppendTextureKey("crafting_table_top");
	}

	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		// 要更换纹理，可以通过 SetGraphicsRootDescriptorTable 改变根参数绑定的 GPUHandle

		// 渲染上下面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["crafting_table_top"]);
		pCommandList->DrawIndexedInstanced(12, 1, 24, 0, 0);

		// 渲染左右背面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["crafting_table_side"]);
		pCommandList->DrawIndexedInstanced(18, 1, 6, 0, 0);

		// 渲染正面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["crafting_table_front"]);
		pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}
};


// 橡树原木 (实例类)，继承自全遮挡固体方块
class Log_Oak : public SoildBlock
{
public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Log_Oak()
	{
		this->AppendTextureKey("log_oak");
		this->AppendTextureKey("log_oak_top");
	}

	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		// 要更换纹理，可以通过 SetGraphicsRootDescriptorTable 改变根参数绑定的 GPUHandle

		// 渲染上下面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["log_oak_top"]);
		pCommandList->DrawIndexedInstanced(12, 1, 24, 0, 0);

		// 渲染左右正背面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["log_oak"]);
		pCommandList->DrawIndexedInstanced(24, 1, 0, 0, 0);
	}
};


// 草方块 (实例类)，继承自全遮挡固体方块
class Grass : public SoildBlock
{
public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Grass()
	{
		this->AppendTextureKey("grass_side");
		this->AppendTextureKey("grass_top");
		this->AppendTextureKey("dirt");
	}

	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		// 要更换纹理，可以通过 SetGraphicsRootDescriptorTable 改变根参数绑定的 GPUHandle

		// 渲染上面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["grass_top"]);
		pCommandList->DrawIndexedInstanced(6, 1, 24, 0, 0);

		// 渲染下面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["dirt"]);
		pCommandList->DrawIndexedInstanced(6, 1, 30, 0, 0);

		// 渲染左右正背面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["grass_side"]);
		pCommandList->DrawIndexedInstanced(24, 1, 0, 0, 0);
	}
};


// 橡木完整台阶 (实例类)，继承自完整台阶方块
class Planks_Oak_SoildStair : public SoildStair
{
public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Planks_Oak_SoildStair()
	{
		this->AppendTextureKey("planks_oak");
	}

	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 渲染
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["planks_oak"]);
		pCommandList->DrawIndexedInstanced(60, 1, 0, 0, 0);
	}
};


// 淡蓝色玻璃 (实例类)，继承自全遮挡固体方块
class Glass_Light_Blue : public SoildBlock
{
public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Glass_Light_Blue()
	{
		this->AppendTextureKey("glass_light_blue");
	}

	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		// 要更换纹理，可以通过 SetGraphicsRootDescriptorTable 改变根参数绑定的 GPUHandle

		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["glass_light_blue"]);

		pCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
	}
};


// 门 (实例类)，继承自模型类
class Door : public Model
{
protected:

	// CPU 高速缓存上的顶点信息数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意 DirectX 使用的是左手坐标系，写顶点信息时，请比一比你的左手！
	inline static VERTEX VertexArray[96] =
	{
		// 门上部分
		{{0,2,0,1},{0,0}},
		{{1,2,0,1},{1,0}},
		{{1,1,0,1},{1,1}},
		{{0,1,0,1},{0,1}},

		{{1,2,0.2,1},{1,0}},
		{{0,2,0.2,1},{0,0}},
		{{0,1,0.2,1},{0,1}},
		{{1,1,0.2,1},{1,1}},

		// 门下部分
		{{0,1,0,1},{0,0}},
		{{1,1,0,1},{1,0}},
		{{1,0,0,1},{1,1}},
		{{0,0,0,1},{0,1}},

		{{1,1,0.2,1},{0,0}},
		{{0,1,0.2,1},{1,0}},
		{{0,0,0.2,1},{1,1}},
		{{1,0,0.2,1},{0,1}},

		// 门隙部分
		{{0,2,0.2,1},{0.1875, 0.4375}},
		{{1,2,0.2,1},{0.1875, 0.4375}},
		{{1,2,0,1},{0.1875, 0.4375}},
		{{0,2,0,1},{0.1875, 0.4375}},

		{{0,0,0,1},{0.1875, 0.4375}},
		{{1,0,0,1},{0.1875, 0.4375}},
		{{1,0,0.2,1},{0.1875, 0.4375}},
		{{0,0,0.2,1},{0.1875, 0.4375}},

		{{0,2,0.2,1},{0.1875, 0.4375}},
		{{0,2,0,1},{0.1875, 0.4375}},
		{{0,0,0,1},{0.1875, 0.4375}},
		{{0,0,0.2,1},{0.1875, 0.4375}},

		{{1,2,0,1},{0.1875, 0.4375}},
		{{1,2,0.2,1},{0.1875, 0.4375}},
		{{1,0,0.2,1},{0.1875, 0.4375}},
		{{1,0,0,1},{0.1875, 0.4375}},

		// 上部分门隙

		{{0.1875, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.3125, 0, 1},{0.1875, 0.4375}},
		{{0.1875, 1.3125, 0, 1},{0.1875, 0.4375}},

		{{0.1875, 1.5, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.5, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.5, 0, 1},{0.1875, 0.4375}},
		{{0.1875, 1.5, 0, 1},{0.1875, 0.4375}},

		{{0.1875, 1.5, 0, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.5, 0.2, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.3125, 0.2, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.3125, 0, 1}, {0.1875, 0.4375}},
		
		{{0.4375, 1.5, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.5, 0, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.3125, 0, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.3125, 0.2, 1}, {0.1875, 0.4375}},


		{{0.1875, 1.625, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.625, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.625, 0, 1},{0.1875, 0.4375}},
		{{0.1875, 1.625, 0, 1}, {0.1875, 0.4375}},

		{{0.1875, 1.8125, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.8125, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.8125, 0, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.8125, 0, 1}, {0.1875, 0.4375}},

		{{0.1875, 1.8125, 0, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.8125, 0.2, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.625, 0.2, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.625, 0, 1}, {0.1875, 0.4375}},

		{{0.4375, 1.8125, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.8125, 0, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.625, 0, 1},{0.1875, 0.4375}},
		{{0.4375, 1.625, 0.2, 1},{0.1875, 0.4375}},


		{{0.5625, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.8125, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.8125, 1.3125, 0, 1}, {0.1875, 0.4375}},
		{{0.5625, 1.3125, 0, 1}, {0.1875, 0.4375}},

		{{0.5625, 1.5, 0.2, 1}, {0.1875, 0.4375}},
		{{0.8125, 1.5, 0.2, 1}, {0.1875, 0.4375}},
		{{0.8125, 1.5, 0, 1},{0.1875, 0.4375}},
		{{0.5625, 1.5, 0, 1},{0.1875, 0.4375}},

		{{0.5625, 1.5, 0, 1},{0.1875, 0.4375}},
		{{0.5625, 1.5, 0.2, 1},{0.1875, 0.4375}},
		{{0.5625, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.5625, 1.3125, 0, 1},{0.1875, 0.4375} },

		{{0.8125, 1.5, 0.2, 1},{0.1875, 0.4375} },
		{{0.8125, 1.5, 0, 1},{0.1875, 0.4375} },
		{{0.8125, 1.3125, 0, 1},{0.1875, 0.4375} },
		{{0.8125, 1.3125, 0.2, 1},{0.1875, 0.4375} },


		{ {0.5625, 1.625, 0.2, 1},{0.1875, 0.4375} },
		{ {0.8125, 1.625, 0.2, 1},{0.1875, 0.4375} },
		{ {0.8125, 1.625, 0, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.625, 0, 1}, {0.1875, 0.4375} },

		{ {0.5625, 1.8125, 0.2, 1}, {0.1875, 0.4375} },
		{ {0.8125, 1.8125, 0.2, 1}, {0.1875, 0.4375} },
		{ {0.8125, 1.8125, 0, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.8125, 0, 1}, {0.1875, 0.4375} },

		{ {0.5625, 1.8125, 0, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.8125, 0.2, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.625, 0.2, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.625, 0, 1}, {0.1875, 0.4375} },

		{ {0.8125, 1.8125, 0.2, 1},{0.1875, 0.4375} },
		{ {0.8125, 1.8125, 0, 1},{0.1875, 0.4375} },
		{ {0.8125, 1.625, 0, 1}, {0.1875, 0.4375} },
		{ {0.8125, 1.625, 0.2, 1}, {0.1875, 0.4375} }
	};

	// 顶点索引数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意这里的 UINT == UINT32，后面填的格式 (步长) 必须是 DXGI_FORMAT_R32_UINT，否则会出错
	inline static UINT IndexArray[144] =
	{
		// 门上部分
		0,1,2,0,2,3,
		4,5,6,4,6,7,

		// 门下部分
		8,9,10,8,10,11,
		12,13,14,12,14,15,

		// 门隙部分
		16,17,18,16,18,19,
		20,21,22,20,22,23,
		24,25,26,24,26,27,
		28,29,30,28,30,31,

		// 上部分门隙
		32,33,34,32,34,35,
		36,37,38,36,38,39,
		40,41,42,40,42,43,
		44,45,46,44,46,47,

		48,49,50,48,50,51,
		52,53,54,52,54,55,
		56,57,58,56,58,59,
		60,61,62,60,62,63,

		64,65,66,64,66,67,
		68,69,70,68,70,71,
		72,73,74,72,74,75,
		76,77,78,76,78,79,

		80,81,82,80,82,83,
		84,85,86,84,86,87,
		88,89,90,88,90,91,
		92,93,94,92,94,95
	};

public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Door()
	{
		this->AppendTextureKey("door_wood_lower");
		this->AppendTextureKey("door_wood_upper");
	}

	// 创建资源与描述符，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void CreateResourceAndDescriptor(ComPtr<ID3D12Device4>& pD3D12Device) override
	{
		// 临时设置 XMFLOAT4X4 类型的模型矩阵，XMFLOAT4X4 擅长存储与传递，XMMATRIX 擅长并行运算
		XMFLOAT4X4 _temp_ModelMatrix = {};
		XMStoreFloat4x4(&_temp_ModelMatrix, ModelMatrix);

		// 用于批量复制模型矩阵的 vector，vector 的底层是一块连续内存，memcpy 复制连续内存有 CPU 优化，能快很多
		std::vector<XMFLOAT4X4> _temp_ModelMatrixGroup;
		// 批量填充 ModelMatrix 到 ModelMatrixGroup
		_temp_ModelMatrixGroup.assign(96, _temp_ModelMatrix);


		// 用于创建上传堆资源的 D3D12Resource 信息结构体，这个结构体可复用
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 上传堆资源类型都是 BUFFER 缓冲
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// row_major 行主序，上传堆资源都是按行存储的
		UploadResourceDesc.Height = 1;										// 上传堆资源高度都是 1 (线性存储)
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 上传堆资源都是 DXGI_FORMAT_UNKNOWN
		UploadResourceDesc.MipLevels = 1;									// 上传堆资源没有 Mipmap，所以都设 1
		UploadResourceDesc.DepthOrArraySize = 1;							// 上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Count = 1;							// 采样次数，上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Quality = 0;							// 采样质量，上传堆资源都设 0

		// 上传堆属性
		D3D12_HEAP_PROPERTIES HeapProperties = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建 VertexResource
		UploadResourceDesc.Width = 96 * sizeof(VERTEX);		// 宽度就是 VertexGroup 总元素大小

		// 以隐式堆方式创建资源，好处是简单方便，坏处是隐式堆由操作系统全权管理，开发者无法手动管理隐式堆的属性和生命周期
		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexResource));

		// 创建 ModelMatrix
		UploadResourceDesc.Width = 96 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ModelMatrixResource));

		// 创建 IndexResource
		UploadResourceDesc.Width = 144 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexResource));


		// 将数据进行转移: CPU 高速缓存 -> CPU 共享内存

		BYTE* TransmitPointer = nullptr;	// 用于传递数据的指针

		// 映射资源，获取 D3D12 资源的地址，同时 D3D12 资源开放写权限
		m_VertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		// 用 memcpy 将数据复制到 D3D12 资源
		memcpy(TransmitPointer, VertexArray, 96 * sizeof(VERTEX));
		// 关闭映射，资源只写，这样静态资源读取效率会快很多，动态资源无需关闭映射
		m_VertexResource->Unmap(0, nullptr);


		m_ModelMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, &_temp_ModelMatrixGroup[0], 96 * sizeof(XMFLOAT4X4));
		m_ModelMatrixResource->Unmap(0, nullptr);


		m_IndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, IndexArray, 144 * sizeof(UINT));
		m_IndexResource->Unmap(0, nullptr);


		// 创建完所有资源，就可以填写 VBV 和 IBV 描述符了

		// VBV[0]: 描述顶点位置与纹理 UV 坐标，占据第 0 号输入槽
		VertexBufferView[0].BufferLocation = m_VertexResource->GetGPUVirtualAddress();		// D3D12 资源地址
		VertexBufferView[0].SizeInBytes = 96 * sizeof(VERTEX);								// D3D12 资源总大小
		VertexBufferView[0].StrideInBytes = sizeof(VERTEX);									// D3D12 资源单个元素的大小 (步长)

		// VBV[1]: 描述顶点的模型矩阵，占据第 1 号输入槽
		VertexBufferView[1].BufferLocation = m_ModelMatrixResource->GetGPUVirtualAddress();	// D3D12 资源地址
		VertexBufferView[1].SizeInBytes = 96 * sizeof(XMFLOAT4X4);							// D3D12 资源总大小
		VertexBufferView[1].StrideInBytes = sizeof(XMFLOAT4X4);								// D3D12 资源单个元素的大小 (步长)

		// IBV: 描述顶点的索引
		IndexBufferView.BufferLocation = m_IndexResource->GetGPUVirtualAddress();			// D3D12 资源地址
		IndexBufferView.SizeInBytes = 144 * sizeof(UINT);									// D3D12 资源总大小
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;										// D3D12 资源单个元素的大小 (步长)
	}


	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		// 要更换纹理，可以通过 SetGraphicsRootDescriptorTable 改变根参数绑定的 GPUHandle

		// 门上部分
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["door_wood_upper"]);
		pCommandList->DrawIndexedInstanced(12, 1, 0, 0, 0);

		// 门下部分
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["door_wood_lower"]);
		pCommandList->DrawIndexedInstanced(12, 1, 12, 0, 0);

		// 门隙部分
		pCommandList->DrawIndexedInstanced(24, 1, 24, 0, 0);

		// 上门隙部分
		pCommandList->DrawIndexedInstanced(96, 1, 48, 0, 0);
	}

};


// 门的镜像版本 (实例类)，继承自门
class Mirror_Door : public Door
{
protected:

	// CPU 高速缓存上的顶点信息数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意 DirectX 使用的是左手坐标系，写顶点信息时，请比一比你的左手！
	inline static VERTEX VertexArray[96] =
	{
		// 门上部分
		{{0,2,0,1},{1,0}},
		{{1,2,0,1},{0,0}},
		{{1,1,0,1},{0,1}},
		{{0,1,0,1},{1,1}},

		{{1,2,0.2,1},{0,0}},
		{{0,2,0.2,1},{1,0}},
		{{0,1,0.2,1},{1,1}},
		{{1,1,0.2,1},{0,1}},

		// 门下部分
		{{0,1,0,1},{0,0}},
		{{1,1,0,1},{1,0}},
		{{1,0,0,1},{1,1}},
		{{0,0,0,1},{0,1}},

		{{1,1,0.2,1},{0,0}},
		{{0,1,0.2,1},{1,0}},
		{{0,0,0.2,1},{1,1}},
		{{1,0,0.2,1},{0,1}},

		// 门隙部分
		{{0,2,0.2,1},{0.1875, 0.4375}},
		{{1,2,0.2,1},{0.1875, 0.4375}},
		{{1,2,0,1},{0.1875, 0.4375}},
		{{0,2,0,1},{0.1875, 0.4375}},

		{{0,0,0,1},{0.1875, 0.4375}},
		{{1,0,0,1},{0.1875, 0.4375}},
		{{1,0,0.2,1},{0.1875, 0.4375}},
		{{0,0,0.2,1},{0.1875, 0.4375}},

		{{0,2,0.2,1},{0.1875, 0.4375}},
		{{0,2,0,1},{0.1875, 0.4375}},
		{{0,0,0,1},{0.1875, 0.4375}},
		{{0,0,0.2,1},{0.1875, 0.4375}},

		{{1,2,0,1},{0.1875, 0.4375}},
		{{1,2,0.2,1},{0.1875, 0.4375}},
		{{1,0,0.2,1},{0.1875, 0.4375}},
		{{1,0,0,1},{0.1875, 0.4375}},

		// 上部分门隙

		{{0.1875, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.3125, 0, 1},{0.1875, 0.4375}},
		{{0.1875, 1.3125, 0, 1},{0.1875, 0.4375}},

		{{0.1875, 1.5, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.5, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.5, 0, 1},{0.1875, 0.4375}},
		{{0.1875, 1.5, 0, 1},{0.1875, 0.4375}},

		{{0.1875, 1.5, 0, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.5, 0.2, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.3125, 0.2, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.3125, 0, 1}, {0.1875, 0.4375}},
		
		{{0.4375, 1.5, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.5, 0, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.3125, 0, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.3125, 0.2, 1}, {0.1875, 0.4375}},


		{{0.1875, 1.625, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.625, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.625, 0, 1},{0.1875, 0.4375}},
		{{0.1875, 1.625, 0, 1}, {0.1875, 0.4375}},

		{{0.1875, 1.8125, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.8125, 0.2, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.8125, 0, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.8125, 0, 1}, {0.1875, 0.4375}},

		{{0.1875, 1.8125, 0, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.8125, 0.2, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.625, 0.2, 1}, {0.1875, 0.4375}},
		{{0.1875, 1.625, 0, 1}, {0.1875, 0.4375}},

		{{0.4375, 1.8125, 0.2, 1},{0.1875, 0.4375}},
		{{0.4375, 1.8125, 0, 1}, {0.1875, 0.4375}},
		{{0.4375, 1.625, 0, 1},{0.1875, 0.4375}},
		{{0.4375, 1.625, 0.2, 1},{0.1875, 0.4375}},


		{{0.5625, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.8125, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.8125, 1.3125, 0, 1}, {0.1875, 0.4375}},
		{{0.5625, 1.3125, 0, 1}, {0.1875, 0.4375}},

		{{0.5625, 1.5, 0.2, 1}, {0.1875, 0.4375}},
		{{0.8125, 1.5, 0.2, 1}, {0.1875, 0.4375}},
		{{0.8125, 1.5, 0, 1},{0.1875, 0.4375}},
		{{0.5625, 1.5, 0, 1},{0.1875, 0.4375}},

		{{0.5625, 1.5, 0, 1},{0.1875, 0.4375}},
		{{0.5625, 1.5, 0.2, 1},{0.1875, 0.4375}},
		{{0.5625, 1.3125, 0.2, 1},{0.1875, 0.4375}},
		{{0.5625, 1.3125, 0, 1},{0.1875, 0.4375} },

		{{0.8125, 1.5, 0.2, 1},{0.1875, 0.4375} },
		{{0.8125, 1.5, 0, 1},{0.1875, 0.4375} },
		{{0.8125, 1.3125, 0, 1},{0.1875, 0.4375} },
		{{0.8125, 1.3125, 0.2, 1},{0.1875, 0.4375} },


		{ {0.5625, 1.625, 0.2, 1},{0.1875, 0.4375} },
		{ {0.8125, 1.625, 0.2, 1},{0.1875, 0.4375} },
		{ {0.8125, 1.625, 0, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.625, 0, 1}, {0.1875, 0.4375} },

		{ {0.5625, 1.8125, 0.2, 1}, {0.1875, 0.4375} },
		{ {0.8125, 1.8125, 0.2, 1}, {0.1875, 0.4375} },
		{ {0.8125, 1.8125, 0, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.8125, 0, 1}, {0.1875, 0.4375} },

		{ {0.5625, 1.8125, 0, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.8125, 0.2, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.625, 0.2, 1}, {0.1875, 0.4375} },
		{ {0.5625, 1.625, 0, 1}, {0.1875, 0.4375} },

		{ {0.8125, 1.8125, 0.2, 1},{0.1875, 0.4375} },
		{ {0.8125, 1.8125, 0, 1},{0.1875, 0.4375} },
		{ {0.8125, 1.625, 0, 1}, {0.1875, 0.4375} },
		{ {0.8125, 1.625, 0.2, 1}, {0.1875, 0.4375} }
	};

public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理，后面的 Door() 表示用父类方法构造对象
	Mirror_Door() : Door()
	{

	}

	// 创建资源与描述符，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void CreateResourceAndDescriptor(ComPtr<ID3D12Device4>& pD3D12Device) override
	{
		// 临时设置 XMFLOAT4X4 类型的模型矩阵，XMFLOAT4X4 擅长存储与传递，XMMATRIX 擅长并行运算
		XMFLOAT4X4 _temp_ModelMatrix = {};
		XMStoreFloat4x4(&_temp_ModelMatrix, ModelMatrix);

		// 用于批量复制模型矩阵的 vector，vector 的底层是一块连续内存，memcpy 复制连续内存有 CPU 优化，能快很多
		std::vector<XMFLOAT4X4> _temp_ModelMatrixGroup;
		// 批量填充 ModelMatrix 到 ModelMatrixGroup
		_temp_ModelMatrixGroup.assign(96, _temp_ModelMatrix);


		// 用于创建上传堆资源的 D3D12Resource 信息结构体，这个结构体可复用
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 上传堆资源类型都是 BUFFER 缓冲
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// row_major 行主序，上传堆资源都是按行存储的
		UploadResourceDesc.Height = 1;										// 上传堆资源高度都是 1 (线性存储)
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 上传堆资源都是 DXGI_FORMAT_UNKNOWN
		UploadResourceDesc.MipLevels = 1;									// 上传堆资源没有 Mipmap，所以都设 1
		UploadResourceDesc.DepthOrArraySize = 1;							// 上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Count = 1;							// 采样次数，上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Quality = 0;							// 采样质量，上传堆资源都设 0

		// 上传堆属性
		D3D12_HEAP_PROPERTIES HeapProperties = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建 VertexResource
		UploadResourceDesc.Width = 96 * sizeof(VERTEX);		// 宽度就是 VertexGroup 总元素大小

		// 以隐式堆方式创建资源，好处是简单方便，坏处是隐式堆由操作系统全权管理，开发者无法手动管理隐式堆的属性和生命周期
		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexResource));

		// 创建 ModelMatrix
		UploadResourceDesc.Width = 96 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ModelMatrixResource));

		// 创建 IndexResource
		UploadResourceDesc.Width = 144 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexResource));


		// 将数据进行转移: CPU 高速缓存 -> CPU 共享内存

		BYTE* TransmitPointer = nullptr;	// 用于传递数据的指针

		// 映射资源，获取 D3D12 资源的地址，同时 D3D12 资源开放写权限
		m_VertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		// 用 memcpy 将数据复制到 D3D12 资源
		memcpy(TransmitPointer, VertexArray, 96 * sizeof(VERTEX));
		// 关闭映射，资源只写，这样静态资源读取效率会快很多，动态资源无需关闭映射
		m_VertexResource->Unmap(0, nullptr);


		m_ModelMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, &_temp_ModelMatrixGroup[0], 96 * sizeof(XMFLOAT4X4));
		m_ModelMatrixResource->Unmap(0, nullptr);


		m_IndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, IndexArray, 144 * sizeof(UINT));
		m_IndexResource->Unmap(0, nullptr);


		// 创建完所有资源，就可以填写 VBV 和 IBV 描述符了

		// VBV[0]: 描述顶点位置与纹理 UV 坐标，占据第 0 号输入槽
		VertexBufferView[0].BufferLocation = m_VertexResource->GetGPUVirtualAddress();		// D3D12 资源地址
		VertexBufferView[0].SizeInBytes = 96 * sizeof(VERTEX);								// D3D12 资源总大小
		VertexBufferView[0].StrideInBytes = sizeof(VERTEX);									// D3D12 资源单个元素的大小 (步长)

		// VBV[1]: 描述顶点的模型矩阵，占据第 1 号输入槽
		VertexBufferView[1].BufferLocation = m_ModelMatrixResource->GetGPUVirtualAddress();	// D3D12 资源地址
		VertexBufferView[1].SizeInBytes = 96 * sizeof(XMFLOAT4X4);							// D3D12 资源总大小
		VertexBufferView[1].StrideInBytes = sizeof(XMFLOAT4X4);								// D3D12 资源单个元素的大小 (步长)

		// IBV: 描述顶点的索引
		IndexBufferView.BufferLocation = m_IndexResource->GetGPUVirtualAddress();			// D3D12 资源地址
		IndexBufferView.SizeInBytes = 144 * sizeof(UINT);									// D3D12 资源总大小
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;										// D3D12 资源单个元素的大小 (步长)
	}

};


// 床 (实例类)，继承自模型类
class Bed : public Model
{
protected:

	// CPU 高速缓存上的顶点信息数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意 DirectX 使用的是左手坐标系，写顶点信息时，请比一比你的左手！
	inline static VERTEX VertexArray[32] =
	{
		// 床脚正面
		{{0,0.5,0,1},{0,0.5}},
		{{1,0.5,0,1},{1,0.5}},
		{{1,0,0,1},{1,1}},
		{{0,0,0,1},{0,1}},

		// 床头正面
		{{1,0.5,2,1},{0,0.5}},
		{{0,0.5,2,1},{1,0.5}},
		{{0,0,2,1},{1,1}},
		{{1,0,2,1},{0,1}},

		// 床顶
		{{0,0.5,2,1},{1,0}},
		{{1,0.5,2,1},{1,1}},
		{{1,0.5,1,1},{0,1}},
		{{0,0.5,1,1},{0,0}},

		{{0,0.5,1,1},{1,0}},
		{{1,0.5,1,1},{1,1}},
		{{1,0.5,0,1},{0,1}},
		{{0,0.5,0,1},{0,0}},

		// 床头一侧
		{{0,0.5,2,1},{1,0.5}},
		{{0,0.5,1,1},{0,0.5}},
		{{0,0,1,1},{0,1}},
		{{0,0,2,1},{1,1}},

		{{1,0.5,1,1},{0,0.5}},
		{{1,0.5,2,1},{1,0.5}},
		{{1,0,2,1},{1,1}},
		{{1,0,1,1},{0,1}},

		// 床脚一侧
		{{0,0.5,1,1},{1,0.5}},
		{{0,0.5,0,1},{0,0.5}},
		{{0,0,0,1},{0,1}},
		{{0,0,1,1},{1,1}},

		{{1,0.5,0,1},{0,0.5}},
		{{1,0.5,1,1},{1,0.5}},
		{{1,0,1,1},{1,1}},
		{{1,0,0,1},{0,1}}
	};

	// 顶点索引数组 (静态成员，属于类，不属于类实例对象，只初始化一次)
	// 注意这里的 UINT == UINT32，后面填的格式 (步长) 必须是 DXGI_FORMAT_R32_UINT，否则会出错
	inline static UINT IndexArray[48] =
	{
		// 床脚正面
		0,1,2,0,2,3,

		// 床头正面
		4,5,6,4,6,7,

		// 床顶
		8,9,10,8,10,11,
		12,13,14,12,14,15,

		// 床头一侧
		16,17,18,16,18,19,
		20,21,22,20,22,23,

		// 床脚一侧
		24,25,26,24,26,27,
		28,29,30,28,30,31
	};

public:

	// 构造函数，调用 AppendTextureKey 添加需要的纹理
	Bed()
	{
		this->AppendTextureKey("bed_feet_end");
		this->AppendTextureKey("bed_feet_side");
		this->AppendTextureKey("bed_feet_top");
		this->AppendTextureKey("bed_head_top");
		this->AppendTextureKey("bed_head_side");
		this->AppendTextureKey("bed_head_end");
	}

	// 创建资源与描述符，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void CreateResourceAndDescriptor(ComPtr<ID3D12Device4>& pD3D12Device) override
	{
		// 临时设置 XMFLOAT4X4 类型的模型矩阵，XMFLOAT4X4 擅长存储与传递，XMMATRIX 擅长并行运算
		XMFLOAT4X4 _temp_ModelMatrix = {};
		XMStoreFloat4x4(&_temp_ModelMatrix, ModelMatrix);

		// 用于批量复制模型矩阵的 vector，vector 的底层是一块连续内存，memcpy 复制连续内存有 CPU 优化，能快很多
		std::vector<XMFLOAT4X4> _temp_ModelMatrixGroup;
		// 批量填充 ModelMatrix 到 ModelMatrixGroup
		_temp_ModelMatrixGroup.assign(32, _temp_ModelMatrix);


		// 用于创建上传堆资源的 D3D12Resource 信息结构体，这个结构体可复用
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// 上传堆资源类型都是 BUFFER 缓冲
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// row_major 行主序，上传堆资源都是按行存储的
		UploadResourceDesc.Height = 1;										// 上传堆资源高度都是 1 (线性存储)
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// 上传堆资源都是 DXGI_FORMAT_UNKNOWN
		UploadResourceDesc.MipLevels = 1;									// 上传堆资源没有 Mipmap，所以都设 1
		UploadResourceDesc.DepthOrArraySize = 1;							// 上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Count = 1;							// 采样次数，上传堆资源都设 1
		UploadResourceDesc.SampleDesc.Quality = 0;							// 采样质量，上传堆资源都设 0

		// 上传堆属性
		D3D12_HEAP_PROPERTIES HeapProperties = { D3D12_HEAP_TYPE_UPLOAD };

		// 创建 VertexResource
		UploadResourceDesc.Width = 32 * sizeof(VERTEX);		// 宽度就是 VertexGroup 总元素大小

		// 以隐式堆方式创建资源，好处是简单方便，坏处是隐式堆由操作系统全权管理，开发者无法手动管理隐式堆的属性和生命周期
		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexResource));

		// 创建 ModelMatrix
		UploadResourceDesc.Width = 32 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_ModelMatrixResource));

		// 创建 IndexResource
		UploadResourceDesc.Width = 48 * sizeof(XMFLOAT4X4);

		pD3D12Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE,
			&UploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexResource));


		// 将数据进行转移: CPU 高速缓存 -> CPU 共享内存

		BYTE* TransmitPointer = nullptr;	// 用于传递数据的指针

		// 映射资源，获取 D3D12 资源的地址，同时 D3D12 资源开放写权限
		m_VertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		// 用 memcpy 将数据复制到 D3D12 资源
		memcpy(TransmitPointer, VertexArray, 32 * sizeof(VERTEX));
		// 关闭映射，资源只写，这样静态资源读取效率会快很多，动态资源无需关闭映射
		m_VertexResource->Unmap(0, nullptr);


		m_ModelMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, &_temp_ModelMatrixGroup[0], 32 * sizeof(XMFLOAT4X4));
		m_ModelMatrixResource->Unmap(0, nullptr);


		m_IndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransmitPointer));
		memcpy(TransmitPointer, IndexArray, 48 * sizeof(UINT));
		m_IndexResource->Unmap(0, nullptr);


		// 创建完所有资源，就可以填写 VBV 和 IBV 描述符了

		// VBV[0]: 描述顶点位置与纹理 UV 坐标，占据第 0 号输入槽
		VertexBufferView[0].BufferLocation = m_VertexResource->GetGPUVirtualAddress();		// D3D12 资源地址
		VertexBufferView[0].SizeInBytes = 32 * sizeof(VERTEX);								// D3D12 资源总大小
		VertexBufferView[0].StrideInBytes = sizeof(VERTEX);									// D3D12 资源单个元素的大小 (步长)

		// VBV[1]: 描述顶点的模型矩阵，占据第 1 号输入槽
		VertexBufferView[1].BufferLocation = m_ModelMatrixResource->GetGPUVirtualAddress();	// D3D12 资源地址
		VertexBufferView[1].SizeInBytes = 32 * sizeof(XMFLOAT4X4);							// D3D12 资源总大小
		VertexBufferView[1].StrideInBytes = sizeof(XMFLOAT4X4);								// D3D12 资源单个元素的大小 (步长)

		// IBV: 描述顶点的索引
		IndexBufferView.BufferLocation = m_IndexResource->GetGPUVirtualAddress();			// D3D12 资源地址
		IndexBufferView.SizeInBytes = 48 * sizeof(UINT);									// D3D12 资源总大小
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;										// D3D12 资源单个元素的大小 (步长)
	}


	// 绘制模型，函数声明的 override 在编译期能检查虚函数是否被重写，重写/实现函数常写 override 是一种好习惯
	virtual void DrawModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList) override
	{
		// 设置 IBV 索引缓冲描述符
		pCommandList->IASetIndexBuffer(&IndexBufferView);
		// 设置 VBV 顶点缓冲描述符，注意我们这里使用了多槽输入！
		pCommandList->IASetVertexBuffers(0, 2, VertexBufferView);

		// 设置根参数，将纹理 SRV 描述符设置到 GPU 的寄存器上，这样着色器就可以找到纹理了
		// 要更换纹理，可以通过 SetGraphicsRootDescriptorTable 改变根参数绑定的 GPUHandle

		// 床脚正面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["bed_feet_end"]);
		pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// 床头正面
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["bed_head_end"]);
		pCommandList->DrawIndexedInstanced(6, 1, 6, 0, 0);

		// 床顶
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["bed_head_top"]);
		pCommandList->DrawIndexedInstanced(6, 1, 12, 0, 0);
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["bed_feet_top"]);
		pCommandList->DrawIndexedInstanced(6, 1, 18, 0, 0);

		// 床头一侧
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["bed_head_side"]);
		pCommandList->DrawIndexedInstanced(12, 1, 24, 0, 0);

		// 床脚一侧
		pCommandList->SetGraphicsRootDescriptorTable(1, Texture_GPUHandle_Map["bed_feet_side"]);
		pCommandList->DrawIndexedInstanced(12, 1, 36, 0, 0);
	}
};


// 模型管理器
class ModelManager
{
public:

	// 纹理映射表元素结构体
	struct TEXTURE_MAP_INFO
	{
		std::wstring TextureFilePath;			// 文件路径

		// 位于默认堆上纹理资源
		ComPtr<ID3D12Resource> DefaultHeapTextureResource;
		// 位于上传堆的纹理资源
		ComPtr<ID3D12Resource> UploadHeapTextureResource;

		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;	// SRV 描述符堆的 CPU 句柄，用于创建纹理 SRV 描述符，将纹理与描述符绑定
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;	// SRV 描述符堆的 GPU 句柄，用于设置根参数，索引到对应纹理

		// 不用担心，创建 SRV 堆描述符堆，就会确定 CPU 句柄和 GPU 句柄的地址，后续只要描述符堆不销毁重新构造，句柄地址就是固定的
		// CPU 句柄和 GPU 句柄只有位置和用法不同的区别 (CPU 句柄在 CPU 端，GPU 句柄在 GPU 端)
		// 同一索引的 CPU 和 GPU 句柄能共享数据，所以只需创建 CPU 句柄，GPU 句柄无需创建
		// SetDescriptorHeap 会把描述符堆映射到 GPU 端的渲染管线，对应的 GPU 句柄就会有数据 (和同索引的 CPU 句柄一致)
	};

	// 纹理映射表
	std::unordered_map<std::string, TEXTURE_MAP_INFO> Texture_SRV_Map;


	// 非透明模型组，存储 Model 类指针的 vector，注意存储的是指针，指针可以指向不同类的对象
	std::vector<Model*> OpaqueGroup;
	// 透明模型组
	std::vector<Model*> TransparentGroup;
	// 半透明模型组
	std::vector<Model*> TranslucenceGroup;


public:

	// 构造函数，我们在构造函数上创建纹理映射表
	ModelManager()
	{
		Texture_SRV_Map["dirt"].TextureFilePath = L"resource/dirt.png";
		Texture_SRV_Map["grass_top"].TextureFilePath = L"resource/grass_top.png";
		Texture_SRV_Map["grass_side"].TextureFilePath = L"resource/grass_side.png";
		Texture_SRV_Map["log_oak"].TextureFilePath = L"resource/log_oak.png";
		Texture_SRV_Map["log_oak_top"].TextureFilePath = L"resource/log_oak_top.png";
		Texture_SRV_Map["furnace_front_off"].TextureFilePath = L"resource/furnace_front_off.png";
		Texture_SRV_Map["furnace_side"].TextureFilePath = L"resource/furnace_side.png";
		Texture_SRV_Map["furnace_top"].TextureFilePath = L"resource/furnace_top.png";
		Texture_SRV_Map["crafting_table_front"].TextureFilePath = L"resource/crafting_table_front.png";
		Texture_SRV_Map["crafting_table_side"].TextureFilePath = L"resource/crafting_table_side.png";
		Texture_SRV_Map["crafting_table_top"].TextureFilePath = L"resource/crafting_table_top.png";
		Texture_SRV_Map["planks_oak"].TextureFilePath = L"resource/planks_oak.png";

		Texture_SRV_Map["glass_light_blue"].TextureFilePath = L"resource/glass_light_blue.png";
		Texture_SRV_Map["door_wood_lower"].TextureFilePath = L"resource/door_wood_lower.png";
		Texture_SRV_Map["door_wood_upper"].TextureFilePath = L"resource/door_wood_upper.png";
		Texture_SRV_Map["bed_feet_end"].TextureFilePath = L"resource/bed_feet_end.png";
		Texture_SRV_Map["bed_feet_side"].TextureFilePath = L"resource/bed_feet_side.png";
		Texture_SRV_Map["bed_feet_top"].TextureFilePath = L"resource/bed_feet_top.png";
		Texture_SRV_Map["bed_head_end"].TextureFilePath = L"resource/bed_head_end.png";
		Texture_SRV_Map["bed_head_side"].TextureFilePath = L"resource/bed_head_side.png";
		Texture_SRV_Map["bed_head_top"].TextureFilePath = L"resource/bed_head_top.png";
	}

	// 创建方块，我们在这里写上创建方块的代码
	void CreateBlock()
	{
		// 两层泥土地基，y 是高度
		for (int x = 0; x < 10; x++)
		{
			for (int z = -4; z < 10; z++)
			{
				for (int y = -2; y < 0; y++)
				{
					Model* dirt = new Dirt();							// 创建对象指针，调用时会根据虚函数表调用不同的函数
					dirt->SetModelMatrix(XMMatrixTranslation(x, y, z));	// 设置不同的模型矩阵，XMMatrixTranslation 平移模型
					OpaqueGroup.push_back(dirt);							// 将新模型添加到模型组
				}
			}
		}

		// 一层草方块地基
		for (int x = 0; x < 10; x++)
		{
			for (int z = -4; z < 10; z++)
			{
				Model* grass = new Grass();
				grass->SetModelMatrix(XMMatrixTranslation(x, 0, z));
				OpaqueGroup.push_back(grass);
			}
		}

		// 4x4 木板房基

		for (int x = 3; x < 7; x++)
		{
			for (int z = 3; z < 7; z++)
			{
				Model* plank = new Planks_Oak();
				plank->SetModelMatrix(XMMatrixTranslation(x, 2, z));
				OpaqueGroup.push_back(plank);
			}
		}


		// 8 柱原木 

		for (int y = 1; y < 7; y++)
		{
			Model* log_oak = new Log_Oak();
			log_oak->SetModelMatrix(XMMatrixTranslation(3, y, 2));
			OpaqueGroup.push_back(log_oak);
		}

		for (int y = 1; y < 7; y++)
		{
			Model* log_oak = new Log_Oak();
			log_oak->SetModelMatrix(XMMatrixTranslation(2, y, 3));
			OpaqueGroup.push_back(log_oak);
		}

		for (int y = 1; y < 7; y++)
		{
			Model* log_oak = new Log_Oak();
			log_oak->SetModelMatrix(XMMatrixTranslation(6, y, 2));
			OpaqueGroup.push_back(log_oak);
		}

		for (int y = 1; y < 7; y++)
		{
			Model* log_oak = new Log_Oak();
			log_oak->SetModelMatrix(XMMatrixTranslation(7, y, 3));
			OpaqueGroup.push_back(log_oak);
		}

		for (int y = 1; y < 7; y++)
		{
			Model* log_oak = new Log_Oak();
			log_oak->SetModelMatrix(XMMatrixTranslation(7, y, 6));
			OpaqueGroup.push_back(log_oak);
		}

		for (int y = 1; y < 7; y++)
		{
			Model* log_oak = new Log_Oak();
			log_oak->SetModelMatrix(XMMatrixTranslation(6, y, 7));
			OpaqueGroup.push_back(log_oak);
		}

		for (int y = 1; y < 7; y++)
		{
			Model* log_oak = new Log_Oak();
			log_oak->SetModelMatrix(XMMatrixTranslation(2, y, 6));
			OpaqueGroup.push_back(log_oak);
		}

		for (int y = 1; y < 7; y++)
		{
			Model* log_oak = new Log_Oak();
			log_oak->SetModelMatrix(XMMatrixTranslation(3, y, 7));
			OpaqueGroup.push_back(log_oak);
		}


		// 其他木板与门前台阶
		{
			Model* plank = new Planks_Oak();
			plank->SetModelMatrix(XMMatrixTranslation(4, 2, 2));
			OpaqueGroup.push_back(plank);

			plank = new Planks_Oak();
			plank->SetModelMatrix(XMMatrixTranslation(5, 2, 2));
			OpaqueGroup.push_back(plank);

			for (int y = 5; y < 7; y++)
			{
				for (int x = 4; x < 6; x++)
				{
					plank = new Planks_Oak();
					plank->SetModelMatrix(XMMatrixTranslation(x, y, 2));
					OpaqueGroup.push_back(plank);
				}
			}

			for (int y = 2; y < 4; y++)
			{
				for (int z = 4; z < 6; z++)
				{
					plank = new Planks_Oak();
					plank->SetModelMatrix(XMMatrixTranslation(2, y, z));
					OpaqueGroup.push_back(plank);
				}
			}

			for (int y = 2; y < 4; y++)
			{
				for (int x = 4; x < 6; x++)
				{
					plank = new Planks_Oak();
					plank->SetModelMatrix(XMMatrixTranslation(x, y, 7));
					OpaqueGroup.push_back(plank);
				}
			}

			for (int y = 2; y < 4; y++)
			{
				for (int z = 4; z < 6; z++)
				{
					plank = new Planks_Oak();
					plank->SetModelMatrix(XMMatrixTranslation(7, y, z));
					OpaqueGroup.push_back(plank);
				}
			}

			plank = new Planks_Oak();
			plank->SetModelMatrix(XMMatrixTranslation(2, 6, 4));
			OpaqueGroup.push_back(plank);

			plank = new Planks_Oak();
			plank->SetModelMatrix(XMMatrixTranslation(2, 6, 5));
			OpaqueGroup.push_back(plank);

			plank = new Planks_Oak();
			plank->SetModelMatrix(XMMatrixTranslation(4, 6, 7));
			OpaqueGroup.push_back(plank);

			plank = new Planks_Oak();
			plank->SetModelMatrix(XMMatrixTranslation(5, 6, 7));
			OpaqueGroup.push_back(plank);

			plank = new Planks_Oak();
			plank->SetModelMatrix(XMMatrixTranslation(7, 6, 4));
			OpaqueGroup.push_back(plank);

			plank = new Planks_Oak();
			plank->SetModelMatrix(XMMatrixTranslation(7, 6, 5));
			OpaqueGroup.push_back(plank);

			Model* stair = new Planks_Oak_SoildStair();
			stair->SetModelMatrix(XMMatrixTranslation(4, 2, 1));
			OpaqueGroup.push_back(stair);

			stair = new Planks_Oak_SoildStair();
			stair->SetModelMatrix(XMMatrixTranslation(5, 2, 1));
			OpaqueGroup.push_back(stair);

			stair = new Planks_Oak_SoildStair();
			stair->SetModelMatrix(XMMatrixTranslation(4, 1, 0));
			OpaqueGroup.push_back(stair);

			stair = new Planks_Oak_SoildStair();
			stair->SetModelMatrix(XMMatrixTranslation(5, 1, 0));
			OpaqueGroup.push_back(stair);
		}

		// 4x4 木板房顶

		for (int x = 3; x < 7; x++)
		{
			for (int z = 3; z < 7; z++)
			{
				Model* plank = new Planks_Oak();
				plank->SetModelMatrix(XMMatrixTranslation(x, 6, z));
				OpaqueGroup.push_back(plank);
			}
		}

		// 屋顶

		{
			// 第一层

			for (int x = 3; x < 7; x++)
			{
				Model* stair = new Planks_Oak_SoildStair();
				stair->SetModelMatrix(XMMatrixTranslation(x, 6, 1));
				OpaqueGroup.push_back(stair);
			}

			for (int x = 3; x < 7; x++)
			{
				// 旋转橡木台阶用的模型矩阵
				// 这里本来是可以不用 XMMatrixTranslation(-0.5, -0.5, -0.5) 平移到模型中心的
				// 因为作者本人 (我) 的设计失误，把模型坐标系原点建立在模型左下角了 (见上文的 VertexArray)
				// 导致还要先把原点平移到模型中心，旋转完再还原，增大计算量，这个是完全可以规避的
				// 读者可以自行修改 VertexArray，使方块以自身中心为原点建系，这样就可以直接 XMMatrixRotationY() 进行旋转了
				XMMATRIX transform = XMMatrixTranslation(-0.5, -0.5, -0.5);
				transform *= XMMatrixRotationY(XM_PI);						// 平移中心后，再旋转，否则会出错 (旋转角度是弧度)
				transform *= XMMatrixTranslation(0.5, 0.5, 0.5);			// 旋转完再还原
				transform *= XMMatrixTranslation(x, 6, 8);					// 再平移到对应的坐标
				Model* stair = new Planks_Oak_SoildStair();
				stair->SetModelMatrix(transform);
				OpaqueGroup.push_back(stair);
			}

			for (int z = 3; z < 7; z++)
			{
				XMMATRIX transform = XMMatrixTranslation(-0.5, -0.5, -0.5);
				transform *= XMMatrixRotationY(XM_PIDIV2);					// 旋转 90°
				transform *= XMMatrixTranslation(0.5, 0.5, 0.5);
				transform *= XMMatrixTranslation(1, 6, z);
				Model* stair = new Planks_Oak_SoildStair();
				stair->SetModelMatrix(transform);
				OpaqueGroup.push_back(stair);
			}

			for (int z = 3; z < 7; z++)
			{
				XMMATRIX transform = XMMatrixTranslation(-0.5, -0.5, -0.5);
				transform *= XMMatrixRotationY(XM_PI + XM_PIDIV2);			// 旋转 270°
				transform *= XMMatrixTranslation(0.5, 0.5, 0.5);
				transform *= XMMatrixTranslation(8, 6, z);
				Model* stair = new Planks_Oak_SoildStair();
				stair->SetModelMatrix(transform);
				OpaqueGroup.push_back(stair);
			}

			// 第二层

			for (int x = 3; x < 7; x++)
			{
				Model* stair = new Planks_Oak_SoildStair();
				stair->SetModelMatrix(XMMatrixTranslation(x, 7, 2));
				OpaqueGroup.push_back(stair);
			}

			for (int x = 3; x < 7; x++)
			{
				XMMATRIX transform = XMMatrixTranslation(-0.5, -0.5, -0.5);
				transform *= XMMatrixRotationY(XM_PI);
				transform *= XMMatrixTranslation(0.5, 0.5, 0.5);
				transform *= XMMatrixTranslation(x, 7, 7);
				Model* stair = new Planks_Oak_SoildStair();
				stair->SetModelMatrix(transform);
				OpaqueGroup.push_back(stair);
			}

			for (int z = 3; z < 7; z++)
			{
				XMMATRIX transform = XMMatrixTranslation(-0.5, -0.5, -0.5);
				transform *= XMMatrixRotationY(XM_PIDIV2);
				transform *= XMMatrixTranslation(0.5, 0.5, 0.5);
				transform *= XMMatrixTranslation(2, 7, z);
				Model* stair = new Planks_Oak_SoildStair();
				stair->SetModelMatrix(transform);
				OpaqueGroup.push_back(stair);
			}

			for (int z = 3; z < 7; z++)
			{
				XMMATRIX transform = XMMatrixTranslation(-0.5, -0.5, -0.5);
				transform *= XMMatrixRotationY(XM_PI + XM_PIDIV2);
				transform *= XMMatrixTranslation(0.5, 0.5, 0.5);
				transform *= XMMatrixTranslation(7, 7, z);
				Model* stair = new Planks_Oak_SoildStair();
				stair->SetModelMatrix(transform);
				OpaqueGroup.push_back(stair);
			}

			// 补上屋顶空位

			for (int x = 3; x < 7; x++)
			{
				for (int z = 3; z < 7; z++)
				{
					Model* plank = new Planks_Oak();
					plank->SetModelMatrix(XMMatrixTranslation(x, 7, z));
					OpaqueGroup.push_back(plank);
				}
			}
		}

		// 工作台和熔炉
		{
			Model* craft_table = new Crafting_Table();
			craft_table->SetModelMatrix(XMMatrixTranslation(3, 3, 6));
			OpaqueGroup.push_back(craft_table);

			Model* furnace = new Furnace();
			furnace->SetModelMatrix(XMMatrixTranslation(4, 3, 6));
			OpaqueGroup.push_back(furnace);

			furnace = new Furnace();
			furnace->SetModelMatrix(XMMatrixTranslation(5, 3, 6));
			OpaqueGroup.push_back(furnace);
		}

		// 门
		{
			Model* door = new Door();
			door->SetModelMatrix(XMMatrixTranslation(4, 3, 2));
			TransparentGroup.push_back(door);

			door = new Mirror_Door();
			door->SetModelMatrix(XMMatrixTranslation(5, 3, 2));
			TransparentGroup.push_back(door);
		}

		// 床
		{
			Model* bed = new Bed();
			bed->SetModelMatrix(XMMatrixTranslation(6, 3, 5));
			TransparentGroup.push_back(bed);
		}

		// 玻璃
		{
			for (int y = 4; y < 6; y++)
			{
				for (int z = 4; z < 6; z++)
				{
					Model* glass = new Glass_Light_Blue();
					glass->SetModelMatrix(XMMatrixTranslation(2, y, z));
					TranslucenceGroup.push_back(glass);
				}
				for (int z = 4; z < 6; z++)
				{
					Model* glass = new Glass_Light_Blue();
					glass->SetModelMatrix(XMMatrixTranslation(7, y, z));
					TranslucenceGroup.push_back(glass);
				}
			}

			for (int x = 4; x < 6; x++)
			{
				for (int y = 4; y < 6; y++)
				{
					Model* glass = new Glass_Light_Blue();
					glass->SetModelMatrix(XMMatrixTranslation(x, y, 7));
					TranslucenceGroup.push_back(glass);
				}
			}
		}
	}

	// 当一切准备就绪后，就可以正式创建模型资源，准备渲染了
	// 调用该函数的前提是: 依次完成 DX12Engine::CreateModelTextureResource (读取并创建纹理资源)，CreateBlock (创建方块，设置模型矩阵)
	void CreateModelResource(ComPtr<ID3D12Device4>& pD3D12Device)
	{
		// 遍历模型组
		for (auto& model : OpaqueGroup)
		{
			// 创建模型资源
			model->CreateResourceAndDescriptor(pD3D12Device);
			// 遍历模型自身的映射表，设置模型需要用到的纹理
			for (const auto& texture : model->RequestForTextureMap())
			{
				// 设置模型的 SRV 描述符
				model->SetTextureGPUHandle(texture.first, Texture_SRV_Map[texture.first].GPUHandle);
			}
		}

		// 遍历模型组
		for (auto& model : TranslucenceGroup)
		{
			// 创建模型资源
			model->CreateResourceAndDescriptor(pD3D12Device);
			// 遍历模型自身的映射表，设置模型需要用到的纹理
			for (const auto& texture : model->RequestForTextureMap())
			{
				// 设置模型的 SRV 描述符
				model->SetTextureGPUHandle(texture.first, Texture_SRV_Map[texture.first].GPUHandle);
			}
		}

		// 遍历模型组
		for (auto& model : TransparentGroup)
		{
			// 创建模型资源
			model->CreateResourceAndDescriptor(pD3D12Device);
			// 遍历模型自身的映射表，设置模型需要用到的纹理
			for (const auto& texture : model->RequestForTextureMap())
			{
				// 设置模型的 SRV 描述符
				model->SetTextureGPUHandle(texture.first, Texture_SRV_Map[texture.first].GPUHandle);
			}
		}
	}

	// 渲染半透明模型
	// 调用该函数的前提是: 完成上面的 CreateModelResource
	void RenderTranslucenceModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList)
	{
		// 遍历模型组
		for (const auto& model : TranslucenceGroup)
		{
			model->DrawModel(pCommandList);
		}
	}

	// 渲染透明模型
	// 调用该函数的前提是: 完成上面的 CreateModelResource
	void RenderTransparentModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList)
	{
		// 遍历模型组
		for (const auto& model : TransparentGroup)
		{
			model->DrawModel(pCommandList);
		}
	}

	// 渲染不透明模型
	// 调用该函数的前提是: 完成上面的 CreateModelResource
	void RenderOpaqueModel(ComPtr<ID3D12GraphicsCommandList>& pCommandList)
	{
		// 遍历模型组
		for (const auto& model : OpaqueGroup)
		{
			model->DrawModel(pCommandList);
		}
	}

};



// DX12 引擎
class DX12Engine
{
private:

	int WindowWidth = 640;		// 窗口宽度
	int WindowHeight = 480;		// 窗口高度
	HWND m_hwnd;				// 窗口句柄

	ComPtr<ID3D12Debug> m_D3D12DebugDevice;					// D3D12 调试层设备
	UINT m_DXGICreateFactoryFlag = NULL;					// 创建 DXGI 工厂时需要用到的标志
	UINT m_D3DShaderCompileFlag = NULL;						// 编译 shader 时需要用到的标志

	ComPtr<IDXGIFactory5> m_DXGIFactory;					// DXGI 工厂
	ComPtr<IDXGIAdapter1> m_DXGIAdapter;					// 显示适配器 (显卡)
	ComPtr<ID3D12Device4> m_D3D12Device;					// D3D12 核心设备

	ComPtr<ID3D12CommandQueue> m_CommandQueue;				// 命令队列
	ComPtr<ID3D12CommandAllocator> m_CommandAllocator;		// 命令分配器
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;		// 命令列表

	ComPtr<IDXGISwapChain3> m_DXGISwapChain;				// DXGI 交换链
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;					// RTV 描述符堆
	ComPtr<ID3D12Resource> m_RenderTarget[3];				// 渲染目标缓冲区数组，每一副渲染缓冲对应一个窗口缓冲区
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


	ModelManager m_ModelManager;							// 模型管理器，帮助管理并渲染模型

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



	ComPtr<ID3D12Resource> m_CBVResource;		// 常量缓冲资源，用于存放 MVP 矩阵，MVP 矩阵每帧都要更新，所以需要存储在常量缓冲区中
	struct CBuffer								// 常量缓冲结构体
	{
		XMFLOAT4X4 MVPMatrix;		// MVP 矩阵，用于将顶点数据从顶点空间变换到齐次裁剪空间
	};
	CBuffer* MVPBuffer = nullptr;	// 常量缓冲结构体指针，里面存储的是 MVP 矩阵信息，下文 Map 后指针会指向 CBVResource 的地址

	Camera m_FirstCamera;			// 第一人称摄像机

	ComPtr<ID3D12RootSignature> m_RootSignature;			// 根签名
	
	D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};				// 输入样式信息结构体
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[6] = {};			// 输入元素信息结构体数组

	D3D12_GRAPHICS_PIPELINE_STATE_DESC OpaquePSODesc = {};		// 不透明物体 PSO 信息结构体
	ComPtr<ID3D12PipelineState> m_OpaquePSO;					// 不透明物体的渲染管线状态
	D3D12_GRAPHICS_PIPELINE_STATE_DESC TransparentPSODesc = {};	// 透明物体 PSO 信息结构体
	ComPtr<ID3D12PipelineState> m_TransparentPSO;				// 透明物体的渲染管线状态
	D3D12_GRAPHICS_PIPELINE_STATE_DESC TranslucencePSODesc = {};// 半透明物体 PSO 信息结构体，Pass 2
	ComPtr<ID3D12PipelineState> m_TranslucencePSO;				// 半透明物体的渲染管线状态，Pass 2


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
		m_hwnd = CreateWindow(wc.lpszClassName, L"Minecraft", WS_SYSMENU | WS_OVERLAPPED,
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
		// 设置 D3DCOMPILE_DEBUG 标志用于获取着色器调试信息。该标志可以提升调试体验，但仍然允许着色器进行优化操作
		m_D3DShaderCompileFlag |= D3DCOMPILE_DEBUG;
		// 在 Debug 环境下禁用优化以避免出现一些不合理的情况
		m_D3DShaderCompileFlag |= D3DCOMPILE_SKIP_OPTIMIZATION;
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
					OutputDebugStringW(adap.Description);		// 在输出窗口上输出创建 D3D12 设备所用的显卡名称
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

	// 创建 RTV 渲染目标描述符堆 (Non-Shader Visible)，创建渲染目标，将渲染目标设置为窗口
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
	void CreateDSVHeap()
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
	void CreateDepthStencilBuffer()
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
	void CreateDSV()
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVViewDesc = {};
		DSVViewDesc.Format = DSVFormat;								// DSV 描述符格式要和资源一致
		DSVViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// 深度缓冲本质也是一块 2D 纹理
		DSVViewDesc.Flags = D3D12_DSV_FLAG_NONE;					// 这个 Flag 是用来设置读写权限的，深度值和模板值均可以读写

		// 创建 DSV 描述符 (Depth Stencil View，深度模板描述符)
		m_D3D12Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &DSVViewDesc, DSVHandle);
	}


	// 创建 SRV Descriptor Heap 着色器资源描述符堆 (Shader Visible)
	void CreateSRVHeap()
	{
		// 创建 SRV 描述符堆 (Shader Resource View，着色器资源描述符)
		D3D12_DESCRIPTOR_HEAP_DESC SRVHeapDesc = {};
		SRVHeapDesc.NumDescriptors = m_ModelManager.Texture_SRV_Map.size();	// 描述符堆的容量
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

	// 加载纹理到内存中
	bool LoadTextureFromFile(std::wstring TextureFilename)
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

	// 上取整算法，对 A 向上取整，判断至少要多少个长度为 B 的空间才能容纳 A，用于内存对齐
	inline UINT Ceil(UINT A, UINT B)
	{
		return (A + B - 1) / B;
	}

	// 创建用于上传的 UploadResource 与用于放纹理的 DefaultResource
	void CreateUploadAndDefaultResource(ModelManager::TEXTURE_MAP_INFO& Info)
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
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&Info.UploadHeapTextureResource));


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
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&Info.DefaultHeapTextureResource));
	}

	// CommandList 录制命令，录制将纹理数据复制到默认堆资源的命令
	void CopyTextureDataToDefaultResource(ModelManager::TEXTURE_MAP_INFO& Info)
	{
		// 用于暂时存储纹理数据的指针，这里要用 malloc 分配空间
		BYTE* TextureData = (BYTE*)malloc(TextureSize);

		// 将整块纹理数据读到 TextureData 中，方便后文的 memcpy 复制操作
		m_WICBitmapSource->CopyPixels(nullptr, BytePerRowSize, TextureSize, TextureData);

		// 用于传递资源的指针
		BYTE* TransferPointer = nullptr;

		// Map 开始映射，Map 方法会得到上传堆资源的地址 (在共享内存上)，传递给指针，这样我们就能通过 memcpy 操作复制数据了
		Info.UploadHeapTextureResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));

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
		Info.UploadHeapTextureResource->Unmap(0, nullptr);

		TextureData -= TextureSize;		// 纹理资源指针偏移回初始位置
		free(TextureData);				// 释放上文 malloc 分配的空间，后面我们用不到它，不要让它占内存

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint = {};								// 资源脚本，用来描述要复制的资源
		D3D12_RESOURCE_DESC DefaultResourceDesc = Info.DefaultHeapTextureResource->GetDesc();	// 默认堆资源结构体

		// 获取纹理复制脚本，用于下文的纹理复制
		m_D3D12Device->GetCopyableFootprints(&DefaultResourceDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION DstLocation = {};						// 复制目标位置 (默认堆资源) 结构体
		DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		// 纹理复制类型，这里必须指向纹理
		DstLocation.SubresourceIndex = 0;									// 指定要复制的子资源索引
		DstLocation.pResource = Info.DefaultHeapTextureResource.Get();		// 要复制到的资源

		D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};						// 复制源位置 (上传堆资源) 结构体
		SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// 纹理复制类型，这里必须指向缓冲区
		SrcLocation.PlacedFootprint = PlacedFootprint;						// 指定要复制的资源脚本信息
		SrcLocation.pResource = Info.UploadHeapTextureResource.Get();		// 被复制数据的缓冲



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
	void CreateSRV(ModelManager::TEXTURE_MAP_INFO& Info,
		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
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
		m_D3D12Device->CreateShaderResourceView(Info.DefaultHeapTextureResource.Get(), &SRVDescriptorDesc, CPUHandle);

		// 将当前 SRV 描述符句柄存储到 ModelManager 的纹理映射表中，注意我们传的是引用参数，可以直接对参数进行修改
		Info.CPUHandle = CPUHandle;
		Info.GPUHandle = GPUHandle;
	}


	// 读取并创建纹理资源
	void CreateModelTextureResource()
	{
		CreateSRVHeap();	// 创建 SRV 描述符堆，创建时就会确定描述符的 CPU 和 GPU 地址，无需担心

		// 当前元素的 CPU 句柄
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();
		// 当前元素的 GPU 句柄
		D3D12_GPU_DESCRIPTOR_HANDLE CurrentGPUHandle = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();
		// SRV 描述符的大小
		UINT SRVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		StartCommandRecord();	// 启动命令列表，开始录制命令

		// 对纹理映射表进行遍历
		for (auto& CurrentElem : m_ModelManager.Texture_SRV_Map)
		{
			// 从纹理文件中加载数据
			LoadTextureFromFile(CurrentElem.second.TextureFilePath);
			// 创建上传堆和默认堆资源
			CreateUploadAndDefaultResource(CurrentElem.second);
			// 将纹理数据复制到上传堆，并记录一条上传堆复制到默认堆的命令
			CopyTextureDataToDefaultResource(CurrentElem.second);
			// 最终创建 SRV 描述符
			CreateSRV(CurrentElem.second, CurrentCPUHandle, CurrentGPUHandle);

			// CPU 和 GPU 句柄偏移，准备下一个纹理
			CurrentCPUHandle.ptr += SRVDescriptorSize;
			CurrentGPUHandle.ptr += SRVDescriptorSize;
		}

		StartCommandExecute();	// 关闭命令列表，交给命令队列执行
	}


	// 创建模型顶点与索引资源
	void CreateModelVertexAndIndexResource()
	{
		m_ModelManager.CreateBlock();
		m_ModelManager.CreateModelResource(m_D3D12Device);
	}


	// 创建 Constant Buffer Resource 常量缓冲资源，常量缓冲是一块预先分配的高速显存，用于存储每一帧都要变换的资源，这里我们要存储 MVP 矩阵
	void CreateCBVResource()
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
	void CreateRootSignature()
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

	// 创建输入布局，因为下文的渲染管线需要共用输入布局，数组不能在函数内声明，否则出作用域会销毁，所以单独写一个创建函数
	void CreateInputLayout()
	{
		// 第 0 号输入槽: 输入顶点位置与纹理 UV 坐标

		InputElementDesc[0].SemanticName = "POSITION";					// 要锚定的语义
		InputElementDesc[0].SemanticIndex = 0;							// 语义索引，目前我们填 0 就行
		InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;	// 输入格式
		InputElementDesc[0].InputSlot = 0;								// 输入槽编号，目前我们填 0 就行
		InputElementDesc[0].AlignedByteOffset = 0;						// 在输入槽中的偏移
		// 输入流类型，一种是我们现在用的 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA 逐顶点输入流,还有一种叫逐实例输入流，后面再学
		InputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[0].InstanceDataStepRate = 0;					// 实例数据步进率，目前我们没有用到实例化，填 0


		InputElementDesc[1].SemanticName = "TEXCOORD";										// 要锚定的语义
		InputElementDesc[1].SemanticIndex = 0;												// 语义索引
		InputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;								// 输入格式
		InputElementDesc[1].InputSlot = 0;													// 输入槽编号
		// 在输入槽中的偏移，因为 position 与 texcoord 在同一输入槽(0号输入槽)
		// position 是 float4，有 4 个 float ，每个 float 占 4 个字节，所以要偏移 4*4=16 个字节，这样才能确定 texcoord 参数的位置，不然装配的时候会覆盖原先 position 的数据
		InputElementDesc[1].AlignedByteOffset = 16;											// 在输入槽中的偏移
		InputElementDesc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;	// 输入流类型
		InputElementDesc[1].InstanceDataStepRate = 0;										// 实例数据步进率


		// 第 1 号输入槽: 输入模型矩阵 (因为 4x4 矩阵太大，需要分成 4 个 float4 向量传输)

		// MATRIX0
		// SemanticName 语义名:							MATRIX (语义名后面不带数字)
		// SemanticIndex 语义索引:						0
		// InputSlot 输入槽:								第 1 号输入槽
		// AlignedByteOffset 相对输入槽起始位置的偏移:		0
		InputElementDesc[2].SemanticName = "MATRIX";
		InputElementDesc[2].SemanticIndex = 0;
		InputElementDesc[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		InputElementDesc[2].InputSlot = 1;
		InputElementDesc[2].AlignedByteOffset = 0;
		InputElementDesc[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[2].InstanceDataStepRate = 0;

		// MATRIX1
		// SemanticName 语义名:							MATRIX (语义名后面不带数字)
		// SemanticIndex 语义索引:						1
		// InputSlot 输入槽:								第 1 号输入槽
		// AlignedByteOffset 相对输入槽起始位置的偏移:		0 + 4*4 = 16
		InputElementDesc[3].SemanticName = "MATRIX";
		InputElementDesc[3].SemanticIndex = 1;
		InputElementDesc[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		InputElementDesc[3].InputSlot = 1;
		InputElementDesc[3].AlignedByteOffset = 16;
		InputElementDesc[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[3].InstanceDataStepRate = 0;

		// MATRIX2
		// SemanticName 语义名:							MATRIX (语义名后面不带数字)
		// SemanticIndex 语义索引:						2
		// InputSlot 输入槽:								第 1 号输入槽
		// AlignedByteOffset 相对输入槽起始位置的偏移:		16 + 4*4 = 32
		InputElementDesc[4].SemanticName = "MATRIX";
		InputElementDesc[4].SemanticIndex = 2;
		InputElementDesc[4].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		InputElementDesc[4].InputSlot = 1;
		InputElementDesc[4].AlignedByteOffset = 32;
		InputElementDesc[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[4].InstanceDataStepRate = 0;

		// MATRIX3
		// SemanticName 语义名:							MATRIX (语义名后面不带数字)
		// SemanticIndex 语义索引:						3
		// InputSlot 输入槽:								第 1 号输入槽
		// AlignedByteOffset 相对输入槽起始位置的偏移:		32 + 4*4 = 48
		InputElementDesc[5].SemanticName = "MATRIX";
		InputElementDesc[5].SemanticIndex = 3;
		InputElementDesc[5].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		InputElementDesc[5].InputSlot = 1;
		InputElementDesc[5].AlignedByteOffset = 48;
		InputElementDesc[5].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[5].InstanceDataStepRate = 0;



		InputLayoutDesc.NumElements = 6;						// 输入元素个数
		InputLayoutDesc.pInputElementDescs = InputElementDesc;	// 输入元素结构体数组指针
	}

	// 创建不透明渲染管线状态对象 (Opaque Pipeline State Object, Opaque PSO)
	void CreateOpaquePSO()
	{
		// Input Assembler 输入装配阶段
		OpaquePSODesc.InputLayout = InputLayoutDesc;			// 设置渲染管线 IA 阶段的输入布局


		ComPtr<ID3DBlob> VertexShaderBlob;		// 顶点着色器二进制字节码
		ComPtr<ID3DBlob> PixelShaderBlob;		// 像素着色器二进制字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码，根签名创建失败时用 OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); 可以获取报错信息

		// 编译顶点着色器 Vertex Shader
		D3DCompileFromFile(L"OpaqueShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", m_D3DShaderCompileFlag, NULL, &VertexShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// 编译像素着色器 Pixel Shader
		D3DCompileFromFile(L"OpaqueShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", m_D3DShaderCompileFlag, NULL, &PixelShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		OpaquePSODesc.VS.pShaderBytecode = VertexShaderBlob->GetBufferPointer();	// VS 字节码数据指针
		OpaquePSODesc.VS.BytecodeLength = VertexShaderBlob->GetBufferSize();		// VS 字节码数据长度
		OpaquePSODesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();		// PS 字节码数据指针
		OpaquePSODesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();			// PS 字节码数据长度

		// Rasterizer 光栅化
		OpaquePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;		// 剔除模式，指定是否开启背面/正面/不剔除，这里选背面剔除
		OpaquePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;		// 填充模式，指定是否开启纯色/线框填充，这里选纯色填充

		// 第一次设置根签名！本次设置是将根签名与 PSO 绑定，设置渲染管线的输入参数状态
		OpaquePSODesc.pRootSignature = m_RootSignature.Get();

		// 设置深度测试状态
		OpaquePSODesc.DSVFormat = DSVFormat;											// 设置深度缓冲的格式
		OpaquePSODesc.DepthStencilState.DepthEnable = true;								// 开启深度缓冲
		OpaquePSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			// 深度缓冲的比较方式
		OpaquePSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// 深度缓冲的读写权限

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
		OpaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// 设置渲染目标数量，我们只有一副渲染目标 (颜色缓冲) 需要进行渲染，所以填 1
		OpaquePSODesc.NumRenderTargets = 1;
		// 设置渲染目标的格式，这里要和交换链指定窗口缓冲的格式一致，这里的 0 指的是渲染目标的索引
		OpaquePSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		// 设置混合阶段 (输出合并阶段) 下 RGBA 颜色通道的开启和关闭，D3D12_COLOR_WRITE_ENABLE_ALL 表示 RGBA 四色通道全部开启
		OpaquePSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		// 设置采样次数，我们这里填 1 就行
		OpaquePSODesc.SampleDesc.Count = 1;
		// 设置采样掩码，这个是用于多重采样的，我们直接填全采样 (UINT_MAX，就是将 UINT 所有的比特位全部填充为 1) 就行
		OpaquePSODesc.SampleMask = UINT_MAX;

		// 最终创建 PSO 对象
		m_D3D12Device->CreateGraphicsPipelineState(&OpaquePSODesc, IID_PPV_ARGS(&m_OpaquePSO));
	}

	// 创建透明渲染管线状态对象 (Transparent Pipeline State Object, Transparent PSO)
	void CreateTransparentPSO()
	{
		// 复用上文的 OpaquePSODesc 结构体
		TransparentPSODesc = OpaquePSODesc;

		ComPtr<ID3DBlob> VertexShaderBlob;		// 顶点着色器二进制字节码
		ComPtr<ID3DBlob> PixelShaderBlob;		// 像素着色器二进制字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码，根签名创建失败时用 OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); 可以获取报错信息


		// 编译顶点着色器 Vertex Shader
		D3DCompileFromFile(L"TransparentShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", m_D3DShaderCompileFlag, NULL, &VertexShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// 编译像素着色器 Pixel Shader
		D3DCompileFromFile(L"TransparentShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", m_D3DShaderCompileFlag, NULL, &PixelShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		TransparentPSODesc.VS.pShaderBytecode = VertexShaderBlob->GetBufferPointer();	// VS 字节码数据指针
		TransparentPSODesc.VS.BytecodeLength = VertexShaderBlob->GetBufferSize();		// VS 字节码数据长度
		TransparentPSODesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();	// PS 字节码数据指针
		TransparentPSODesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();		// PS 字节码数据长度

		// 关闭背面剔除
		TransparentPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		// 最终创建 PSO 对象
		m_D3D12Device->CreateGraphicsPipelineState(&TransparentPSODesc, IID_PPV_ARGS(&m_TransparentPSO));
	}

	// 创建半透明渲染管线状态对象 (Translucence Pipeline State Object, Translucence PSO)
	void CreateTranslucencePSO()
	{
		// 复用上文的 OpaquePSODesc 结构体
		TranslucencePSODesc = OpaquePSODesc;

		ComPtr<ID3DBlob> VertexShaderBlob;		// 顶点着色器二进制字节码
		ComPtr<ID3DBlob> PixelShaderBlob;		// 像素着色器二进制字节码
		ComPtr<ID3DBlob> ErrorBlob;				// 错误字节码，根签名创建失败时用 OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); 可以获取报错信息


		// 编译顶点着色器 Vertex Shader
		D3DCompileFromFile(L"TranslucenceShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", m_D3DShaderCompileFlag, NULL, &VertexShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// 编译像素着色器 Pixel Shader
		D3DCompileFromFile(L"TranslucenceShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", m_D3DShaderCompileFlag, NULL, &PixelShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// 如果着色器编译出错，ErrorBlob 可以提供报错信息
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		TranslucencePSODesc.VS.pShaderBytecode = VertexShaderBlob->GetBufferPointer();	// VS 字节码数据指针
		TranslucencePSODesc.VS.BytecodeLength = VertexShaderBlob->GetBufferSize();		// VS 字节码数据长度
		TranslucencePSODesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();	// PS 字节码数据指针
		TranslucencePSODesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();		// PS 字节码数据长度

		// 关闭背面裁剪
		TranslucencePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		// 关闭深度写入，但仍然保留深度测试
		TranslucencePSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;


		// 结果色彩 = 上层色彩 * 上层色彩 alpha + 下层色彩 * (1 - 上层色彩 alpha)，这套公式仅适用于下层是不透明物体的情况
		// Src = 源色彩 = 上层色彩，Dest = 目标色彩 = 下层色彩
		// SrcA = 源色彩 Alpha 值，DstA = 目标色彩 Alpha 值
		// Result = Src * SrcA + Dest * (1 - SrcA)

		TranslucencePSODesc.BlendState.RenderTarget[0].BlendEnable = true;							// 开启混合

		// 下面三个选项控制 RGB 通道的混合，Alpha 通道与 RGB 通道的混合是分开的，这一点请留意！
		// Result = Src * SrcA + Dest * (1 - SrcA)

		// 让上层色彩乘上 SrcA，Src * SrcA
		TranslucencePSODesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		// 让下层色彩乘上 1 - SrcA，Dest * (1 - SrcA)
		TranslucencePSODesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		// 两种色彩相加，Src * SrcA + Dest * (1 - SrcA)
		TranslucencePSODesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;


		// 下面的三个选项控制 Alpha 通道的混合，Alpha 通道与 RGB 通道的混合是分开的，这一点请留意！
		// ResultA = SrcA * 1 + DstA * 0

		// 让上层色彩透明度乘 1，表示使用 SrcA
		TranslucencePSODesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		// 让下层色彩透明度乘 0，表示不使用 DstA
		TranslucencePSODesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		// 最终要混合的色彩 alpha 是 ResultA
		TranslucencePSODesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

		// 最终创建 PSO 对象
		m_D3D12Device->CreateGraphicsPipelineState(&TranslucencePSODesc, IID_PPV_ARGS(&m_TranslucencePSO));
	}



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


		// 先设置 Opaque PSO，渲染不透明物体
		m_CommandList->SetPipelineState(m_OpaquePSO.Get());
		m_ModelManager.RenderOpaqueModel(m_CommandList);

		// 再设置 Transparent PSO，渲染透明物体
		m_CommandList->SetPipelineState(m_TransparentPSO.Get());
		m_ModelManager.RenderTransparentModel(m_CommandList);

		// 最后设置 Translucence PSO，渲染半透明物体
		m_CommandList->SetPipelineState(m_TranslucencePSO.Get());
		m_ModelManager.RenderTranslucenceModel(m_CommandList);


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
		engine.InitWindow(hins);
		engine.CreateDebugDevice();
		engine.CreateDevice();
		engine.CreateCommandComponents();
		engine.CreateRenderTarget();
		engine.CreateFenceAndBarrier();

		engine.CreateDSVHeap();
		engine.CreateDepthStencilBuffer();
		engine.CreateDSV();

		engine.CreateModelTextureResource();

		engine.CreateModelVertexAndIndexResource();

		engine.CreateCBVResource();

		engine.CreateRootSignature();
		engine.CreateInputLayout();
		engine.CreateOpaquePSO();
		engine.CreateTransparentPSO();
		engine.CreateTranslucencePSO();


		engine.RenderLoop();
	}
};


// 主函数
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}