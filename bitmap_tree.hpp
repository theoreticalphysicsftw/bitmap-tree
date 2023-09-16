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

#if defined(_MSC_VER)
    #include <intrin.h>
    #pragma intrinsic(_BitScanForward)
    #pragma intrinsic(_BitScanForward64)
#endif


namespace bmt
{
    using u32_t = uint32_t;
    using u64_t = uint64_t;
    using bool_t = bool;
    using void_t = void;


    template <typename T>
    concept sub32bit = sizeof(T) < sizeof(u32_t);


    // These ctz functions should not be used with n = 0!
    template <typename T>
    inline auto ctz(T n) -> u32_t = delete;

    template <>
    inline auto ctz<u64_t>(u64_t) -> u32_t;

    template <>
    inline auto ctz<u32_t>(u32_t) -> u32_t;

    template <typename T>
        requires std::unsigned_integral<T> && sub32bit<T>
    inline auto ctz(T n) -> u32_t
    {
        return ctz<u32_t>(u32_t(n));
    }


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
        requires::std::unsigned_integral<T>
    class node_allocator_t
    {
        public:
        ~node_allocator_t();
        node_allocator_t();
        auto allocate() -> node_t<T>*;

        private:
        std::vector<node_t<T>*> chunks;
        static constexpr u64_t nodes_in_chunk = node_t<T>::branching_factor;
        u32_t last_chunk_offset;
    };
    

    template <typename T>
        requires std::unsigned_integral<T>
    class tree_t
    {
        public:
        tree_t();
        
        auto allocate_at(u64_t idx) -> void_t;
        auto is_allocated(u64_t idx) const -> bool_t;

        auto allocate() -> T;
        auto deallocate(T idx) -> void_t;

        auto allocated_slots() -> T;
        auto current_capacity() -> T;

        static constexpr u32_t bits_per_word = node_t<T>::bits_per_word;
        static constexpr u32_t branching_factor = node_t<T>::branching_factor;

        private:

        auto move_max_size(T idx) -> void_t;

        node_t<T>* root;
        u64_t levels;
        u64_t allocated_resources;
        u64_t current_max_size;

        node_allocator_t<T> allocator;
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


    template <typename T>
        requires::std::unsigned_integral<T>
    node_allocator_t<T>::node_allocator_t()
    {
        chunks.push_back((node_t<T>*)malloc(sizeof(node_t<T>) * nodes_in_chunk));
        last_chunk_offset = 0;
    }

    
    template <typename T>
        requires::std::unsigned_integral<T>
    node_allocator_t<T>::~node_allocator_t()
    {
        for (auto chunk_ptr : chunks)
        {
            free(chunk_ptr);
        }
    }


    template <typename T>
        requires::std::unsigned_integral<T>
    auto node_allocator_t<T>::allocate() -> node_t<T>*
    {
        if (last_chunk_offset >= nodes_in_chunk)
        {
            chunks.push_back((node_t<T>*)malloc(sizeof(node_t<T>) * nodes_in_chunk));
            last_chunk_offset = 0;
        }

        return chunks.back() + (last_chunk_offset++);
    }


    template<typename T>
        requires std::unsigned_integral<T>
    tree_t<T>::tree_t()
    {
        root = allocator.allocate();
        root->init_leaf();
        levels = 0;
        allocated_resources = 0;
        current_max_size = branching_factor * bits_per_word;
    }


    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::move_max_size(T idx) -> void_t
    {
        while (current_max_size <= idx)
        {
            auto new_root = allocator.allocate();
            new_root->is_leaf = false;
            new_root->allocated_branches = T(1);
            new_root->unset_branches = 
                (root->unset_branches == 0)? ~T(1) : ~T(0);
            
            new_root->branches[0] = root;
            root = new_root;

            current_max_size *= branching_factor;
            levels++;
        }
    }


    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::allocated_slots() -> T
    {
        return allocated_resources;
    }


    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::current_capacity() -> T
    {
        return current_max_size;
    }


    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::allocate_at(u64_t idx) -> void_t
    {
        if (idx >= current_max_size)
        {
            move_max_size(idx);
        }

        std::array<std::pair<node_t<T>*, T>, branching_factor> prev_nodes;
        u32_t prev_nodes_count = 0;

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

                if (!current_node->bits[bucket])
                {
                    clear_bit(current_node->unset_branches, bucket);
                }

                while (!current_node->unset_branches && prev_nodes_count)
                {
                    prev_nodes_count--;
                    current_node = prev_nodes[prev_nodes_count].first;
                    bucket = prev_nodes[prev_nodes_count].second;
                    clear_bit(current_node->unset_branches, bucket);
                }

                break;
            }
            
