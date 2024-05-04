#include <cstdlib>
#include <iostream>
#include <mutex>
#include <memory>
#include <chrono>
#include <execution>
#include <algorithm>
#include <thread>
#include <vector>

using namespace std::chrono;
using namespace std;

#include <stdlib.h>
#include <cmath>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/fast_exponential.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#pragma once

struct bbox_t {
  glm::vec3 min;
  glm::vec3 max;
  bool unset = true;

  bbox_t& operator+=(const bbox_t& b) {
    *this += b.min;
    *this += b.max;
    return *this;
  }
  bbox_t& operator+=(const glm::vec3& p) {
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

  glm::vec3 center(void) const {
    return (min + max) / 2.0f;
  }
  
  void  print(void) const {
    cout << "bbox[" << glm::to_string(min) << "," << glm::to_string(max) << "]";
  }
};

struct Node {
  glm::vec3 position;
  float weight;
  glm::vec3 velocity;
  float unused;
};

class otNode
{
 private:
  enum type_e { LEAF, NODE };
  type_e type;

  int threshold;
  vector<Node*> nodes;
  vector<otNode*> children;
  std::mutex lock;

  glm::vec3 center;

  float weight = 0;
  bbox_t bbox;
  float bboxSize = 0.0;
  glm::vec3 baryCenter;

 public:
 otNode(int t = 4) : threshold(t), center(0), baryCenter(0), type(LEAF)
    { }
  ~otNode() {
    for (auto c : children) {
      delete c;
    }
  }

  void updateStats(bool parallel) {
    if (type == NODE) {
      // First update all children
      if (parallel) {
	std::for_each(std::execution::par_unseq, children.begin(), children.end(),
		      [&](auto& c) {
			c->updateStats(false);
		      });
      }
      else {
	for (auto& c : children) {
	  c->updateStats(false);
	}
      }

      for (auto& c : children) {
	if (c->type == LEAF && c->nodes.empty())
	  continue;
	bbox += c->bbox;
	weight += c->weight;
	baryCenter += c->baryCenter * float(c->weight);
      }
      baryCenter /= weight;
      bboxSize = ((bbox.max.x - bbox.min.x) +
		  (bbox.max.y - bbox.min.y) +
		  (bbox.max.z - bbox.min.z)) / 3.0;
    }
    else {
      // LEAF
      if (!nodes.empty())
	{
	  for (auto n : nodes) {
	    bbox += n->position;
	    weight += n->weight;
	    baryCenter += n->position * n->weight;
	  }
	  baryCenter /= weight;
	}
    }
  }

  void print(int i = 0, bool leafNodes = false) {
    if (type == LEAF) {
      if (nodes.empty())
	return;
      for (int c = 0; c < i; ++c) {
	cout << " ";
      }
      cout << "Level " << i << " LEAF " << nodes.size() << " ";
      bbox.print();
      cout << " BC " << glm::to_string(baryCenter) << " W " << weight
	   << " size " << bboxSize << endl;
      if (leafNodes) {
	for (auto& n : nodes) {
	  for (int c = 0; c < i + 1; ++c) {
	    cout << " ";
	  }
	  cout << glm::to_string(n->position) << " " << n->weight << endl;
	}
      }
    }
    else {
      for (int c = 0; c < i; ++c) {
	cout << " ";
      }
      cout << "Level " << i << " " << "NODE ";
      bbox.print();
      cout << " BC " << glm::to_string(baryCenter) << " W " << weight
	   << " size " << bboxSize << endl;
    }
    if (type == NODE) {
      for (auto& c : children) {
	c->print(i + 1, leafNodes);
      }
    }
  }

  // Function for calculating the accelerate on m1 by m2
  glm::vec3 force(const glm::vec3& p1, float m1, const glm::vec3& p2, float m2) {
    double d = glm::distance(p2, p1);

    // m1*m2 / r^2
    float force = (m1 * m2) / ((d * d) + sqrt(m1 + m2));

    glm::vec3 direction = glm::normalize(p2 - p1);
    return direction * force;
  }

  void insertNodes(vector<Node>& nodes) {
    std::for_each(std::execution::par_unseq, nodes.begin(), nodes.end(),
		  [&](auto& n) {
		    insert(&n);
		  });
  }
  void insert(Node* n) {
    if (type == NODE) {
      glm::vec3 cmp = glm::lessThan(n->position, center);
      int oct = cmp.x * 2 + cmp.y * 1 + cmp.z * 4;
      children[oct]->insert(n);
    }
    else {
      // LEAF
      lock.lock();
      // If this became a NODE while we were waiting for the lock then we
      // reinsert and folow the NODE path.
      if (type == NODE) {
	lock.unlock();
	this->insert(n);
      }
      else {
	nodes.push_back(n);
	if (nodes.size() >= threshold) {
	  // Initialize child nodes.
	  for (int i = 0; i < 8; ++i) {
	    children.push_back(new otNode(threshold));
	  }

	  // Calculate the center of the points
	  for (auto& n : nodes) {
	    center += n->position;
	  }
	  center /= nodes.size();
	  type = NODE;
	  lock.unlock();
	  std::for_each(std::execution::par_unseq, nodes.begin(), nodes.end(),
			[&](auto& n) {
			  this->insert(n);
			});
	} else {
	  lock.unlock();
	}
      } // if (nodes.size() == threshold)
    } // else
  } // void insert(Node* n)

  void calcForces(vector<Node>& nodes, float theta) {
    std::for_each(std::execution::par_unseq, nodes.begin(), nodes.end(),
		  [&](auto& n) {
		    n.velocity += calcForce(&n, theta);
		  });
  }
  glm::vec3 calcForce(Node* n, float theta) {
    glm::vec3 f(0);
    if (type == NODE) {
      float distance = glm::distance(n->position, baryCenter);
      if (distance / bboxSize > theta) {
	f = force(n->position, n->weight, baryCenter, weight);
      }
      else {
	for (auto& c : children) {
	  f += c->calcForce(n, theta);
	}
      }
    }
    else {
      for (auto& c : nodes) {
	if (c == n) continue;
	f += force(n->position, n->weight, c->position, c->weight);
      }
    }
    return f;
  }

  void updatePositions(vector<Node>& nodes) {
    for (auto& n : nodes) {
      n.position += n.velocity;
    }
  }
  
  struct debugData {
    int maxDepth = 0;
    int numLeafs = 0;
    int numInternalNodes = 0;
    int maxNumNodes = 0;
  };
  void debug(debugData *d = nullptr, int depth = 0) {
    bool root = false;
    if (d == nullptr) {
      root = true;
      d = new debugData();
    }
    if (type == LEAF) {
      if (d->maxDepth < depth)
	d->maxDepth = depth;
      d->numLeafs++;
      if (nodes.size() > d->maxNumNodes)
	d->maxNumNodes = nodes.size();
    }
    else {
      for (auto& c : children)
	c->debug(d, depth + 1);
      d->numInternalNodes++;
    }

    if (root) {
      cout << "debug"
	   << " maxDepth " << d->maxDepth
	   << " numLeafs " << d->numLeafs
	   << " numInternalNodes " << d->numInternalNodes
	   << " maxNumNodes " << d->maxNumNodes
	   << endl;
      delete d;
    }
  }
};

