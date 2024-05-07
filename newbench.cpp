#include <chrono>
using namespace std::chrono;

#include "octTree.h"

high_resolution_clock::time_point
printTimer(high_resolution_clock::time_point start, std::string msg) {
  cout << " " << msg << " "
       << duration_cast<microseconds>(high_resolution_clock::now() -
				      start).count() / 1000000.0
       << endl;
  return high_resolution_clock::now();
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

    auto iter_start = high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
      auto start = high_resolution_clock::now();
      otNode root(threshold);
      start = printTimer(start, "root");

      root.insertNodes(nodes);
      start = printTimer(start, "insert");

      root.updateStats(true);
      start = printTimer(start, "stats");

      root.debug();
      if (printTree)
	root.print();

      root.calcForces(nodes, theta);
      start = printTimer(start, "forces");

      root.updatePositions(nodes);
      start = printTimer(start, "update");
    }
    printTimer(iter_start, "iterations");
    
    return 0;
}
