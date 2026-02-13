
// (9) AssimpAcquaintance: 初步认识并使用 Assimp 库，获取并打印《蔚蓝档案》中的 霞沢美游 gltf 模型的数据
// 鸣谢原作者大大: Onerui(momo) (https://sketchfab.com/hswangrui)
// 模型项目地址: https://sketchfab.com/3d-models/blue-archivekasumizawa-miyu-108d81dfd5a44dab92e4dccf0cc51a02


#define NOMINMAX		// windows.h 与标准库里的 min/max 函数重名导致冲突了，禁用 windows.h 里面的 min/max 函数

#include<Windows.h>		// Windows 窗口编程核心头文件

#include<iostream>		// C++ 标准输入输出库
#include<vector>		// C++ STL vector 容器库

#include<assimp/Importer.hpp>		// Assimp Importer 模型导入器，用于导入模型,读取模型数据
#include<assimp/postprocess.h>		// PostProcess 后处理，提供多种标志 (aiProcess_xxx)，用于改善模型的导入质量与性能
#include<assimp/scene.h>			// Scene 核心组件库，用于存储与管理导入的 3D 模型的所有数据

#pragma comment(lib,"assimp-vc143-mtd.lib")		// 链接 Assimp DLL


// 1.项目 -> 属性 -> VC++ 目录 -> 包含目录 -> 添加 Assimp/include (或者写 $(SolutionDir)Assimp/include )
// 2.项目 -> 属性 -> VC++ 目录 -> 库目录 -> 添加 Assimp/lib (或者写 $(SolutionDir)Assimp/lib )
// 3.将 Assimp/lib 里面的 assimp-vc143-mtd.lib 与 assimp-vc143-mtd.dll 复制到
//   exe 所在的目录下 (x64/Debug 文件夹)，生成的 exe 需要依赖这两个链接库


// GLTF 是一种 3D 模型文件格式，用于引擎和应用程序高效传输和加载 3D 场景和模型
// 它的文件结构如下：
// textures			模型纹理文件夹
// license.txt		模型使用的许可证 (常用许可证是 CC-BY: 发布必须署名原作者，可商用)
// scene.bin		储存模型图元，顶点，骨骼动画等二进制数据
// scene.gltf		JSON 文件，定义 scene 的结构与元素，并储存 bin 和 textures 的链接



// ---------------------------------------------------------------------------------------------------------------



UINT TotalNodeNum = 0;		// 总节点数

// 递归展开计算模型骨骼节点，打印每个骨骼节点的名称，大部分情况下节点都是骨骼，节点名和骨骼名相同
// 骨骼在模型中的呈现形式是骨骼树，在骨骼动画中，父节点会影响子节点，子节点拿到的偏移矩阵是相对于所属父节点的，所以要递归展开计算
// Assimp 中，节点的变换矩阵会影响节点下的全部属性，包括网格、骨骼、子节点，骨骼名是唯一的
void ModelNodeTraversal(const aiNode* node, std::string NodeBaseChar, UINT tier)
{
	std::cout << NodeBaseChar
		<< "┗━━ 节点名: " << node->mName.C_Str()		// 节点名
		<< " (层级: " << tier						// 层级
		<< ", 子节点数: " << node->mNumChildren		// 子节点数
		<< ")" << std::endl;

	// 当前节点打印完成，下一行打印子节点时，在前面添加空格，便于区分
	NodeBaseChar += "  ";

	// 遍历子节点，打印子节点的名称
	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		ModelNodeTraversal(node->mChildren[i], NodeBaseChar, tier + 1);
	}

	TotalNodeNum++;		// 总节点数 +1
}


