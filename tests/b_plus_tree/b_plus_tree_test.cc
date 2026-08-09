#include <gtest/gtest.h>
#include "b_plus_tree.h"
#include <iostream>
#include <algorithm>

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
    BPlusTree tree{3};
    tree.insert_key(1, 1);
    tree.validate();
    tree.insert_key(2, 2);
    tree.validate();
    tree.insert_key(4, 4);
    tree.validate();
    tree.insert_key(5, 5);
    tree.validate();
    tree.insert_key(3, 3);
    tree.validate();
    tree.insert_key(0, 0);
    tree.validate();
}

TEST(BPlusTreeTestInsert, TestSplitEdgeInternalLeafNode)
{
    BPlusTree tree{3};
    for (int i = 0; i < 10; i++)
    {
        tree.insert_key(i, i);
        tree.validate();
    }
}

TEST(BPlusTreeTestInsert, TestSplitInternalNode)
{
    BPlusTree tree{3};
    for (int i = 0; i < 10; i++)
    {
        tree.insert_key(i, i);
        tree.validate();
    }
}

TEST(BPlusTreeTestInsert, TestSplitUpToRoot)
{
    BPlusTree tree{3};
    for (int i = 0; i < 10; i++)
    {
        tree.insert_key(i, i);
        tree.validate();
    }
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
    BPlusTree tree{10};
    // permutation of 50
    std::vector<std::pair<int, int>> data = {
        {37, 812}, {4, 291}, {46, 573}, {12, 104}, {29, 936}, {1, 447}, {23, 685}, {41, 219}, {8, 771}, {34, 358}, {17, 624}, {50, 183}, {6, 905}, {27, 412}, {14, 759}, {39, 526}, {21, 873}, {3, 341}, {45, 617}, {10, 298}, {32, 954}, {19, 146}, {48, 733}, {25, 509}, {7, 861}, {43, 275}, {16, 694}, {30, 381}, {2, 927}, {38, 453}, {11, 716}, {49, 128}, {24, 842}, {36, 567}, {5, 319}, {28, 780}, {44, 205}, {13, 631}, {20, 496}, {47, 915}, {9, 367}, {31, 548}, {22, 803}, {40, 174}, {15, 722}, {35, 439}, {18, 990}, {42, 256}, {26, 671}, {33, 384}};
    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
    }

    std::vector<std::pair<int, int>> result = tree.range_query_inclusive(36, 36);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], std::make_pair(36, 567));
}

TEST(BPlusTreeTestSearch, SingleSearchDNE)
{
    BPlusTree tree{10};
    // permutation of 50
    std::vector<std::pair<int, int>> data = {
        {37, 812}, {4, 291}, {46, 573}, {12, 104}, {29, 936}, {1, 447}, {23, 685}, {41, 219}, {8, 771}, {34, 358}, {17, 624}, {50, 183}, {6, 905}, {27, 412}, {14, 759}, {39, 526}, {21, 873}, {3, 341}, {45, 617}, {10, 298}, {32, 954}, {19, 146}, {48, 733}, {25, 509}, {7, 861}, {43, 275}, {16, 694}, {30, 381}, {2, 927}, {38, 453}, {11, 716}, {49, 128}, {24, 842}, {36, 567}, {5, 319}, {28, 780}, {44, 205}, {13, 631}, {20, 496}, {47, 915}, {9, 367}, {31, 548}, {22, 803}, {40, 174}, {15, 722}, {35, 439}, {18, 990}, {42, 256}, {26, 671}, {33, 384}};
    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
    }

    std::vector<std::pair<int, int>> result = tree.range_query_inclusive(51, 51);
    EXPECT_EQ(result.size(), 0);
}

TEST(BPlusTreeTestSearch, RangeSearchBadBounds)
{
    BPlusTree tree{10};
    tree.insert_key(1, 1);
    EXPECT_THROW(tree.range_query_inclusive(1, 0), std::invalid_argument);
}

