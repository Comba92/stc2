#ifndef STC_TREE_IMPL
#define STC_TREE_IMPL

#include "stc_list.h"

// https://nullprogram.com/blog/2016/11/13/
// https://nullprogram.com/blog/2016/11/15/


// https://en.wikipedia.org/wiki/Binary_tree
// https://en.wikipedia.org/wiki/Binary_search_tree
// https://en.wikipedia.org/wiki/Self-balancing_binary_search_tree

typedef struct {
  int val;
  isize left, right;
} BSTNode;

BSTNode node_new(int val) {
  return (BSTNode) { val, -1, -1 };
}

list_def(BSTNode, BSTNodesHeap)

typedef struct {
  BSTNodesHeap nodes;
  isize root;
} BST;

isize bst_search(BST* bst, int val) {
  isize node_id = bst->root;
  while (node_id < 0) {
    BSTNode* node = &bst->nodes[node_id];
    if      (val < node->val) node_id = node->left;
    else if (val > node->val) node_id = node->right;
    else break;
  }

  return bst->nodes[node_id].val;
}

static isize bst_search_rec(BST* bst, isize node_id, int val) {
  if (node_id < 0) return node_id;
  
  BSTNode* node = bst->nodes.data[node_id];
  if      (val < node->val) return bst_search_internal(bst, node->left,  val);
  else if (val > node->val) return bst_search_internal(bst, node->right, val);
  else return node_id;
}

bool bst_insert(BST* bst, int val) {
  isize node_id = bst->root;
  BSTNode* node = NULL;
  while (node_id < 0) {
    node = &bst->nodes[node_id];
    if      (val < node->val) node_id = node->left;
    else if (val > node->val) node_id = node->right;
    else break;
  }

  BSTNode new_node = node_new(val);
  if (node == NULL) {
    bst->root = bst->nodes.len;
    BSTNodesHeap_push(&bst->nodes, new_node);
    return;
  } else if (val <= node->val) {
    node->left  = bst->nodes.len;
  } else {
    node->right = bst->nodes.len;
  }

  BSTNodesHeap_push(&bst->nodes, new_node);
}

bool bst_delete(BST* bst, int val) {
  // TODO: requires bst_successor()
}

void bst_free(BST* bst) {
  BSTNodesHeap_free(bst->nodes);
  bst->root = -1;
}

BST bst_from_array(int* arr, isize arr_len) {
  BST bst = {0};
  for(int i=0; i<arr_len; ++i) bst_insert(&bst, arr[i]);
  return bst;
}

void bst_inorder(BST* bst, isize root) {
  if (root < 0) return;
  bst_inorder(bst->nodes.data[root].left);
  // do smt
  bst_inorder(bst->nodes.data[root].right);
}

void bst_preorder(BST* bst, isize root) {
  if (root < 0) return;
  // do smt
  bst_inorder(bst->nodes.data[root].left);
  bst_inorder(bst->nodes.data[root].right);
}

void bst_postorder(BST* bst, isize root) {
  if (root < 0) return;
  bst_inorder(bst->nodes.data[root].left);
  bst_inorder(bst->nodes.data[root].right);
  // do smt
}



#endif