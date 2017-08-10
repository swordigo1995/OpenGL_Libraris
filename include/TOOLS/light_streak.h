#pragma once

#include <GL/glew.h>
// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// GL includes
#include "Shader.h"

class LightStreaker
{
public:
	LightStreaker(GLuint highlightMap, GLuint screenWidth, GLuint screenHeight) : highlightMap(highlightMap), SCR_WIDTH(screenWidth), SCR_HEIGHT(screenHeight)
	{
		setupIntermediateFrameBuffer();
		setupPingPongFrameBuffers();
	}
	//draw filter result to intermediate frame buffer
	//--------------------------------- 4--------------------- glm::vec2(1, 1)------------ 0.5---------------2-------------------
	void draw(Shader kawaseLightStreak, GLuint iterationNum, glm::vec2 streakDirection, GLfloat attenuation, GLuint streakSamples)
	{
		GLboolean first_itertation = true, bufferChoose = true;
		GLuint iterNum = iterationNum;
		//kawaseBlur.Use();
		kawaseLightStreak.Use();
		for (GLuint i = 0; i < iterNum; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[bufferChoose]);
			//glUniform1i(glGetUniformLocation(kawaseBlur.program, "iteration"), i);
			glUniform1i(glGetUniformLocation(kawaseLightStreak.program, "iteration"), i);
			glUniform2fv(glGetUniformLocation(kawaseLightStreak.program, "streakDirection"), 1, glm::value_ptr(streakDirection)); // X direction
			//glUniform2f(glGetUniformLocation(kawaseLightStreak.program, "streakDirection"), -1, 1); // Y direction
			glUniform1f(glGetUniformLocation(kawaseLightStreak.program, "attenuation"), attenuation); //about 0.9 - 0.95
			glUniform1i(glGetUniformLocation(kawaseLightStreak.program, "streakSamples"), streakSamples);
			glBindTexture(GL_TEXTURE_2D, first_itertation ? highlightMap : pingpongColorbuffers[!bufferChoose]);
			RenderQuad();
			bufferChoose = !bufferChoose;
			if (first_itertation)
				first_itertation = false;
		}
		//save filtered image one into starFitlerFrameBuffer[0]

		glBindFramebuffer(GL_READ_FRAMEBUFFER, pingpongFBO[!bufferChoose]);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
		glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

	GLuint getFilteredTexture() const
	{
		return intermediateCB;
	}
	~LightStreaker()
	{
		glDeleteFramebuffers(1, &intermediateFBO);
	}
private:
	GLuint intermediateFBO, intermediateCB; // for intermediate framebuffer
	GLuint pingpongFBO[2], pingpongColorbuffers[2];// for framebuffers to blur with iteration
	GLuint SCR_WIDTH, SCR_HEIGHT;
	GLuint highlightMap;

	void setupIntermediateFrameBuffer()
	{
		glGenFramebuffers(1, &intermediateFBO);
		glGenTextures(1, &intermediateCB);
		glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
		glBindTexture(GL_TEXTURE_2D, intermediateCB);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intermediateCB, 0);
		// Also check if framebuffers are complete (no need for depth buffer)
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	//this buffer can set outside this class
	void setupPingPongFrameBuffers()
	{
		glGenFramebuffers(2, pingpongFBO);
		glGenTextures(2, pingpongColorbuffers);
		for (GLuint i = 0; i < 2; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
			glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // We clamp to the edge as the blur filter would otherwise sample repeated texture values!
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
			// Also check if framebuffers are complete (no need for depth buffer)
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "Framebuffer not complete!" << std::endl;
		}
	}

	// RenderQuad() Renders a 1x1 quad in NDC
	GLuint quadVAO = 0;
	GLuint quadVBO;
	void RenderQuad()
	{
		if (quadVAO == 0)
		{
			GLfloat quadVertices[] = {
				// Positions     //TexCoords  // Normal
				-1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
				1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
				1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0,
			};
			// Setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}
};