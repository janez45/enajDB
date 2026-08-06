#include "b_plus_tree.h"
#include <algorithm>
#include <stack>
#include <cassert>

BPlusTree::BPlusTree(size_t fanout) : fanout{fanout}, root{nullptr}
{
    if (fanout < 3)
    {
        throw std::invalid_argument("B+ tree fanout must be at least 2");
    }
}

BPlusTree::~BPlusTree()
{
    delete root;
}

[[noreturn]] void unreachable()
{
    assert(false && "Unreachable");
    std::abort();
}

// stack defaults to nullptr, so no tracing
BPlusTree::LeafNode *BPlusTree::search(Node *cur, int key, std::stack<Node *> *stack) const
{
    if (cur == nullptr)
    {
        return nullptr; // this should only be null if the root is empty
    }

    if (stack != nullptr)
    {
        stack->push(cur);
    }

    if (auto *leaf_node = dynamic_cast<LeafNode *>(cur))
    {
        return leaf_node;
    }
    else if (auto *internal_node = dynamic_cast<InternalNode *>(cur))
    {
        // internal
        auto it = std::upper_bound(internal_node->keys.begin(), internal_node->keys.end(), key);
        int index = it - internal_node->keys.begin(); // largest pointer would result in .end()
        return BPlusTree::search(internal_node->children[index], key, stack);
    }
    unreachable();
}

// a search is just this with low = high
std::vector<std::pair<int, int>> BPlusTree::range_query_inclusive(int low, int high) const
{
    // return empty if B+ tree is empty
    if (empty())
    {
        return {};
    }

    LeafNode *leaf_node = search(root, low);
    int index = std::lower_bound(leaf_node->keys.begin(), leaf_node->keys.end(), low) - leaf_node->keys.begin();
    std::vector<std::pair<int, int>> ans;

    while (leaf_node != nullptr && index < leaf_node->values.size() && leaf_node->keys[index] <= high)
    {
        ans.push_back({leaf_node->keys[index], leaf_node->values[index]});
        index++;

        if (index == leaf_node->values.size())
        {
            leaf_node = leaf_node->next;
            index = 0;
        }
    }
    return ans;
}

bool BPlusTree::empty() const
{
    return root == nullptr;
}

// Precondition: This is already full and the key + pointer is already inserted, so this is a bloated node
BPlusTree::SplitResult BPlusTree::split_leaf_node(LeafNode *leafNode)
{
    LeafNode *right = new LeafNode;

    const int mid = leafNode->keys.size() / 2;

    // assign to right
    right->keys.assign(leafNode->keys.begin() + mid, leafNode->keys.end());
    right->values.assign(leafNode->values.begin() + mid, leafNode->values.end());

    // remove from original
    leafNode->keys.erase(leafNode->keys.begin() + mid, leafNode->keys.end());
    leafNode->values.erase(leafNode->values.begin() + mid, leafNode->values.end());

    right->next = leafNode->next;
    leafNode->next = right;

    return {.separator = right->keys.front(), .right = right};
}

// Note: This can only be hit after a leaf node is hit first. This node must be bloated (children.size() = fanout + 1) first
BPlusTree::SplitResult BPlusTree::split_internal_node(InternalNode *internalNode)
{
    InternalNode *right = new InternalNode;

    const int mid = internalNode->keys.size() / 2;

    // assign to right
    right->keys.assign(internalNode->keys.begin() + mid + 1, internalNode->keys.end()); // mid is not captured
    right->children.assign(internalNode->children.begin() + mid + 1, internalNode->children.end());

    internalNode->keys.erase(internalNode->keys.begin() + mid, internalNode->keys.end());
    internalNode->children.erase(internalNode->children.begin() + mid + 1, internalNode->children.end());

    return {.separator = right->keys.front(), .right = right};
}

void BPlusTree::insert_key(int key, int value)
{
    std::stack<Node *> stack; // top should be leafnode, rest internal
    search(root, key, &stack);

    // the root is empty
    if (stack.empty())
    {
        LeafNode *newRoot = new LeafNode;
        newRoot->keys = {key};
        newRoot->values = {value};
        root = newRoot;
        return;
    }

    /*
        Attempt to insert: If you can just shove it in and terminate there
        If you cannot, recursively split the node and and attempt to shove it into the parent
    */
    bool complete = false;
    Node *cur = nullptr;
    BPlusTree::SplitResult splitResult;

    while (!stack.empty() && !complete)
    {
        cur = stack.top();
        stack.pop();
        if (auto *leaf_node = dynamic_cast<LeafNode *>(cur))
        {
            int insertionIndex = std::upper_bound(leaf_node->keys.begin(), leaf_node->keys.end(), key) - leaf_node->keys.begin();
            leaf_node->keys.insert(leaf_node->keys.begin() + insertionIndex, key);
            leaf_node->values.insert(leaf_node->values.begin() + insertionIndex, value);

            if (leaf_node->keys.size() < fanout)
            {
                complete = true;
            }
            else
            {
                splitResult = split_leaf_node(leaf_node);
            }
        }
        else if (auto *internal_node = dynamic_cast<InternalNode *>(cur))
        {
            // insert into parent
            int insertionIndex = std::upper_bound(internal_node->keys.begin(), internal_node->keys.end(), splitResult.separator) - internal_node->keys.begin();
            internal_node->keys.insert(internal_node->keys.begin() + insertionIndex, splitResult.separator);
            internal_node->children.insert(internal_node->children.begin() + insertionIndex + 1, splitResult.right);

            if (internal_node->keys.size() < fanout)
            {
                complete = true;
            }
            else
            {
                splitResult = split_internal_node(internal_node);
            }
        }
    }

    // it split all the way up to the root
    if (!complete)
    {
        InternalNode *newRoot = new InternalNode;
        newRoot->keys = {splitResult.separator};
        newRoot->children = {cur, splitResult.right};
        root = newRoot;
    }
}

void BPlusTree::delete_key(int key)
{
}