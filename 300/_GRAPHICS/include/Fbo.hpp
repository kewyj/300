/*!*****************************************************************************
\file Fbo.hpp
\author Lee Fu Sheng Roy
\par DP email: f.lee@digipen.edu
\par Group: Pepe Production
\date 28-09-2023
\brief
FBO class implementation. Creates the framebuffer object. Consists of functions
to handle resizing of framebuffers and object picking
*******************************************************************************/

#ifndef FBO_HPP
#define FBO_HPP

#include "Common.hpp"
#include <xkeycheck.h>
#include <Shader.hpp>
#include <Mesh.hpp>


namespace GFX
{
	struct Quad2D
	{
		float quadVertices[24] =
		{
			// Positions    // Texture coordinates
			-1.0f,  1.0f,    0.0f, 1.0f,
			-1.0f, -1.0f,    0.0f, 0.0f,
			 1.0f, -1.0f,    1.0f, 0.0f,

			-1.0f,  1.0f,    0.0f, 1.0f,
			 1.0f, -1.0f,    1.0f, 0.0f,
			 1.0f,  1.0f,    1.0f, 1.0f
		};

		unsigned int quadVAO, quadVBO;

		Quad2D();
		void Bind();
		void Unbind();
	};


	class FBO
	{
	public:

/*!*****************************************************************************
Creates the framebuffer object and the colorAttachment, Entity ID attachment
*******************************************************************************/
		void Create(int width, int height, bool editorMode = true, bool resize = false);

/*!*****************************************************************************
Prepares the Framebuffer to be drawn by selecting the attachments to be bound
and rendered to
*******************************************************************************/
		void PrepForDraw();

/*!*****************************************************************************
Clears the framebuffer's attachments
*******************************************************************************/
		void Clear();

		void DrawBuffers(bool color = false, bool entityID = false, bool bright = false);

/*!*****************************************************************************
Reads the entity ID in the entityIDAttachment at the posX and posY. posX and 
posY are normalized to be from [0.f, 1.f]
*******************************************************************************/
		unsigned int ReadEntityID(float posX, float posY);

/*!*****************************************************************************
Binds the framebuffer to be rendered to
*******************************************************************************/
		void Bind()		{ glBindFramebuffer(GL_FRAMEBUFFER, mID); }

/*!*****************************************************************************
Unbinds the framebuffer
*******************************************************************************/
		void Unbind()	{ glBindFramebuffer(GL_FRAMEBUFFER, 0); }

		
/*!*****************************************************************************
Returns the ID of the color attachment
*******************************************************************************/
		unsigned int GetColorAttachment()		{ return mColorAttachment; }
		
/*!*****************************************************************************
Returns the ID of the entity ID attachment
*******************************************************************************/
		unsigned int GetEntityIDAttachment()	{ return mEntityIDAttachment; }

/*!*****************************************************************************
Returns the Bright Color Attachment array
*******************************************************************************/
		unsigned int GetBrightColorsAttachment() { return mBrightColorsAttachment; }
		
/*!*****************************************************************************
Returns the width of the framebuffer
*******************************************************************************/
		int GetWidth()							{ return mWidth; }
		
/*!*****************************************************************************
Returns the height of the framebuffer
*******************************************************************************/
		int GetHeight()							{ return mHeight; }

/*!*****************************************************************************
Returns the ID of the framebuffer
*******************************************************************************/
		unsigned GetID()						{ return mID; }

/*!*****************************************************************************
Resizes the size of the framebuffer
*******************************************************************************/
		void Resize(int width, int height);
	
/*!*****************************************************************************
Destructor of the FBO class. Deletes the resources allocated on the GPU
*******************************************************************************/
		~FBO()
		{
			glDeleteFramebuffers(1, &mID);
			glDeleteRenderbuffers(1, &mRboID);
			glDeleteTextures(1, &mColorAttachment);
			glDeleteTextures(1, &mEntityIDAttachment);
			glDeleteTextures(1, &mBrightColorsAttachment);
		}

	public:
		unsigned int mID{};							// Framebuffer Object ID
		unsigned int mRboID{};						// Renderbuffer Object ID
		unsigned int mColorAttachment{};			// Color Attachment of scene
		unsigned int mEntityIDAttachment{};			// Attachment that contains Entity ID of each corresponding pixel
		unsigned int mBrightColorsAttachment;		// Bloom attachment

		int mWidth{};
		int mHeight{};

		// -- Flags --
		bool mEditorMode;		// True: Writes to all attachments
	};


/*!*****************************************************************************
	FBO For Bloom
*******************************************************************************/
	class PingPongFBO
	{
	public:
		void Create(int width, int height);
		void GaussianBlur(GFX::Shader& blurShader, GFX::FBO& hostFramebuffer, float texelOffset, float SamplingWeight);

		// testing
		void GaussianBlurShader(GFX::Shader& blurShader, GFX::FBO& sourceFramebuffer, float SamplingWeight);

		void PrepForDraw();
		void Resize(int width, int height);
		void UnloadAndClear();

		unsigned int pingpongFBO;
		unsigned int pingpongColorbuffers[2];
		unsigned int mblurAmount = 10;
		int mWidth{}, mHeight;

		Quad2D       m_Quad;

		~PingPongFBO()
		{
			glDeleteFramebuffers(1, &pingpongFBO);
			glDeleteTextures(2, pingpongColorbuffers);
		}
	};

	/*!*****************************************************************************
	FBO For Shadow Mapping
	*******************************************************************************/
	class ShadowFBO
	{
	public:
		void Create(int width, int height);
		void PrepForDraw();
		void Clear();
		void Resize(int width, int height);

		unsigned int GetDepthMap() { return mDepthMap; }

		void Bind() { glBindFramebuffer(GL_FRAMEBUFFER, mID); }
		void Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

		unsigned int mID{};			// Framebuffer Object ID
		unsigned int mDepthMap;		// Depth map of ShadowFBO

		int mWidth{};
		int mHeight{};

		~ShadowFBO()
		{
			glDeleteFramebuffers(1, &mID);
			glDeleteTextures(1, &mDepthMap);
		}
	};
}

#endif	//	FBO_HPP