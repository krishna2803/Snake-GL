#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "cglm/cglm.h"
#include "gl_debug.h"

const uint8_t SIZE = 16; // size of the grid
const uint8_t FPS  = 8;  // keep this low
const uint16_t width  = 480; // window width (== window height )
const uint16_t height = width;

typedef struct Snake {
    int x;
    int y;
    struct Snake *next;
} Snake;

bool gameover = false;

// 1 = fine
// 0 = gameover
int update_snake(char grid[SIZE][SIZE], Snake *head, int dx, int dy) {
    if (!dx && !dy) return 1;
    if (gameover) return 0;

    int px = head->x;
    int py = head->y;

    if (px + dx < 0 || px + dx >= SIZE) return 0;
    if (py + dy < 0 || py + dy >= SIZE) return 0;
    bool apple = grid[px+dx][py+dy];
    if (grid[px+dx][py+dy] > 1) return 0;

    head->x += dx;
    head->y += dy;

    grid[head->x][head->y] = 2;

    Snake *ptr = head->next;
    while (1) {
        grid[px][py] = 3;
        int ppx = ptr->x;
        int ppy = ptr->y;
        ptr->x = px;
        ptr->y = py;
        px = ppx;
        py = ppy;
        if (!ptr->next) break;
        else ptr = ptr->next;
    }
    if (apple) {
        ptr->next = malloc(sizeof(Snake));
        ptr->next->x = px;
        ptr->next->y = py;
        ptr->next->next = NULL;

        int ax = px, ay = py;
        while (grid[ax][ay] != 0) {
            ax = rand() % SIZE;
            ay = rand() % SIZE;
        }
        grid[ax][ay] = 1;
    }
    grid[px][py] = apple ? 3 : 0;

    return 1;
}

void destroy_snake(Snake *head) {
    Snake *snek;
    while (head) {
        snek = head;
        head = head->next;
        free(snek);
    }
}

