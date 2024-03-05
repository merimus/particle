#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <thread>
#include <execution>
#include <algorithm>
#include <vector>
#include "bhqt.hpp"

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
  unsigned int pthreads = 1;

  for( int cnt = 1; cnt < argc; cnt++ )
    {
      if( strcmp(argv[cnt], "-n") == 0 )
	_numNodes = atoi(argv[cnt+1]);
      if( strcmp(argv[cnt], "-t") == 0 )
	threshold = atoi(argv[cnt+1]);
      if( strcmp(argv[cnt], "-e") == 0 )
	theta = atof(argv[cnt+1]);
      if( strcmp(argv[cnt], "-i") == 0 )
	iterations = atoi(argv[cnt+1]);
      if( strcmp(argv[cnt], "-p") == 0 )
	pthreads = atoi(argv[cnt+1]);
    }

  std::cout << "nodes: " << _numNodes << " threshold: " << threshold
	    << " theta: " << theta << " iterations: " << iterations
	    << " num_threads: " << pthreads
	    << std::endl;

  std::vector<myNode*> _nodes;
  myNode *_sun;

  //srandomdev();

  _sun = new myNode(bhqt::point_t(0.0, 0.0), 10*sqrt(_numNodes));
  _nodes.resize(_numNodes);

  int dimension = 2*_numNodes;
  for(unsigned int cnt = 0; cnt < _numNodes; cnt++)
      {
	_nodes[cnt] = 
	  new myNode(bhqt::point_t((float)(random() % (dimension - dimension/2)),
				   (float)(random() % (dimension - dimension/2)),
				   (float)(random() % (dimension - dimension/2))), 
		     (float)(random() % 10 + 0.1));
      }

  for(int iteration = 0; iteration < iterations; iteration++ )
    {
      bhqt::octTree ot(threshold);

      for(unsigned int cnt = 0; cnt < _numNodes; cnt++)
	{
	  ot.insert(_nodes[cnt]);
	}
      ot.insert(_sun);

      std::for_each(std::execution::par, _nodes.begin(), _nodes.end(), [&](myNode* n){
	n->_force 
	  += ot.forceForNode(n, theta) * 5.0 / n->weight;
      });
    }

  return 0;
}

