layout(location = 0) in vec2 texCoords;

layout(binding = 10) uniform sampler2DArray tex_dirLights;

layout(location = 0) uniform int lightIndex;

out vec4 fragColor;

void main()
{
	fragColor = vec4(texture(tex_dirLights, vec3(texCoords, float(lightIndex))).r);
}