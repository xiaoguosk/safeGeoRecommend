// From the software distribution accompanying the textbook
// "A Practical Introduction to Data Structures and Algorithm Analysis,
// Third Edition (C++)" by Clifford A. Shaffer.
// Source code Copyright (C) 2007-2011 by Clifford A. Shaffer.

// This is the file to include for access to the complete binary node
// template implementation

#include "BinNode.h"
#include <stdio.h>
// Simple binary tree node implementation
template <typename Key, typename E>
class DTNode : public BinNode<E> {
private:
  Key k;                // The node's key
  E it;                 // The node's value
  DTNode* lc;			//Pointer to left child
  DTNode* mc;			//Pointer to middle child
  DTNode* rc;           // Pointer to right child
public:
	DTNode(){ lc = mc = rc = NULL; };
	DTNode(Key K, E e, DTNode* l = NULL, DTNode* m = NULL, DTNode* r = NULL)
	{
		k = K; it = e; lc = l; mc = m; rc = r;
	}
	~DTNode() {
		cout << "destructor is called";
	}             // Destructor
	DTNode* createDTNode(){
		
	}
	// Functions to set and return the value and key
	E& element() { return it; }
	void setElement(const E& e) { it = e; }
	Key& key() { return k; }
	void setKey(const Key& K) { k = K; }

	// Functions to set and return the children
	inline DTNode* left() const { return lc; }
	void setLeft(BinNode<E>* b) { lc = (DTNode*)b; }

	inline DTNode* middle() const { return mc; }
	void setMiddle(BinNode<E>* b) { mc = (DTNode*)b; }

	inline DTNode* right() const { return rc; }
	void setRight(BinNode<E>* b) { rc = (DTNode*)b; }

	// Return true if it is a leaf, false otherwise
	bool isLeaf() { return (lc == NULL) && (mc == NULL) && (rc == NULL); }

};
