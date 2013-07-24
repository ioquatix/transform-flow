@shader

#version 150

@vertex

uniform mat4 display_matrix;

in vec2 position;
in vec2 mapping;

out vec2 diffuse_mapping;

void main () {
	diffuse_mapping = mapping;
	gl_Position = display_matrix * vec4(position, 0.0, 1.0);
}

@fragment

uniform sampler2D diffuse_texture;

in vec2 diffuse_mapping;
out vec4 fragment_color;

void main() {
	fragment_color = vec4(diffuse_mapping, 0.0, 0.0) + texture(diffuse_texture, diffuse_mapping);
}
