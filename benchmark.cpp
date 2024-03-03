#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <time.h>
//#include <algorithm>
//#include <execution>
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
using namespace tbb;

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

class applyCalcForce {
  std::vector<myNode*> my_nodes;
  bhqt::octTree &ot;
  float theta;
public:
  void operator()(const blocked_range<size_t>& r) const
  {
    std::vector<myNode*> nodes = my_nodes;
    for( size_t i=r.begin(); i!=r.end(); ++i )
      {
	nodes[i]->_force 
	  += ot.forceForNode(nodes[i], theta) * 5.0 / nodes[i]->weight;  
      }
  }
  applyCalcForce(bhqt::octTree &ot_, std::vector<myNode*> &n, float t = 0.6):my_nodes(n),ot(ot_),theta(t) {}
};

int main(int argc, char *argv[])
{
  unsigned int _numNodes = 1000;
  unsigned int threshold = 2;
  float theta = 0.9;
  int iterations = 100;
  int pthreads = 1;

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

  int iteration = 0;
  task_arena arena(pthreads);
  for( iteration = 0; iteration < iterations; iteration++ )
    {
      bhqt::octTree ot(threshold);

      for(unsigned int cnt = 0; cnt < _numNodes; cnt++)
	{
	  ot.insert(_nodes[cnt]);
	}
      ot.insert(_sun);
      arena.execute([&]{
      parallel_for(blocked_range<size_t>(0,_numNodes), 
		   applyCalcForce(ot, _nodes, theta),
		   auto_partitioner());
      });

      /*      
      for(unsigned int cnt = 0; cnt < _numNodes; cnt++)
	{
	  _nodes[cnt]->_force 
	    += ot.forceForNode(_nodes[cnt], theta) * 5.0 / _nodes[cnt]->weight;
	}
      */
    }

  return 0;
}

