#pragma once

#include <string>

class TrieNode;
class Trie {
public:
    Trie();
    ~Trie();

    // Inserts a word into the trie.
    void insert(std::string word); 
    // Returns if the word is in the trie.
    bool search(std::string word);
    // Returns if there is any word in the trie
    // that starts with the given prefix.
    bool startsWith(std::string prefix);

private:
    TrieNode* root;
};
