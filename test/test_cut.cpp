#include "cut/cut.hpp"

#include <iostream>
#include <vector>
#include <assert.h>

int main()
{
  std::vector<uint32_t> data{ 1, 2, 3, 4, 5 ,  6 , 7 , 8 , 9 , 10};
  
  iFPGA_NAMESPACE::cut<10 , uint32_t> cut1;
  
  cut1.set_leaves( std::begin(data) , std::end(data) );
  
  struct cut_data
  {
    uint32_t costs;
    uint32_t area;
  };
  
  iFPGA_NAMESPACE::cut<30 , cut_data> cut2;
  cut2.set_leaves( data );
  cut2->costs = 50;
  cut2->area = 46;
  
  iFPGA_NAMESPACE::cut<30 , cut_data> cut3;
  cut3.set_leaves( std::vector<uint32_t>{1,2,3,4} );
  
  iFPGA_NAMESPACE::cut<30 , cut_data> cut4;
  cut4.set_leaves( std::vector<uint32_t>{1,2,3} );
  
  // test signature
  assert(cut3.signature() == 30);
  assert(cut4.signature() == 14);
  //assert(cut4.get_signature() == 15);
  
  // test dominates
  assert(! cut3.dominates(cut4) );
  assert( cut4.dominates(cut3) );
  
  iFPGA_NAMESPACE::cut<20> c1 , c2 , c3 , res12 , res13;
  c1.set_leaves(data.begin() , data.begin() + 2);
  c2.set_leaves(data.begin() + 1 , data.begin() + 3);
  c3.set_leaves(data.begin() + 2 , data.begin() + 4);
  
  // test merge
  assert( c1.merge(c2 , res12 , 4) );
  assert( res12.signature() == 14);
  
  assert( c1.merge(c3 , res13 , 4) );
  assert( res13.signature());
  
  return 0;
}
