# DX12 快速教程

- [前言](#前言)
- [配置方法](#配置方法)
- [教程目录](#教程目录)
  - [(1) InitWindow](#1-InitWindow)
  - [(2) DrawSkyblueWindow](#2-DrawSkyblueWindow)
  - [(3) DrawRectangle](#3-DrawRectangle)
  - [(4) DrawTexture](#4-DrawTexture)
  - [(5) DrawBlock](#5-DrawBlock)
  - [(6) FirstPersonView](#6-FirstPersonView)
  - [(7) RenderMatchbox](#7-RenderMatchbox)
  - [(8) AlphaBlend](#8-AlphaBlend)
  - [(9) AssimpAcquaintance](#9-AssimpAcquaintance)
  - [(10) RenderGLTFModel](#10-RenderGLTFModel)
  - [(11) RenderGLTFSkinnedModel](#11-RenderGLTFSkinnedModel)
  - [(12) RenderGLTFLightingModel](#12-RenderGLTFLightingModel)

## 前言

DirectX 12 从 2015 年发布起，经过了长时间的变革与发展，成为了图形界一致的认可，被应用到现代游戏中。众多技术大神 (例如 Frank D. Luna, GameBabyRockSun, Yigarlor, 辰月二十七, 指掀涛澜天下惊, Dylan 等等) 对 DX12 API 进行了全面的深度解读，示例代码完全匹配他们高超的能力。然而无论是微软官方示例的 DirectX-Sample, 还是这些技术大神的示例代码，都对初学者 (尤其是初次接触底层 API 的初学者) 造成了不少的困扰。

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/434924d4bc0ffcf4c9847705c1aeb90a.jpg)

DirectX 12 API 全面变革的静态 PSO，资源绑定，异步渲染架构，使它无异于图形学的 C++，学习难度与成本直线升高，很多初学者望而止步。为了降低初学者学习 DirectX 12 的难度，让初学者保持学习的乐趣与激情，本教程将一步步引导初学者理解 DX12，循序渐进，仅使用 DX12 API + C++ 标准库 + 必要的第三方库 (会内置在项目中，开盒即用)，将 全注释代码 + 生动的图文教程 + 接近新一代的风格 有机结合，尽可能降低学习难度与代码复杂度，使初学者体验到图形学的乐趣，感受底层渲染逻辑的规律，为后面的工业化/企业级图形引擎/工具做良好铺垫。

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/c408b84e50aad90dedf0886654d5d334.jpg)

## 配置方法

## 教程目录

### (1) InitWindow

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/1.PNG)

初步认识 Win32 API，学会做一个 Win32 窗口，认识窗口创建与消息循环，为下一节正式学习 DX12 做铺垫。

### (2) DrawSkyblueWindow

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/2.PNG)

正式开始学习 DirectX 12 API，用 DirectX 12 渲染一个天蓝色窗口，初步认识并创建 DX12 的基本设备、描述符堆、描述符与资源

### (3) DrawRectangle

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/3.PNG)

用 DirectX 12 画一个矩形，初步接触渲染管线、PSO 渲染管线状态与根签名，认识 shader 着色器的概念，了解相关的渲染原理

### (4) DrawTexture

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/4.PNG)

用 DirectX 12 画一个钻石原矿，初步认识纹理，进一步学习根签名，认识 Root Descriptor Table 根描述表，接触 Shader Resource View 着色器资源描述符，体验 CPU 与 GPU 之间的交互与绑定过程

### (5) DrawBlock

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/5.PNG)

用 DirectX 12 画一个钻石原矿方块，理解 3D 渲染中场景与模型呈现到屏幕画面的基本原理，接触 MVP 矩阵，认识 Constant Resource View 常量资源描述符，认识 Root Descriptor 根描述符

### (6) FirstPersonView

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/6.gif)

初步认识 Camera 摄像机，在 DirectX 12 上构建第一人称视角，理解摄像机平移、视角旋转对 MVP 的影响，理解 DX12 的资源绑定与传递

### (7) RenderMatchbox

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/7.gif)

渲染一个火柴盒，初步认识 Depth Stencil Buffer 深度模板缓冲，进一步认识顶点、模型与模型矩阵，体验多槽输入在 DirectX 12 中的使用，思考 C++ class 在实际游戏中的灵活运用

### (8) AlphaBlend

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/8.gif)

用 DirectX 12 绘制玻璃等有透明像素的物体，初步了解透明测试/混合与渲染顺序的关系，了解 shader 文件的编译过程，学会不同 PSO 渲染管线状态的创建细节与设置顺序

### (9) AssimpAcquaintance

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/9_1.PNG)

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/9_2.PNG)

初步认识并使用 Assimp 库，学会 Assimp 在 VS 上的编译与配置，在控制台上获取并打印《蔚蓝档案》中的 霞沢美游 gltf 模型的数据 (模型骨骼节点信息，材质贴图信息，网格信息与 AABB 包围盒)

### (10) RenderGLTFModel

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/10.gif)

使用 DirectX 12 + Assimp 渲染《Minecraft》中的 苦力怕 gltf 模型 (该模型没有骨骼)，学会导入并使用模型文件、材质贴图、网格数据

### (11) RenderGLTFSkinnedModel

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/11.gif)

使用 DirectX 12 + Assimp 渲染《约会大作战》中的 时崎狂三 gltf 骨骼模型，学会导入并使用骨骼模型，认识骨骼偏移矩阵与蒙皮网格，认识骨骼坐标系与网格坐标系，理解并使用骨骼权值与骨骼索引

### (12) RenderGLTFLightingModel

![](https://raw.githubusercontent.com/DG-AF/DX12-Quick-Tutorial/master/GitPicDir/12.gif)

加入光照，使用 DirectX 12 + Assimp 渲染《Ave Mujica》的 丰川祥子 gltf 骨骼模型，初步认识 Blinn-Phong 光照模型，了解 Assimp、DX12、光照模型 三者的交互，了解 Root Constants 根常量





