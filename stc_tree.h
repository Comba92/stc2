#ifndef STC_TREE_IMPL
#define STC_TREE_IMPL

#include "stc_list.h"

// https://nullprogram.com/blog/2016/11/13/
// https://nullprogram.com/blog/2016/11/15/

typedef struct {
  int value;
  long long left, right;
} TreeNode;


list_def(TreeNode, TreeNodeHeap)

typedef struct {
  TreeNodeHeap heap;
} BST;

#endif