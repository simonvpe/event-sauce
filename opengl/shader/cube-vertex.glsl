#version 450 core
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in mat4 in_instance_matrix;

uniform mat4 projection_matrix, view_matrix;

out vec4 color;

void main()
{
  color = in_color;
	gl_Position = projection_matrix * view_matrix * in_instance_matrix * vec4(in_position, 1);
}
