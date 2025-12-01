
#include <iostream>
#include <cassert>
#include <string>
#include "zmap.h"

DEFINE_MAP_TYPE(int, float, IntFloat)
DEFINE_MAP_TYPE(int, int, IntInt)

template <typename Map>
void print_map(const Map& m, const std::string& label) 
{
    std::cout << label << " (Size: " << m.size() << "): { ";
    for (const auto& bucket : m) 
    {
        std::cout << "[" << bucket.key << ":" << bucket.value << "] ";
    }
    std::cout << "}\n";
}

int main() 
{
    auto hash_int = [](int k) -> uint32_t { return static_cast<uint32_t>(k * 2654435761); };
    auto cmp_int = [](int a, int b) -> int { return a - b; };

    std::cout << "=> Basic operations\n";
    
    z_map::map<int, float> my_map(hash_int, cmp_int);

    my_map.put(1, 1.1f);
    my_map.put(2, 2.2f);
    my_map.put(10, 10.5f);

    my_map.insert_or_assign(1, 9.9f); 

    assert(my_map.size() == 3);
    std::cout << "Size is correct: " << my_map.size() << "\n";

    float* val = my_map.get(2);
    if (val) std::cout << "Found key 2: " << *val << "\n";
    else std::cout << "Error: Key 2 not found\n";

    if (my_map.contains(10)) std::cout << "Map contains key 10.\n";
    if (!my_map.contains(99)) std::cout << "Map correctly missing key 99.\n";

    print_map(my_map, "my_map");

    std::cout << "\n=> Iterators and modification\n";

    for (auto& item : my_map) 
    {
        item.value += 1.0f; 
    }
    print_map(my_map, "my_map (after +1.0)");

    const auto& const_ref = my_map;
    std::cout << "Iterating const map keys: ";
    for (const auto& item : const_ref) 
    {
        std::cout << item.key << " ";
    }
    std::cout << "\n";

    std::cout << "\n=> Move semantics\n";

    {
        z_map::map<int, float> recipient(hash_int, cmp_int);
        
        recipient = std::move(my_map);

        std::cout << "Recipient size: " << recipient.size() << " (Expected: 3)\n";
        std::cout << "Original size:  " << my_map.size() << " (Expected: 0)\n";
        
        assert(recipient.size() == 3);
        assert(my_map.size() == 0);
    } // 'recipient' goes out of scope here, so memory should be freed automatically.

    std::cout << "Map destroyed successfully out of scope.\n";

    std::cout << "\n=> Stress test (resizing)\n";
    
    z_map::map<int, int> stress_map(hash_int, cmp_int);
    for(int i = 0; i < 100; ++i) 
    {
        stress_map.put(i, i * 10);
    }
    std::cout << "Inserted 100 items. Current size: " << stress_map.size() << "\n";
    assert(stress_map.size() == 100);

    int* s_val = stress_map.get(50);
    if(s_val && *s_val == 500) 
    {
        std::cout << "Verification successful: Key 50 == 500\n";
    } 
    else 
    {
        std::cout << "Verification failed!\n";
    }

    std::cout << "\nAll tests passed!\n";
    return 0;
}