int main(int argc, char **argv) {
    srand(time(NULL));

    Snake head = {(SIZE-1)>>1, (SIZE-1)>>1, NULL}; // x, y, next
    char grid[SIZE][SIZE];
    memset(&grid, 0, SIZE*SIZE);
    // 0 - empty
    // 1 - apple
    // 2 - snake head
    // 3 - snake body
    grid[head.x][head.y] = 2;

    // add 3 units to head
    Snake *snek = &head;
    for (int i=0; i<3; i++) {
        snek->next = malloc(sizeof(Snake));
        snek->next->x = snek->x-1;
        snek->next->y = snek->y;
        grid[snek->x-1][snek->y] = 2;
        snek->next->next = NULL;
        snek = snek->next;
    }

    int ax, ay = head.y;
    while (ay == head.y) {  
        ax = rand() % SIZE;
        ay = rand() % SIZE;
    }
    grid[ax][ay] = 1;

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, GLFW_DONT_CARE);

    // glfwWindowHint(GLFW_DECORATED, false);
    glfwWindowHint(GLFW_RESIZABLE, false);


    GLFWwindow *window = glfwCreateWindow(width, height, "Snek",  NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    {
        int scr_width, scr_height;
        const GLFWvidmode *vm = glfwGetVideoMode(glfwGetPrimaryMonitor());
        scr_width  = vm->width;
        scr_height = vm->height;
        glfwSetWindowPos(window, (scr_width-width)>>1, (scr_height-height)>>1);
    }
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glViewport(0, 0, width, height);

    unsigned int vao;
    glCreateVertexArrays(1, &vao);

    unsigned int vbo;
    // unit square
    const float vertices[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f };
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, sizeof(vertices), vertices, 0);

    unsigned int ebo;
    const unsigned char indices[] = { 0, 1, 2, 2, 3, 0 };
    glCreateBuffers(1, &ebo);
    glNamedBufferStorage(ebo, sizeof(indices), indices, 0);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 8);
    glVertexArrayElementBuffer(vao, ebo);

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    
    const char *vertex_src = "\
    #version 460 core\n\
    layout(location=0) in vec2 a_pos;\
    uniform mat3 u_transform;\
    void main() {\
        gl_Position = vec4(u_transform * vec3(a_pos, 1.0), 1.0);\
    }";

    const char *fragment_src = "\
    #version 460 core\n\
    uniform vec3 u_color;\
    void main() {\
        gl_FragColor = vec4(u_color, 1.0);\
    }";

    unsigned int vs, fs, program;
    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_src, NULL);
    glCompileShader(vs);

    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_src, NULL);
    glCompileShader(fs);

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);
    glValidateProgram(program);

    glDetachShader(program, vs);
    glDetachShader(program, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glUseProgram(program);
    glBindVertexArray(vao);
    
    float s = 1.8 / SIZE;

    int dx = 0;
    int dy = 0;

    double delta = 1.0 / FPS;
    
    mat3 u_transform;
    
    int loc_transform = glGetUniformLocation(program, "u_transform");
    int loc_color = glGetUniformLocation(program, "u_color");

    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) || glfwGetKey(window, GLFW_KEY_Q))
            glfwSetWindowShouldClose(window, true);
        
        if (glfwGetKey(window, GLFW_KEY_D)|| glfwGetKey(window, GLFW_KEY_RIGHT)) if (!dx) { dx= 1; dy= 0; }
        if (glfwGetKey(window, GLFW_KEY_A)|| glfwGetKey(window, GLFW_KEY_LEFT))  if (!dx && dy) { dx=-1; dy= 0; }
        if (glfwGetKey(window, GLFW_KEY_W)|| glfwGetKey(window, GLFW_KEY_UP))    if (!dy) { dx= 0; dy=-1; }
        if (glfwGetKey(window, GLFW_KEY_S)|| glfwGetKey(window, GLFW_KEY_DOWN))  if (!dy) { dx= 0; dy= 1; }

        double now = glfwGetTime();
        if (now - last < delta) {
            glfwPollEvents();
            continue;
        }
        last = now;

        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        float x, y;

        for (int i=0; i<SIZE; i++) {
            for (int j=0; j<SIZE; j++) {
                if (!grid[i][j]) continue;

                x = (i * 2.0 + 1.0) / SIZE - 1.0;
                y = 1.0 - (j * 2.0 + 1.0) / SIZE;

                glm_translate2d_make(u_transform, (vec2){x, y});
                glm_scale2d(u_transform, (vec2){s, s});

                glUniformMatrix3fv(loc_transform, 1, 0, u_transform[0]);

                if (grid[i][j] == 1)
                    glUniform3f(loc_color, 1.0, 0.2, 0.2);
                else if (grid[i][j] == 2 || grid[i][j] == 3)
                    glUniform3f(loc_color, 0.2, 1.0, 0.2);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
            }
        }

        if (gameover && glfwGetKey(window, GLFW_KEY_R)) {
            destroy_snake(head.next);
            head.x = (SIZE-1)>>1;
            head.y = (SIZE-1)>>1;
            head.next = NULL;

            memset(&grid, 0, SIZE*SIZE);
            grid[head.x][head.y] = 2;
            snek = &head;

            for (int i=0; i<3; i++) {
                if (!snek->next) {
                    snek->next = malloc(sizeof(Snake));
                    snek->next->x = snek->x-1;
                    snek->next->y = snek->y;
                    grid[snek->x-1][snek->y] = 2;
                    snek->next->next = NULL;
                    snek = snek->next;
                }
            }

            int ax, ay = head.y;
            while (ay == head.y) {
                ax = rand() % SIZE;
                ay = rand() % SIZE;
            }
            grid[ax][ay] = 1;

            gameover = false;
            dx = dy = 0;
        }

        gameover = !update_snake(grid, &head, dx, dy);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(program);

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();
    
    destroy_snake(head.next);

    return 0;
}
