#include "renderer.h"

RendererGL::RendererGL() :
   Window( nullptr ), FrameWidth( 1920 ), FrameHeight( 1080 ), ShadowMapSize( 1024 ), SplitNum( 4 ),
   ActiveLightIndex( 0 ), FBO( 0 ), DepthTextureID( 0 ), ClickedPoint( -1, -1 ), LightTheta( 0.0f ),
   MainCamera( std::make_unique<CameraGL>() ), LightCamera( std::make_unique<CameraGL>() ),
   ObjectShader( std::make_unique<ShaderGL>() ), ShadowShader( std::make_unique<ShaderGL>() ),
   GroundObject( std::make_unique<ObjectGL>() ), TigerObject( std::make_unique<ObjectGL>() ),
   PandaObject( std::make_unique<ObjectGL>() ), Lights( std::make_unique<LightGL>() )
{
   Renderer = this;

   initialize();
   printOpenGLInformation();
}

RendererGL::~RendererGL()
{
   if (DepthTextureID != 0) glDeleteTextures( 1, &DepthTextureID );
   if (FBO != 0) glDeleteFramebuffers( 1, &FBO );
}

void RendererGL::printOpenGLInformation()
{
   std::cout << "****************************************************************\n";
   std::cout << " - GLFW version supported: " << glfwGetVersionString() << "\n";
   std::cout << " - OpenGL renderer: " << glGetString( GL_RENDERER ) << "\n";
   std::cout << " - OpenGL version supported: " << glGetString( GL_VERSION ) << "\n";
   std::cout << " - OpenGL shader version supported: " << glGetString( GL_SHADING_LANGUAGE_VERSION ) << "\n";
   std::cout << "****************************************************************\n\n";
}

void RendererGL::initialize()
{
   if (!glfwInit()) {
      std::cout << "Cannot Initialize OpenGL...\n";
      return;
   }
   glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
   glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 6 );
   glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );
   glfwWindowHint( GLFW_DEPTH_BITS, 32 );
   glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

   Window = glfwCreateWindow( FrameWidth, FrameHeight, "Main Camera", nullptr, nullptr );
   glfwMakeContextCurrent( Window );

   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return;
   }

   registerCallbacks();

   glEnable( GL_DEPTH_TEST );
   glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );

   MainCamera->updatePerspectiveCamera( FrameWidth, FrameHeight );
   LightCamera->updateOrthographicCamera( ShadowMapSize, ShadowMapSize );

   const std::string shader_directory_path = std::string(CMAKE_SOURCE_DIR) + "/shaders";
   ObjectShader->setShader(
      std::string(shader_directory_path + "/BasicPipeline.vert").c_str(),
      std::string(shader_directory_path + "/BasicPipeline.frag").c_str()
   );
   ShadowShader->setShader(
      std::string(shader_directory_path + "/Shadow.vert").c_str(),
      std::string(shader_directory_path + "/Shadow.frag").c_str()
   );
}

void RendererGL::writeDepthTexture(const std::string& name) const
{
   const int size = ShadowMapSize * ShadowMapSize;
   auto* buffer = new uint8_t[size];
   auto* raw_buffer = new GLfloat[size];
   glGetTextureImage( DepthTextureID, 0, GL_DEPTH_COMPONENT, GL_FLOAT, static_cast<GLsizei>(size * sizeof( GLfloat )), raw_buffer );

   for (int i = 0; i < size; ++i) {
      buffer[i] = static_cast<uint8_t>(LightCamera->linearizeDepthValue( raw_buffer[i] ) * 255.0f);
   }

   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, ShadowMapSize, ShadowMapSize, ShadowMapSize, 8,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, name.c_str() );
   FreeImage_Unload( image );
   delete [] raw_buffer;
   delete [] buffer;
}

void RendererGL::cleanup(GLFWwindow* window)
{
   glfwSetWindowShouldClose( window, GLFW_TRUE );
}

void RendererGL::cleanupWrapper(GLFWwindow* window)
{
   Renderer->cleanup( window );
}

