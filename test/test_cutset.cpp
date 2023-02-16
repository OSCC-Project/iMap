#include "cut/cut_set.hpp"
#include <vector>
#include <algorithm>
#include <assert.h>

iFPGA_NAMESPACE_USING_NAMESPACE

int main()
{
    using cut_type = cut<10>;

    cut_type c1, c2, c3, c4, c5;
    c1.set_leaves( std::vector<uint32_t>{3, 6} );
    c2.set_leaves( std::vector<uint32_t>{1, 2, 3} );
    c3.set_leaves( std::vector<uint32_t>{1, 2, 3, 8} );
    c4.set_leaves( std::vector<uint32_t>{3, 4, 5} );
    c5.set_leaves( std::vector<uint32_t>{7, 8} );

    cut_set<cut_type, 25> set;

    assert( !set.is_dominated( c1 ) );
    set.insert( c1 );
    assert( set.size() == 1 );

    assert( !set.is_dominated( c2 ) );
    set.insert( c2 );
    assert( set.size() == 2 );

    assert( set.is_dominated( c3 ) );
    assert( set.size() == 2 );

    return 0;
}