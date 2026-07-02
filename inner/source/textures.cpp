#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "textures.h"
#include "textures_path.h"
#include <GL/glut.h>
#if defined(__linux__)
#include <GL/glx.h>
#endif
#include <cstring>
#include <iostream>

static std::string resolveTextureFile(const std::string& fileName)
{
    if (!fileName.empty() && fileName[0] == '/')
        return fileName;

    const char* t1 = "textures/";
    const size_t n1 = std::strlen(t1);
    if (fileName.size() >= n1 && fileName.compare(0, n1, t1) == 0)
        return texturesPath() + "/" + fileName.substr(n1);

    const char* t2 = "inner/textures/";
    const size_t n2 = std::strlen(t2);
    if (fileName.size() >= n2 && fileName.compare(0, n2, t2) == 0)
        return texturesPath() + "/" + fileName.substr(n2);

    return texturesPath() + "/" + fileName;
}

GLuint LoadTexID(const std::string& FileName)
{
    const std::string full = resolveTextureFile(FileName);
    int width, height, channels;
    unsigned char* img = stbi_load(full.c_str(), &width, &height, &channels, 0);
    if (!img)
    {
        std::cerr << "Failed to load texture: " << full << " (requested as: " << FileName << ")" << std::endl;
        return 0;
    }

#if defined(__linux__)
    if (glXGetCurrentContext() == nullptr) {
        stbi_image_free(img);
        return 0;
    }
#endif

    GLenum format;
    if (channels == 1)
        format = GL_RED;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;
    else
    {
        std::cerr << "Unsupported number of channels: " << channels << std::endl;
        stbi_image_free(img);
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, img);

    stbi_image_free(img);
    return texID;
}