void RendererGL::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (action != GLFW_PRESS) return;

   switch (key) {
      case GLFW_KEY_UP:
         MainCamera->moveForward();
         break;
      case GLFW_KEY_DOWN:
         MainCamera->moveBackward();
         break;
      case GLFW_KEY_LEFT:
         MainCamera->moveLeft();
         break;
      case GLFW_KEY_RIGHT:
         MainCamera->moveRight();
         break;
      case GLFW_KEY_W:
         MainCamera->moveUp();
         break;
      case GLFW_KEY_S:
         MainCamera->moveDown();
         break;
      case GLFW_KEY_I:
         MainCamera->resetCamera();
         break;
      case GLFW_KEY_L:
         Lights->toggleLightSwitch();
         std::cout << "Light Turned " << (Lights->isLightOn() ? "On!\n" : "Off!\n");
         break;
      case GLFW_KEY_P: {
         const glm::vec3 pos = MainCamera->getCameraPosition();
         std::cout << "Camera Position: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
      } break;
      case GLFW_KEY_Q:
      case GLFW_KEY_ESCAPE:
         cleanupWrapper( window );
         break;
      default:
         return;
   }
}

void RendererGL::keyboardWrapper(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   Renderer->keyboard( window, key, scancode, action, mods );
}

void RendererGL::cursor(GLFWwindow* window, double xpos, double ypos)
{
   if (MainCamera->getMovingState()) {
      const auto x = static_cast<int>(round( xpos ));
      const auto y = static_cast<int>(round( ypos ));
      const int dx = x - ClickedPoint.x;
      const int dy = y - ClickedPoint.y;
      MainCamera->moveForward( -dy );
      MainCamera->rotateAroundWorldY( -dx );

      if (glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS) {
         MainCamera->pitch( -dy );
      }

      ClickedPoint.x = x;
      ClickedPoint.y = y;
   }
}

void RendererGL::cursorWrapper(GLFWwindow* window, double xpos, double ypos)
{
   Renderer->cursor( window, xpos, ypos );
}

void RendererGL::mouse(GLFWwindow* window, int button, int action, int mods)
{
   if (button == GLFW_MOUSE_BUTTON_LEFT) {
      const bool moving_state = action == GLFW_PRESS;
      if (moving_state) {
         double x, y;
         glfwGetCursorPos( window, &x, &y );
         ClickedPoint.x = static_cast<int>(std::round( x ));
         ClickedPoint.y = static_cast<int>(std::round( y ));
      }
      MainCamera->setMovingState( moving_state );
   }
}

void RendererGL::mouseWrapper(GLFWwindow* window, int button, int action, int mods)
{
   Renderer->mouse( window, button, action, mods );
}

void RendererGL::mousewheel(GLFWwindow* window, double xoffset, double yoffset) const
{
   if (yoffset >= 0.0) MainCamera->zoomIn();
   else MainCamera->zoomOut();
}

void RendererGL::mousewheelWrapper(GLFWwindow* window, double xoffset, double yoffset)
{
   Renderer->mousewheel( window, xoffset, yoffset );
}

void RendererGL::reshape(GLFWwindow* window, int width, int height) const
{
   MainCamera->updatePerspectiveCamera( width, height );
   glViewport( 0, 0, width, height );
}

void RendererGL::reshapeWrapper(GLFWwindow* window, int width, int height)
{
   Renderer->reshape( window, width, height );
}

void RendererGL::registerCallbacks() const
{
   glfwSetWindowCloseCallback( Window, cleanupWrapper );
   glfwSetKeyCallback( Window, keyboardWrapper );
   glfwSetCursorPosCallback( Window, cursorWrapper );
   glfwSetMouseButtonCallback( Window, mouseWrapper );
   glfwSetScrollCallback( Window, mousewheelWrapper );
   glfwSetFramebufferSizeCallback( Window, reshapeWrapper );
}

