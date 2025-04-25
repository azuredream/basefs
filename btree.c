#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "basefs.h"

/*
 * This B+ Tree is a simplistic, in-memory demonstration.
 * All data (keys, pointers) are stored in allocated kernel memory.
 * For a production filesystem, you would store nodes on disk blocks
 * and implement buffering, caching, journaling, etc.
 */

/* You may tune the order as needed. */
#define BTREE_ORDER        4  /* Maximum number of children per node */
#define BTREE_MAX_KEYS    (BTREE_ORDER - 1)  /* Max keys in a node */
#define BTREE_MIN_KEYS    (BTREE_MAX_KEYS / 2) /* Min keys after split */

/*
 * B+ tree leaf nodes hold actual data records.
 * Internal nodes hold child pointers.
 *
 * For simplicity, we define a single struct btree_node
 * that can act as either internal or leaf by checking "is_leaf".
 */
struct btree_node {
	bool           is_leaf;
	int            num_keys;           /* how many keys are used */
	u64            keys[BTREE_MAX_KEYS];   /* array of keys */
	struct btree_node *children[BTREE_ORDER]; 
	/*
	 * For a typical B+ tree, leaves might store an array of data pointers.
	 * We will store them in 'children' array for demonstration,
	 * or in a separate array if you want clarity.
	 */

	/*
	 * Optional: for B+ tree, leaf nodes are often linked in a chain.
	 * If you want leaf chaining:
	 * struct btree_node *next_leaf;
	 */
};

/*
 * The btree_root structure contains a pointer to the actual root node.
 * In a real filesystem context, you'd also have references to disk blocks,
 * buffer heads, etc.
 */
struct btree_root {
	struct btree_node *root;
};

/* Forward declarations */
static void btree_split_child(struct btree_node *parent, int index);
static void btree_insert_nonfull(struct btree_node *node, u64 key);

/* ------------------------------------------------------------------------- */

/*
 * btree_init - Allocates and initializes an empty B+ tree.
 */
struct btree_root *btree_init(void)
{
	struct btree_root *tree;
	struct btree_node *root_node;

	tree = kzalloc(sizeof(*tree), GFP_KERNEL);
	if (!tree)
		return NULL;

	/*
	 * Allocate a root node (initially a leaf).
	 */
	root_node = kzalloc(sizeof(*root_node), GFP_KERNEL);
	if (!root_node) {
		kfree(tree);
		return NULL;
	}
	root_node->is_leaf = true;
	root_node->num_keys = 0;

	tree->root = root_node;
	return tree;
}

/*
 * btree_free_node - Recursively free all child nodes, then free itself.
 */
static void btree_free_node(struct btree_node *node)
{
	int i;

	if (!node->is_leaf) {
		/* Free children recursively. */
		for (i = 0; i <= node->num_keys; i++) {
			if (node->children[i])
				btree_free_node(node->children[i]);
		}
	}
	kfree(node);
}

/*
 * btree_destroy - Frees the entire tree.
 */
void btree_destroy(struct btree_root *tree)
{
	if (!tree)
		return;
	if (tree->root)
		btree_free_node(tree->root);
	kfree(tree);
}

/*
 * btree_search - Search for 'key' in the B+ tree.
 * Returns true if found, false otherwise.
 * In a more complete design, you'd return a pointer to the record data.
 */
bool btree_search(struct btree_root *tree, u64 key)
{
	struct btree_node *node;
	int i;

	if (!tree || !tree->root)
		return false;

	node = tree->root;

	while (1) {
		/* Find the position of key in this node. */
		for (i = 0; i < node->num_keys; i++) {
			if (key < node->keys[i]) {
				break;
			} else if (key == node->keys[i]) {
				return true; /* Found key */
			}
		}
		if (node->is_leaf) {
			/* Key is not found in leaf. */
			return false;
		} else {
			/* Descend to child. */
			node = node->children[i];
		}
	}
}

/*
 * btree_insert - Insert a key into the B+ tree.
 * In a real scenario, you'd also store associated data (value, block index, etc.).
 */
void btree_insert(struct btree_root *tree, u64 key)
{
	struct btree_node *root;
	struct btree_node *new_root;

	if (!tree)
		return;
	root = tree->root;
	if (!root)
		return;

	/*
	 * If the root is full, we must grow the tree in height.
	 */
	if (root->num_keys == BTREE_MAX_KEYS) {
		/* Allocate a new root and make the old root a child. */
		new_root = kzalloc(sizeof(*new_root), GFP_KERNEL);
		if (!new_root)
			return;

		new_root->is_leaf = false;
		new_root->num_keys = 0;
		new_root->children[0] = root;

		/*
		 * Split the old root node if it is full.
		 */
		btree_split_child(new_root, 0);

		/*
		 * Now insert the key into the appropriate child.
		 */
		btree_insert_nonfull(new_root, key);

		/* Update tree->root pointer */
		tree->root = new_root;
	} else {
		/* Root is not full, so insert directly. */
		btree_insert_nonfull(root, key);
	}
}

