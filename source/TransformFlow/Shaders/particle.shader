@shader

#version 150

@vertex

uniform mat4 display_matrix;

in vec3 offset;
in vec3 position;
in vec2 mapping;
in vec4 color;

out vec4 surface_color;
out vec2 diffuse_mapping;

void main () {
#ifdef BILLBOARD
	// Screen-aligned particles:
	vec3 u = normalize(vec3(display_matrix[0][0], display_matrix[1][0], display_matrix[2][0]));
	vec3 v = normalize(vec3(display_matrix[0][1], display_matrix[1][1], display_matrix[2][1]));
	
	vec3 corner = (offset.x * u + offset.y * v) + position;
#else
	vec3 corner = vec3(offset.x, offset.y, offset.z) + position;
#endif
	
	diffuse_mapping = mapping;
	surface_color = color;
	
	gl_Position = display_matrix * vec4(corner, 1.0);
}


@fragment

uniform sampler2D diffuse_texture;

in vec4 surface_color;
in vec2 diffuse_mapping;
out vec4 fragment_color;

void main() {
	fragment_color = texture(diffuse_texture, diffuse_mapping) * surface_color;
}