TEST(BPlusTreeTestSearch, RangeSearch)
{
    BPlusTree tree{10};
    // permutation of 50
    std::vector<std::pair<int, int>> data = {
        {37, 812}, {4, 291}, {46, 573}, {12, 104}, {29, 936}, {1, 447}, {23, 685}, {41, 219}, {8, 771}, {34, 358}, {17, 624}, {50, 183}, {6, 905}, {27, 412}, {14, 759}, {39, 526}, {21, 873}, {3, 341}, {45, 617}, {10, 298}, {32, 954}, {19, 146}, {48, 733}, {25, 509}, {7, 861}, {43, 275}, {16, 694}, {30, 381}, {2, 927}, {38, 453}, {11, 716}, {49, 128}, {24, 842}, {36, 567}, {5, 319}, {28, 780}, {44, 205}, {13, 631}, {20, 496}, {47, 915}, {9, 367}, {31, 548}, {22, 803}, {40, 174}, {15, 722}, {35, 439}, {18, 990}, {42, 256}, {26, 671}, {33, 384}};

    // sort the data;
    std::vector<std::pair<int, int>> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
    }

    std::vector<std::pair<int, int>> result = tree.range_query_inclusive(22, 30);
    std::vector<std::pair<int, int>> expected(sorted_data.begin() + 21, sorted_data.begin() + 30);

    EXPECT_EQ(result, expected);
}

TEST(BPlusTreeTestSearch, RangeSearchEmpty)
{
    BPlusTree tree{10};
    // permutation of 50
    std::vector<std::pair<int, int>> data = {
        {37, 812}, {4, 291}, {46, 573}, {12, 104}, {29, 936}, {1, 447}, {23, 685}, {41, 219}, {8, 771}, {34, 358}, {17, 624}, {50, 183}, {6, 905}, {27, 412}, {14, 759}, {39, 526}, {21, 873}, {3, 341}, {45, 617}, {10, 298}, {32, 954}, {19, 146}, {48, 733}, {25, 509}, {7, 861}, {43, 275}, {16, 694}, {30, 381}, {2, 927}, {38, 453}, {11, 716}, {49, 128}, {24, 842}, {36, 567}, {5, 319}, {28, 780}, {44, 205}, {13, 631}, {20, 496}, {47, 915}, {9, 367}, {31, 548}, {22, 803}, {40, 174}, {15, 722}, {35, 439}, {18, 990}, {42, 256}, {26, 671}, {33, 384}};

    // sort the data;
    std::vector<std::pair<int, int>> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
    }

    std::vector<std::pair<int, int>> result = tree.range_query_inclusive(57, 100);
    EXPECT_EQ(result.size(), 0);
}

TEST(BPlusTreeTestSearch, RangeSearchAll)
{
    BPlusTree tree{10};
    // permutation of 50
    std::vector<std::pair<int, int>> data = {
        {37, 812}, {4, 291}, {46, 573}, {12, 104}, {29, 936}, {1, 447}, {23, 685}, {41, 219}, {8, 771}, {34, 358}, {17, 624}, {50, 183}, {6, 905}, {27, 412}, {14, 759}, {39, 526}, {21, 873}, {3, 341}, {45, 617}, {10, 298}, {32, 954}, {19, 146}, {48, 733}, {25, 509}, {7, 861}, {43, 275}, {16, 694}, {30, 381}, {2, 927}, {38, 453}, {11, 716}, {49, 128}, {24, 842}, {36, 567}, {5, 319}, {28, 780}, {44, 205}, {13, 631}, {20, 496}, {47, 915}, {9, 367}, {31, 548}, {22, 803}, {40, 174}, {15, 722}, {35, 439}, {18, 990}, {42, 256}, {26, 671}, {33, 384}};

    // sort the data;
    std::vector<std::pair<int, int>> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
    }
    std::vector<std::pair<int, int>> result = tree.range_query_inclusive(1, 50);
    EXPECT_EQ(result, sorted_data);
}

