#ifndef __GEOM__
#define __GEOM__

#include <iostream>
namespace bhqt 
{
  class point_t
  {
  public:
    double x;
    double y;
    double z;
		
    friend std::ostream &operator<<(std::ostream &stream, point_t p);
		
    point_t(): x(0.0), y(0.0), z(0.0){}
    point_t(double _x, double _y, double _z = 0.0):x(_x), y(_y), z(_z){}
		
    point_t& operator*=(const double scalar)
    {
      x *= scalar;
      y *= scalar;
      z *= scalar;
      return *this;
    }
		
    point_t operator*(const double scalar)
    {
      return point_t(x*scalar, y*scalar, z*scalar);
    }
		
    point_t& operator/=(const double scalar)
    {
      x /= scalar;
      y /= scalar;
      z /= scalar;
      return *this;
    }
		
    point_t operator/(const double scalar)
    {
      return point_t(x/scalar, y/scalar, z/scalar);
    }
		
    point_t operator-(const point_t rhs)
    {
      return point_t(x-rhs.x, y-rhs.y, z-rhs.z);
    }
		
    point_t& operator+=(const point_t rhs)
    {
      x += rhs.x;
      y += rhs.y;
      z += rhs.z;
      return *this;
    }
		
    point_t operator+(const point_t rhs)
    {
      return point_t(x+rhs.x, y+rhs.y, z+rhs.z);
    }
		
    bool operator==(const point_t rhs)
    {
      return fabs(rhs.x - x + rhs.y - y + rhs.z - z) < 1.0e-10;
    }
		
    bool operator!=(const point_t rhs)
    {
      return !operator==(rhs);
    }
		
    point_t& operator=(const point_t rhs)
    {
      x = rhs.x;
      y = rhs.y;
      z = rhs.z;
      return *this;
    }
  };

  std::ostream &operator<<(std::ostream &stream, point_t p)
  {
    stream << "[" << p.x << ", " << p.y << ", " << p.z << "]";
    return stream;
  }

  class cube_t
  {
  public:
    double left;
    double right;
    double top;
    double bottom;
    double front;
    double back;
		
    cube_t(): left(0.0), right(0.0), top(0.0), bottom(0.0), 
	      front(0.0), back(0.0)
    {
			
    }
    cube_t& operator=(const point_t rhs)
    {
      left = right = rhs.x;
      top = bottom = rhs.y;
      front = back = rhs.z;
      return *this;
    }
		
    cube_t& operator+=(const point_t p)
    {
      if( p.x < left )
	left = p.x;
      if( p.x > right )
	right = p.x;
      if( p.y > top )
	top = p.y;
      if( p.y < bottom )
	bottom = p.y;
      if( p.z < back )
	back = p.z;
      if( p.z > front )
	front = p.z;
			
      return *this;
    }
		
    cube_t& operator+=(const cube_t rhs)
    {
      if( rhs.left < left )
	left = rhs.left;
      if( rhs.right > right )
	right = rhs.right;
      if( rhs.bottom < bottom )
	bottom = rhs.bottom;
      if( rhs.top > top )
	top = rhs.top;
      if( rhs.back < back )
	back = rhs.back;
      if( rhs.front > front )
	front = rhs.front;
			
      return *this;
    }
		
    point_t center(void)
    {
      return point_t((right+left)/2.0, (top+bottom)/2.0, (front+back)/2.0);
    }
  };

  double dist3d(point_t p1, point_t p2)
  {
    double xd, yd, zd;
    xd = p2.x - p1.x;
    yd = p2.y - p1.y;
    zd = p2.z - p1.z;
    return sqrt(xd*xd + yd*yd + zd*zd);
  }
}

#endif

