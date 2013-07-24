@shader

#version 150

@vertex

uniform mat4 display_transform;

in vec2 position;
in vec2 mapping;

out vec2 diffuse_mapping;

void main () {
	diffuse_mapping = mapping;
	gl_Position = display_transform * vec4(position, 0.0, 1.0);
}

@fragment

uniform sampler2D diffuse_texture;
uniform vec4 highlight_color;

in vec4 surface_position;
in vec2 diffuse_mapping;

out vec4 fragment_color;

void main() {
	vec4 sample = texture(diffuse_texture, diffuse_mapping);
	fragment_color.rgb = highlight_color.rgb * sample.r;
	fragment_color.a = sample.g;
}
