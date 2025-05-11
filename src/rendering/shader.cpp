#include "shader.h"
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const std::string vertexPath, const std::string fragmentPath) {
    std::string vSource;
    std::string fSource;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // ensure ifstream objects can throw exceptions
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vSource = vShaderStream.str();
        fSource = fShaderStream.str();
    }
    catch (std::ifstream::failure e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
    }

    const char* vShaderCode = vSource.c_str();
    const char* fShaderCode = fSource.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, nullptr, infoLog);

        std::ostringstream s;
        s << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog;
        std::cerr << s.str() << std::endl;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, nullptr, infoLog);

        std::ostringstream s;
        s << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog;
        std::cerr << s.str() << std::endl;
    }

    handle = glCreateProgram();
    glAttachShader(handle, vertex);
    glAttachShader(handle, fragment);
    glLinkProgram(handle);

    glGetProgramiv(handle, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(handle, 512, nullptr, infoLog);
        std::ostringstream s;
        s << "ERROR::SHADER::LINKING_FAILED\n" << infoLog;
        std::cerr << s.str() << std::endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::bind() {
    glUseProgram(handle);
}

void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(handle, name.c_str()), (int)value);
}
void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(handle, name.c_str()), value);
}
void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(handle, name.c_str()), value);
}

void Shader::setVec2(const std::string &name, glm::vec2 value) const {
    glUniform2fv(glGetUniformLocation(handle, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string &name, glm::vec3 value) const {
    glUniform3fv(glGetUniformLocation(handle, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string &name, glm::vec4 value) const {
    glUniform4fv(glGetUniformLocation(handle, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string &name, glm::mat4 matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(handle, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

int Shader::getAttribLoc(const std::string attribName) {
    return glGetAttribLocation(handle, attribName.c_str());
}