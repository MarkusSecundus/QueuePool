#ifndef MATH_UTILS__guard____gfdsf94sd9g4ds9rg52v6dfs5g4f6d51fg1dfs6
#define MATH_UTILS__guard____gfdsf94sd9g4ds9rg52v6dfs5g4f6d51fg1dfs6

#include<cstddef>

namespace math{

    template<std::convertible_to<std::int64_t> TNumber>
    TNumber divide_round_up(TNumber a, TNumber divider){
        return a / divider + !!(a % divider);
    }







}
#endif