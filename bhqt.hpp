#ifndef __FD3_PLACEMENT__
#define __FD3_PLACEMENT__

#include <map>
#include <vector>
#include <stdlib.h>
#include <math.h>

#include "geom.hpp"

using namespace std;
namespace bhqt {
  enum nodeType { LEAF, OCTNODE };

  // entry in octNode tree
  class node
  {
  public:
    point_t point;
    double weight;
		
    node(point_t p, double w) : point(p), weight(w) 
    {
    }
    virtual ~node()
    {
    }
  };

  // base class for octNode and leaf
  class baseNode
  {
  public:
    virtual ~baseNode() {}
    virtual nodeType type(void) = 0;
    virtual void insert(node*) = 0;
    virtual point_t forceForNode(node*, float) = 0;
    virtual point_t barycenter(void) = 0;
    virtual double qumWeight(void) = 0;
    virtual point_t getPoint(node *node, point_t p, float theta) = 0;
    virtual void print(int) = 0;
  };

  class leafNode : public baseNode
  {
    vector<node*> nodes_;
  public:
    typedef vector<node*>::iterator iterator;
		
    ~leafNode()
    {
      /*
	iterator b = begin();
	while( b != end() )
	{
	delete *b;
	b++;
	}
      */
    }
		
    void print(int i)
    {
      iterator b = begin();
			
      cout << i << " leafNode points ";
      while( b != end() )
	{
	  cout << "[" << (*b)->point.x << ", " << (*b)->point.y << "] ";
	  b++;
	}
      cout << endl;
			
    }
		
    nodeType type(void) { return LEAF; }
		
    void insert(node *node)
    {
      nodes_.push_back(node);
    }
		
    unsigned int size(void)
    {
      return nodes_.size();
    }
		
    iterator begin(void)
    {
      return nodes_.begin();
    }
		
    iterator end(void)
    {
      return nodes_.end();
    }
		
    void clear(void)
    {
      nodes_.clear();
    }
		
    point_t barycenter(void)
    {
      point_t bc;
      double w = 0.0;
      iterator b = begin();
			
      while( b != end() )
	{
	  w += (*b)->weight;
	  bc += (*b)->point * (*b)->weight;
	  b++;
	}
      bc /= w;
			
      return bc;
    }
		
    double qumWeight(void)
    {
      double w = 0.0;
      iterator b = begin();
			
      while( b != end() )
	{
	  w += (*b)->weight;
	  b++;
	}
			
      return w;
    }
		
    point_t getPoint(node*, point_t p, float)
    {
      return p;
    }
		
    point_t forceForNode(node *node, float)
    {
      point_t force(0.0, 0.0);
      point_t p1 = node->point;
      double p1Weight = node->weight;
			
      iterator b;
      b = begin();
      while( b != end() )
	{
	  if( *b != node )
	    {
	      point_t p2 = (*b)->point;
	      double p2Weight = (*b)->weight;
					
	      double dist = dist3d(p1, p2);
					
	      point_t unit = (p2 - p1) / dist;
	      force += unit * (p2Weight)/(dist*dist+1.0);
	    }
	  b++;
	}
      return force;
    }
  };

  class octNode : public baseNode
  {
    // center of this octNode
    point_t center;
		
    baseNode *children[8];
    unsigned int threshold;
		
    // bounding cube of all the points in this octNode
    cube_t bbox;
		
    // qumulative weight of all points in this octNode
    double weight;
    point_t accumPoints;
    point_t barycenter_;
		
  public:
    nodeType type(void) { return OCTNODE; }
		
    octNode(int t, leafNode *leafNode) : threshold(t)
    {
      weight = 0.0;
			
      for( int octNode = 0; octNode < 8; octNode++ )
	{
	  children[octNode] = NULL;
	}
			
      leafNode::iterator begin, end;
      begin = leafNode->begin();
      end = leafNode->end();
			
      bbox = (*begin)->point;
			
      // compute center of the points
      while( begin != end )
	{
	  center += (*begin)->point;
	  begin++;
	}
      center /= leafNode->size();
			
      begin = leafNode->begin();
      while(begin != end)
	{
	  insert(*begin);
	  begin++;
	}
			
      leafNode->clear();
    }
		