void RendererGL::splitViewFrustum()
{
   constexpr float split_weight = 0.5f;
   const float n = MainCamera->getNearPlane();
   const float f = MainCamera->getFarPlane();
   SplitPositions.resize( SplitNum + 1 );
   SplitPositions[0] = n;
   SplitPositions[SplitNum] = f;
   for (int i = 1; i < SplitNum; ++i) {
      const auto r = static_cast<float>(i) / static_cast<float>(SplitNum);
      const float logarithmic_split = n * std::pow( f / n, r );
      const float uniform_split = n + (f - n) * r;
      SplitPositions[i] = glm::mix( uniform_split, logarithmic_split, split_weight );
   }
}

void RendererGL::setLights() const
{
   const glm::vec4 light_position(256.0f, 500.0f, 512.0f, 1.0f);
   const glm::vec4 ambient_color(1.0f, 1.0f, 1.0f, 1.0f);
   const glm::vec4 diffuse_color(0.7f, 0.7f, 0.7f, 1.0f);
   const glm::vec4 specular_color(0.9f, 0.9f, 0.9f, 1.0f);
   Lights->addLight( light_position, ambient_color, diffuse_color, specular_color );
}

void RendererGL::setGroundObject() const
{
   constexpr float size = 512.0f;
   std::vector<glm::vec3> ground_vertices;
   ground_vertices.emplace_back( 0.0f, 0.0f, 0.0f );
   ground_vertices.emplace_back( size, 0.0f, 0.0f );
   ground_vertices.emplace_back( size, 0.0f, size );

   ground_vertices.emplace_back( 0.0f, 0.0f, 0.0f );
   ground_vertices.emplace_back( size, 0.0f, size );
   ground_vertices.emplace_back( 0.0f, 0.0f, size );

   std::vector<glm::vec3> ground_normals;
   ground_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   ground_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   ground_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   ground_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   ground_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   ground_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   std::vector<glm::vec2> ground_textures;
   ground_textures.emplace_back( 0.0f, 0.0f );
   ground_textures.emplace_back( 1.0f, 0.0f );
   ground_textures.emplace_back( 1.0f, 1.0f );

   ground_textures.emplace_back( 0.0f, 0.0f );
   ground_textures.emplace_back( 1.0f, 1.0f );
   ground_textures.emplace_back( 0.0f, 1.0f );

   const std::string sample_directory_path = std::string(CMAKE_SOURCE_DIR) + "/samples";
   GroundObject->setObject(
      GL_TRIANGLES,
      ground_vertices,
      ground_normals,
      ground_textures,
      std::string(sample_directory_path + "/sand.jpg")
   );
   GroundObject->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
}

void RendererGL::setTigerObject() const
{
   const std::string sample_directory_path = std::string(CMAKE_SOURCE_DIR) + "/samples";
   std::ifstream file(std::string(sample_directory_path + "/Tiger/tiger.txt"));
   int polygon_num;
   file >> polygon_num;

   const int vertex_num = polygon_num * 3;
   std::vector<glm::vec3> tiger_vertices(vertex_num);
   std::vector<glm::vec3> tiger_normals(vertex_num);
   std::vector<glm::vec2> tiger_textures(vertex_num);
   for (int i = 0; i < polygon_num; ++i) {
      int triangle_vertex_num;
      file >> triangle_vertex_num;
      for (int v = 0; v < triangle_vertex_num; ++v) {
         const int index = i * triangle_vertex_num + v;
         file >> tiger_vertices[index].x >> tiger_vertices[index].y >> tiger_vertices[index].z;
         file >> tiger_normals[index].x >> tiger_normals[index].y >> tiger_normals[index].z;
         file >> tiger_textures[index].x >> tiger_textures[index].y;
      }
   }
   file.close();

   TigerObject->setObject(
      GL_TRIANGLES,
      tiger_vertices,
      tiger_normals,
      tiger_textures,
      std::string(sample_directory_path + "/Tiger/tiger.jpg")
   );
   TigerObject->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
}

void RendererGL::setPandaObject() const
{
   const std::string sample_directory_path = std::string(CMAKE_SOURCE_DIR) + "/samples";
   PandaObject->setObject(
      GL_TRIANGLES,
      std::string(sample_directory_path + "/Panda/panda.obj"),
      std::string(sample_directory_path + "/Panda/panda.png")
   );
   PandaObject->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
}

