
// (9) AssimpAcquaintance: ������ʶ��ʹ�� Assimp �⣬��ȡ����ӡ��ε���������е� ϼ�g���� gltf ģ�͵�����
// ��лԭ���ߴ��: Onerui(momo) (https://sketchfab.com/hswangrui)
// ģ����Ŀ��ַ: https://sketchfab.com/3d-models/blue-archivekasumizawa-miyu-108d81dfd5a44dab92e4dccf0cc51a02


#define NOMINMAX		// windows.h ���׼����� min/max �����������³�ͻ�ˣ����� windows.h ����� min/max ����

#include<Windows.h>		// Windows ���ڱ�̺���ͷ�ļ�

#include<iostream>		// C++ ��׼���������
#include<vector>		// C++ STL vector ������

#include<assimp/Importer.hpp>		// Assimp Importer ģ�͵����������ڵ���ģ��,��ȡģ������
#include<assimp/postprocess.h>		// PostProcess �����ṩ���ֱ�־ (aiProcess_xxx)�����ڸ���ģ�͵ĵ�������������
#include<assimp/scene.h>			// Scene ��������⣬���ڴ洢�������� 3D ģ�͵���������

#pragma comment(lib,"assimp-vc143-mtd.lib")		// ���� Assimp DLL


// 1.��Ŀ -> ���� -> VC++ Ŀ¼ -> ����Ŀ¼ -> ��� Assimp/include (����д $(SolutionDir)Assimp/include )
// 2.��Ŀ -> ���� -> VC++ Ŀ¼ -> ��Ŀ¼ -> ��� Assimp/lib (����д $(SolutionDir)Assimp/lib )
// 3.�� Assimp/lib ����� assimp-vc143-mtd.lib �� assimp-vc143-mtd.dll ���Ƶ�
//   exe ���ڵ�Ŀ¼�� (x64/Debug �ļ���)�����ɵ� exe ��Ҫ�������������ӿ�


// GLTF ��һ�� 3D ģ���ļ���ʽ�����������Ӧ�ó����Ч����ͼ��� 3D ������ģ��
// �����ļ��ṹ���£�
// textures			ģ�������ļ���
// license.txt		ģ��ʹ�õ����֤ (�������֤�� CC-BY: ������������ԭ���ߣ�������)
// scene.bin		����ģ��ͼԪ�����㣬���������ȶ���������
// scene.gltf		JSON �ļ������� scene �Ľṹ��Ԫ�أ������� bin �� textures ������



// ---------------------------------------------------------------------------------------------------------------



UINT TotalNodeNum = 0;		// �ܽڵ���

// �ݹ�չ������ģ�͹����ڵ㣬��ӡÿ�������ڵ�����ƣ��󲿷�����½ڵ㶼�ǹ������ڵ����͹�������ͬ
// ������ģ���еĳ�����ʽ�ǹ��������ڹ��������У����ڵ��Ӱ���ӽڵ㣬�ӽڵ��õ���ƫ�ƾ�����������������ڵ�ģ�����Ҫ�ݹ�չ������
// Assimp �У��ڵ�ı任�����Ӱ��ڵ��µ�ȫ�����ԣ��������񡢹������ӽڵ㣬��������Ψһ��
void ModelNodeTraversal(const aiNode* node, std::string NodeBaseChar, UINT tier)
{
	std::cout << NodeBaseChar
		<< "������ �ڵ���: " << node->mName.C_Str()		// �ڵ���
		<< " (�㼶: " << tier						// �㼶
		<< ", �ӽڵ���: " << node->mNumChildren		// �ӽڵ���
		<< ")" << std::endl;

	// ��ǰ�ڵ��ӡ��ɣ���һ�д�ӡ�ӽڵ�ʱ����ǰ����ӿո񣬱�������
	NodeBaseChar += "  ";

	// �����ӽڵ㣬��ӡ�ӽڵ������
	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		ModelNodeTraversal(node->mChildren[i], NodeBaseChar, tier + 1);
	}

	TotalNodeNum++;		// �ܽڵ��� +1
}


