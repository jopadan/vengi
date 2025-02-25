$in vec4 a_pos;
$in vec3 a_color;
layout(std140) uniform u_vert {
	mat4 u_view;
};
$out vec3 g_color;

void main() {
	gl_Position = u_view * a_pos;
	g_color = a_color;
}
