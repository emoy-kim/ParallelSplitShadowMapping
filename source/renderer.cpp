#include "renderer.h"

RendererGL::RendererGL() :
   Window( nullptr ), Pause( false ), FrameWidth( 1920 ), FrameHeight( 1080 ), ShadowMapSize( 1024 ), SplitNum( 4 ),
   ActiveLightIndex( 0 ), FBO( 0 ), DepthTextureID( 0 ), ClickedPoint( -1, -1 ),
   Texter( std::make_unique<TextGL>() ), MainCamera( std::make_unique<CameraGL>() ),
   TextCamera( std::make_unique<CameraGL>() ), LightCamera( std::make_unique<CameraGL>() ),
   TextShader( std::make_unique<ShaderGL>() ), SceneShader( std::make_unique<ShaderGL>() ),
   LightViewShader( std::make_unique<ShaderGL>() ), WallObject( std::make_unique<ObjectGL>() ),
   BunnyObject( std::make_unique<ObjectGL>() ), Lights( std::make_unique<LightGL>() )
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
   glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
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

   Texter->initialize();

   TextCamera->update2DCamera( FrameWidth, FrameHeight );
   MainCamera->updatePerspectiveCamera( FrameWidth, FrameHeight );
   LightCamera->updateOrthographicCamera( ShadowMapSize, ShadowMapSize );

   const std::string shader_directory_path = std::string(CMAKE_SOURCE_DIR) + "/shaders";
   TextShader->setShader(
      std::string(shader_directory_path + "/text.vert").c_str(),
      std::string(shader_directory_path + "/text.frag").c_str()
   );
   SceneShader->setShader(
      std::string(shader_directory_path + "/scene_shader.vert").c_str(),
      std::string(shader_directory_path + "/scene_shader.frag").c_str()
   );
   LightViewShader->setShader(
      std::string(shader_directory_path + "/light_view_generator.vert").c_str(),
      std::string(shader_directory_path + "/light_view_generator.frag").c_str()
   );
}

void RendererGL::writeFrame(const std::string& name) const
{
   const int size = FrameWidth * FrameHeight * 3;
   auto* buffer = new uint8_t[size];
   glPixelStorei( GL_PACK_ALIGNMENT, 1 );
   glReadBuffer( GL_COLOR_ATTACHMENT0 );
   glReadPixels( 0, 0, FrameWidth, FrameHeight, GL_BGR, GL_UNSIGNED_BYTE, buffer );
   FIBITMAP* image = FreeImage_ConvertFromRawBits(
      buffer, FrameWidth, FrameHeight, FrameWidth * 3, 24,
      FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, false
   );
   FreeImage_Save( FIF_PNG, image, name.c_str() );
   FreeImage_Unload( image );
   delete [] buffer;
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

void RendererGL::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (action != GLFW_PRESS) return;

   switch (key) {
      case GLFW_KEY_C:
         Renderer->writeFrame( "../result.png" );
         break;
      case GLFW_KEY_L:
         Renderer->Lights->toggleLightSwitch();
         std::cout << "Light Turned " << (Renderer->Lights->isLightOn() ? "On!\n" : "Off!\n");
         break;
      case GLFW_KEY_P: {
         const glm::vec3 pos = Renderer->MainCamera->getCameraPosition();
         std::cout << "Camera Position: " << pos.x << ", " << pos.y << ", " << pos.z << "\n";
      } break;
      case GLFW_KEY_SPACE:
         Renderer->Pause = !Renderer->Pause;
         break;
      case GLFW_KEY_Q:
      case GLFW_KEY_ESCAPE:
         cleanup( window );
         break;
      default:
         return;
   }
}

void RendererGL::cursor(GLFWwindow* window, double xpos, double ypos)
{
   if (Renderer->Pause) return;

   if (Renderer->MainCamera->getMovingState()) {
      const auto x = static_cast<int>(std::round( xpos ));
      const auto y = static_cast<int>(std::round( ypos ));
      const int dx = x - Renderer->ClickedPoint.x;
      const int dy = y - Renderer->ClickedPoint.y;
      Renderer->MainCamera->moveForward( -dy );
      Renderer->MainCamera->rotateAroundWorldY( -dx );

      if (glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT ) == GLFW_PRESS) {
         Renderer->MainCamera->pitch( -dy );
      }

      Renderer->ClickedPoint.x = x;
      Renderer->ClickedPoint.y = y;
   }
}

