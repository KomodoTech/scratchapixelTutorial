// To compile: c++ geometry.cpp -o geometry -std=c++11
//

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>

// Generic vector class used for points and normals as well
template<typename T>
class Vec3
{
  public:
      /*===================INITIALIZATION===============================*/
      
      // Init with 0 value
      Vec3() : x(T(0)), y(T(0)), z(T(0)) {}

      // Init with x = y = z
      Vec3(const T &xx) : x(xx), y(xx), z(xx) {}
      
      // Init with given x, y, z values
      Vec3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) {}
     
      /*===============VECTOR OPERATORS==================================*/ 
      
      /*------------------OVERLOADED-------------------------------------*/
      
      // Vector addition
      Vec3 operator + (const Vec3 &v) const
      {
          return Vec3(x + v.x, y + v.y, z + v.z);
      }

      // Vector subtraction
      Vec3 operator - (const Vec3 &v) const
      {
          return Vec3(x - v.x, y - v.y, z - v.z);
      }

      // Scalar multiplication
      Vec3 operator * (const T &r) const
      {
          return Vec3(x * r, y * r, z * r);
      }

      /*------------------NOT OVERLOADED--------------------------------*/ 
      
      // Dot product
      T dotProduct(const Vec3<T> &v) const
      {
          return (x * v.x) + (y * v.y) + (z * v.z);
      }

      // Cross product
      T crossProduct(const Vec3<T> &v) const
      {
          return Vec3<T>(
              y * v.z - z * v.y,
              z * v.x - x * v.z,
              x * v.y - y * v.x);
      }

      // Norm
      T norm() const
      {
        return (x * x) + (y * y) + (z * z);
      }

      // Length/Magnitude
      T length() const
      {
        return sqrt(norm());
      }

      // Normalize
      Vec3& normalize()
      {
          T length = length();

          if (length > 0)
          {
              T inverseLength = 1 / length;

              x = x * inverseLength;
              y = y * inverseLength;
              z = z * inverseLength;
          }

          return *this;
      }
      
      /*========================UTILITY OPERATORS=========================*/
      
      /*----------------------------ACESSORS------------------------------*/

      const T& operator [] (uint8_t i) const
      {
          return (&x)[i];
      }

      T& operator [] (uint8_t i)
      {
          return (&x)[i];
      }

      /*----------------------------STREAMS------------------------------*/
      
      // Output Vec3 values to stream
      // Generates a non-template operator << for this T and gives it access
      // to private and protected members of Vec3<T>
      
      friend std::ostream& operator << (std::ostream &s, const Vec3<T> &v)
      {
          return s << '(' << v.x << ' ' << v.y << ' ' << v.z << ')';
      }

      
      /*============================DATA====================================*/
      
      // 3-TUPLE
      T x, y, z;
};


/*===================================SPECIALIZED CLASSES====================*/

// Use template to make a versions of Vec3 with different types
typedef Vec3<float> Vec3f;
typedef Vec3<int> Vec3i;
typedef Vec3<double> Vec3d;


// TEST

int main()
{
    int ai = 3;
    float af = 3, bf = 4, cf = 5;
    double ad = 3 , bd = 4 , cd = 5;

    Vec3f floatZero = Vec3f();
    Vec3i intOneValue = Vec3i(ai);
    Vec3d doubleThreeValues = Vec3d(ad, bd, cd);
    
    std::cout << floatZero << '\n' << intOneValue << '\n' << doubleThreeValues << '\n';
}

