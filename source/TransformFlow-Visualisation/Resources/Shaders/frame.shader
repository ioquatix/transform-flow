@shader

#version 150

@vertex

uniform mat4 display_matrix;
uniform mat4 transform_matrix;

in vec2 position;
in vec2 mapping;

out vec2 diffuse_mapping;

void main () {
	diffuse_mapping = mapping;
	gl_Position = display_matrix * transform_matrix * vec4(position, 0.0, 1.0);
}

@fragment

uniform sampler2D diffuse_texture;

in vec2 diffuse_mapping;
out vec4 fragment_color;

void main() {
	fragment_color = texture(diffuse_texture, diffuse_mapping);
	//fragment_color.w = 1.0 - abs((diffuse_mapping.x - 0.5) * 2.0);
}