void RendererGL::mouse(GLFWwindow* window, int button, int action, int mods)
{
   if (Renderer->Pause) return;

   if (button == GLFW_MOUSE_BUTTON_LEFT) {
      const bool moving_state = action == GLFW_PRESS;
      if (moving_state) {
         double x, y;
         glfwGetCursorPos( window, &x, &y );
         Renderer->ClickedPoint.x = static_cast<int>(std::round( x ));
         Renderer->ClickedPoint.y = static_cast<int>(std::round( y ));
      }
      Renderer->MainCamera->setMovingState( moving_state );
   }
}

void RendererGL::mousewheel(GLFWwindow* window, double xoffset, double yoffset)
{
   if (Renderer->Pause) return;

   if (yoffset >= 0.0) Renderer->MainCamera->zoomIn();
   else Renderer->MainCamera->zoomOut();
}

void RendererGL::registerCallbacks() const
{
   glfwSetWindowCloseCallback( Window, cleanup );
   glfwSetKeyCallback( Window, keyboard );
   glfwSetCursorPosCallback( Window, cursor );
   glfwSetMouseButtonCallback( Window, mouse );
   glfwSetScrollCallback( Window, mousewheel );
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
   const glm::vec4 light_position(500.0f, 500.0f, 500.0f, 0.0f);
   const glm::vec4 ambient_color(1.0f, 1.0f, 1.0f, 1.0f);
   const glm::vec4 diffuse_color(0.9f, 0.9f, 0.9f, 1.0f);
   const glm::vec4 specular_color(0.9f, 0.9f, 0.9f, 1.0f);
   Lights->addLight( light_position, ambient_color, diffuse_color, specular_color );
}

void RendererGL::setWallObject() const
{
   constexpr float half_length = 128.0f;
   std::vector<glm::vec3> wall_vertices;
   wall_vertices.emplace_back( -half_length, 0.0f, -half_length );
   wall_vertices.emplace_back( half_length, 0.0f, -half_length );
   wall_vertices.emplace_back( half_length, 0.0f, half_length );

   wall_vertices.emplace_back( -half_length, 0.0f, -half_length );
   wall_vertices.emplace_back( half_length, 0.0f, half_length );
   wall_vertices.emplace_back( -half_length, 0.0f, half_length );

   std::vector<glm::vec3> wall_normals;
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );
   wall_normals.emplace_back( 0.0f, 1.0f, 0.0f );

   WallObject->setObject( GL_TRIANGLES, wall_vertices, wall_normals );
   WallObject->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
}

