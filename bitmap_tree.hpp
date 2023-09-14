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


#ifndef BITMAP_TREE_HPP_INCLUDED
#define BITMAP_TREE_HPP_INCLUDED

#include <cstdint>
#include <cstdlib>

#include <concepts>

#include <vector>
#include <array>


namespace bmt
{
    using u32_t = uint32_t;
    using u64_t = uint64_t;
    using bool_t = bool;
    using void_t = void;


    template <typename T>
        requires std::unsigned_integral<T>
    struct node_t
    {
        T unset_branches;
        T allocated_branches;
        bool_t is_leaf;

        static constexpr u64_t bits_per_word = sizeof(T) * 8;
        static constexpr u64_t branching_factor = bits_per_word;

        union
        {
            std::array<T, branching_factor> bits;
            std::array<node_t*, branching_factor> branches;
        };

        auto init_leaf() -> void_t;
    };

    
    template <typename T>
        requires std::unsigned_integral<T>
    class tree_t
    {
        public:
        tree_t();
        ~tree_t();
        
        auto allocate_at(u64_t idx) -> void_t;
        auto is_allocated(u64_t idx) const -> bool_t;

        auto allocate() -> T;
        auto deallocate(T idx) -> void_t;

        static constexpr u32_t bits_per_word = node_t<T>::bits_per_word;
        static constexpr u32_t branching_factor = node_t<T>::branching_factor;

        private:

        auto move_max_size(T idx) -> void_t;

        node_t<T>* root;
        u64_t levels;
        u64_t allocated_resources;
        u64_t current_max_size;
    };


    template<typename T>
        requires std::unsigned_integral<T>
    auto node_t<T>::init_leaf() -> void_t
    {
        is_leaf = true;
        unset_branches = ~T(0); 
        for (auto i = 0; i < branching_factor; ++i)
        {
            bits[i] = ~T(0); 
        }
    }
    

    template <typename T, typename U>
        requires std::unsigned_integral<T> && std::unsigned_integral<U>
    auto set_bit(T& n, U bit) -> void_t
    {
        n |= T(1) << bit;
    }
    

    template <typename T, typename U>
        requires std::unsigned_integral<T> && std::unsigned_integral<U>
    auto clear_bit(T& n, U bit) -> void_t
    {
        n &= ~(T(1) << bit);
    }


    template <typename T, typename U>
        requires std::unsigned_integral<T> && std::unsigned_integral<U>
    auto test_bit(T n, U bit) -> bool_t
    {
        return n & (T(1) << bit);
    }


    template<typename T>
        requires std::unsigned_integral<T>
    tree_t<T>::tree_t()
    {
        root = (node_t<T>*) malloc(sizeof(node_t<T>));
        root->init_leaf();
        levels = 0;
        allocated_resources = 0;
        current_max_size = branching_factor * bits_per_word;
    }


    template<typename T>
        requires std::unsigned_integral<T>
    tree_t<T>::~tree_t()
    {
        std::vector<node_t<T>*> stack;
        stack.push_back(root);

        while (!stack.empty())
        {
            auto node = stack.back();

            if (node->is_leaf)
            {
                free(node);
                stack.pop_back();
            }
            else
            {
                node->is_leaf = true;

                for (auto i = 0u; i < branching_factor; ++i)
                {
                    if (test_bit(node->allocated_branches, i))
                    {
                        stack.push_back(node->branches[i]);
                    }
                }
            }
        }
    }


    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::move_max_size(T idx) -> void_t
    {
        while (current_max_size <= idx)
        {
            auto new_root = (node_t<T>*) malloc(sizeof(node_t<T>));
            new_root->is_leaf = false;
            new_root->allocated_branches = T(1);
            new_root->unset_branches = 
                (root->unset_branches == ~T(0))? ~T(0) : ~T(1);
            
            new_root->branches[0] = root;
            root = new_root;

            current_max_size *= branching_factor;
            levels++;
        }
    }

    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::allocate_at(u64_t idx) -> void_t
    {
        if (idx >= current_max_size)
        {
            move_max_size(idx);
        }

        auto current_node = root;
        auto subtree_size = current_max_size;
        auto current_level = levels;
        for(;;)
        {
            subtree_size /= branching_factor;
            auto bucket = idx / subtree_size;
            idx = idx % subtree_size;

            if (current_node->is_leaf)
            {
                if (test_bit(current_node->bits[bucket], idx))
                {
                    allocated_resources++;
                }
                clear_bit(current_node->bits[bucket], idx);
                break;
            }
            
            if (!test_bit(current_node->allocated_branches, bucket))
            {
                auto& new_node = current_node->branches[bucket];
                new_node = (node_t<T>*) malloc(sizeof(node_t<T>));
                set_bit(current_node->allocated_branches, bucket);

                if (current_level == 0)
                {
                    new_node->init_leaf();
                }
                else
                {
                    new_node->allocated_branches = 0;
                    new_node->is_leaf = false;
                }
            }

            current_node = current_node->branches[bucket];
            current_level--;
        }
    }


    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::is_allocated(u64_t idx) const -> bool_t
    {
        if (idx >= current_max_size)
        {
            return false;
        }

        auto current_node = root;
        auto subtree_size = current_max_size;
        for(;;)
        {
            subtree_size /= branching_factor;
            auto bucket = idx / subtree_size;
            idx = idx % subtree_size;

            if (current_node->is_leaf)
            {
                return !test_bit(current_node->bits[bucket], idx);
            }
            
            if (!test_bit(current_node->allocated_branches, bucket))
            {
                return false;
            }

            current_node = current_node->branches[bucket];
        }
    }

    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::allocate() -> T
    {
        return T();
    }


    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::deallocate(T idx) -> void_t
    {
    }
}


#endif
