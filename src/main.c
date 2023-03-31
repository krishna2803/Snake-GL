#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include <glad/glad.h>
#include "GLFW/glfw3.h"

#include "cglm/cglm.h"

const uint8_t SIZE = 16; // size of the grid
const uint8_t FPS  = 8; // keep this low
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
    while (ptr) {
        grid[px][py] = 3;
        int ppx = ptr->x;
        int ppy = ptr->y;
        ptr->x = px;
        ptr->y = py;
        px = ppx;
        py = ppy;
        ptr = ptr->next;
    }
    if (apple) {
        Snake *tail = head->next;
        while (tail->next)
            tail = tail->next;
        tail->next = malloc(sizeof(Snake));
        tail->next->x = px;
        tail->next->y = py;
        tail->next->next = NULL;

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

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

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
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    unsigned int vbo;
    // unit square
    const float vbo_data[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, -0.5f };
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 32, vbo_data, GL_STATIC_DRAW);

    unsigned int ebo;
    const unsigned char ebo_data[] = { 0, 1, 2, 0, 2, 3 };
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6, ebo_data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    const char *vertex_src = "#version 460 core\n\
    layout(location=0) in vec2 a_pos;\n\
uniform mat3 u_transform;\n\
void main() { gl_Position = vec4(u_transform * vec3(a_pos, 1.0), 1.0); }\n";

    const char *fragment_src = "#version 460 core\n\
    uniform vec3 u_color;\n\
void main(){ gl_FragColor = vec4(u_color, 1.0);}\n";

    unsigned int vs, fs, program;
    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const char**) &vertex_src, NULL);
    glCompileShader(vs);

    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const char**) &fragment_src, NULL);
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

    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) || glfwGetKey(window, GLFW_KEY_Q))
            glfwSetWindowShouldClose(window, true);

        double now = glfwGetTime();
        if (now - last < delta) {
            glfwPollEvents();
            continue;
        }
        last = now;

        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        for (int i=0; i<SIZE; i++) {
            for (int j=0; j<SIZE; j++) {
                if (!grid[i][j]) continue;

                float x = (i * 2.0 + 1.0) / SIZE - 1.0;
                float y = 1.0 - (j * 2.0 + 1.0) / SIZE;

                mat3 u_transform;
                glm_translate2d_make(u_transform, (vec2){x, y});
                glm_scale2d(u_transform, (vec2){s, s});

                int loc = glGetUniformLocation(program, "u_transform");
                glUniformMatrix3fv(loc, 1, 0, u_transform[0]);
                loc = glGetUniformLocation(program, "u_color");

                if (grid[i][j] == 1)
                    glUniform3f(loc, 1.0, 0.2, 0.2);
                else if (grid[i][j] == 2 || grid[i][j] == 3)
                    glUniform3f(loc, 0.2, 1.0, 0.2);
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
        
        if (glfwGetKey(window, GLFW_KEY_D)|| glfwGetKey(window, GLFW_KEY_RIGHT)) if (!dx) { dx= 1; dy= 0; }
        if (glfwGetKey(window, GLFW_KEY_A)|| glfwGetKey(window, GLFW_KEY_LEFT))  if (!dx && dy) { dx=-1; dy= 0; }
        if (glfwGetKey(window, GLFW_KEY_W)|| glfwGetKey(window, GLFW_KEY_UP))    if (!dy) { dx= 0; dy=-1; }
        if (glfwGetKey(window, GLFW_KEY_S)|| glfwGetKey(window, GLFW_KEY_DOWN))  if (!dy) { dx= 0; dy= 1; }

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
