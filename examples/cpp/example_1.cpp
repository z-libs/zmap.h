
#include <iostream>

// Register types (visible to C++ compiler).
#define REGISTER_ZMAP_TYPES(X) \
    X(int, float, IntFloat)

#include "zmap.h"

int main()
{
    // Constructor handles init and seed.
    // Note: We use unary '+' to force conversion from lambda to function pointer.
    z_map::map<int, float> data(
        +[](int k, uint32_t s) -> uint32_t { return (k * 2654435761u) ^ s; }, // Simple Hash.
        +[](int a, int b) -> int { return a - b; }                            // Compare.
    );

    // STL-style insertion.
    data.put(42, 3.14f);
        
    // STL-compatible Iterators.
    for (auto& bucket : data) 
    {
        std::cout << bucket.key << ": " << bucket.value << "\n";
    }
        
    return 0;
}
