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

// declaration of global variables
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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
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

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh(); // desk
	m_basicMeshes->LoadCylinderMesh(); // mug, pencil holder, lamp arm
	m_basicMeshes->LoadTorusMesh(); // mug handle, coffee

	m_basicMeshes->LoadBoxMesh(); // notebook, lamp base
	m_basicMeshes->LoadConeMesh(); // lamp shade
	m_basicMeshes->LoadSphereMesh(); // lamp bulb

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

	// ======================================================
	// DESK SURFACE - (PLANE)
	// -Creates a flat surface for the mug to sit on
	// ======================================================

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

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

	SetShaderColor(1, 1, 1, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// ======================================================
	// COFFEE MUG - BODY
	// Scaled taller to form the mug shape
	// Positioned on the desk surface
	// ======================================================

	scaleXYZ = glm::vec3(1.0f, 1.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.5f, 0.2f, 5.2f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.9f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// ======================================================
	// COFFEE MUG - HANDLE
	// Scaled smaller to form the handle
	// Positioned to the right side of the mug
	// ======================================================

	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.5f, 0.8f, 5.2f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.9f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	// ======================================================
	// COFFEE MUG - TOP SURFACE (LIQUID)
	// - Creates a flat surface that sits on top of the mug
	// - The image makes it look like there's coffee inside
	// ======================================================

	scaleXYZ = glm::vec3(0.8f, 0.0f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.5f, 1.72f, 5.2f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.4f, 0.25f, 0.1f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

// ====================================================================
// NOTEBOOK
// ====================================================================

	// ====================================================================
	// NOTEBOOK - PAGE LINES (BOX)
	// ====================================================================

		//RED LINE

	scaleXYZ = glm::vec3(3.8f, 0.01f, 0.02f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.03f, 0.31f, 6.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.6f, 0.6f, 1.0f);

	m_basicMeshes->DrawBoxMesh();


		// PAGE LINE 1

	scaleXYZ = glm::vec3(2.8f, 0.01f, 0.02f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 4.6f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();


	// PAGE LINE 2
	// 
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 4.8f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 3
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 5.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 4
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 5.2f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 5
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 5.4f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 6
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 5.6f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 7
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 5.8f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 8
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 6.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 9
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 6.2f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 10
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 6.4f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 11
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 6.6f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 12
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 6.8f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 13
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 7.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 14
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 7.2f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// PAGE LINE 15
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 7.4f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();


	// LAST PAGE LINE
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.31f, 7.62f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawBoxMesh();




	// ====================================================================
	// NOTEBOOK - (BOX)
	// ====================================================================

	scaleXYZ = glm::vec3(3.0f, 0.2f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.2f, 6.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);

	m_basicMeshes->DrawBoxMesh();


	// ====================================================================
	// NOTEBOOK - PAGES (BOX)
	// ====================================================================

	scaleXYZ = glm::vec3(2.8f, 0.18f, 3.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.0f, 0.22f, 6.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.95f, 0.95f, 0.9f, 1.0f);

	m_basicMeshes->DrawBoxMesh();

	// ====================================================================
	// NOTEBOOK - SPIRAL RING (TORUS)
	// ====================================================================

	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	//	RING 1	
	positionXYZ = glm::vec3(-0.4f, 0.35f, 7.7f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// MULTIPLE DIFFERENT RINGS

	// RING 2
	positionXYZ = glm::vec3(-0.4f, 0.35f, 7.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 3
	positionXYZ = glm::vec3(-0.4f, 0.35f, 7.3f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 4
	positionXYZ = glm::vec3(-0.4f, 0.35f, 7.1f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 5
	positionXYZ = glm::vec3(-0.4f, 0.35f, 6.9f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 6
	positionXYZ = glm::vec3(-0.4f, 0.35f, 6.7f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 7
	positionXYZ = glm::vec3(-0.4f, 0.35f, 6.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 8
	positionXYZ = glm::vec3(-0.4f, 0.35f, 6.3f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 9
	positionXYZ = glm::vec3(-0.4f, 0.35f, 6.1f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 10
	positionXYZ = glm::vec3(-0.4f, 0.35f, 5.9f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 11
	positionXYZ = glm::vec3(-0.4f, 0.35f, 5.7f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 12
	positionXYZ = glm::vec3(-0.4f, 0.35f, 5.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 13
	positionXYZ = glm::vec3(-0.4f, 0.35f, 5.3f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();
	
	// RING 14
	positionXYZ = glm::vec3(-0.4f, 0.35f, 5.1f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();


	// RING 15
	positionXYZ = glm::vec3(-0.4f, 0.35f, 4.9f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();


	// RING 16
	positionXYZ = glm::vec3(-0.4f, 0.35f, 4.7f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 17
	positionXYZ = glm::vec3(-0.4f, 0.35f, 4.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();

	// RING 18 - (LAST RING)
	positionXYZ = glm::vec3(-0.4f, 0.35f, 4.3f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawTorusMesh();
//===============================================================================================


	// ====================================================================
	// PENCIL HOLDER - (CYLINDER)
	// ====================================================================

	scaleXYZ = glm::vec3(0.8f, 2.0f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.2f, 4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();


// ====================================================================
// PENCIL IN HOLDER
// ====================================================================


	// ====================================================================
	// PENCIL - BODY (CYLINDER)
	// ====================================================================


	scaleXYZ = glm::vec3(0.1f, 1.5f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 2.0f, 4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.9f, 0.2f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();


	// ====================================================================
	// PENCIL - ERASER (CYLINDER)
	// ====================================================================


	scaleXYZ = glm::vec3(0.1f, 0.15f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.5f, 3.73f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.6f, 0.7f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

	// ====================================================================
	// PENCIL - METAL RING (CYLINDER)
	// ====================================================================


	scaleXYZ = glm::vec3(0.1f, 0.08f, 0.102f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.45f, 3.74f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

// =============================================================================================


// ====================================================================
// PENCIL ON TABLE 
// ====================================================================


	// ====================================================================
	// PENCIL - BODY (CYLINDER)
	// ====================================================================


	scaleXYZ = glm::vec3(0.1f, 2.2f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.6f, 6.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.9f, 0.2f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();


	// ====================================================================
	// PENCIL - ERASER (CYLINDER)
	// ====================================================================


	scaleXYZ = glm::vec3(0.1f, 0.15f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.6f, 5.9f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.6f, 0.7f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

	// ====================================================================
	// PENCIL - METAL RING (CYLINDER)
	// ====================================================================


	scaleXYZ = glm::vec3(0.1f, 0.08f, 0.102f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.6f, 6.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();


// ====================================================================
// LAMP
// ====================================================================


	// ====================================================================
	// LAMP BASE - (BOX)
	// ====================================================================

	scaleXYZ = glm::vec3(1.5f, 0.5f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.5f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f);

	m_basicMeshes->DrawBoxMesh();


	// ====================================================================
	// LAMP ARM - (CYLINDER)
	// ====================================================================

	scaleXYZ = glm::vec3(0.2f, 3.0f, 0.2f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.5f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

	// ====================================================================
	// LAMP SHADE - (CONE)
	// ====================================================================

	scaleXYZ = glm::vec3(2.0f, 2.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 3.5f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.9f, 0.8f, 0.6f, 1.0f);

	m_basicMeshes->DrawConeMesh();

	// ====================================================================
	// RING AT THE BOTTOM OF LAMP SHADE - (TORUS)
	// ====================================================================

	scaleXYZ = glm::vec3(2.1f, 0.1f, 2.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 3.5f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.85f, 0.75f, 0.55f, 1.0f);

	m_basicMeshes->DrawConeMesh();


	// ====================================================================
	// LAMP PULL CHAIN - (CYLINDER)
	// ====================================================================

	scaleXYZ = glm::vec3(0.05f, 1.5f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 2.0f, 4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawCylinderMesh();

	// ====================================================================
	// LAMP PULL CHAIN BALL - (SPHERE)
	// ====================================================================

	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 2.0f, 4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);

	m_basicMeshes->DrawSphereMesh();

	// ====================================================================
	// LAMP BULB - (SPHERE)
	// ====================================================================

	scaleXYZ = glm::vec3(0.4f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 3.2f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.95f, 0.7f, 1.0f);

	m_basicMeshes->DrawSphereMesh();




}
