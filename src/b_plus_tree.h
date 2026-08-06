#pragma once
#include <map>
#include <vector>
#include <stack>

// This is a basic B+ tree, I just want it to get the correct values and understand node splitting
// Using basic int/int
// this implementation requires unique keys

// this stores the root
class BPlusTree
{
private:
    struct Node
    {
        virtual ~Node() = default;
        virtual bool is_leaf() const = 0;
        std::vector<int> keys;
    };

    // this does not need to be deleted because they'll never alone.
    // if it is the root, it'll just immediately be deleted and have no next
    struct LeafNode : Node
    {
        std::vector<int> values; // INVARIANT: size(children) = size(keys)
        LeafNode *next = nullptr;

        bool is_leaf() const override
        {
            return true;
        }
    };

    struct InternalNode : Node
    {
        std::vector<Node *> children; // INVARIANT: size(children) = size(keys) + 1
        bool is_leaf() const override
        {
            return false;
        }
        ~InternalNode()
        {
            for (Node *child : children)
            {
                delete child;
            }
        }
    };

    struct SplitResult
    {
        int separator;
        Node *right;
    };

    size_t fanout;
    Node *root;
    bool empty() const;
    LeafNode *search(Node *cur, int key, std::stack<Node *> *stack = nullptr) const;
    void insert_key_(std::stack<Node *> &stack, int key, int value);

    // Precondition: These are used when at capacity + 1. The pointers are already configured
    SplitResult split_leaf_node(LeafNode *leafNode);
    SplitResult split_internal_node(InternalNode *internalNode);

public:
    explicit BPlusTree(size_t fanout);
    ~BPlusTree();

    std::vector<std::pair<int, int>> range_query_inclusive(int low, int high) const;
    void insert_key(int key, int value); // updates if exists, creates if not
    void delete_key(int key);
};