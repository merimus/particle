// Open a window with GLFW, check default OpenGL version, set background color red
// Register callback functions for GLFW errors, key pressed and window resized events
// Require minimum OpenGL 3.2 core profile and remove the fixed pipeline functionality
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <memory>
using namespace std;

#include <stdlib.h>
#include <cmath>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/fast_exponential.hpp>
#include <glm/gtc/type_ptr.hpp>

struct bbox_t {
    glm::vec3 min;
    glm::vec3 max;
    bool unset = true;

    bbox_t& operator+=(const glm::vec3 p) {
        if (unset) {
            min = max = p;
            unset = false;
        }
        else {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }
        return *this;
    }

    glm::vec3 center(void) {
        return (min + max) / 2.0f;
    }
};

struct Node {
    glm::vec3 position;
    float weight;
    glm::vec3 velocity;
    float unused;
};

#include <vector>
class otNode
{
private:
    enum type_e { LEAF, NODE };
    type_e type;
    int threshold;
    vector<Node*> nodes;
    vector<otNode> children;

    glm::vec3 center;
    bbox_t bbox;

    float weight = 0;
    float bboxSize = 0.0;
    glm::vec3 baryCenter;

public:
    otNode(int t = 4) : threshold(t), center(0), baryCenter(0), type(LEAF)
    { }

    void updateStats(void) {
        if (type == NODE) {
            for (auto c : children) {
                c.updateStats();
            }
            glm::vec3 bc(0);
            for (auto c : children) {
                weight += c.weight;
                baryCenter += c.baryCenter * float(c.weight);
                baryCenter /= weight;
            }
            bboxSize = (bbox.max.x - bbox.min.x) + 
                (bbox.max.y - bbox.min.y) + 
                (bbox.max.z - bbox.min.z) / 3.0;
        }
        else {
            for (auto n : nodes) {
                weight += n->weight;
                baryCenter += n->position * n->weight;
            }
            baryCenter /= weight;
        }
    }

    void print(int i = 0) {
        for (int c = 0; c < i; ++c) {
            cout << " ";
        }
        cout << "Level " << i << " " << ((type == NODE) ? "NODE " : "LEAF ") << nodes.size() << endl;
        if (type == NODE) {
            for (auto c : children) {
                c.print(i + 1);
            }
        }
    }
  glm::vec3 __attribute__ ((noinline)) force(glm::vec3& p1, float m1, glm::vec3& p2, float m2) {
    
        double d = glm::distance(p2, p1);
        
        // m1*m2 / r^2
        float force = (m1 * m2) / ((d * d) + sqrt(m1 + m2));

        glm::vec3 direction = glm::normalize(p2 - p1);
        return direction * force;
	/*
    double d = glm::distance(p2, p1);
    glm::vec3 direction = glm::normalize(p2 - p1);
    return direction * (float)(m2/(d*d+1.0));
	*/
  }
    void insert(Node* n) {
        bbox += n->position;

        if (type == NODE) {
            glm::vec3 cmp = glm::lessThan(n->position, center);
            int oct = cmp.x * 2 + cmp.y * 1 + cmp.z * 4;
            children[oct].insert(n);
        }
        else {
            // LEAF
            nodes.push_back(n);
            if (nodes.size() == threshold) {
                // Initialize child nodes.
                for (int i = 0; i < threshold; ++i) {
                    children.push_back(otNode(threshold));
                }
                center = bbox.center();
                type = NODE;
                for (auto n : nodes) {
                    this->insert(n);
                }
                nodes.clear();
            } // if (nodes.size() == threshold)
        } // else
    } // void insert(Node* n)

    glm::vec3 calcForce(Node* n, float theta) {
        glm::vec3 f(0);
        if (type == NODE) {
            float distance = glm::distance(n->position, baryCenter);
            if (distance / bboxSize > theta) {
                f = force(n->position, n->weight, baryCenter, weight);
            } 
            else {
                for (auto c : children) {
                    f += c.calcForce(n, theta);
                }
            }
        }
        else {
            for (auto c : nodes) {
                if (c == n) continue;
                f = force(n->position, n->weight, c->position, c->weight);
            }
        }
        return f;
    }
};

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
    gl_Position = view * model * vec4(aPos, 1.0);
*/
const char* vertexShaderSource = R"glsl(
#version 330 core
in vec3 position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
   gl_Position = projection * view * model * vec4(position, 1.0);
   gl_PointSize = 2.0;
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 outColor;

void main()
{
    outColor = vec4(1.0f, 1.0f, 1.0f, 0.5f);
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

GLFWwindow* initGlfw(void) {
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
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Particles", NULL, NULL);
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
    return window;
}

int main() {
    GLFWwindow* window = initGlfw();

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

    const int numNodes = 1000;

    vector<Node> nodes(numNodes);

    // Initialize positions in a ball.
    float r = 40 * std::cbrt(numNodes);
    for (auto& n : nodes) {
        n.position = glm::ballRand(r);
        n.weight = glm::fastExp(glm::linearRand(0.0, 6.0));
        n.velocity = glm::vec3(0.0f);
    }

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(nodes), &nodes, GL_STREAM_DRAW);

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
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_TRUE, sizeof(Node), (void*)0);
    glEnableVertexAttribArray(positionAttrib);

    glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(0.0, 0.0, 1.0));
    glm::mat4 projection = glm::perspective(35.0f, 1.0f, 0.1f, 10000000.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, 2*r),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));

    GLint modelUniform = glGetUniformLocation(shaderProgram, "model");
    glProgramUniformMatrix4fv(shaderProgram, modelUniform, 1, GL_FALSE, glm::value_ptr(model));
    GLint viewUniform = glGetUniformLocation(shaderProgram, "view");
    glProgramUniformMatrix4fv(shaderProgram, viewUniform, 1, GL_FALSE, glm::value_ptr(view));
    GLint projectionUniform = glGetUniformLocation(shaderProgram, "projection");
    glProgramUniformMatrix4fv(shaderProgram, projectionUniform, 1, GL_FALSE, glm::value_ptr(projection));

    int frameCount = 0;
    double prevTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - prevTime >= 1.0) {
            cout << "fps " << frameCount << endl;
            frameCount = 0;
            prevTime = currentTime;
        }

        otNode root(1280);

        for (auto& n : nodes) {
            root.insert(&n);
        }
        root.updateStats();
        // Calculate forces.
        for (auto& n : nodes) {
            n.velocity += root.calcForce(&n, 0.7 /* theta */);
        }
        for (auto& n : nodes) {
            n.position += n.velocity;
        }

        glBufferData(GL_ARRAY_BUFFER, nodes.size() * sizeof(Node), nodes.data(), GL_STREAM_DRAW);

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_POINTS, 0, numNodes);

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

