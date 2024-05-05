#include <chrono>
using namespace std::chrono;

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "octTree.h"

high_resolution_clock::time_point
printTimer(high_resolution_clock::time_point start, std::string msg) {
  cout << " " << msg << " "
       << duration_cast<microseconds>(high_resolution_clock::now() -
				      start).count() / 1000000.0
       << endl;
  return high_resolution_clock::now();
}

glm::vec3 project(bbox_t& bbox, glm::vec3 point) {
    float width = bbox.max.x - bbox.min.x;
    float height = bbox.max.y - bbox.min.y;

    return glm::vec3((point.x - bbox.min.x) / width * 1024.0,
        (point.y - bbox.min.y) / height * 1024.0,
        1.0);
}

void drawNode(SDL_Renderer* renderer, bbox_t& root, bbox_t bbox, otNode* node) {
    if (node->type == otNode::LEAF) {
        glm::vec3 p1 = project(root, bbox.min);
        glm::vec3 p2 = project(root, bbox.max);
        SDL_FRect rect;
        rect.x = p1.x;
        rect.y = p1.y;
        rect.w = p2.x - p1.x;
        rect.h = p2.y - p1.y;
        SDL_RenderRect(renderer, &rect);
        for (auto& n : node->nodes) {
            glm::vec3 p = project(root, n->position);
            SDL_RenderPoint(renderer, p.x, p.y);
        }
    }
    else {
        { // < y
            bbox_t bb;
            bb += node->center;
            bb += bbox.min;
            drawNode(renderer, root, bb, node->children[0]);
        }
        { // > y
            bbox_t bb;
            bb += node->center;
            bb += glm::vec3(bbox.min.x, bbox.max.y, 0.0);
            drawNode(renderer, root, bb, node->children[1]);
        }
        { // < x
            bbox_t bb;
            bb += node->center;
            bb += glm::vec3(bbox.max.x, bbox.min.y, 0.0);
            drawNode(renderer, root, bb, node->children[2]);
        }
        { // > x
            bbox_t bb;
            bb += node->center;
            bb += bbox.max;
            drawNode(renderer, root, bb, node->children[3]);
        }
    }
}

void draw(SDL_Renderer* renderer, otNode& root) {
    if (root.type == otNode::LEAF)
        return;
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    if (!SDL_RenderClear(renderer)) {
        cout << SDL_GetError() << endl;
    }
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    { // < y
        bbox_t bbox;
        bbox += root.center;
        bbox += root.bbox.min;
        drawNode(renderer, root.bbox, bbox, root.children[0]);
    }
    { // > y
        bbox_t bbox;
        bbox += root.center;
        bbox += glm::vec3(root.bbox.min.x, root.bbox.max.y, 0.0);
        drawNode(renderer, root.bbox, bbox, root.children[1]);
    }
    { // < x
        bbox_t bbox;
        bbox += root.center;
        bbox += glm::vec3(root.bbox.max.x, root.bbox.min.y, 0.0);
        drawNode(renderer, root.bbox, bbox, root.children[2]);
    }
    { // > x
        bbox_t bbox;
        bbox += root.center;
        bbox += root.bbox.max;
        drawNode(renderer, root.bbox, bbox, root.children[3]);
    }
}

int main(int argc, char* argv[]) {
    unsigned int numNodes = 100;
    unsigned int threshold = 8;
    float theta = 0.7;
    unsigned int iterations = 60;
    bool printTree = false;

    for (int cnt = 1; cnt < argc; cnt++)
    {
        if (strcmp(argv[cnt], "-n") == 0)
            numNodes = atoi(argv[cnt + 1]);
        if (strcmp(argv[cnt], "-t") == 0)
            threshold = atoi(argv[cnt + 1]);
        if (strcmp(argv[cnt], "-e") == 0)
            theta = atof(argv[cnt + 1]);
        if (strcmp(argv[cnt], "-i") == 0)
            iterations = atoi(argv[cnt + 1]);
        if (strcmp(argv[cnt], "-p") == 0)
            printTree = true;
    }

    std::cout << "nodes: " << numNodes << " threshold: " << threshold
        << " theta: " << theta << " iterations: " << iterations
        << std::endl;
    vector<Node> nodes(numNodes);

    // Initialize positions in a ball.
    float r = 40 * std::cbrt(numNodes);
    for (auto& n : nodes) {
        //	n.position = glm::ballRand(r);
        n.position = glm::vec3(glm::diskRand(r), 1.0);
        n.weight = glm::fastExp(glm::linearRand(0.0, 6.0));
        n.velocity = glm::vec3(0.0f);
    }

    const int WIDTH = 1024;
    const int HEIGHT = 1024;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Hello SDL", WIDTH, HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    SDL_Event event;
    int done = 0;
    otNode* root = new otNode(threshold);
    while (!done) {
        /* look for an event */
        if (SDL_PollEvent(&event)) {
            /* an event was found */
            switch (event.type) {
                /* close button clicked */
            case SDL_EVENT_QUIT:
                done = 1;
                break;

                /* handle the keyboard */
            case SDL_EVENT_KEY_DOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    done = 1;
                    break;
                case SDLK_s: {
                    auto start = high_resolution_clock::now();
                    delete root;
                    root = new otNode(threshold);
                    start = printTimer(start, "root");


                    root->insertNodes(nodes);
                    start = printTimer(start, "insert");

                    root->updateStats(true);
                    start = printTimer(start, "stats");

                    root->debug();
                    if (printTree)
                        root->print();

                    root->calcForces(nodes, theta);
                    start = printTimer(start, "forces");

                    root->updatePositions(nodes);
                    start = printTimer(start, "update");
                    break;
                }
                }
                break;
            }
        }
        draw(renderer, *root);
        /* update the screen */
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;

    return 0;
}
