@shader

#version 150

@vertex

uniform mat4 display_transform;

in vec2 offset;
in vec3 position;
in vec3 axis;
in vec2 mapping;
in vec4 color;

out vec4 surface_color;
out vec2 diffuse_mapping;

void main () {
	// Pointing towards scene:
	vec3 w = normalize(vec3(display_transform[0][2], display_transform[1][2], display_transform[2][2]));

	// Pointing outwards from axis:
	vec3 u = normalize(cross(w, axis));

	// The corner position:
	vec3 corner = (offset.x * u + offset.y * axis) + position;

	diffuse_mapping = mapping;
	surface_color = color;

	gl_Position = display_transform * vec4(corner, 1.0);
}

@fragment

uniform sampler2D diffuse_texture;

in vec4 surface_color;
in vec2 diffuse_mapping;
out vec4 fragment_color;

void main() {
	vec4 s = texture(diffuse_texture, diffuse_mapping);

	// Perform a simple threasholding based on alpha:
	if (s.a < 0.2) {
		s.a = 0.0;
	}

	fragment_color = s * surface_color;
}
