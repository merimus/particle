// Open a window with GLFW, check default OpenGL version, set background color red
// Register callback functions for GLFW errors, key pressed and window resized events
// Require minimum OpenGL 3.2 core profile and remove the fixed pipeline functionality
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>

// Define a few callback functions:
void window_resized(GLFWwindow* window, int width, int height);
void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods);
void show_glfw_error(int error, const char* description);


/*
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT
{
    vec2 TexCoords;
} vs_out;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
*/
const char* vertexShaderSource = R"glsl(
#version 330 core
in vec3 position;
void main()
{
   gl_Position = vec4(position, 1.0);
   gl_PointSize = 100.0;
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 outColor;

void main()
{
    outColor = vec4(1.0f, 0.5f, 0.2f, 0.1f);
}
)glsl";

void compileShader(unsigned int shaderId, const char** source) {
    glShaderSource(shaderId, 1, source, NULL);
    glCompileShader(shaderId);

    int  success;
    char infoLog[512];
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        exit(-1);
    }
}

int main() {
    // Register the GLFW error callback function
    glfwSetErrorCallback(show_glfw_error);

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW! I'm out!" << '\n';
        exit(-1);
    }

    // Require minimum OpenGL 3.2 core profile and remove the fixed pipeline functionality
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // Open a window and attach an OpenGL context to the window surface
    GLFWwindow* window = glfwCreateWindow(600, 600, "OpenGL 101", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to open a window! I'm out!" << '\n';
        glfwTerminate();
        exit(-1);
    }

    // Set the window context current
    glfwMakeContextCurrent(window);

    // Register the GLFW  window resized callback function
    glfwSetWindowSizeCallback(window, window_resized);

    // Register the GLFW  window key pressed callback function
    glfwSetKeyCallback(window, key_pressed);

    // Set the swap interval, 1 will use your screen refresh rate (vsync)
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cerr << "Error:" << glewGetErrorString(err) << std::endl;
        exit(-1);
    }
    std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
    std::cout << glGetString(GL_VERSION) << '\n';


    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    float vertices[] = {
      -0.1f, -0.1f, 0.0f,
      0.1f, -0.0f, 0.0f,
      0.0f,  0.1f, 0.0f
    };

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    compileShader(vertexShader, &vertexShaderSource);
    compileShader(fragmentShader, &fragmentShaderSource);
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Link failed " << infoLog << std::endl;
        exit(-1);
    }
    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    GLint positionAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    while (!glfwWindowShouldClose(window)) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_POINTS, 0, 3);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // Destroy the window and its context
    glfwDestroyWindow(window);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

void show_glfw_error(int error, const char* description) {
    std::cerr << "Error: " << description << '\n';
}

void window_resized(GLFWwindow* window, int width, int height) {
    std::cout << "Window resized, new window size: " << width << " x " << height << '\n';
}

void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == 'Q' && action == GLFW_PRESS) {
        glfwTerminate();
        exit(0);
    }
}