int main()
{
	std::string ModelFileName = "miyu/scene.gltf";					// ģ���ļ���
	Assimp::Importer* m_ModelImporter = new Assimp::Importer;		// ģ�͵�����
	const aiScene* m_ModelScene = nullptr;							// ģ��/��������

	// ����ģ��ʹ�õı�־
	// aiProcess_ConvertToLeftHanded: Assimp �����ģ������ OpenGL ������ϵͳΪ�����ģ���ģ��ת���� DirectX ������ϵͳ
	// aiProcess_Triangulate��ģ�����ʦ����ʹ�ö���ζ�ģ�ͽ��н�ģ�ģ������ö���ν�ģ��ģ�ͣ������Ƕ�ת���ɻ��������ν�ģ
	// aiProcess_FixInfacingNormals����ģ�������˫����ʾ�ģ��������ʦ�������ⶥ�������򣬲�����ᱻ�޳��޷�������ʾ����Ҫ��ת����
	// aiProcess_LimitBoneWeights: ���ƶ���Ĺ���Ȩ�����Ϊ 4 ��������Ȩ�����账��
	// aiProcess_GenBoundBoxes: ��ÿ�����񣬶�����һ�� AABB �����
	// aiProcess_JoinIdenticalVertices: ��λ����ͬ�Ķ���ϲ�Ϊһ�����㣬�Ӷ�����ģ�͵Ķ����������Ż��ڴ�ʹ�ú�������ȾЧ�ʡ�
	UINT ModelImportFlag = aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_FixInfacingNormals |
		aiProcess_LimitBoneWeights | aiProcess_GenBoundingBoxes | aiProcess_JoinIdenticalVertices;

	// ��ȡģ�����ݣ����ݻ�洢�� aiScene ����
	// ʹ�� ReadFile ����ֱ�Ӵ����ļ�·�����ļ���·������������������Щ utf-8 �ַ� (Assimp ����޸������ bug)
	m_ModelScene = m_ModelImporter->ReadFile(ModelFileName, ModelImportFlag);

	// ���ģ��û�гɹ����� (�޷����룬����δ��ɣ�������޸��ڵ�)
	if (!m_ModelScene || m_ModelScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_ModelScene->mRootNode)
	{
		// Assimp ����ģ�͵Ĵ�����Ϣ
		std::string Assimp_error_msg = m_ModelImporter->GetErrorString();

		std::string errorMsg = "�����ļ� ";
		errorMsg += ModelFileName;
		errorMsg += " ʧ�ܣ�����ԭ��";
		errorMsg += Assimp_error_msg;
		MessageBoxA(NULL, errorMsg.c_str(), "����", MB_ICONERROR | MB_OK);
		return 1;
	}

	std::cout << "�ɹ����� " << ModelFileName << " ! \n" << std::endl;



	// ---------------------------------------------------------------------------------------------------------------



	std::cout << "��ʼ�����ڵ�! \n\n";

	// ����ƫ���ַ��������ڴ�ӡʱ���ָ��ӽڵ�
	std::string NodeBaseChar = "";

	// �Ӹ��ڵ㿪ʼ�ݹ��ӡ
	ModelNodeTraversal(m_ModelScene->mRootNode, NodeBaseChar, 1);

	std::cout << "\n" << "�ܽڵ���: " << TotalNodeNum << "\n\n";

	std::cout << "------------------------------------------------------------------------\n\n";



	// ---------------------------------------------------------------------------------------------------------------



	// �� 3D ��ģ����������� (Material) �����Ǹ�ģ�ʹ��ϵ�һ�������¡�
	// ������������ģ�͵���ɫ�͹��󣬻���չ�ֳ����ϵ�͸���ȡ����������Լ���Щϸ΢�ı�������

	// ������
	std::vector<std::string> MaterialGroup;

	// ����ģ���е����в���
	for (UINT i = 0; i < m_ModelScene->mNumMaterials; i++)
	{
		// Assimp ����������ģ�Ͳ���
		aiMaterial* material = m_ModelScene->mMaterials[i];

		// ��Ӳ������� MaterialGroup ��
		std::cout << "������: " << material->GetName().C_Str() << std::endl;
		MaterialGroup.push_back(material->GetName().C_Str());
		

		// �����ǲ��ʵ��Ӽ���һ�����ʿ����кܶ��鲻ͬ���͵�����
		// ��ȡ�����е���������Ŀǰ����ֻ���õ� EMISSIVE, DIFFUSE, NORMAL �����������������ǻ���һ������Щ����Ĺ���������
		// �� Assimp �У���һЩ���� (���� DIFFUSE �� BASE_COLOR) ��ʵָ����ͬһ��������������һЩ�����ϵ�����
		std::cout
			<< "EMISSIVE �Է���������: " << material->GetTextureCount(aiTextureType_EMISSIVE) << "\n"
			<< "DIFFUSE ������������: " << material->GetTextureCount(aiTextureType_DIFFUSE) << "\n"
			<< "NORMAL ����������: " << material->GetTextureCount(aiTextureType_NORMALS) << "\n";

		// ���ʶ�Ӧ�������ļ���
		aiString materialPath;

		// ��ȡ���ʶ�Ӧ��������ʱ��һ�������������ж��ͬ���͵�������Щͬ���͵������൱����Ϸ��ɫ�Ĳ�ͬƤ��
		// Assimp Ϊ��������Щ��������������һ���� Channel ͨ���Ķ��������������ͬ����ͬ�����ռ�ݲ�ͬͨ��
		// GetTexture �ĵڶ�����������ͨ���������󲿷ֲ���ͬ���������ֻ��һ���������Եڶ�������ֱ��ָ�� 0 ����
		// ע�� GetTexture �ķ���ֵ��ʾ״̬��aiReturn_SUCCESS �����ȡ�ɹ�
		if (material->GetTexture(aiTextureType_EMISSIVE, 0, &materialPath) == aiReturn_SUCCESS)
		{
			std::cout << "EMISSIVE �Է��������ļ���: " << materialPath.C_Str() << "\n";
		}
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &materialPath) == aiReturn_SUCCESS)
		{
			std::cout << "DIFFUSE �����������ļ���: " << materialPath.C_Str() << "\n";
		}
		if (material->GetTexture(aiTextureType_NORMALS, 0, &materialPath) == aiReturn_SUCCESS)
		{
			std::cout << "NORMAL ���������ļ���: " << materialPath.C_Str() << "\n";
		}

		std::cout << std::endl;

	}
	
	std::cout << "\n" << "�ܲ�����: " << m_ModelScene->mNumMaterials << "\n\n";

	std::cout << "------------------------------------------------------------------------\n\n";



	// ---------------------------------------------------------------------------------------------------------------



	std::cout << "��ʼ��������! \n\n";

	// Mesh �����൱��ģ�͵�Ƥ�������洢��ģ��Ҫ��Ⱦ�Ķ�����Ϣ���ڹ���ģ���У�Mesh ��Ҫ���������ڵ������ȷ��Ⱦ
	// ����ģ�͵����� Mesh ����
	for (UINT i = 0; i < m_ModelScene->mNumMeshes; i++)
	{
		// ��ǰ����
		const aiMesh* mesh = m_ModelScene->mMeshes[i];

		std::cout
			<< "������: " << mesh->mName.C_Str() << "\n"
			<< "������: " << mesh->mNumVertices << "\n"
			<< "������: " << mesh->mNumFaces * 3 << "\n"
			<< "���ò�������: " << mesh->mMaterialIndex
			<< " (��Ӧ����: " << MaterialGroup[mesh->mMaterialIndex] << ") \n";

		// ��� Mesh �б�����Ӱ�쵽���������ع������Թ���ģ�Ͷ��Էǳ���Ҫ
		// һ��������Ա��������Ӱ�쵽��������Ϊ�������������������ڹ����ϣ�ͨ�����������鸽��
		// �ؽ��ǹ���֮������ӵ㣬��Щ����ܶ඼�Ḳ�ǹؽ� (��ռ�ݹؽڲ���λ��)�������ϵĶ����ܲ�ͬ������Ӱ��̶ȸ�����ͬ
		if (mesh->HasBones())
		{
			std::cout << "��Ӱ��Ĺ���: \n";

			// ��������
			for (UINT i = 0; i < mesh->mNumBones; i++)
			{
				std::cout << mesh->mBones[i]->mName.C_Str() << std::endl;
			}
		}
		else	// û�а󶨹����������ϵĶ�������ͱ�ʾ���������ģ�͵ľ���λ�ã���ʹ�ǹ���ģ�ͣ�Ҳ��������û������Ӱ�쵽
		{
			std::cout << "û�й���Ӱ�죡\n";
		}

		std::cout << "\n";
	}

	std::cout << "��������: " << m_ModelScene->mNumMeshes << "\n\n";



	// ---------------------------------------------------------------------------------------------------------------



	std::cout << "��ʼ�����Χ��! \n\n";

	struct AABB		// AABB ��Χ�У���һ���д���
	{
		float minBoundsX;	// ��С����� X ֵ
		float minBoundsY;	// ��С����� Y ֵ
		float minBoundsZ;	// ��С����� Z ֵ

		float maxBoundsX;	// �������� X ֵ
		float maxBoundsY;	// �������� Y ֵ
		float maxBoundsZ;	// �������� Z ֵ
	};

	AABB ModelBoundingBox;		// ģ�� AABB ��Χ�У����ڵ����������Ұ����ֹģ�����������Ұ��ɳ�ȥ

	// ���ó�ʼֵ
	ModelBoundingBox =
	{
		m_ModelScene->mMeshes[0]->mAABB.mMin.x,
		m_ModelScene->mMeshes[0]->mAABB.mMin.y,
		m_ModelScene->mMeshes[0]->mAABB.mMin.z,

		m_ModelScene->mMeshes[0]->mAABB.mMax.x,
		m_ModelScene->mMeshes[0]->mAABB.mMax.y,
		m_ModelScene->mMeshes[0]->mAABB.mMax.z
	};

	// �������������������ģ�͵� AABB ��Χ�У���ע�⵼��ģ��ʱҪָ�� aiProcess_GenBoundingBoxes������ mAABB ��Ա��û������
	for (UINT i = 1; i < m_ModelScene->mNumMeshes; i++)
	{
		// ��ǰ����
		const aiMesh* mesh = m_ModelScene->mMeshes[i];

		// �����ܰ�Χ��
		ModelBoundingBox.minBoundsX = std::min(mesh->mAABB.mMin.x, ModelBoundingBox.minBoundsX);
		ModelBoundingBox.minBoundsY = std::min(mesh->mAABB.mMin.y, ModelBoundingBox.minBoundsY);
		ModelBoundingBox.minBoundsZ = std::min(mesh->mAABB.mMin.z, ModelBoundingBox.minBoundsZ);

		ModelBoundingBox.maxBoundsX = std::max(mesh->mAABB.mMax.x, ModelBoundingBox.maxBoundsX);
		ModelBoundingBox.maxBoundsY = std::max(mesh->mAABB.mMax.y, ModelBoundingBox.maxBoundsY);
		ModelBoundingBox.maxBoundsZ = std::max(mesh->mAABB.mMax.z, ModelBoundingBox.maxBoundsZ);
	}

	std::cout << "ģ�Ͱ�Χ��: "
		<< "min(" << ModelBoundingBox.minBoundsX << ","
		<< ModelBoundingBox.minBoundsY << ","
		<< ModelBoundingBox.minBoundsZ << ") "
		<< "max(" << ModelBoundingBox.maxBoundsX << ","
		<< ModelBoundingBox.maxBoundsY << ","
		<< ModelBoundingBox.maxBoundsZ << ")" << std::endl;

	return 0;
}