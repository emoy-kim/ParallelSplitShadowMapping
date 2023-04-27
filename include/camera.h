#pragma once

#include "base.h"

class CameraGL
{
public:
   CameraGL();
   CameraGL(
      const glm::vec3& cam_position,
      const glm::vec3& view_reference_position,
      const glm::vec3& view_up_vector,
      float fov = 60.0f,
      float near_plane = 1.0f,
      float far_plane = 1000.0f
   );

   [[nodiscard]] bool getMovingState() const { return IsMoving; }
   [[nodiscard]] float getFOV() const { return FOV; }
   [[nodiscard]] float getNearPlane() const { return NearPlane; }
   [[nodiscard]] float getFarPlane() const { return FarPlane; }
   [[nodiscard]] float getAspectRatio() const { return AspectRatio; }
   [[nodiscard]] glm::vec3 getInitialCameraPosition() const { return InitCamPos; }
   [[nodiscard]] glm::vec3 getInitialReferencePosition() const { return InitRefPos; }
   [[nodiscard]] glm::vec3 getInitialUpVector() const { return InitUpVec; }
   [[nodiscard]] glm::vec3 getCameraPosition() const { return CamPos; }
   [[nodiscard]] const glm::mat4& getViewMatrix() const { return ViewMatrix; }
   [[nodiscard]] const glm::mat4& getProjectionMatrix() const { return ProjectionMatrix; }
   [[nodiscard]] float linearizeDepthValue(float depth) const;
   void setMovingState(bool is_moving) { IsMoving = is_moving; }
   void updateCamera();
   void pitch(int angle);
   void yaw(int angle);
   void rotateAroundWorldY(int angle);
   void moveForward(int delta = 1);
   void moveBackward(int delta = 1);
   void moveLeft(int delta = 1);
   void moveRight(int delta = 1);
   void moveUp(int delta = 1);
   void moveDown(int delta = 1);
   void zoomIn();
   void zoomOut();
   void resetCamera();
   void updateOrthographicCamera(int width, int height);
   void updatePerspectiveCamera(int width, int height);
   void updateCameraPosition(
      const glm::vec3& cam_position,
      const glm::vec3& view_reference_position,
      const glm::vec3& view_up_vector
   );

private:
   bool IsPerspective;
   bool IsMoving;
   int Width;
   int Height;
   float FOV;
   float InitFOV;
   float NearPlane;
   float FarPlane;
   float AspectRatio;
   const float ZoomSensitivity;
   const float MoveSensitivity;
   const float RotationSensitivity;
   glm::vec3 InitCamPos;
   glm::vec3 InitRefPos;
   glm::vec3 InitUpVec;
   glm::vec3 CamPos;
   glm::mat4 ViewMatrix;
   glm::mat4 ProjectionMatrix;
};