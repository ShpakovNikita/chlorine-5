#include "headers/engine.hxx"
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <iostream>
#include "headers/picopng.hxx"

namespace CHL {
texture::texture() {}
texture::texture(const std::string& path) {
    if (!load_texture(path))
        std::cerr << "Texture not found!" << std::endl;
}
texture::~texture() {}

bool texture::load_texture(const std::string& path) {
    std::vector<unsigned char> png_file_in_memory;
    std::ifstream ifs(path.data(), std::ios_base::binary);
    if (!ifs) {
        return false;
    }
    ifs.seekg(0, std::ios_base::end);
    size_t pos_in_file = ifs.tellg();
    png_file_in_memory.resize(pos_in_file);
    ifs.seekg(0, std::ios_base::beg);
    if (!ifs) {
        return false;
    }

    ifs.read(reinterpret_cast<char*>(png_file_in_memory.data()), pos_in_file);
    if (!ifs.good()) {
        return false;
    }

    std::vector<unsigned char> image;
    unsigned long w = 0;
    unsigned long h = 0;
    int error = decodePNG(image, w, h, &png_file_in_memory[0],
                          png_file_in_memory.size(), true);

    // if there's an error, display it
    if (error != 0) {
        std::cerr << "error: " << error << std::endl;
        return false;
    }

    tex = 0;
    glGenTextures(1, &tex);
    bind();

    GLint mipmap_level = 0;    // test
    GLint border = 0;

    glTexImage2D(GL_TEXTURE_2D, mipmap_level, GL_RGBA, w, h, border, GL_RGBA,
                 GL_UNSIGNED_BYTE, &image[0]);

    //        glGenerateMipmap(GL_TEXTURE_2D);
    //        GL_CHECK();

    //        float borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
    //        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
    //        borderColor);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Set texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return true;
}

void texture::bind() {
    glBindTexture(GL_TEXTURE_2D, tex);
}

void texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
}    // namespace CHL
