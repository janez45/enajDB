#include "b_plus_tree.h"
#include <algorithm>
#include <stack>
#include <cassert>
#include <memory>
#include <iostream>
#include <sstream>

BPlusTree::BPlusTree(size_t fanout) : size_{0}, fanout{fanout}, root{nullptr}
{
    if (fanout < 3)
    {
        throw std::invalid_argument("B+ tree fanout must be at least 3");
    }
}

[[noreturn]] void unreachable()
{
    assert(false && "Unreachable");
    std::abort();
}

// stack defaults to nullptr, so no tracing
BPlusTree::LeafNode *BPlusTree::search(Node *cur, int key, std::stack<std::pair<Node *, int>> *stack) const
{
    if (cur == nullptr)
    {
        return nullptr; // this should only be null if the root is empty
    }

    auto it = std::upper_bound(cur->keys.begin(), cur->keys.end(), key);
    int index = it - cur->keys.begin(); // largest pointer would result in .end()
    if (stack != nullptr)
    {
        stack->push({cur, index});
    }

    if (auto *leaf_node = dynamic_cast<LeafNode *>(cur))
    {
        return leaf_node;
    }
    else if (auto *internal_node = dynamic_cast<InternalNode *>(cur))
    {
        // internal
        return BPlusTree::search(internal_node->children[index].get(), key, stack);
    }
    unreachable();
}

// a search is just this with low = high
std::vector<std::pair<int, int>> BPlusTree::range_query_inclusive(int low, int high) const
{
    if (low > high)
    {
        throw std::invalid_argument("range must be valid, low <= high");
    }
    // return empty if B+ tree is empty
    if (empty())
    {
        return {};
    }

    LeafNode *leaf_node = search(root.get(), low);
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
    auto right = std::make_unique<LeafNode>();

    const int mid = leafNode->keys.size() / 2;

    // assign to right
    right->keys.assign(leafNode->keys.begin() + mid, leafNode->keys.end());
    right->values.assign(leafNode->values.begin() + mid, leafNode->values.end());

    // remove from original
    leafNode->keys.erase(leafNode->keys.begin() + mid, leafNode->keys.end());
    leafNode->values.erase(leafNode->values.begin() + mid, leafNode->values.end());

    right->next = leafNode->next;
    leafNode->next = right.get();

    return {.separator = right->keys.front(), .right = std::move(right)};
}

// Note: This can only be hit after a leaf node is hit first. This node must be bloated (children.size() = fanout + 1) first
BPlusTree::SplitResult BPlusTree::split_internal_node(InternalNode *internalNode)
{
    auto right = std::make_unique<InternalNode>();

    const int mid = internalNode->keys.size() / 2;
    const int separator = internalNode->keys[mid];

    // assign to right
    right->keys.assign(internalNode->keys.begin() + mid + 1, internalNode->keys.end()); // mid is not captured
    right->children.assign(std::make_move_iterator(internalNode->children.begin() + mid + 1),
                           std::make_move_iterator(internalNode->children.end()));

    internalNode->keys.erase(internalNode->keys.begin() + mid, internalNode->keys.end());
    internalNode->children.erase(internalNode->children.begin() + mid + 1, internalNode->children.end());

    return {.separator = separator, .right = std::move(right)};
}

