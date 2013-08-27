@shader

#version 150

@vertex

uniform mat4 display_matrix;
in vec3 position;

out vec4 surface_position;

void main () {
	surface_position = vec4(position, 1.0);	
	gl_Position = display_matrix * vec4(position, 1.0);
}

@fragment

uniform vec4 major_color;

in vec4 surface_position;
out vec4 fragment_color;

void main(void) {
	fragment_color = major_color;
}
