#ifndef __GL_DEBUG__
#define __GL_DEBUG__ 1

#include "glad/glad.h"
#include <stdio.h>
#include <string.h>

static
GLenum
__gl_check_err(const char *filename, long line) {
    GLenum err_code; 
    while ((err_code = glGetError()) != GL_NO_ERROR) {
        char err[32];
        switch (err_code) {
            case GL_INVALID_ENUM:
                strncpy(err, "INVALID ENUM", 32);
                break;
            case GL_INVALID_VALUE:
                strncpy(err, "INVALID VALUE", 32);
                break;
            case GL_INVALID_OPERATION:
                strncpy(err, "INVALID OPERATION", 32);
                break;
            case GL_STACK_OVERFLOW:
                strncpy(err, "STACK OVERFLOW", 32);
                break;
            case GL_STACK_UNDERFLOW:
                strncpy(err, "STACK UNDERFLOW", 32);
                break;
            case GL_OUT_OF_MEMORY:
                strncpy(err, "OUT OF MEMORY", 32);
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                strncpy(err, "INVALID FRAMEBUFFER OPERATION", 32);
                break;
            default:
                break;
        }

        printf("GL Error \"%s\" in file \"%s\" at line (%d)\n", err, filename, line);
    }
    return err_code;
}

#define gl_check_errors() __gl_check_err(__FILE__, __LINE__)

#endif /* __GL_DEBUG__ */