// TODO: Keys are unique for now.
// If you try to hit a key that already exists, it throws an error
bool BPlusTree::insert_key(int key, int recordId)
{
    std::stack<std::pair<Node *, int>> stack; // top should be leafnode, rest internal
    BPlusTree::LeafNode *targetLeaf = search(root.get(), key, &stack);

    // will never be empty so long as this is valid
    if (targetLeaf)
    {
        auto it = std::lower_bound(targetLeaf->keys.begin(), targetLeaf->keys.end(), key);
        if (it != targetLeaf->keys.end() && *it == key)
        {
            return false;
        }
    }
    size_++;

    // the root is empty
    if (stack.empty())
    {
        auto newRoot = std::make_unique<LeafNode>();
        newRoot->keys = {key};
        newRoot->values = {recordId};
        root = std::move(newRoot);
        return true;
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
        cur = stack.top().first;
        stack.pop();
        if (auto *leaf_node = dynamic_cast<LeafNode *>(cur))
        {
            int insertionIndex = std::upper_bound(leaf_node->keys.begin(), leaf_node->keys.end(), key) - leaf_node->keys.begin();
            leaf_node->keys.insert(leaf_node->keys.begin() + insertionIndex, key);
            leaf_node->values.insert(leaf_node->values.begin() + insertionIndex, recordId);

            if (leaf_node->values.size() <= fanout)
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
            internal_node->children.insert(internal_node->children.begin() + insertionIndex + 1, std::move(splitResult.right));

            if (internal_node->children.size() <= fanout)
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
        auto newRoot = std::make_unique<InternalNode>();
        newRoot->keys = {splitResult.separator};
        newRoot->children.push_back(std::move(root));
        newRoot->children.push_back(std::move(splitResult.right));
        root = std::move(newRoot);
    }

    return true;
}

// assume that parameters are valid. So idx isn't the parent's final child
void BPlusTree::stealRightChild(InternalNode *parent, int idx)
{
    if (auto *leaf_node = dynamic_cast<LeafNode *>(parent->children[idx].get()))
    {
        // more simple, because values are just values
        LeafNode *right = static_cast<LeafNode *>(parent->children[idx + 1].get());
        leaf_node->keys.push_back(right->keys[0]);
        leaf_node->values.push_back(right->values[0]);
        right->keys.erase(right->keys.begin());
        right->values.erase(right->values.begin());
        parent->keys[idx] = right->keys[0];
        return;
    }
    else if (auto *internal_node = dynamic_cast<InternalNode *>(parent->children[idx].get()))
    {
        // these are all internal nodes
        InternalNode *right = static_cast<InternalNode *>(parent->children[idx + 1].get());
        int parent_sep = parent->keys[idx];
        internal_node->keys.push_back(parent_sep);
        internal_node->children.push_back(std::move(right->children[0]));
        parent->keys[idx] = right->keys[0];
        right->keys.erase(right->keys.begin());
        right->children.erase(right->children.begin());
        return;
    }
    unreachable();
}

// assume that parameters are valid. So idx isn't the parent's first
void BPlusTree::stealLeftChild(InternalNode *parent, int idx)
{
    if (auto *leaf_node = dynamic_cast<LeafNode *>(parent->children[idx].get()))
    {
        LeafNode *left = static_cast<LeafNode *>(parent->children[idx - 1].get());
        leaf_node->keys.insert(leaf_node->keys.begin(), left->keys.back());
        leaf_node->values.insert(leaf_node->values.begin(), left->values.back());
        left->keys.pop_back();
        left->values.pop_back();
        parent->keys[idx - 1] = leaf_node->keys[0];
        return;
    }
    else if (auto *internal_node = dynamic_cast<InternalNode *>(parent->children[idx].get()))
    {
        InternalNode *left = static_cast<InternalNode *>(parent->children[idx - 1].get());
        int parent_sep = parent->keys[idx - 1];
        internal_node->keys.insert(internal_node->keys.begin(), parent_sep);
        internal_node->children.insert(internal_node->children.begin(), std::move(left->children.back()));
        parent->keys[idx - 1] = left->keys.back();
        left->keys.pop_back();
        left->children.pop_back();
        return;
    }
    unreachable();
}

void BPlusTree::mergeWithRight(InternalNode *parent, int idx)
{
    if (auto *leaf_node = dynamic_cast<LeafNode *>(parent->children[idx].get()))
    {
        LeafNode *right = static_cast<LeafNode *>(parent->children[idx + 1].get());
        leaf_node->next = right->next; // remember to merge the leaf nodes
        leaf_node->keys.insert(leaf_node->keys.end(), right->keys.begin(), right->keys.end());
        leaf_node->values.insert(leaf_node->values.end(), right->values.begin(), right->values.end());
        parent->children.erase(parent->children.begin() + idx + 1); // remove the right child
        parent->keys.erase(parent->keys.begin() + idx);

        return;
    }
    else if (auto *internal_node = dynamic_cast<InternalNode *>(parent->children[idx].get()))
    {
        // these are all internal nodes
        InternalNode *right = static_cast<InternalNode *>(parent->children[idx + 1].get());

        int parent_separator = parent->keys[idx];
        internal_node->keys.push_back(parent_separator);
        internal_node->keys.insert(internal_node->keys.end(), right->keys.begin(), right->keys.end());
        for (auto &child : right->children)
        {
            internal_node->children.push_back(std::move(child)); // need to move all the children because they're unique pointers
        }
        parent->keys.erase(parent->keys.begin() + idx);
        parent->children.erase(parent->children.begin() + idx + 1);

        return;
    }
    unreachable();
}

bool BPlusTree::delete_key(int key)
{
    std::stack<std::pair<Node *, int>> stack; // top should be leafnode, rest internal
    BPlusTree::LeafNode *targetLeaf = search(root.get(), key, &stack);

    // tree is empty?
    if (!targetLeaf)
    {
        return false;
    }

    // does the key exist?
    auto it = std::lower_bound(targetLeaf->keys.begin(), targetLeaf->keys.end(), key);
    if (it == targetLeaf->keys.end() || *it != key)
    {
        return false;
    }

    // remove the element
    int index = it - targetLeaf->keys.begin();
    targetLeaf->keys.erase(it);
    targetLeaf->values.erase(targetLeaf->values.begin() + index);

    size_--;

    // case 1: leaf is the root, so you deleted the last one so the root should be nullptr
    if (targetLeaf == root.get())
    {
        if (root->keys.empty() && !size_)
        {
            root = nullptr;
        }
        return true;
    }

    bool complete = fanout / 2 <= targetLeaf->keys.size();
    if (complete)
    {
        return true;
    }

    Node *cur = targetLeaf;
    stack.pop();

    // if stack is not empty, this means cur is not the root
    while (!stack.empty() && !complete)
    {
        // this gives us the index on the parent
        auto [node, idx] = stack.top();
        InternalNode *parent = dynamic_cast<InternalNode *>(node);
        stack.pop();

        Node *left_sibling = idx ? parent->children[idx - 1].get() : nullptr;
        Node *right_sibling = idx < parent->keys.size() ? parent->children[idx + 1].get() : nullptr;

        bool canStealLeft = false;
        bool canStealRight = false;

        if (dynamic_cast<LeafNode *>(cur))
        {
            canStealLeft = left_sibling && (fanout / 2 <= left_sibling->keys.size() - 1);
            canStealRight = right_sibling && (fanout / 2 <= right_sibling->keys.size() - 1);
        }
        else if (dynamic_cast<InternalNode *>(cur))
        {
            canStealLeft = left_sibling && ((fanout + 1) / 2 <= left_sibling->keys.size()); // keys.size() = children.size() - 1
            canStealRight = right_sibling && ((fanout + 1) / 2 <= right_sibling->keys.size());
        }

        if (canStealLeft)
        {
            if (canStealRight && right_sibling->keys.size() > left_sibling->keys.size())
            {
                stealRightChild(parent, idx);
            }
            else
            {
                stealLeftChild(parent, idx);
            }
            return true;
        }
        else if (canStealRight)
        {
            stealRightChild(parent, idx);
            return true;
        }

        // If we reached here, that means we cannot steal from a sibling. All three are at min cap
        // Merge with the right if possible. If not, left. At least one must exist if this is not the root
        if (right_sibling)
        {
            mergeWithRight(parent, idx);
        }
        else
        {
            mergeWithRight(parent, idx - 1);
        }

        // check if the parent is valid TODO what if it's the root?
        if ((fanout + 1) / 2 <= parent->children.size())
        {
            return true;
        }

        cur = parent;
    }

    // cur is now the root, which is more lax
    if (cur->keys.empty())
    {
        root = std::move(static_cast<InternalNode *>(cur)->children[0]);
    }

    return true;
}

// ----------------
// Debugging tools
// ----------------
void BPlusTree::LeafNode::output(std::ostream &os) const
{
    os << "LeafNode: [";
    for (int i = 0; i < this->keys.size(); i++)
    {
        if (i)
        {
            os << ", ";
        }
        os << "(" << this->keys[i] << ", " << this->values[i] << ")";
    }
    os << "]";
}

void BPlusTree::InternalNode::output(std::ostream &os) const
{
    os << "InternalNode: [";
    for (int i = 0; i < this->keys.size(); i++)
    {
        if (i)
        {
            os << ", ";
        }
        os << this->keys[i];
    }
    os << "]";
}

size_t BPlusTree::size() const
{
    return size_;
}

void BPlusTree::dumpHelper(Node *node, int indent, std::ostream &os) const
{
    os << std::string(indent * 2, ' ');
    node->output(os);
    os << '\n';

    if (auto *internal_node = dynamic_cast<InternalNode *>(node))
    {
        os << std::string(indent * 2, ' ') << "{\n";

        for (const auto &child : internal_node->children)
        {
            dumpHelper(child.get(), indent + 1, os);
        }

        os << std::string(indent * 2, ' ') << "}\n";
    }
}

void BPlusTree::dump(std::ostream &os) const
{
    if (!root.get())
    {
        os << "{}" << std::endl;
        return;
    }
    dumpHelper(root.get(), 0, os);
}

// assume node is not nullptr
std::pair<int, int> BPlusTree::internalNodeValidator(Node *node, bool isRoot) const
{
    assert(node != nullptr && "Null pointer node in tree");
    assert(std::is_sorted(node->keys.begin(), node->keys.end()) && "Keys not sorted");

    if (auto *leaf_node = dynamic_cast<LeafNode *>(node))
    {
        assert(leaf_node->keys.size() == leaf_node->values.size());
        if (isRoot)
        {
            assert(1 <= leaf_node->keys.size() && "Root leaf node has no keys");
        }
        else
        {
            assert(fanout / 2 <= leaf_node->keys.size());
        }
        assert(leaf_node->keys.size() <= fanout);

        return std::make_pair(leaf_node->keys.front(), leaf_node->keys.back());
    }
    else if (auto *internal_node = dynamic_cast<InternalNode *>(node))
    {
        assert(internal_node->keys.size() + 1 == internal_node->children.size());
        // the min differs here
        if (isRoot)
        {
            assert(2 <= internal_node->children.size() && "Root internal node has less than two pointers");
        }
        else
        {
            assert((fanout + 1) / 2 <= internal_node->children.size() && "Non-root internal node has less than ceil(fanout/2) pointers");
        }
        assert(internal_node->children.size() <= fanout && "Internal node has more pointers than allowed by fanout");

        // check all the children
        int min, max;
        for (int i = 0; i < internal_node->children.size(); i++)
        {
            auto [lo, hi] = internalNodeValidator(internal_node->children[i].get(), false);

            if (i == 0)
            {
                min = lo;
            }

            if (i == internal_node->children.size() - 1)
            {
                max = hi;
            }

            if (i)
            {
                assert(internal_node->keys[i - 1] <= lo);
            }

            if (i < internal_node->children.size() - 1)
            {
                assert(internal_node->keys[i] > hi);
            }
        }

        return std::make_pair(min, max);
    }
    unreachable();
}

void BPlusTree::validate() const
{
    if (root == nullptr)
    {
        if (size_)
        {
            assert(false && "Nonezero size for null root");
        }
        return;
    }

    internalNodeValidator(root.get(), true); // validate the tree structure

    // get the leftmost tree node
    Node *node = root.get();

    while (auto *internal_node = dynamic_cast<InternalNode *>(node))
    {
        node = internal_node->children[0].get();
    }

    auto *leafNode = dynamic_cast<LeafNode *>(node);
    assert(leafNode != nullptr);

    int trackedSize = leafNode->keys.size();

    LeafNode *prev = leafNode;
    LeafNode *cur = leafNode->next;

    while (cur)
    {
        trackedSize += cur->keys.size();
        assert(prev->keys.back() < cur->keys.front());
        prev = cur;
        cur = cur->next;
    }

    assert(size_ == trackedSize);
}