/*
 * btree_insert_nonfull - Insert 'key' into a node that is known not to be full.
 */
static void btree_insert_nonfull(struct btree_node *node, u64 key)
{
	int i = node->num_keys;

	if (node->is_leaf) {
		/*
		 * Insert the key into this leaf in sorted order.
		 */
		while (i >= 1 && key < node->keys[i - 1]) {
			node->keys[i] = node->keys[i - 1];
			i--;
		}
		node->keys[i] = key;
		node->num_keys++;
	} else {
		/*
		 * Find the child that should receive the key.
		 */
		while (i >= 1 && key < node->keys[i - 1]) {
			i--;
		}
		/*
		 * If the child is full, split it first.
		 */
		if (node->children[i]->num_keys == BTREE_MAX_KEYS) {
			btree_split_child(node, i);
			if (key > node->keys[i])
				i++;
		}
		btree_insert_nonfull(node->children[i], key);
	}
}

/*
 * btree_split_child - Split child node of 'parent' at 'index'.
 * We assume child is full (has BTREE_MAX_KEYS).
 */
static void btree_split_child(struct btree_node *parent, int index)
{
	struct btree_node *full_child = parent->children[index];
	struct btree_node *new_node;
	int mid = BTREE_MAX_KEYS / 2;
	int i;

	/*
	 * Allocate new node which will hold the right half of full_child's keys.
	 * For a B+ tree, you might want to replicate the mid key or manage pointers differently.
	 */
	new_node = kzalloc(sizeof(*new_node), GFP_KERNEL);
	if (!new_node)
		return;

	new_node->is_leaf = full_child->is_leaf;
	new_node->num_keys = BTREE_MAX_KEYS - mid - 1;

	/* Copy the right half of keys from full_child to new_node. */
	for (i = 0; i < new_node->num_keys; i++) {
		new_node->keys[i] = full_child->keys[i + mid + 1];
	}
	/*
	 * For a B+ tree, if it's not a leaf, copy child pointers as well.
	 */
	if (!full_child->is_leaf) {
		for (i = 0; i <= new_node->num_keys; i++) {
			new_node->children[i] = full_child->children[i + mid + 1];
		}
	}

	full_child->num_keys = mid; /* left side keeps 'mid' keys */

	/*
	 * Shift parent's children to make room for the new child pointer.
	 */
	for (i = parent->num_keys; i >= index + 1; i--) {
		parent->children[i + 1] = parent->children[i];
	}

	parent->children[index + 1] = new_node;

	/*
	 * Shift parent's keys to insert the median key from full_child.
	 * For a B+ tree, you might handle the median differently (e.g., keep it in child).
	 */
	for (i = parent->num_keys - 1; i >= index; i--) {
		parent->keys[i + 1] = parent->keys[i];
	}
	parent->keys[index] = full_child->keys[mid];

	parent->num_keys++;
}

/*
 * btree_print - A debugging function to print the B+ tree structure.
 * This uses printk, so it should be used carefully (e.g., with small trees).
 */
void btree_print(struct btree_root *tree)
{
	/* You could implement a level-order or recursive print here. */
	if (!tree || !tree->root) {
		printk(KERN_INFO "B+ Tree is empty.\n");
		return;
	}

	printk(KERN_INFO "---- B+ Tree Print (in-order) ----\n");
	/* A simple recursive approach: */
	btree_print_recursive(tree->root, 0);
	printk(KERN_INFO "-----------------------------------\n");
}

/*
 * btree_print_recursive - Helper for btree_print
 */
static void btree_print_recursive(struct btree_node *node, int level)
{
	int i;

	if (!node)
		return;

	if (node->is_leaf) {
		printk(KERN_INFO "%*sLeaf Node: ", level * 4, "");
		for (i = 0; i < node->num_keys; i++) {
			printk(KERN_CONT "%llu ", node->keys[i]);
		}
		printk(KERN_CONT "\n");
	} else {
		/* Internal node */
		printk(KERN_INFO "%*sInternal Node: ", level * 4, "");
		for (i = 0; i < node->num_keys; i++) {
			printk(KERN_CONT "[%llu] ", node->keys[i]);
		}
		printk(KERN_CONT "\n");
		/* Recursively print children */
		for (i = 0; i <= node->num_keys; i++) {
			btree_print_recursive(node->children[i], level + 1);
		}
	}
}

/*
 * For convenience, you could export these symbols to allow usage in other .c files.
 * E.g., if building as part of a kernel module:
 */
/*
EXPORT_SYMBOL_GPL(btree_init);
EXPORT_SYMBOL_GPL(btree_destroy);
EXPORT_SYMBOL_GPL(btree_search);
EXPORT_SYMBOL_GPL(btree_insert);
EXPORT_SYMBOL_GPL(btree_print);
*/

/*
 * End of file
 */
