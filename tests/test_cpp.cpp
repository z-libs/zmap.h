
#include <iostream>
#include <string>
#include <cassert>

#define REGISTER_ZMAP_TYPES(X) \
    X(int, int, IntInt)        \
    X(std::string, float, StrFloat)

#include "zmap.h"

#define TEST(name) printf("[TEST] %-40s", name);
#define PASS() std::cout << "\033[0;32mPASS\033[0m\n";

uint32_t hash_int(int k, uint32_t s) { return (uint32_t)k ^ s; }
int cmp_int(int a, int b) { return a - b; }

uint32_t hash_str(std::string k, uint32_t s) { return ZMAP_HASH_FUNC(k.c_str(), k.length(), s); }
int cmp_str(std::string a, std::string b) { return a.compare(b); }

void test_cpp_wrappers() 
{
    TEST("C++ Wrapper (Put, [], At)");

    // Constructor handles init.
    z_map::map<int, int> m(hash_int, cmp_int);
    
    // Put.
    m.put(1, 100);
    m.put(2, 200);

    // Operator [] (read/write).
    assert(m[1] == 100);
    m[3] = 300; // Auto-insert.
    assert(m.size() == 3);

    // At (exceptions).
    try 
    {
        m.at(99); // Should throw.
        assert(false);
    } 
    catch (const std::out_of_range&) 
    {
        // Good.
    }

    PASS(); // Destructor handles free.
}

void test_complex_types() 
{
    TEST("Complex Types (std::string Keys)");

    z_map::map<std::string, float> prices(hash_str, cmp_str);

    prices["Apple"] = 1.50f;
    prices["Banana"] = 0.80f;

    assert(prices.contains("Apple"));
    assert(!prices.contains("Cherry"));
    assert(prices["Banana"] == 0.80f);

    prices.erase("Apple");
    assert(prices.size() == 1);

    PASS();
}

void test_stl_iterators() 
{
    TEST("STL Iterators (Range-based for)");

    z_map::map<int, int> m(hash_int, cmp_int);
    m[10] = 1;
    m[20] = 2;
    m[30] = 3;

    int sum_vals = 0;
    // C++11 Range Loop compatible.
    for (auto entry : m) {
        sum_vals += entry.value;
    }

    assert(sum_vals == 6);
    PASS();
}

void test_move_semantics() 
{
    TEST("Move Semantics (Rule of 5)");

    z_map::map<int, int> m1(hash_int, cmp_int);
    m1[1] = 100;

    // Move Constructor.
    z_map::map<int, int> m2 = std::move(m1);
    
    assert(m2.size() == 1);
    assert(m2[1] == 100);
    
    // Original should be empty/nullified (capacity 0).
    assert(m1.size() == 0);

    PASS();
}

int main() 
{
    std::cout << "=> Running tests (zmap.h, C++)\n";
    test_cpp_wrappers();
    test_complex_types();
    test_stl_iterators();
    test_move_semantics();
    std::cout << "=> All tests passed successfully.\n";
    return 0;
}

