#include "utils.h"

bst_node *new_bst_node(struct node *n) {
    bst_node *b = (bst_node *) malloc(sizeof(bst_node));
    bst_node_count ++;
    b->n = n;
    b->left = b->right = BST_NULL;
    b->h = 0;
    b->s = 1;
    return b;
}

void free_bst_node(bst_node *b) {
    free(b);
    bst_node_count --;
}

void init_bst() {
    bst_node_count = 0;
    BST_NULL = new_bst_node(NULL);
    BST_NULL->left = BST_NULL->right = BST_NULL;
    BST_NULL->h = -1;
    BST_NULL->s = 0;
}

void clear_bst() {
    free_bst_node(BST_NULL);
    assert(bst_node_count == 0);
}

bst_node *new_bst() {
    return BST_NULL;
}

void traverse_bst(bst_node *root, bst_node **queue, int *tail) {
    if (root == BST_NULL) 
        return ;
    traverse_bst(root->left, queue, tail);
    queue[(*tail) ++] = root;
    traverse_bst(root->right, queue, tail);
}

void free_bst(bst_node *root) {
    bst_node **queue = (bst_node **) new_buffer();
    int head, tail = 0;
    traverse_bst(root, queue, & tail);
    for (head = 0; head < tail; head ++) 
        free_bst_node(queue[head]);
    free_buffer((void **) queue);
}

void update(bst_node *root) {
    root->s = root->left->s + root->right->s + 1;
    if (root->left->h > root->right->h)
        root->h = root->left->h + 1;
    else
        root->h = root->right->h + 1;
}

bst_node *rotate_right(bst_node *root) {
    bst_node *temp = root->left;
    root->left = temp->right;
    temp->right = root;
    update(root); update(temp);
    return temp;
}

bst_node *rotate_left(bst_node *root) {
    bst_node *temp = root->right;
    root->right = temp->left;
    temp->left = root;
    update(root); update(temp);
    return temp;
}

bst_node *re_balance(bst_node *root) {
    if (root->left->h > root->right->h + 1) {
        if (root->left->left->h >= root->left->right->h)
            root = rotate_right(root);
        else {
            root->left = rotate_left(root->left);
            root = rotate_right(root);
        }
    }
    if (root->right->h > root->left->h + 1) {
        if (root->right->right->h >= root->right->left->h)
            root = rotate_left(root);
        else {
            root->right = rotate_right(root->right);
            root = rotate_left(root);
        }
    }
    return root;
}

bst_node *bst_insert(bst_node *root, node *n) {
    if (root == BST_NULL) {
        bst_node *temp = new_bst_node(n);
        return temp;
    }
    if (n->node2id < root->n->node2id)
        root->left = bst_insert(root->left, n);
    else
        root->right = bst_insert(root->right, n);
    update(root);
    root = re_balance(root);
    return root;
}

bst_node *bst_search(bst_node *root, long long id) {
    if (root == BST_NULL) 
        return NULL;
    if (id == root->n->node2id) 
        return root;
    if (id < root->n->node2id) 
        return bst_search(root->left, id);
    else 
        return bst_search(root->right, id);
}
