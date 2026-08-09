#include <gtest/gtest.h>
#include "b_plus_tree.h"
#include <iostream>

TEST(BPlusTreeTest, DisallowFanoutLessThan3)
{
    EXPECT_NO_THROW(BPlusTree{3});
    EXPECT_THROW(BPlusTree{2}, std::invalid_argument);
}

TEST(BPlusTreeTestInsert, SingleInsertionComplete)
{
    BPlusTree tree{3};
    tree.insert_key(1, 1);
    tree.validate();
}

TEST(BPlusTreeTestInsert, TestSplitRootLeafNode)
{
    BPlusTree tree{3};
    tree.insert_key(1, 15);
    tree.validate();
    tree.insert_key(2, 3); // scrambling the values
    tree.validate();
    tree.insert_key(3, 2);
    tree.validate();
}

TEST(BPlusTreeTestInsert, TestSplitInternalLeafNode)
{
    BPlusTree tree{4};
    tree.insert_key(1, 5);
    tree.validate();
    tree.insert_key(4, 3);
    tree.validate();
    tree.insert_key(2, 9);
    tree.validate();
}

TEST(BPlusTreeTestInsert, TestSplitInternalNode)
{
}

TEST(BPlusTreeTestInsert, TestSplitUpToRoot)
{
}

TEST(BPlusTreeTestInsert, DuplicateKeyShouldFail)
{
    BPlusTree tree{5};
    EXPECT_NO_THROW(tree.insert_key(1, 1));
    EXPECT_THROW(tree.insert_key(1, 2), std::invalid_argument);
}

TEST(BPlusTreeTestInsert, MultipleInsertion)
{
    BPlusTree tree{5};

    std::vector<std::pair<int, int>> data = {
        {73, 418},
        {12, 905},
        {847, 31},
        {294, 762},
        {501, 184},
        {66, 973},
        {728, 245},
        {359, 817},
        {914, 53},
        {187, 634},
        {442, 291},
        {805, 726},
        {38, 559},
        {671, 103},
        {256, 888},
        {923, 417},
        {119, 682},
        {534, 76},
        {781, 345},
        {407, 951},
        {62, 214},
        {895, 507},
        {328, 839},
        {756, 165},
        {203, 694},
        {569, 27},
        {981, 432},
        {146, 773},
        {617, 358},
        {375, 926},
    };

    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
        tree.validate();
    }

    tree.dump(std::cerr);
}

TEST(BPlusTreeTestSearch, SingleSearch)
{
}

TEST(BPlusTreeTestSearch, RangeSearch)
{
}

TEST(BPlusTreeTestSearch, RangeSearchAll)
{
}

TEST(BPlusTreeTestSearch, RangeSearchExceedBounds)
{
}