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
#include "light.h"
#include "object.h"

class RendererGL
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
   int FrameWidth;
   int FrameHeight;
   int ShadowMapSize;
   int SplitNum;
   GLuint FBO;
   GLuint DepthTextureID;
   glm::ivec2 ClickedPoint;
   float LightTheta;
   std::unique_ptr<CameraGL> MainCamera;
   std::unique_ptr<CameraGL> LightCamera;
   std::unique_ptr<ShaderGL> ObjectShader;
   std::unique_ptr<ShaderGL> ShadowShader;
   std::unique_ptr<ObjectGL> GroundObject;
   std::unique_ptr<ObjectGL> TigerObject;
   std::unique_ptr<ObjectGL> PandaObject;
   std::unique_ptr<LightGL> Lights;
   std::vector<float> SplitPositions;

   void registerCallbacks() const;
   void initialize();
   void writeDepthTexture(const std::string& name) const;

   static void printOpenGLInformation();

   void cleanup(GLFWwindow* window);
   void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
   void cursor(GLFWwindow* window, double xpos, double ypos);
   void mouse(GLFWwindow* window, int button, int action, int mods);
   void mousewheel(GLFWwindow* window, double xoffset, double yoffset) const;
   void reshape(GLFWwindow* window, int width, int height) const;
   static void cleanupWrapper(GLFWwindow* window);
   static void keyboardWrapper(GLFWwindow* window, int key, int scancode, int action, int mods);
   static void cursorWrapper(GLFWwindow* window, double xpos, double ypos);
   static void mouseWrapper(GLFWwindow* window, int button, int action, int mods);
   static void mousewheelWrapper(GLFWwindow* window, double xoffset, double yoffset);
   static void reshapeWrapper(GLFWwindow* window, int width, int height);

   void splitViewFrustum();
   void setLights() const;
   void setGroundObject() const;
   void setTigerObject() const;
   void setPandaObject() const;
   void setDepthFrameBuffer();
   void getSplitFrustum(std::array<glm::vec3, 8>& frustum, float near, float far) const;
   static void getBoundingBox(std::array<glm::vec3, 8>& bounding_box, const std::array<glm::vec3, 8>& points);
   [[nodiscard]] glm::mat4 calculateLightCropMatrix(std::array<glm::vec3, 8>& bounding_box) const;

   void drawGroundObject(ShaderGL* shader, CameraGL* camera) const;
   void drawTigerObject(ShaderGL* shader, CameraGL* camera) const;
   void drawPandaObject(ShaderGL* shader, CameraGL* camera) const;
   void drawDepthMapFromLightView(const glm::mat4& light_crop_matrix) const;
   void drawShadow(const glm::mat4& light_crop_matrix, int light_index = 0) const;
   void render() const;
};