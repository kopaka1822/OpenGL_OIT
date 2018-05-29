layout(binding = 8) uniform samplerCube tex;

layout(location = 0) in vec3 viewDir;
out vec4 fragColor;

void main()
{
	fragColor = pow(texture(tex, viewDir), vec4(1.0 / 2.2));
}