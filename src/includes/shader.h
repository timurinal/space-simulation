#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include "glm/glm.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    Shader() = default;
    Shader(const std::string vertexPath, const std::string fragmentPath); 
   
    void bind();
   
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string &name, glm::vec2 value) const;
    void setVec3(const std::string &name, glm::vec3 value) const;
    void setVec4(const std::string &name, glm::vec4 value) const;
    void setMat4(const std::string &name, glm::mat4 matrix) const;
    
    int getAttribLoc(const std::string attribName);
private:
    unsigned int handle;
};

#endif //SHADER_H