void RendererGL::setBunnyObject() const
{
   const std::string sample_directory_path = std::string(CMAKE_SOURCE_DIR) + "/samples";
   BunnyObject->setObject(
      GL_TRIANGLES,
      std::string(sample_directory_path + "/Bunny/bunny.obj")
   );
   BunnyObject->setDiffuseReflectionColor( { 1.0f, 1.0f, 1.0f, 1.0f } );
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
   const glm::mat4& projection_matrix = MainCamera->getProjectionMatrix();
   const float scale_x = 1.0f / projection_matrix[0][0];
   const float scale_y = 1.0f / projection_matrix[1][1];
   const float near_plane_half_width = near * scale_x;
   const float near_plane_half_height = near * scale_y;
   const float far_plane_half_width = far * scale_x;
   const float far_plane_half_height = far * scale_y;

#if 0
   const glm::vec3 n = glm::normalize( MainCamera->getInitialCameraPosition() - MainCamera->getInitialReferencePosition() );
   const glm::vec3 u = glm::normalize( glm::cross( MainCamera->getInitialUpVector(), n ) );
   const glm::vec3 v = glm::normalize( glm::cross( n, u ) );

   const glm::vec3 near_plane_center = MainCamera->getInitialCameraPosition() + n * (-near);
   const glm::vec3 far_plane_center = MainCamera->getInitialCameraPosition() + n * (-far);

   frustum[0] = glm::vec3(near_plane_center - u * near_plane_half_width - v * near_plane_half_height);
   frustum[1] = glm::vec3(near_plane_center - u * near_plane_half_width + v * near_plane_half_height);
   frustum[2] = glm::vec3(near_plane_center + u * near_plane_half_width + v * near_plane_half_height);
   frustum[3] = glm::vec3(near_plane_center + u * near_plane_half_width - v * near_plane_half_height);

   frustum[4] = glm::vec3(far_plane_center - u * far_plane_half_width - v * far_plane_half_height);
   frustum[5] = glm::vec3(far_plane_center - u * far_plane_half_width + v * far_plane_half_height);
   frustum[6] = glm::vec3(far_plane_center + u * far_plane_half_width + v * far_plane_half_height);
   frustum[7] = glm::vec3(far_plane_center + u * far_plane_half_width - v * far_plane_half_height);
#else
   frustum[0] = glm::vec3(-near_plane_half_width, -near_plane_half_height, -near);
   frustum[1] = glm::vec3(-near_plane_half_width, near_plane_half_height, -near);
   frustum[2] = glm::vec3(near_plane_half_width, near_plane_half_height, -near);
   frustum[3] = glm::vec3(near_plane_half_width, -near_plane_half_height, -near);

   frustum[4] = glm::vec3(-far_plane_half_width, -far_plane_half_height, -far);
   frustum[5] = glm::vec3(-far_plane_half_width, far_plane_half_height, -far);
   frustum[6] = glm::vec3(far_plane_half_width, far_plane_half_height, -far);
   frustum[7] = glm::vec3(far_plane_half_width, -far_plane_half_height, -far);

   const glm::mat4 inverse_view_matrix = glm::inverse( MainCamera->getViewMatrix() );
   for (int i = 0; i < 8; ++i) {
      frustum[i] = glm::vec3(inverse_view_matrix * glm::vec4(frustum[i], 1.0f));
   }
#endif
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

void RendererGL::drawBoxObject(ShaderGL* shader, const CameraGL* camera) const
{
   glm::mat4 to_world(1.0f);
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 0.0f, 0.0f, 1.0f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
   glBindVertexArray( WallObject->getVAO() );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );

   to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(0.0f, 128.0f, -128.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( 90.0f ), glm::vec3(1.0f, 0.0f, 0.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 0.0f, 1.0f, 0.0f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );

   to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(-128.0f, 128.0f, 0.0f) ) *
      glm::rotate( glm::mat4(1.0f), glm::radians( -90.0f ), glm::vec3(0.0f, 0.0f, 1.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   WallObject->setDiffuseReflectionColor( { 1.0f, 0.0f, 0.0f, 1.0f } );
   WallObject->transferUniformsToShader( shader );
   glDrawArrays( WallObject->getDrawMode(), 0, WallObject->getVertexNum() );
}

void RendererGL::drawBunnyObject(ShaderGL* shader, const CameraGL* camera) const
{
   const glm::mat4 to_world =
      glm::translate( glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, -30.0f) ) *
      glm::scale( glm::mat4(1.0f), glm::vec3(100.0f, 100.0f, 100.0f) );
   shader->transferBasicTransformationUniforms( to_world, camera );
   BunnyObject->transferUniformsToShader( shader );

   glBindVertexArray( BunnyObject->getVAO() );
   glDrawArrays( BunnyObject->getDrawMode(), 0, BunnyObject->getVertexNum() );
}

void RendererGL::drawDepthMapFromLightView(const glm::mat4& light_crop_matrix) const
{
   glViewport( 0, 0, ShadowMapSize, ShadowMapSize );
   glBindFramebuffer( GL_FRAMEBUFFER, FBO );

   constexpr GLfloat one = 1.0f;
   glClearNamedFramebufferfv( FBO, GL_DEPTH, 0, &one );
   glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );

   glUseProgram( LightViewShader->getShaderProgram() );
   glUniformMatrix4fv( LightViewShader->getLocation( "LightCropMatrix" ), 1, GL_FALSE, &light_crop_matrix[0][0] );

   drawBunnyObject( LightViewShader.get(), LightCamera.get() );
   drawBoxObject( LightViewShader.get(), LightCamera.get() );
}

