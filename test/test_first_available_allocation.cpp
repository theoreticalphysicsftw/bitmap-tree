// MIT License
// 
// Copyright (c) 2023 Mihail Mladenov
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <cstdint>

#include "../bitmap_tree.hpp"


int main()
{
    static constexpr uint64_t total_allocated_resources = uint64_t(1) << 28;
    static constexpr uint64_t max_index = total_allocated_resources - 1;
    uint64_t total_ops = total_allocated_resources * 4;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint64_t> dist(0, max_index);

    auto start = std::chrono::high_resolution_clock::now();
    bmt::tree_t<uint64_t> tree;
    for (uint64_t i = 0; i < total_allocated_resources; ++i)
    {
        auto rn = dist(mt);
        auto idx = tree.allocate();

        if (!tree.is_allocated(idx))
        {
            std::cerr<<"Tree first available allocation is broken."<<std::endl;
        }
    }

    for (uint64_t i = 0; i < total_allocated_resources; ++i)
    {
        auto rn = dist(mt);
        tree.deallocate(rn);

        if (tree.is_allocated(rn))
        {
            std::cerr<<"Tree deallocation is broken."<<std::endl;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto ns_passed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    auto s_passed = ns_passed / 1E9;

    std::cout<<std::fixed<<std::setprecision(4);
    std::cout<<(total_ops / s_passed) / 1E6<<" million ops per second"<<std::endl;
    std::cout<<ns_passed / total_ops<<" nanoseconds per op"<<std::endl;
}
