#version 410 core

in vec3 v_vertexColors;
in vec3 v_vertexNormals;
in vec3 FragPos;

struct Light {
    vec3 lightPos;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light u_Light;
uniform vec3 u_ViewPos;

out vec4 FragColor;

void main() {
    // Ambient
    vec3 ambient = u_Light.ambient;
    
    // Diffuse
    vec3 norm = normalize(v_vertexNormals);
    vec3 lightDir = normalize(u_Light.lightPos - FragPos);
    float diffImpact = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffImpact * u_Light.diffuse;

    // Specular
    vec3 viewDir = normalize(u_ViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = spec * u_Light.specular;
    
    vec3 lighting = ambient + diffuse + specular;
    FragColor = vec4(v_vertexColors * lighting, 1.0f);
}
