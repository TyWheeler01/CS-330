///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declare the global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	//load the various textures for the pyramid structure and different object meshes
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/plastic.jpg",
		"plastic");

	bReturn = CreateGLTexture(
		"textures/wood.jpg",
		"wood");

	bReturn = CreateGLTexture(
		"textures/red.jpg",
		"red");

	bReturn = CreateGLTexture(
		"textures/grip.jpg",
		"grip");

	bReturn = CreateGLTexture(
		"textures/brick.jpg",
		"brick");

	bReturn = CreateGLTexture(
		"textures/blue.jpg",
		"blue");

	bReturn = CreateGLTexture(
		"textures/silver.jpg",
		"silver");

	bReturn = CreateGLTexture(
		"textures/yellow.jpg",
		"yellow");

	bReturn = CreateGLTexture(
		"textures/metal2.jpg",
		"metal2");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		// find the defined material that matches the tag
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			// pass the material properties into the shader
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
	OBJECT_MATERIAL goldMaterial; //first more reflective materal
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 5.0f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 12.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial; //second less reflective materal
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 2.0f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 4.0;
	woodMaterial.tag = "wood";

	OBJECT_MATERIAL plasticMaterial; //second less reflective materal
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 1.5f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 1.0;
	woodMaterial.tag = "plastic";


}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable custom lighting in shaders
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 10.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 42.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[1].position", -15.0f, 10.0f, -15.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 38.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 20.0f, 10.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 74.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[3].position", 0.0f, 0.0f, -25.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 26.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 5.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.540, 0.540, 0.540);
	//Set the texture of the table to wood
	SetShaderTexture("wood");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	//First Clinder Flashlight Base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 4.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.184, 0.310, 0.310, 1);
	//set the flashlight piece to the plastic texture
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Clyinder for flashlight grip
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 4.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.502, 0.502, 0.502, 1);
	SetShaderTexture("grip");
	SetTextureUVScale(0.5, 0.5);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Second cylinder for base of flashlight after grip
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 6.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.184, 0.310, 0.310, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Third base cylinder tapering off slightly
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 3.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 7.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.184, 0.310, 0.310, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Fourth cylinder for base back to original width near battery compartment
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.7f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.184, 0.310, 0.310, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//Fifth cylinder for the flashlight button
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.7f, 0.3f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 11.6f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.000, 0.000, 0.000, 1);
	SetShaderTexture("red");
	SetTextureUVScale(2.0, 2.0);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//First box representing the flashlight belt clip
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.1f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 11.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//Second box representing the flashlight belt clip
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.5f, 9.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// OBJECT 2: Card Box
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(12.0f, 14.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.0f, 7.2f, -12.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("blue");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	//Card Box logo using cones
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 2.9f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.0f, 9.8f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	//Card Box logo using cones 2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 2.9f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 60.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-14.0f, 9.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	//Card Box logo using cones 3
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 2.9f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -60.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.6f, 9.0f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	//Card Box logo using cones 4
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 2.9f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -150.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.2f, 7.4f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	//Card Box logo using cones 5
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 2.9f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 150.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.3f, 7.4f, -7.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();

	//Card Box logo using cylinder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 0.3f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.3f, 8.4f, -7.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// OBJECT 3: Wallet
	// 
	// Bottom of metallic wallet
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.4f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 0.6f, -13.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("metal2");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Middle of wallet holding plastic cards
	// Card 1
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.2f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 0.9f, -13.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("red");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Card 2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.2f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 1.1f, -13.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("blue");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Card 3
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.2f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 1.3f, -13.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("yellow");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Top of metallic wallet
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.4f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 1.6f, -13.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("metal2");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Elastic holding wallet together 1
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.4f, 4.4f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 1.15f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Elastic holding wallet together 2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.4f, 4.4f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 1.15f, -18.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Elastic holding wallet together 3
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.4f, 3.0f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 1.15f, -13.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Elastic holding wallet together 4
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.4f, 3.0f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.0f, 1.15f, -13.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Wallet logo
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(8.4f, 1.8f, -9.5f);;

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	//OBJECT 4: Metallic silver watch
	// Watch face
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.7f, 0.3f, 1.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.0f, 0.5f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0, 1.0, 1.0, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// Watch face border
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.8f, 0.3f, 1.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.0f, 0.4f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("metal2");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// Watch button
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2f, 0.1f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.85f, 0.54f, 3.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	// Watch link left
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 0.3f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-15.0f, 0.52f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Watch link right
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.2f, 0.3f, 2.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.0f, 0.52f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Watch link right
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.5f, 0.3f, 1.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.0f, 0.2f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("silver");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Watch face minute hand
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.1f, 1.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-13.2f, 0.9f, 1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// Watch face hour hand
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-12.9f, 0.9f, 1.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.663, 0.663, 0.663, 1);
	SetShaderTexture("plastic");
	SetTextureUVScale(0.1, 0.1);
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
}