/*
 * Author: Jeesun Kim
 * E-mail: emoy.kim_AT_gmail.com
 * 
 * This code is a free software; it can be freely used, changed and redistributed.
 * If you use any version of the code, please reference the code.
 * 
 */

#pragma once

#include "base.h"
#include "text.h"
#include "light.h"

class RendererGL final
{
public:
   RendererGL();
   ~RendererGL();

   RendererGL(const RendererGL&) = delete;
   RendererGL(const RendererGL&&) = delete;
   RendererGL& operator=(const RendererGL&) = delete;
   RendererGL& operator=(const RendererGL&&) = delete;

   void play();

private:
   inline static RendererGL* Renderer = nullptr;
   GLFWwindow* Window;
   bool Pause;
   int FrameWidth;
   int FrameHeight;
   int ShadowMapSize;
   int SplitNum;
   int ActiveLightIndex;
   GLuint FBO;
   GLuint DepthTextureID;
   glm::ivec2 ClickedPoint;
   std::unique_ptr<TextGL> Texter;
   std::unique_ptr<CameraGL> MainCamera;
   std::unique_ptr<CameraGL> TextCamera;
   std::unique_ptr<CameraGL> LightCamera;
   std::unique_ptr<ShaderGL> TextShader;
   std::unique_ptr<ShaderGL> SceneShader;
   std::unique_ptr<ShaderGL> LightViewShader;
   std::unique_ptr<ObjectGL> WallObject;
   std::unique_ptr<ObjectGL> BunnyObject;
   std::unique_ptr<LightGL> Lights;
   std::vector<float> SplitPositions;

   void registerCallbacks() const;
   void initialize();
   void writeFrame(const std::string& name) const;
   void writeDepthTexture(const std::string& name) const;

   static void printOpenGLInformation();

   static void cleanup(GLFWwindow* window);
   static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
   static void cursor(GLFWwindow* window, double xpos, double ypos);
   static void mouse(GLFWwindow* window, int button, int action, int mods);
   static void mousewheel(GLFWwindow* window, double xoffset, double yoffset);

   void splitViewFrustum();
   void setLights() const;
   void setWallObject() const;
   void setBunnyObject() const;
   void setDepthFrameBuffer();
   void getSplitFrustum(std::array<glm::vec3, 8>& frustum, float near, float far) const;
   static void getBoundingBox(std::array<glm::vec3, 8>& bounding_box, const std::array<glm::vec3, 8>& points);
   [[nodiscard]] glm::mat4 calculateLightCropMatrix(std::array<glm::vec3, 8>& bounding_box) const;

   void drawBoxObject(ShaderGL* shader, const CameraGL* camera) const;
   void drawBunnyObject(ShaderGL* shader, const CameraGL* camera) const;
   void drawDepthMapFromLightView(const glm::mat4& light_crop_matrix) const;
   void drawShadow(const glm::mat4& light_crop_matrix) const;
   void drawText(const std::string& text) const;
   void render() const;
};