// From the software distribution accompanying the textbook
// "A Practical Introduction to Data Structures and Algorithm Analysis,
// Third Edition (C++)" by Clifford A. Shaffer.
// Source code Copyright (C) 2007-2011 by Clifford A. Shaffer.

// This file includes all of the pieces of the BST implementation

// Include the node implementation
#include "DTNode.h"
#include <vector>
// Include the dictionary ADT
#include "dictionary.h"
#include "DynamicStruct.h"
using namespace std;
// Binary Search Tree implementation for the Dictionary ADT
template <typename Key, typename E>
class DT {
private:
	DTNode<Key, E>* root;   // Root of the BST
	int nodecount;         // Number of nodes in the BST
	 
	
public:
	DT(){
	}
	DT(DTNode<Key, E>* root){
		this->root = root;
	}
	~DT(){
	}
	// Private "helper" functions
	void insertChildsHelper(DTNode<Key, E>*, DTNode<Key, E>*, DTNode<Key, E>*, DTNode<Key, E>* rc = NULL);
	void getAllLeafsHelper(vector<DTNode<Key, E>*>*, DTNode<Key, E>*);
	void zyToggleEdge(vector<DTNode<Key, E>*>*, DTNode<Key, E>*);
	DTNode<Key, E>* getRoot(){
		return root;
	}
};
/*��ĳһ�ڵ�����ӽڵ㣬�������*/
template<typename Key, typename E> void DT<Key, E>
	::insertChildsHelper(DTNode<Key, E>* root, DTNode<Key, E>* lc, DTNode<Key, E>* mc, DTNode<Key, E>* rc = NULL){
		if (root != NULL && root->isLeaf()){ // If this is a left node,it can insert child
			root->setLeft(lc);
			root->setMiddle(mc);
			root->setRight(rc);
		}
	}
	/*�õ����е�Ҷ�ӽڵ�*/
	template<typename Key, typename E> void DT<Key, E>
		::getAllLeafsHelper(vector<DTNode<Key, E>*>* zyTriangles, DTNode<Key, E>* root){
		if (root == NULL){ return; }
		if (root->isLeaf() && root->key() >= 0){
			zyTriangles->push_back(root);
			return;
		}
		getAllLeafsHelper(zyTriangles, root->left());
		getAllLeafsHelper(zyTriangles, root->middle());
		getAllLeafsHelper(zyTriangles, root->right());
		}
	static int leafCount = 1;
	template<typename Key, typename E> void DT<Key, E>::
		zyToggleEdge(vector<DTNode<Key, E>*>* triangles, DTNode<Key, E>* src){
		//1. ͨ��zyFindCommonEdge()��mZyTriangle����ƥ�䵽���С�����ι��ߵ������Σ�����D��Ҷ�ڵ��У�
		triangles->clear();
		this->getAllLeafsHelper(triangles, this->getRoot());
		
	/*	for (int i = 0; i < triangles->size(); i++){
			ZyTriangle* zt = &(triangles->at(i)->element());
			cout << "{" << "\n";
			printMat(zt->points[0][2]);
			printMat(zt->points[1][2]);
			printMat(zt->points[2][2]);
			cout << "}" << "\n";
		}*/
		DTNode<Key, E > *dt = zyFindCommonEdge(triangles, src);
		if (dt == NULL) return;
		if (dt != NULL){
			ZyTriangle* zt = &(dt->element());
			//2. �������������γɵ��ı����У���isCicrle()�������,�������0�ͷ�ת
			if (isCicrle((src->element().points)[0][2], (zt->points)[0][2], (zt->points)[1][2], (zt->points)[2][2])){
			
				leafCount++;
				DTNode<Key, E>* newDt1 = NULL;
				DTNode<Key, E>* newDt2 =NULL;
				//�ع����������Σ���0��Ϊ�����ĸ�����ʽ
				for (int i = 0; i < 3; i++){
				/*	cout << "src = {" << "\n";
					printMat((src->element().points)[1][0]);
					printMat((src->element().points)[2][0]);
					printMat(zt->points[0][0]);
					printMat(zt->points[1][0]);
					printMat(zt->points[2][0]);
					cout << "}" << "\n";*/
					if (!(zt->points[i][0] == (src->element().points)[1][0] || zt->points[i][0] == (src->element().points)[2][0])){
						newDt1 = new DTNode<Key, E>(1, ZyTriangle((src->element().points)[0][0], (src->element().points)[1][0], zt->points[i][0],
							(src->element().points)[0][1], (src->element().points)[1][1], zt->points[i][1], 
							(src->element().points)[0][2], (src->element().points)[1][2], zt->points[i][2]));
						newDt2 = new DTNode<Key, E>(1, ZyTriangle((src->element().points)[0][0], (src->element().points)[2][0], zt->points[i][0],
							(src->element().points)[0][1], (src->element().points)[2][1], zt->points[i][1],
							(src->element().points)[0][2], (src->element().points)[2][2], zt->points[i][2]));
					}
				}//��src��zt���������ǵĹ��߻�������һ����
				if (newDt2 == NULL || newDt1 == NULL){
					return;
				}
				this->insertChildsHelper(src, newDt1, newDt2);
				//this->insertChildsHelper(dt, newDt1, newDt2);
				dt->setKey(-1);
				//3. �ݹ�
				zyToggleEdge(triangles, newDt1);
				zyToggleEdge(triangles, newDt2);
			}else { 
				return;
			};//��0�д�����ǲ�������Ϣ
		
		}
	
	
		}

