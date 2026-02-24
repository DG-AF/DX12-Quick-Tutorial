
// (18) ScreenSpaceRaycast: 认识屏幕射线相交检测，学会方块的破坏与放置


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
#include<iomanip>				// C++ 输入输出控制格式化库，用于 CallBackFunc 的 std::fixed 与 std::setprecision


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