            if (!test_bit(current_node->allocated_branches, bucket))
            {
                auto& new_node = current_node->branches[bucket];
                new_node = allocator.allocate();
                set_bit(current_node->allocated_branches, bucket);

                if (current_level == 1)
                {
                    new_node->init_leaf();
                }
                else
                {
                    new_node->allocated_branches = 0;
                    new_node->unset_branches = ~T(0);
                    new_node->is_leaf = false;
                }
            }

            prev_nodes[prev_nodes_count++] = std::make_pair(current_node, bucket);
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
        if (!root->unset_branches)
        {
            move_max_size(current_max_size);
        }

        std::array<std::pair<node_t<T>*, T>, branching_factor> prev_nodes;
        auto prev_nodes_count = 0;

        auto current_node = root;
        auto current_level = levels;
        auto subtree_size = current_max_size;
        T final_index = 0;
        while (!current_node->is_leaf)
        {
            auto next_branch = ctz(current_node->unset_branches);
            subtree_size /= branching_factor;
            final_index += next_branch * subtree_size;

            if (!test_bit(current_node->allocated_branches, next_branch))
            {
                auto& new_node = current_node->branches[next_branch];
                new_node = allocator.allocate();
                set_bit(current_node->allocated_branches, next_branch);

                if (current_level == 1)
                {
                    new_node->init_leaf();
                }
                else
                {
                    new_node->allocated_branches = 0;
                    new_node->unset_branches = ~T(0);
                    new_node->is_leaf = false;
                }
            }

            prev_nodes[prev_nodes_count++] = std::make_pair(current_node, next_branch);
            current_node = current_node->branches[next_branch];
            current_level--;
        }

        auto bucket = ctz(current_node->unset_branches);
        auto idx = ctz(current_node->bits[bucket]);
        final_index += bucket * branching_factor + idx;
        
        if (test_bit(current_node->bits[bucket], idx))
        {
            allocated_resources++;
        }

        clear_bit(current_node->bits[bucket], idx);

        if (!current_node->bits[bucket])
        {
            clear_bit(current_node->unset_branches, bucket);
        }

        while (!current_node->unset_branches && prev_nodes_count)
        {
            prev_nodes_count--;
            current_node = prev_nodes[prev_nodes_count].first;
            bucket = prev_nodes[prev_nodes_count].second;
            clear_bit(current_node->unset_branches, bucket);
        }

        return final_index;
    }


    template<typename T>
        requires std::unsigned_integral<T>
    auto tree_t<T>::deallocate(T idx) -> void_t
    {
        if (idx >= current_max_size)
        {
            return;
        }

        std::array<std::pair<node_t<T>*, T>, branching_factor> prev_nodes;
        auto prev_nodes_count = 0;

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
                if (!test_bit(current_node->bits[bucket], idx))
                {
                    allocated_resources--;
                }

                set_bit(current_node->bits[bucket], idx);

                if (current_node->bits[bucket] == ~T(0))
                {
                    set_bit(current_node->unset_branches, bucket);
                }

                while (current_node->unset_branches == ~T(0) && prev_nodes_count)
                {
                    prev_nodes_count--;
                    current_node = prev_nodes[prev_nodes_count].first;
                    bucket = prev_nodes[prev_nodes_count].second;
                    set_bit(current_node->unset_branches, bucket);
                }

                return;
            }
            
            if (!test_bit(current_node->allocated_branches, bucket))
            {
                return;
            }

            prev_nodes[prev_nodes_count++] = std::make_pair(current_node, bucket);
            current_node = current_node->branches[bucket];
            current_level--;
        }
    }

#if defined(_MSC_VER)
    template <>
    inline auto ctz<u64_t>(u64_t n) -> u32_t
    {
        u64_t result;
        _BitscanForward64(&result, n);
        return result;
    }

    template <>
    inline auto ctz<u32_t>(u32_t) -> u32_t
    {
        u32_t result;
        _BitscanForward(&result, n);
        return result;
    }
#elif defined(__GNUC__)
    template <>
    inline auto ctz<u64_t>(u64_t n) -> u32_t
    {
        return __builtin_ctzl(n);
    }

    template <>
    inline auto ctz<u32_t>(u32_t n) -> u32_t
    {
        return __builtin_ctz(n);
    }
#else
    template <>
    inline auto ctz<u64_t>(u64_t n) -> u32_t
    {
        // This function is not supposed to be used on 0!
        u32_t bits = 0;
        while (!((u64_t(1) << bits) & n)) bits++;
        return bits;
    }

    template <>
    auto ctz<u32_t>(u32_t n) -> u32_t
    {
        // This function is not supposed to be used on 0!
        u32_t bits = 0;
        while (!((u32_t(1) << bits) & n)) bits++;
        return bits;
    }
#endif
}


#endif
