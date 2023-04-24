#version 460

uniform mat4 WorldMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform mat4 LightModelViewProjectionMatrix;

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_tex_coord;

out vec3 position_in_ec;
out vec3 normal_in_ec;
out vec2 tex_coord;

out vec4 depth_map_coord;

void main()
{   
   vec4 e_position = ViewMatrix * WorldMatrix * vec4(v_position, 1.0f);
   vec4 e_normal = transpose( inverse( ViewMatrix * WorldMatrix ) ) * vec4(v_normal, 1.0f);
   position_in_ec = e_position.xyz;
   normal_in_ec = normalize( e_normal.xyz );

   tex_coord = v_tex_coord;

   vec4 position_in_light_cc = LightModelViewProjectionMatrix * vec4(v_position, 1.0f);
   depth_map_coord.x = 0.5f * position_in_light_cc.x + 0.5f * position_in_light_cc.w;
   depth_map_coord.y = 0.5f * position_in_light_cc.y + 0.5f * position_in_light_cc.w;
   depth_map_coord.z = 0.5f * position_in_light_cc.z + 0.5f * position_in_light_cc.w;
   depth_map_coord.w = position_in_light_cc.w;

   gl_Position = ModelViewProjectionMatrix * vec4(v_position, 1.0f);
}