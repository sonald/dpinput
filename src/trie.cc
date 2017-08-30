#include "trie.h"

#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <cassert>

using namespace std;

class TrieNode {
    public:
        // Initialize your data structure here.
        TrieNode() { }
        ~TrieNode() {
            for (int i = 0; i < 26; i++) {
                if (children[i]) {
                    delete children[i];
                    children[i] = nullptr;
                }
            }
        }

        TrieNode* children[26] {nullptr,};
        bool end {false}; 
};

#define ORD(ch) ((ch) - 'a')
Trie::Trie() 
{
    root = new TrieNode();
}

Trie::~Trie()
{
    delete root;
}

// Inserts a word into the trie.
void Trie::insert(string word) 
{
    auto *t = root;
    auto p = word.cbegin();
    while (p != word.cend()) {
        if (!t->children[ORD(*p)])
            t->children[ORD(*p)] = new TrieNode();
        t = t->children[ORD(*p)];
        p++;
    }
    t->end = true;
}

// Returns if the word is in the trie.
bool Trie::search(string word) 
{
    auto *t = root;
    auto p = word.cbegin();
    while (p != word.cend()) {
        if (!t->children[ORD(*p)])
            return false;
        t = t->children[ORD(*p)];
        p++;
    }

    return (t->end);
}

// Returns if there is any word in the trie
// that starts with the given prefix.
bool Trie::startsWith(string prefix) 
{
    auto *t = root;
    auto p = prefix.cbegin();
    while (p != prefix.cend()) {
        if (!t->children[ORD(*p)])
            return false;
        t = t->children[ORD(*p)];
        p++;
    }

    return true;
}

static int test_trie()
{
    Trie trie;
    trie.insert("algo");
    trie.insert("algorithm");
    trie.insert("baby");
    trie.insert("bad");
    trie.insert("bachelor");
    assert(trie.search("algo") == true);
    assert(trie.startsWith("ba") == true);
    assert(trie.startsWith("algo") == true);
    assert(trie.startsWith("algorith") == true);

    return 0;
}
