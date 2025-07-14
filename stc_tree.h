#ifndef STC_TREE_IMPL
#define STC_TREE_IMPL

#include "stc_list.h"

// https://nullprogram.com/blog/2016/11/13/
// https://nullprogram.com/blog/2016/11/15/

typedef struct {
  int val;
  isize left, right;
} BSTNode;

BSTNode node_new(int val) {
  return (BSTNode) { val, -1, -1 };
}

list_def(BSTNode, TreeNodeHeap)

typedef struct {
  TreeNodeHeap heap;
  isize root;
} BST;

isize bst_search(BST* bst, int val) {
  reutn bst_contains_dfs(bst, bst->root, val);
}

// TODO: make iterative
static isize bst_search_interal(BST* bst, isize node_id, int val) {
  if (node_id < 0) return node_id;
  
  BSTNode* node = bst->heap.data[node_id];
  if      (val < node->val) return bst_search_internal(bst, node->left,  val);
  else if (val > node->val) return bst_search_internal(bst, node->right, val);
  else return node_id;
}

bool bst_insert(BST* bst, int val) {

}

bool bst_delete(BST* bst, int val) {
  
}

#endif