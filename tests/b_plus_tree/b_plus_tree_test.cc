#include <gtest/gtest.h>
#include "b_plus_tree.h"
#include <iostream>

TEST(BPlusTreeTest, DestructorFreesTree)
{
    {
        BPlusTree tree{3};

        for (int i = 0; i < 100; ++i)
        {
            tree.insert_key(i, i);
        }
    }
}