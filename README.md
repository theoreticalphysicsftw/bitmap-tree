# bitmap-tree

This is an efficient data structure that can be used to manage index slots for bindless GPU resources.
It can also be used for any other purpose which requires tracking resources.

## Introduction

It works similarily to an ordinary bitmap with the difference that finding the first available slot is very fast.
The tree can be sparse so very big indices are possible (if indices need to be assigned manually).

Internally it's implemented as an 32/64-ary tree of 32/64 bit integers storing the bits.
Leaf nodes are flattened to contain 4096 available bit positions.

For managing 16 777 216 resources the tree has only 3 levels and requires calculating 3 ctz/BitScanForward intrinsics.

## Building

Simply include `bitmap_tree.hpp` and that's it!

## Usage

    // Create new tree. It can dynamically expand as necessary
    bmt::tree_t<uint64_t> tree;

    // This will reserve the first available index
    auto idx = tree.allocate();

    // We can test if the index is reserved
    assert(tree.is_allocated(idx));
    
    // We can deallocate
    tree.deallocate(idx);

    // We can allocate a specific index. Since the tree is sparse
    // this won't be too problematic memory wise like normal bitmap
    // even if the index is very big. However the level of the tree
    // might increase.
    tree.allocate_at(10000000000ull);

    // We can get the total slots allocated
    assert(tree.allocated_slots() == 1);

    // The current maximum capacity can be aquired with this.
    // The tree will automatically expand when the total
    // allocated slots reach the capacity
    assert(tree.current_capacity() == 4096);
