#pragma once
#include <map>
#include <vector>
#include <stack>
#include <memory>
#include <iostream>

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
        virtual void output(std::ostream &os) const = 0;
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
        void output(std::ostream &os) const override;
    };

    struct InternalNode : Node
    {
        std::vector<std::unique_ptr<Node>> children; // INVARIANT: size(children) = size(keys) + 1
        bool is_leaf() const override
        {
            return false;
        }
        void output(std::ostream &os) const override;
    };

    struct SplitResult
    {
        int separator;
        std::unique_ptr<Node> right;
    };

    struct ValidationResult
    {
        bool success;
        std::string reason;
    };

    size_t size_;
    size_t fanout;
    std::unique_ptr<Node> root;
    bool empty() const;
    LeafNode *search(Node *cur, int key, std::stack<std::pair<Node *, int>> *stack = nullptr) const;
    void insert_key_(std::stack<Node *> &stack, int key, int value);

    // Precondition: These are used when at capacity + 1. The pointers are already configured
    SplitResult split_leaf_node(LeafNode *leafNode);
    SplitResult split_internal_node(InternalNode *internalNode);
    std::pair<int, int> internalNodeValidator(Node *node, bool isRoot) const;
    void dumpHelper(Node *node, int indent, std::ostream &os) const;
    void stealRightChild(InternalNode *parent, int idx);
    void stealLeftChild(InternalNode *parent, int idx);
    void mergeWithRight(InternalNode *parent, int idx);

public:
    explicit BPlusTree(size_t fanout);

    std::vector<std::pair<int, int>> range_query_inclusive(int low, int high) const;
    bool insert_key(int key, int recordId); // disallows duplicates, false if it exists
    bool delete_key(int key);               // return false if key does not exist, true if deletion was successful
    size_t size() const;

    void validate() const;
    void dump(std::ostream &os = std::cout) const;
};