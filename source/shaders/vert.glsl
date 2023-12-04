#version 410 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 vertexNormals;
layout(location=2) in vec3 vertexColors;
layout(location=3) in vec3 offset;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_Projection;

// Pass vertex colors into the fragment shader
out vec3 v_vertexColors;
out vec3 v_vertexNormals;
out vec3 FragPos;

void main() {
    v_vertexColors = vertexColors;
    v_vertexNormals = vertexNormals;
    FragPos = vec3(u_ModelMatrix * vec4(position + offset, 1.0f));

    vec4 newPosition = u_Projection * u_ViewMatrix * u_ModelMatrix * vec4(position + offset,1.0f);
    gl_Position = vec4(newPosition.x, newPosition.y, newPosition.z, newPosition.w);

}