int main()
{
	std::string ModelFileName = "miyu/scene.gltf";					// 模型文件名
	Assimp::Importer* m_ModelImporter = new Assimp::Importer;		// 模型导入器
	const aiScene* m_ModelScene = nullptr;							// 模型/场景对象

	// 导入模型使用的标志
	// aiProcess_ConvertToLeftHanded: Assimp 导入的模型是以 OpenGL 的右手坐标系为基础的，将模型转换成 DirectX 的左手坐标系
	// aiProcess_Triangulate：模型设计师可能使用多边形对模型进行建模的，对于用多边形建模的模型，将它们都转换成基于三角形建模
	// aiProcess_FixInfacingNormals：建模软件都是双面显示的，所以设计师不会在意顶点绕序方向，部分面会被剔除无法正常显示，需要翻转过来
	// aiProcess_LimitBoneWeights: 限制顶点的骨骼权重最多为 4 个，其余权重无需处理
	// aiProcess_GenBoundBoxes: 对每个网格，都生成一个 AABB 体积盒
	// aiProcess_JoinIdenticalVertices: 将位置相同的顶点合并为一个顶点，从而减少模型的顶点数量，优化内存使用和提升渲染效率。
	UINT ModelImportFlag = aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_FixInfacingNormals |
		aiProcess_LimitBoneWeights | aiProcess_GenBoundingBoxes | aiProcess_JoinIdenticalVertices;

	// 读取模型数据，数据会存储在 aiScene 对象
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
		return 1;
	}

	std::cout << "成功加载 " << ModelFileName << " ! \n" << std::endl;



	// ---------------------------------------------------------------------------------------------------------------



	std::cout << "开始遍历节点! \n\n";

	// 基础偏移字符串，用于打印时区分父子节点
	std::string NodeBaseChar = "";

	// 从根节点开始递归打印
	ModelNodeTraversal(m_ModelScene->mRootNode, NodeBaseChar, 1);

	std::cout << "\n" << "总节点数: " << TotalNodeNum << "\n\n";

	std::cout << "------------------------------------------------------------------------\n\n";



	// ---------------------------------------------------------------------------------------------------------------



	// 在 3D 建模的世界里，材质 (Material) 就像是给模型穿上的一件“外衣”
	// 它不仅决定了模型的颜色和光泽，还能展现出材料的透明度、反射特性以及那些细微的表面纹理

	// 材质组
	std::vector<std::string> MaterialGroup;

	// 遍历模型中的所有材质
	for (UINT i = 0; i < m_ModelScene->mNumMaterials; i++)
	{
		// Assimp 解析出来的模型材质
		aiMaterial* material = m_ModelScene->mMaterials[i];

		// 添加材质名到 MaterialGroup 中
		std::cout << "材质名: " << material->GetName().C_Str() << std::endl;
		MaterialGroup.push_back(material->GetName().C_Str());
		

		// 纹理是材质的子集，一个材质可能有很多组不同类型的纹理
		// 获取材质中的纹理数，目前我们只会用到 EMISSIVE, DIFFUSE, NORMAL 这三种纹理，后面我们会逐一介绍这些纹理的功能与区别
		// 在 Assimp 中，有一些类型 (例如 DIFFUSE 和 BASE_COLOR) 其实指的是同一个纹理，不过会有一些功能上的区别
		std::cout
			<< "EMISSIVE 自发光纹理数: " << material->GetTextureCount(aiTextureType_EMISSIVE) << "\n"
			<< "DIFFUSE 漫反射纹理数: " << material->GetTextureCount(aiTextureType_DIFFUSE) << "\n"
			<< "NORMAL 法线纹理数: " << material->GetTextureCount(aiTextureType_NORMALS) << "\n";

		// 材质对应的纹理文件名
		aiString materialPath;

		// 获取材质对应的纹理，有时候一个材质甚至会有多个名字相同，但是类型不同的纹理贴图
		// Assimp 为了区分这些纹理，特意设置了一个叫 Channel 通道的东西，如果名字相同，不同类型的纹理贴图会占据不同通道
		// GetTexture 的第二个参数就是通道索引，大部分材质同类型下最多只有一个纹理，所以第二个参数直接指定 0 就行
		// 注意 GetTexture 的返回值表示状态，aiReturn_SUCCESS 才算获取成功
		if (material->GetTexture(aiTextureType_EMISSIVE, 0, &materialPath) == aiReturn_SUCCESS)
		{
			std::cout << "EMISSIVE 自发光纹理文件名: " << materialPath.C_Str() << "\n";
		}
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &materialPath) == aiReturn_SUCCESS)
		{
			std::cout << "DIFFUSE 漫反射纹理文件名: " << materialPath.C_Str() << "\n";
		}
		if (material->GetTexture(aiTextureType_NORMALS, 0, &materialPath) == aiReturn_SUCCESS)
		{
			std::cout << "NORMAL 法线纹理文件名: " << materialPath.C_Str() << "\n";
		}

		std::cout << std::endl;

	}
	
	std::cout << "\n" << "总材质数: " << m_ModelScene->mNumMaterials << "\n\n";

	std::cout << "------------------------------------------------------------------------\n\n";



	// ---------------------------------------------------------------------------------------------------------------



	std::cout << "开始遍历网格! \n\n";

	// Mesh 网格相当于模型的皮肤，它存储了模型要渲染的顶点信息。在骨骼模型中，Mesh 需要依赖骨骼节点才能正确渲染
	// 遍历模型的所有 Mesh 网格
	for (UINT i = 0; i < m_ModelScene->mNumMeshes; i++)
	{
		// 当前网格
		const aiMesh* mesh = m_ModelScene->mMeshes[i];

		std::cout
			<< "网格名: " << mesh->mName.C_Str() << "\n"
			<< "顶点数: " << mesh->mNumVertices << "\n"
			<< "索引数: " << mesh->mNumFaces * 3 << "\n"
			<< "所用材质索引: " << mesh->mMaterialIndex
			<< " (对应材质: " << MaterialGroup[mesh->mMaterialIndex] << ") \n";

		// 如果 Mesh 有被骨骼影响到，就输出相关骨骼。对骨骼模型而言非常重要
		// 一个网格可以被多个骨骼影响到，这是因为网格依赖骨骼，附着在骨骼上，通常是整块整块附着
		// 关节是骨骼之间的连接点，这些网格很多都会覆盖关节 (或占据关节部分位置)，网格上的顶点受不同骨骼的影响程度各不相同
		if (mesh->HasBones())
		{
			std::cout << "受影响的骨骼: \n";

			// 遍历骨骼
			for (UINT i = 0; i < mesh->mNumBones; i++)
			{
				std::cout << mesh->mBones[i]->mName.C_Str() << std::endl;
			}
		}
		else	// 没有绑定骨骼，网格上的顶点坐标就表示相对于整个模型的绝对位置，即使是骨骼模型，也会有网格没被骨骼影响到
		{
			std::cout << "没有骨骼影响！\n";
		}

		std::cout << "\n";
	}

	std::cout << "总网格数: " << m_ModelScene->mNumMeshes << "\n\n";



	// ---------------------------------------------------------------------------------------------------------------



	std::cout << "开始计算包围盒! \n\n";

	struct AABB		// AABB 包围盒，下一章有大用
	{
		float minBoundsX;	// 最小坐标点 X 值
		float minBoundsY;	// 最小坐标点 Y 值
		float minBoundsZ;	// 最小坐标点 Z 值

		float maxBoundsX;	// 最大坐标点 X 值
		float maxBoundsY;	// 最大坐标点 Y 值
		float maxBoundsZ;	// 最大坐标点 Z 值
	};

	AABB ModelBoundingBox;		// 模型 AABB 包围盒，用于调整摄像机视野，防止模型在摄像机视野外飞出去

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

	std::cout << "模型包围盒: "
		<< "min(" << ModelBoundingBox.minBoundsX << ","
		<< ModelBoundingBox.minBoundsY << ","
		<< ModelBoundingBox.minBoundsZ << ") "
		<< "max(" << ModelBoundingBox.maxBoundsX << ","
		<< ModelBoundingBox.maxBoundsY << ","
		<< ModelBoundingBox.maxBoundsZ << ")" << std::endl;

	return 0;
}