TEST(BPlusTreeTestSearch, RangeSearchExceedBounds)
{
    BPlusTree tree{10};
    // permutation of 50
    std::vector<std::pair<int, int>> data = {
        {37, 812}, {4, 291}, {46, 573}, {12, 104}, {29, 936}, {1, 447}, {23, 685}, {41, 219}, {8, 771}, {34, 358}, {17, 624}, {50, 183}, {6, 905}, {27, 412}, {14, 759}, {39, 526}, {21, 873}, {3, 341}, {45, 617}, {10, 298}, {32, 954}, {19, 146}, {48, 733}, {25, 509}, {7, 861}, {43, 275}, {16, 694}, {30, 381}, {2, 927}, {38, 453}, {11, 716}, {49, 128}, {24, 842}, {36, 567}, {5, 319}, {28, 780}, {44, 205}, {13, 631}, {20, 496}, {47, 915}, {9, 367}, {31, 548}, {22, 803}, {40, 174}, {15, 722}, {35, 439}, {18, 990}, {42, 256}, {26, 671}, {33, 384}};

    // sort the data;
    std::vector<std::pair<int, int>> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
    }
    std::vector<std::pair<int, int>> result = tree.range_query_inclusive(0, 51);
    EXPECT_EQ(result, sorted_data);
}

TEST(BPlusTreeTestSearch, RangeSearchStartsInsideGoesOut)
{
    BPlusTree tree{10};
    // permutation of 50
    std::vector<std::pair<int, int>> data = {
        {37, 812}, {4, 291}, {46, 573}, {12, 104}, {29, 936}, {1, 447}, {23, 685}, {41, 219}, {8, 771}, {34, 358}, {17, 624}, {50, 183}, {6, 905}, {27, 412}, {14, 759}, {39, 526}, {21, 873}, {3, 341}, {45, 617}, {10, 298}, {32, 954}, {19, 146}, {48, 733}, {25, 509}, {7, 861}, {43, 275}, {16, 694}, {30, 381}, {2, 927}, {38, 453}, {11, 716}, {49, 128}, {24, 842}, {36, 567}, {5, 319}, {28, 780}, {44, 205}, {13, 631}, {20, 496}, {47, 915}, {9, 367}, {31, 548}, {22, 803}, {40, 174}, {15, 722}, {35, 439}, {18, 990}, {42, 256}, {26, 671}, {33, 384}};

    // sort the data;
    std::vector<std::pair<int, int>> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
    }
    std::vector<std::pair<int, int>> result = tree.range_query_inclusive(27, 100);
    std::vector<std::pair<int, int>> expected(sorted_data.begin() + 26, sorted_data.end());
    EXPECT_EQ(result, expected);
}

TEST(BPlusTreeTestSearch, RangeSearchStartsOutGoesIn)
{
    BPlusTree tree{10};
    // permutation of 50
    std::vector<std::pair<int, int>> data = {
        {37, 812}, {4, 291}, {46, 573}, {12, 104}, {29, 936}, {1, 447}, {23, 685}, {41, 219}, {8, 771}, {34, 358}, {17, 624}, {50, 183}, {6, 905}, {27, 412}, {14, 759}, {39, 526}, {21, 873}, {3, 341}, {45, 617}, {10, 298}, {32, 954}, {19, 146}, {48, 733}, {25, 509}, {7, 861}, {43, 275}, {16, 694}, {30, 381}, {2, 927}, {38, 453}, {11, 716}, {49, 128}, {24, 842}, {36, 567}, {5, 319}, {28, 780}, {44, 205}, {13, 631}, {20, 496}, {47, 915}, {9, 367}, {31, 548}, {22, 803}, {40, 174}, {15, 722}, {35, 439}, {18, 990}, {42, 256}, {26, 671}, {33, 384}};

    // sort the data;
    std::vector<std::pair<int, int>> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    for (auto [key, val] : data)
    {
        tree.insert_key(key, val);
    }
    std::vector<std::pair<int, int>> result = tree.range_query_inclusive(-20, 45);
    std::vector<std::pair<int, int>> expected(sorted_data.begin(), sorted_data.begin() + 45);
    EXPECT_EQ(result, expected);
}