    ~octNode()
    {
      for( int octNode = 0; octNode < 8; octNode++ )
	{
	  if( children[octNode] != NULL )
	    delete children[octNode];
	}
    }
		
    void print(int i)
    {
      cout << i << " OCTNODE bc[" << barycenter_.x << ", " << barycenter_.y << ", " << barycenter_.z << "]" << endl;
      for(int octNode = 0; octNode < 8; octNode++ )
	{
	  cout << i << " " << octNode << " ";
	  if( children[octNode] != NULL )
	    children[octNode]->print(i + 1);
	  else
	    cout << "empty" << endl;
	}
			
    }
		
    void insert(node *node)
    {
      bbox += node->point;
      weight += node->weight;
      accumPoints += node->point * node->weight;
			
      barycenter_ = accumPoints / weight;
			
      int oct = 0;
			
      if( node->point.x > center.x )
	oct += 2;
      if( node->point.y > center.y )
	oct += 1;
      if( node->point.z > center.z )
	oct += 4;

      if( children[oct] == NULL )
	{
	  children[oct] = new leafNode();
	}
			
      if( children[oct]->type() == OCTNODE )
	{
	  children[oct]->insert(node);
	}
      else
	{
	  children[oct]->insert(node);
	  if( ((leafNode*)children[oct])->size() > threshold &&
	      center != node->point)
	    {
	      baseNode *n;
	      n = new octNode(threshold, (leafNode*)children[oct]);
	      delete children[oct];
	      children[oct] = n;
	    }
	}
    }
		
    point_t barycenter(void)
    {
      return barycenter_;
    }
		
    double qumWeight(void)
    {
      return weight;
    }
		
    point_t getPoint(node *node, point_t p, float theta)
    {
      float boxSize = max(max((bbox.top - bbox.bottom), (bbox.right - bbox.left)), bbox.front - bbox.back);
      double dist = dist3d(node->point, barycenter_);
			
      if( boxSize / dist < theta )
	{
	  return barycenter_;
	}
      else
	{
	  int oct = 0;
				
	  if( p.x > center.x )
	    oct += 2;
	  if( p.y > center.y )
	    oct += 1;
	  if( p.z > center.z )
	    oct += 4;
				
	  return children[oct]->getPoint(node, p, theta);
	}
    }
		
    point_t forceForNode(node *node, float theta)
    {
      /// TODO: this could be precomputed.
      float boxSize = max(max((bbox.top - bbox.bottom), (bbox.right - bbox.left)), bbox.front - bbox.back);
      point_t p1 = node->point;
      double p1Weight = node->weight;
			
      double dist = dist3d(p1, barycenter_);
      point_t force;
			
      if( dist / boxSize > theta )
	{
	  point_t p2 = barycenter_;
	  double p2Weight = weight;
								
	  point_t unit = (p2 - p1) / dist;
	  force += unit * (p2Weight)/(dist*dist+1.0);
	}
      else
	{
	  for( int oct = 0; oct < 8; oct++ )
	    {
	      if( children[oct] != NULL )
		force += children[oct]->forceForNode(node, theta);
	    }
	}
			
      return force;
    }
  };

  // a octTree for a variant on the barnes-hut algorithm
  class octTree
  {
  private:
    baseNode *root;
    unsigned int threshold;
  public:
    octTree(unsigned int t = 4) : root(NULL), threshold(t)
    {
      root = new leafNode();
    }
		
    ~octTree()
    {
      delete root;
    }
		
    void print(void)
    {
      root->print(0);
    }
    void insert(node *node)
    {
      if( root->type() == OCTNODE )
	{
	  root->insert(node);
	}
      else
	{
	  root->insert(node);
	  if( ((leafNode*)root)->size() > threshold )
	    {
	      baseNode *n;
	      n = new octNode(threshold, (leafNode*)root);
	      delete root;
	      root = n;
	    }
	}			
    }
		
    point_t getPoint(node *node, point_t p, float theta)
    {
      return root->getPoint(node, p, theta);
    }
		
    point_t forceForNode(node *node, float theta)
    {
      return root->forceForNode(node, theta);
    }
  };
}

#endif