void RendererGL::setDepthFrameBuffer()
{
   glCreateTextures( GL_TEXTURE_2D, 1, &DepthTextureID );
   glTextureStorage2D( DepthTextureID, 1, GL_DEPTH_COMPONENT32F, ShadowMapSize, ShadowMapSize );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
   glTextureParameteri( DepthTextureID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );

   glCreateFramebuffers( 1, &FBO );
   glNamedFramebufferTexture( FBO, GL_DEPTH_ATTACHMENT, DepthTextureID, 0 );

   if (glCheckNamedFramebufferStatus( FBO, GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "FrameBuffer Setup Error\n";
   }
}

void RendererGL::getSplitFrustum(std::array<glm::vec3, 8>& frustum, float near, float far) const
{
   const glm::vec3 n = glm::normalize( MainCamera->getInitialReferencePosition() - MainCamera->getInitialCameraPosition() );
   const glm::vec3 u = glm::normalize( glm::cross( MainCamera->getInitialUpVector(), n ) );
   const glm::vec3 v = glm::normalize( glm::cross( n, u ) );

   const float near_plane_half_height = std::tan( MainCamera->getFOV() * 0.5f ) * near;
   const float near_plane_half_width = near_plane_half_height * MainCamera->getAspectRatio();
   const float far_plane_half_height = std::tan( MainCamera->getFOV() * 0.5f ) * far;
   const float far_plane_half_width = far_plane_half_height * MainCamera->getAspectRatio();

   const glm::vec3 near_plane_center = MainCamera->getInitialCameraPosition() + n * near;
   const glm::vec3 far_plane_center = MainCamera->getInitialCameraPosition() + n * far;

   frustum[0] = glm::vec3(near_plane_center - u * near_plane_half_width - v * near_plane_half_height);
   frustum[1] = glm::vec3(near_plane_center - u * near_plane_half_width + v * near_plane_half_height);
   frustum[2] = glm::vec3(near_plane_center + u * near_plane_half_width + v * near_plane_half_height);
   frustum[3] = glm::vec3(near_plane_center + u * near_plane_half_width - v * near_plane_half_height);

   frustum[4] = glm::vec3(far_plane_center - u * far_plane_half_width - v * far_plane_half_height);
   frustum[5] = glm::vec3(far_plane_center - u * far_plane_half_width + v * far_plane_half_height);
   frustum[6] = glm::vec3(far_plane_center + u * far_plane_half_width + v * far_plane_half_height);
   frustum[7] = glm::vec3(far_plane_center + u * far_plane_half_width - v * far_plane_half_height);
}

void RendererGL::getBoundingBox(std::array<glm::vec3, 8>& bounding_box, const std::array<glm::vec3, 8>& points)
{
   auto min_point = glm::vec3(std::numeric_limits<float>::max());
   auto max_point = glm::vec3(std::numeric_limits<float>::lowest());
   for (int i = 0; i < 8; ++i) {
      if (points[i].x < min_point.x) min_point.x = points[i].x;
      if (points[i].y < min_point.y) min_point.y = points[i].y;
      if (points[i].z < min_point.z) min_point.z = points[i].z;

      if (points[i].x > max_point.x) max_point.x = points[i].x;
      if (points[i].y > max_point.y) max_point.y = points[i].y;
      if (points[i].z > max_point.z) max_point.z = points[i].z;
   }

   bounding_box[0] = glm::vec3(min_point.x, min_point.y, min_point.z);
   bounding_box[1] = glm::vec3(max_point.x, min_point.y, min_point.z);
   bounding_box[2] = glm::vec3(min_point.x, min_point.y, max_point.z);
   bounding_box[3] = glm::vec3(max_point.x, min_point.y, max_point.z);
   bounding_box[4] = glm::vec3(min_point.x, max_point.y, min_point.z);
   bounding_box[5] = glm::vec3(max_point.x, max_point.y, min_point.z);
   bounding_box[6] = glm::vec3(min_point.x, max_point.y, max_point.z);
   bounding_box[7] = glm::vec3(max_point.x, max_point.y, max_point.z);
}

glm::mat4 RendererGL::calculateLightCropMatrix(std::array<glm::vec3, 8>& bounding_box) const
{
   std::array<glm::vec4, 8> ndc_points{};
   const glm::mat4 model_view_projection = LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   for (int i = 0; i < 8; ++i) {
      ndc_points[i] = model_view_projection * glm::vec4(bounding_box[i], 1.0f);
      ndc_points[i].x /= ndc_points[i].w;
      ndc_points[i].y /= ndc_points[i].w;
      ndc_points[i].z /= ndc_points[i].w;
   }

   auto min_point = glm::vec3(std::numeric_limits<float>::max());
   auto max_point = glm::vec3(std::numeric_limits<float>::lowest());
   for (int i = 0; i < 8; ++i) {
      if (ndc_points[i].x < min_point.x) min_point.x = ndc_points[i].x;
      if (ndc_points[i].y < min_point.y) min_point.y = ndc_points[i].y;
      if (ndc_points[i].z < min_point.z) min_point.z = ndc_points[i].z;

      if (ndc_points[i].x > max_point.x) max_point.x = ndc_points[i].x;
      if (ndc_points[i].y > max_point.y) max_point.y = ndc_points[i].y;
      if (ndc_points[i].z > max_point.z) max_point.z = ndc_points[i].z;
   }
   min_point.z = -1.0f;

   glm::mat4 crop(1.0f);
   crop[0][0] = 2.0f / (max_point.x - min_point.x);
   crop[1][1] = 2.0f / (max_point.y - min_point.y);
   crop[2][2] = 2.0f / (max_point.z - min_point.z);
   crop[3][0] = -0.5f * (max_point.x + min_point.x) * crop[0][0];
   crop[3][1] = -0.5f * (max_point.y + min_point.y) * crop[1][1];
   crop[3][2] = -0.5f * (max_point.z + min_point.z) * crop[2][2];
   return crop;
}

void RendererGL::drawGroundObject(ShaderGL* shader, CameraGL* camera) const
{
   shader->transferBasicTransformationUniforms( glm::mat4(1.0f), camera, true );
   GroundObject->transferUniformsToShader( shader );

   glBindTextureUnit( 0, GroundObject->getTextureID( 0 ) );
   glBindVertexArray( GroundObject->getVAO() );
   glDrawArrays( GroundObject->getDrawMode(), 0, GroundObject->getVertexNum() );
}

void RendererGL::drawTigerObject(ShaderGL* shader, CameraGL* camera) const
{
   const glm::mat4 to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(250.0f, 0.0f, 330.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( 180.0f ), glm::vec3(0.0f, 1.0f, 0.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( -90.0f ), glm::vec3(1.0f, 0.0f, 0.0f) ) *
      glm::scale( glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f) );
   shader->transferBasicTransformationUniforms( to_world, camera, true );
   TigerObject->transferUniformsToShader( shader );

   glBindTextureUnit( 0, TigerObject->getTextureID( 0 ) );
   glBindVertexArray( TigerObject->getVAO() );
   glDrawArrays( TigerObject->getDrawMode(), 0, TigerObject->getVertexNum() );
}

void RendererGL::drawPandaObject(ShaderGL* shader, CameraGL* camera) const
{
   const glm::mat4 to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(250.0f, -5.0f, 180.0f) ) *
      glm::scale( glm::mat4(1.0f), glm::vec3(20.0f, 20.0f, 20.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera, true );
   PandaObject->transferUniformsToShader( shader );

   glBindTextureUnit( 0, PandaObject->getTextureID( 0 ) );
   glBindVertexArray( PandaObject->getVAO() );
   glDrawArrays( PandaObject->getDrawMode(), 0, PandaObject->getVertexNum() );
}

void RendererGL::drawDepthMapFromLightView(const glm::mat4& light_crop_matrix) const
{
   glViewport( 0, 0, ShadowMapSize, ShadowMapSize );
   glEnable( GL_POLYGON_OFFSET_FILL );
   glPolygonOffset( 2.0f, 4.0f );

   constexpr GLfloat one = 1.0f;
   glBindFramebuffer( GL_FRAMEBUFFER, FBO );
   glClearNamedFramebufferfv( FBO, GL_DEPTH, 0, &one );
   glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

   glUseProgram( ObjectShader->getShaderProgram() );
   glUniformMatrix4fv( ObjectShader->getLocation( "LightCropMatrix" ), 1, GL_FALSE, &light_crop_matrix[0][0] );

   drawTigerObject( ObjectShader.get(), LightCamera.get() );
   drawPandaObject( ObjectShader.get(), LightCamera.get() );
   drawGroundObject( ObjectShader.get(), LightCamera.get() );

   glDisable( GL_POLYGON_OFFSET_FILL );
}

void RendererGL::drawShadow(const glm::mat4& light_crop_matrix) const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
   glUseProgram( ShadowShader->getShaderProgram() );

   Lights->transferUniformsToShader( ShadowShader.get() );
   glUniform1i( ShadowShader->getLocation( "LightIndex" ), ActiveLightIndex );

   const glm::mat4 model_view_projection = light_crop_matrix * LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   glUniformMatrix4fv( ShadowShader->getLocation( "LightModelViewProjectionMatrix" ), 1, GL_FALSE, &model_view_projection[0][0] );

   glBindTextureUnit( 1, DepthTextureID );
   drawTigerObject( ShadowShader.get(), MainCamera.get() );
   drawPandaObject( ShadowShader.get(), MainCamera.get() );
   drawGroundObject( ShadowShader.get(), MainCamera.get() );
}