void RendererGL::drawShadow(const glm::mat4& light_crop_matrix) const
{
   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
   glUseProgram( SceneShader->getShaderProgram() );

   Lights->transferUniformsToShader( SceneShader.get() );
   glUniform1i( SceneShader->getLocation( "LightIndex" ), ActiveLightIndex );
   glUniform1i( SceneShader->getLocation( "UseTexture" ), 0 );

   const glm::mat4 view_projection = light_crop_matrix * LightCamera->getProjectionMatrix() * LightCamera->getViewMatrix();
   glUniformMatrix4fv( SceneShader->getLocation( "LightViewProjectionMatrix" ), 1, GL_FALSE, &view_projection[0][0] );

   glBindTextureUnit( 1, DepthTextureID );
   drawBunnyObject( SceneShader.get(), MainCamera.get() );
   drawBoxObject( SceneShader.get(), MainCamera.get() );
}

void RendererGL::drawText(const std::string& text) const
{
   std::vector<TextGL::Glyph*> glyphs;
   Texter->getGlyphsFromText( glyphs, text );

   glViewport( 0, 0, FrameWidth, FrameHeight );
   glBindFramebuffer( GL_FRAMEBUFFER, 0 );
   glUseProgram( TextShader->getShaderProgram() );

   glEnable( GL_BLEND );
   glBlendFunc( GL_SRC_ALPHA, GL_ONE );
   glDisable( GL_DEPTH_TEST );

   glm::vec2 text_position(50.0f, 1000.0f);
   const ObjectGL* glyph_object = Texter->getGlyphObject();
   glBindVertexArray( glyph_object->getVAO() );
   for (const auto& glyph : glyphs) {
      const glm::vec2 position(
         std::round( text_position.x + glyph->Bearing.x ),
         std::round( text_position.y + glyph->Bearing.y - glyph->Size.y )
      );
      const glm::mat4 to_world =
         glm::translate( glm::mat4(1.0f), glm::vec3(position, 0.0f) ) *
         glm::scale( glm::mat4(1.0f), glm::vec3(glyph->Size.x, glyph->Size.y, 1.0f) );
      TextShader->transferBasicTransformationUniforms( to_world, TextCamera.get() );
      TextShader->uniform2fv( "TextScale", glyph->TopRightTextureCoord );
      glBindTextureUnit( 0, glyph_object->getTextureID( glyph->TextureIDIndex ) );
      glDrawArrays( glyph_object->getDrawMode(), 0, glyph_object->getVertexNum() );

      text_position.x += glyph->Advance.x;
      text_position.y -= glyph->Advance.y;
   }
   glEnable( GL_DEPTH_TEST );
   glDisable( GL_BLEND );
}

void RendererGL::render() const
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   LightCamera->updateCameraView(
      glm::vec3(Lights->getLightPosition( ActiveLightIndex )),
      glm::vec3(-1.0f, -1.0f, 0.0f),
      glm::vec3(0.0f, 1.0f, 0.0f)
   );

   std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

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

   std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
   const auto fps = 1E+6 / static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
   std::stringstream text;
   text << std::fixed << std::setprecision( 2 ) << fps << " fps";
   drawText( text.str() );
}

void RendererGL::play()
{
   if (glfwWindowShouldClose( Window )) initialize();

   splitViewFrustum();
   setLights();
   setWallObject();
   setBunnyObject();
   setDepthFrameBuffer();

   TextShader->setTextUniformLocations();
   SceneShader->setSceneUniformLocations( 1 );
   LightViewShader->setLightViewUniformLocations();

   while (!glfwWindowShouldClose( Window )) {
      if (!Pause) render();

      glfwSwapBuffers( Window );
      glfwPollEvents();
   }
   glfwDestroyWindow( Window );
}