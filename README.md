DX12 快速教程
==================

- [前言](#前言)
- [配置方法 (教程资源获取方法)](#配置方法-教程资源获取方法)
- [教程目录](#教程目录)
- [基础篇](#1-InitWindow)
  - [(1) InitWindow](#1-InitWindow)
  - [(2) DrawSkyblueWindow](#2-DrawSkyblueWindow)
  - [(3) DrawRectangle](#3-DrawRectangle)
  - [(4) DrawTexture](#4-DrawTexture)
  - [(5) DrawBlock](#5-DrawBlock)
  - [(6) FirstPersonView](#6-FirstPersonView)
  - [(7) RenderMatchbox](#7-RenderMatchbox)
  - [(8) AlphaBlend](#8-AlphaBlend)
- [Assimp 篇](#9-AssimpAcquaintance)
  - [(9) AssimpAcquaintance](#9-AssimpAcquaintance)
  - [(10) RenderGLTFModel](#10-RenderGLTFModel)
  - [(11) RenderGLTFSkinnedModel](#11-RenderGLTFSkinnedModel)
  - [(12) RenderGLTFLightingModel](#12-RenderGLTFLightingModel)
  - [(13) RenderGLTFAnimation](#13-RenderGLTFAnimation)
  - [(14) RenderGLTFWithSkyBox](#14-RenderGLTFWithSkyBox)
- [进阶篇](#15-DrawInstanced)
  - [(15) DrawInstanced](#15-DrawInstanced)
  - [(16) D2DWithDX12](#16-D2DWithDX12)
  - [(17) DrawItemsAndMerge](#17-DrawItemsAndMerge)
  - [(18) ScreenSpaceRaycast](#18-ScreenSpaceRaycast)
  - [(19) PerlinNoise](#19-PerlinNoise)
  - [(20) InfiniteWorld](#20-InfiniteWorld)
  - [(21) GPUFrustumCulling](#21-GPUFrustumCulling)
  - [(22) ParticleSystem](#22-ParticleSystem)

## 读者应该 clone 哪一个分支

![branch.png](https://a2.boltp.com/2026/03/02/69a55c9182fa4.png)

**master** 分支是已经开发完成的项目，这些会加入到标准教程中，建议读者使用这个分支的源码; <br/>
**dev** 分支是实验性项目，这些项目有可能会被丢弃，也有可能被合并到 master 中，不建议读者使用；<br/>
如果有开发者想要与我联系，加入到 dev 分支开发协作的，可以联系我 QQ：3976357120 <br/>
(备用 QQ：1281866925，备用邮箱：3976357120@qq.com)


## 2026.3.28 第二次 Merge 更新

1. 更新第 22 章代码：**022-ParticleSystem**<br/>
2. 修正部分 bug 与文字描述

## 前言

DirectX 12 从 2015 年发布起，经过了长时间的变革与发展，获得了图形界一致的认可，被应用到现代游戏中。众多技术大神 (例如 Frank D. Luna, GameBabyRockSun, Yigarlor, 辰月二十七, 指掀涛澜天下惊, Dylan 等等) 对 DX12 API 进行了全面的深度解读，示例代码完全匹配他们高超的能力。然而无论是微软官方示例的 DirectX-Sample, 还是这些技术大神的示例代码，都对初学者 (尤其是初次接触底层 API 的初学者) 造成了不少的困扰。

![DX12Intro.jpeg](https://a2.boltp.com/2026/01/29/697aed5448297.jpeg)

DirectX 12 API 全面变革的静态 PSO，资源绑定，异步渲染架构，使它无异于图形学的 C++，学习难度与成本直线升高，很多初学者望而止步。为了降低初学者学习 DirectX 12 的难度，让初学者保持学习的乐趣与激情，本教程将一步步引导初学者理解 DX12，循序渐进，仅使用 DX12 API + C++ 标准库 + 必要的第三方库 (会内置在项目中，开盒即用)，将 全注释代码 + 生动的图文教程 + 接近新一代的风格 有机结合，尽可能降低学习难度与代码复杂度，使初学者体验到图形学的乐趣，感受底层渲染逻辑的规律，为后面的工业化/企业级图形引擎/工具做良好铺垫。

![Kiseki.jpg](https://a2.boltp.com/2026/01/29/697aed54367c4.jpg)

## 配置方法 (教程资源获取方法)

![hh.png](https://a1.boltp.com/2026/01/29/697aeb9215d69.png)

方法(1): 使用 git 工具 clone 仓库

方法(2): 直接点击 "Download Zip" 下载压缩包，**解压缩后**，会得到一个文件夹，打开文件夹里的 .sln 即可

如果 Github 下载很慢，这里有国内 Gitee 镜像仓库地址：[https://gitee.com/zzw46589/DX12-Quick-Tutorial](https://gitee.com/zzw46589/DX12-Quick-Tutorial)

## 教程目录

### (1) InitWindow

![1.jpeg](https://a1.boltp.com/2026/01/29/697aec00aed02.jpeg)

初步认识 Win32 API，学会做一个 Win32 窗口，认识窗口创建与消息循环，为下一节正式学习 DX12 做铺垫。

[教程地址：DX12 快速教程(1) —— 做窗口](https://blog.csdn.net/DGAF2198588973/article/details/144488018)


### (2) DrawSkyblueWindow

![2.jpeg](https://a1.boltp.com/2026/01/29/697aec00b2fa2.jpeg)

正式开始学习 DirectX 12 API，用 DirectX 12 渲染一个天蓝色窗口，初步认识并创建 DX12 的基本设备、描述符堆、描述符与资源

[教程地址：DX12 快速教程(2) —— 渲染天蓝色窗口](https://blog.csdn.net/DGAF2198588973/article/details/144543014)


### (3) DrawRectangle

![3.jpeg](https://a1.boltp.com/2026/01/29/697aec00bcc36.jpeg)

用 DirectX 12 画一个矩形，初步接触渲染管线、PSO 渲染管线状态与根签名，认识 shader 着色器的概念，了解相关的渲染原理

[教程地址：DX12 快速教程(3) —— 画矩形](https://blog.csdn.net/DGAF2198588973/article/details/144874380)


### (4) DrawTexture

![4.jpeg](https://a1.boltp.com/2026/01/29/697aec00b2546.jpeg)

用 DirectX 12 画一个钻石原矿，初步认识纹理，进一步学习根签名，认识 Root Descriptor Table 根描述表，接触 Shader Resource View 着色器资源描述符，体验 CPU 与 GPU 之间的交互与绑定过程

[教程地址：DX12 快速教程(4) —— 画钻石原矿](https://blog.csdn.net/DGAF2198588973/article/details/145232320)


### (5) DrawBlock

![5.jpeg](https://a1.boltp.com/2026/01/29/697aec00af10d.jpeg)

用 DirectX 12 画一个钻石原矿方块，理解 3D 渲染中场景与模型呈现到屏幕画面的基本原理，接触 MVP 矩阵，认识 Constant Resource View 常量资源描述符，认识 Root Descriptor 根描述符

[教程地址：DX12 快速教程(5) —— 画方块](https://blog.csdn.net/DGAF2198588973/article/details/145391595)


### (6) FirstPersonView

![6.gif](https://a2.boltp.com/2026/01/29/697aed3a2dcf7.gif)

初步认识 Camera 摄像机，在 DirectX 12 上构建第一人称视角，理解摄像机平移、视角旋转对 MVP 的影响，理解 DX12 的资源绑定与传递

[教程地址：DX12 快速教程(6) —— 第一人称视角](https://blog.csdn.net/DGAF2198588973/article/details/146530258)


### (7) RenderMatchbox

![7.gif](https://a2.boltp.com/2026/01/29/697aed3b4ba55.gif)

渲染一个火柴盒，初步认识 Depth Stencil Buffer 深度模板缓冲，进一步认识顶点、模型与模型矩阵，体验多槽输入在 DirectX 12 中的使用，思考 C++ class 在实际游戏中的灵活运用

[教程地址：DX12 快速教程(7) —— 渲染火柴盒](https://blog.csdn.net/DGAF2198588973/article/details/147233643)


### (8) AlphaBlend

![8.gif](https://a2.boltp.com/2026/01/29/697aed3b26b8b.gif)

用 DirectX 12 绘制玻璃等有透明像素的物体，初步了解透明测试/混合与渲染顺序的关系，了解 shader 文件的编译过程，学会不同 PSO 渲染管线状态的创建细节与设置顺序

[教程地址：DX12 快速教程(8) —— 画玻璃](https://blog.csdn.net/DGAF2198588973/article/details/147780518)


### (9) AssimpAcquaintance

![9_1.png](https://a2.boltp.com/2026/01/29/697aed5418e28.png)

![9_2.png](https://a2.boltp.com/2026/01/29/697aed540a6d6.png)

![9_3.png](https://a2.boltp.com/2026/01/29/697aed540a6de.png)

![9_4.png](https://a2.boltp.com/2026/01/29/697aed5408c74.png)

初步认识并使用 Assimp 库，学会 Assimp 在 VS 上的编译与配置，在控制台上获取并打印《蔚蓝档案》中的 霞沢美游 gltf 模型的数据 (模型骨骼节点信息，材质贴图信息，网格信息与 AABB 包围盒)

[教程地址：DX12 快速教程(9) —— 初识 Assimp 库](https://blog.csdn.net/DGAF2198588973/article/details/155643538)


### (10) RenderGLTFModel

![10.gif](https://a2.boltp.com/2026/01/29/697aed7161329.gif)

使用 DirectX 12 + Assimp 渲染《合金装备崛起:复仇》中的 塞穆尔·罗德里格斯 gltf 模型 (该模型没有绑定骨骼)，学会导入并使用模型文件、材质贴图、网格数据

[教程地址：DX12 快速教程(10) —— 渲染模型](https://blog.csdn.net/DGAF2198588973/article/details/155771199)


### (11) RenderGLTFSkinnedModel

![11.gif](https://a2.boltp.com/2026/01/29/697aed7157b0a.gif)

使用 DirectX 12 + Assimp 渲染《约会大作战》中的 时崎狂三 gltf 骨骼模型，学会导入并使用骨骼模型，认识骨骼偏移矩阵与蒙皮网格，认识骨骼坐标系与网格坐标系，理解并使用骨骼权值与骨骼索引

[教程地址：DX12 快速教程(11) —— 渲染骨骼模型](https://blog.csdn.net/DGAF2198588973/article/details/155880215)


### (12) RenderGLTFLightingModel

![12.gif](https://a2.boltp.com/2026/01/29/697aed7132e89.gif)

加入光照，使用 DirectX 12 + Assimp 渲染《Ave Mujica》的 丰川祥子 gltf 骨骼模型，初步认识 Blinn-Phong 光照模型，了解 Assimp、DX12、光照模型 三者的交互，了解 Root Constants 根常量

[教程地址：DX12 快速教程(12) —— Blinn-Phong 光照模型](https://blog.csdn.net/DGAF2198588973/article/details/156517324)


### (13) RenderGLTFAnimation

![13.gif](https://a2.boltp.com/2026/01/29/697aed70f204e.gif)

加入骨骼动画，使用 DirectX 12 + Assimp 渲染《超次元游戏 海王星》中的 Neptune 涅普顿(绀紫之心) GLTF模型 的骨骼动画，了解骨骼动画的基本原理和流程，认识四元数及其应用，理解 Assimp 的数据存储方式

[教程地址：DX12 快速教程(13) —— 蒙皮骨骼动画](https://blog.csdn.net/DGAF2198588973/article/details/157096831)


### (14) RenderGLTFWithSkyBox

![14.gif](https://a2.boltp.com/2026/02/08/6987f8e2c4202.gif)

加入并渲染 HDR 天空盒，使用 DirectX 12 + Assimp + stb_image 渲染《为美好的生活献上祝福》中的 Q版阿库娅 GLTF 模型

[教程地址：DX12 快速教程(14) —— 屏幕空间天空盒](https://blog.csdn.net/DGAF2198588973/article/details/157870274)


### (15) DrawInstanced

![15.gif](https://a2.boltp.com/2026/02/14/698f8a30dc05d.gif)

进阶篇开门第一篇，学会 DirectX 12 的纹理数组、SRV Structured Buffer 结构化缓冲的创建与使用，GPU Instancing 硬件实例化，以及多实例渲染的应用，一次性快速渲染大量方块 (1125 个方块)，第 7-8 章的那些方块类以及数据不用写了

[教程地址：DX12 快速教程(15) —— 多实例渲染](https://blog.csdn.net/DGAF2198588973/article/details/158066553)


### (16) D2DWithDX12

![16.gif](https://a2.boltp.com/2026/03/02/69a5745fd5ab0.gif)

认识新版本的 Direct2D (d2d 1.3，不是老版 1.0，老版本绑不上 DirectX 12 的交换链)，学会使用 Direct2D 绘制简单的 UI 界面 (9 格物品快捷栏，物品选中框，经验槽，生命值，饥饿值，十字准星)，并与 DirectX 12 互动，将 D2D 绘制同步到 DirectX 12 的渲染目标中

[教程地址：DX12 快速教程(16) —— D2D 与 D3D12 互操作](https://blog.csdn.net/DGAF2198588973/article/details/158853736)


### (17) DrawItemsAndMerge

![17.gif](https://a2.boltp.com/2026/03/02/69a57461ef2f3.gif)

认识等轴变换，学会使用 Direct2D 的 3x2 矩阵变换，并与 DirectXMath 互动，认识位图渲染目标，生成轴侧视图下的方块图标，同时整合 D2D 和 DX12 的渲染，改装部分 WIC 加载纹理代码到 D2DEngine

[教程地址：DX12 快速教程(17) —— 立体图标与合并渲染](https://blog.csdn.net/DGAF2198588973/article/details/158853768)


### (18) ScreenSpaceRaycast

![18.gif](https://a2.boltp.com/2026/03/06/69aa50691c52e.gif)

认识屏幕射线相交检测，学会方块的破坏与放置，理解 PSO 的 IA 输入布局复用，学会如何利用 PSO 解决深度冲突，初步学习 D3D12 资源的数据动态添加与删除。本章不难，但是需要改动的量比较大，新增的业务逻辑较多，需要细心和耐心

[教程地址：DX12 快速教程(18) —— 屏幕射线相交检测](https://blog.csdn.net/DGAF2198588973/article/details/158853797)


### (19) PerlinNoise

![19.gif](https://a2.boltp.com/2026/03/20/69bd6af6a138e.gif)

初步学习计算着色器、UAV Resource 无序访问资源、Readback Heap 回读堆，理解并运用柏林噪声生成简单的地形网格 (柏林噪声就长这样的，想要获得原版那样的游戏效果，需要叠加其他噪声)

[教程地址：DX12 快速教程(19) —— 柏林噪声](https://blog.csdn.net/DGAF2198588973/article/details/158853809)


### (20) InfiniteWorld

![20.gif](https://a2.boltp.com/2026/03/20/69bd6af6ba437.gif)

进一步学习计算着色器，掌握动态资源的管理，学习 HiveBuffer 非连续缓冲与空闲栈的思想，认识 UAV 纹理数组与结构化缓冲区，动态生成区块高度图，模拟 MC 无限世界的生成

[教程地址：DX12 快速教程(20) —— 无限世界生成](https://blog.csdn.net/DGAF2198588973/article/details/159292295)


### (21) GPUFrustumCulling

![21.gif](https://a2.boltp.com/2026/03/22/69bfd056f19b4.gif)

认识视锥剔除，认识命令签名，UAV 原始缓冲区，ExecuteIndirect，学会 GPU 视锥剔除，熟练计算着色器的基本使用方法，进一步理解 Transition 转换屏障的使用，从上一章的 4x4 的区块可见范围，扩大到更大的 8x8 范围，并进一步减少渲染开销

[教程地址：DX12 快速教程(21) —— GPU 视锥剔除](https://blog.csdn.net/DGAF2198588973/article/details/159354380)


### (22) ParticleSystem

![22.gif](https://a2.boltp.com/2026/03/28/69c7d99160d9e.gif)

进一步学习计算着色器，学习粒子的简单动态生成与销毁，模拟 Minecraft 的粒子破坏飞溅效果，在计算着色器上实现并管理 HiveBuffer、数组空闲栈、栈顶指针、待生成列表 这些数据结构，初步认识半隐式欧拉积分，进一步掌握根签名，学习根描述表连续绑定多个描述符的特性，初步学习不同类型描述符对同一资源的创建绑定与状态转换

[教程地址：DX12 快速教程(22) —— 简单粒子系统](https://blog.csdn.net/DGAF2198588973/article/details/159586530)