void RendererGL::render() const
{
   glClear( OPENGL_COLOR_BUFFER_BIT | OPENGL_DEPTH_BUFFER_BIT );

   const float light_x = 256.0f * std::cos( LightTheta ) + 256.0f;
   const float light_z = 256.0f * std::sin( LightTheta ) + 256.0f;
   Lights->setLightPosition( glm::vec4(light_x, 100.0f, light_z, 0.0f), 0 );
   LightCamera->updateCameraView(
      glm::vec3( Lights->getLightPosition( ActiveLightIndex ) ),
      glm::vec3( 256.0f, 0.0f, 256.0f ),
      glm::vec3( 0.0f, 1.0f, 0.0f )
   );

   const float original_n = MainCamera->getNearPlane();
   const float original_f = MainCamera->getFarPlane();
   for (int i = 0; i < SplitNum; ++i) {
      std::array<glm::vec3, 8> frustum{};
      getSplitFrustum( frustum, SplitPositions[i], SplitPositions[i + 1] );

      // really need this?
      //std::array<glm::vec3, 8> bounding_box{};
      //getBoundingBox( bounding_box, frustum );

      const glm::mat4 crop_matrix = calculateLightCropMatrix( frustum );

      drawDepthMapFromLightView( crop_matrix );

      //writeDepthTexture( "../light_view" + std::to_string( i ) + ".png" );

      glDepthRange(
         (SplitPositions[i] - original_n) / (original_f - original_n),
         (SplitPositions[i + 1] - original_n) / (original_f - original_n)
      );
      MainCamera->updateNearFarPlanes( SplitPositions[i], SplitPositions[i + 1] );
      drawShadow( crop_matrix );
      glDepthRange( 0.0f, 1.0f );
      MainCamera->updateNearFarPlanes( original_n, original_f );
   }
}

void RendererGL::play()
{
   if (glfwWindowShouldClose( Window )) initialize();

   splitViewFrustum();
   setLights();
   setGroundObject();
   setTigerObject();
   setPandaObject();
   setDepthFrameBuffer();

   ObjectShader->setBasicUniformLocations();
   ObjectShader->addUniformLocation( "LightCropMatrix" );

   ShadowShader->setUniformLocations( 1 );
   ShadowShader->addUniformLocation( "LightIndex" );
   ShadowShader->addUniformLocation( "LightModelViewProjectionMatrix" );

   while (!glfwWindowShouldClose( Window )) {
      render();

      LightTheta += 0.01f;
      if (LightTheta >= 360.0f) LightTheta -= 360.0f;

      glfwSwapBuffers( Window );
      glfwPollEvents();
   }
   glfwDestroyWindow( Window );
}