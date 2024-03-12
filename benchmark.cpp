#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <thread>
#include <execution>
#include <algorithm>
#include <vector>
#include <chrono>
#include <mutex>

#include "bhqt.hpp"

using namespace std::chrono;

class myNode : public bhqt::node
{
public:
  bhqt::point_t _force;

  myNode(bhqt::point_t p, double w) : node(p, w)
  {
  }
  ~myNode()
  {
  }
};

int main(int argc, char *argv[])
{
  unsigned int _numNodes = 1000;
  unsigned int threshold = 2;
  float theta = 0.9;
  unsigned int iterations = 100;

  for (int cnt = 1; cnt < argc; cnt++)
  {
    if (strcmp(argv[cnt], "-n") == 0)
      _numNodes = atoi(argv[cnt + 1]);
    if (strcmp(argv[cnt], "-t") == 0)
      threshold = atoi(argv[cnt + 1]);
    if (strcmp(argv[cnt], "-e") == 0)
      theta = atof(argv[cnt + 1]);
    if (strcmp(argv[cnt], "-i") == 0)
      iterations = atoi(argv[cnt + 1]);
  }

  std::cout << "nodes: " << _numNodes << " threshold: " << threshold
            << " theta: " << theta << " iterations: " << iterations
            << std::endl;

  myNode _sun(bhqt::point_t(0.0, 0.0), 10 * sqrt(_numNodes));
  std::vector<myNode> _nodes;
  _nodes.resize(_numNodes, _sun);

  // srandomdev();

  auto start = high_resolution_clock::now();
  int dimension = 2 * _numNodes;

  for (auto &n : _nodes)
  {
    n.point.set((float)(rand() % (dimension - dimension / 2)),
                (float)(rand() % (dimension - dimension / 2)),
                (float)(rand() % (dimension - dimension / 2)));
    n.weight = (float)(rand() % 10 + 0.1);
  }
  std::cout << "random " << duration_cast<microseconds>(high_resolution_clock::now() - start).count()
            << std::endl;
  for (int iteration = 0; iteration < iterations; iteration++)
  {
    bhqt::octTree ot(threshold);

    start = high_resolution_clock::now();
    std::vector<std::mutex> locks(63);
    std::for_each(std::execution::par_unseq, _nodes.begin(), _nodes.end(), [&](myNode& n){
      ot.insert(&n, locks);
    });
    ot.insert(&_sun, locks);
    std::cout << "insert " 
      << duration_cast<microseconds>(high_resolution_clock::now() - start).count() / 1000000.0
      << std::endl;

    start = high_resolution_clock::now();
    std::for_each(std::execution::par_unseq, _nodes.begin(), _nodes.end(), [&](myNode& n)
                  { n._force += ot.forceForNode(&n, theta) * 5.0 / n.weight; });
    std::cout << "forces " 
              << duration_cast<microseconds>(high_resolution_clock::now() - start).count() / 1000000.0
              << std::endl;
  }

  bhqt::point_t p;
  double weight = 0;
  for (auto &n : _nodes)
  {
    p += n._force;
    weight += n.weight;
  }
  std::cout << p << " " << weight << std::endl;
  return 0;
}
