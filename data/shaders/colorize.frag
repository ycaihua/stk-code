uniform vec3 col;

out vec4 FragColor;

void main()
{
	FragColor = vec4(col / 3.14, 1.0);
}
