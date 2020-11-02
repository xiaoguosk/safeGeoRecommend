#include "util.h"

using namespace std;

class ZyTriangle
{
public:
	/*0�д洢�����ĸ�����ʽ��ֵ��1��2�д洢��ͨ����ʽ��ֵ*/
	CvMat* points[3][3];
public:
	ZyTriangle(){};
	ZyTriangle(CvMat* pa, CvMat* pb, CvMat* pc, CvMat* ma, CvMat* mb, CvMat* mc, CvMat* sa, CvMat* sb, CvMat* sc){
		//�洢p��ʽ�ļ��ܵ�
		points[0][0] = pa;
		points[1][0] = pb;
		points[2][0] = pc;

		//�洢m��ʽ�ļ��ܵ�
		points[0][1] = ma;
		points[1][1] = mb;
		points[2][1] = mc;

		//�洢s��ʽ�ļ��ܵ�
		points[0][2] = sa;
		points[1][2] = sb; 
		points[2][2] = sc;
	};
	~ZyTriangle(){
		
	}
};


/*���ҵ�������*/
//���ӶȽϸߣ���Ҫ�Ż�
CV_INLINE CvMat* zyWhichMinNeig(vector<DTNode<int, ZyTriangle>*>* zyDTreeLeaf, CvMat* point){
	CvMat* res = CvPoint2CvMatOfP(cvPoint2D32f(99999, 99999));
	size_t addr = 0;
	CvMat* dstTemp = cvCreateMat(3, 1, CV_32FC1);
	CvMat* dstTemp1 = cvCreateMat(3, 1, CV_32FC1);
	CvMat* dstTemp2 = cvCreateMat(1, 1, CV_32FC1);
	for (int j = 0; j < zyDTreeLeaf->size(); j++){
		ZyTriangle* zt = &(*zyDTreeLeaf)[j]->element(); 
		for (int i = 0; i < 3; i++){
			cvSub(res, ((zt->points)[i][0]), dstTemp1);
			cvGEMM(dstTemp1, point, 1, dstTemp1, 0, dstTemp2, 1);//1*3 * 3*1
			if (CV_MAT_ELEM(*dstTemp2, float, 0, 0) <= 0){
				res = (zt->points)[i][0];
				addr = (size_t)((zt->points)[i][0]);
			};
		}
		
	}
	return (CvMat*)addr;
}

CV_INLINE vector<CvMat*> zyFindAllNeig(vector<DTNode<int, ZyTriangle>*>* zyDTreeLeaf, CvMat* point){
	
	vector<CvMat*> vectors;
	for (int i = 0; i < zyDTreeLeaf->size(); i++){
		DTNode<int, ZyTriangle>* it = zyDTreeLeaf->at(i);
			ZyTriangle zt = it->element();
		for (int j = 0; j < 3; j++){
			if (zt.points[j][0] == point){
				int logo = 0;
							for (int k = 0; k < vectors.size(); k++){
								if (vectors[k] == (zt.points)[1][0]){
									logo = logo + 1;
									break;
								}
							}
							for (int k = 0; k < vectors.size(); k++){
								if (vectors[k] == (zt.points)[2][0]){
									logo = logo + 2;
									break;
								}
							}
							switch (logo)
							{
							case 0:
								vectors.push_back((zt.points)[1][0]);
								vectors.push_back((zt.points)[2][0]);
								break;
							case 1:
								vectors.push_back((zt.points)[2][0]);
								break;
							case 2:
								vectors.push_back((zt.points)[1][0]);
								break;
								//�����������logo=3���˳�
							default:
								break;
							}
			}
		}
	}

	return vectors;
	//vector<CvMat*> vectors;
	//for (int j = 0; j < zyDTreeLeaf->size(); j++){
	//	DTNode<int, ZyTriangle>* it = zyDTreeLeaf->at(j);
	//	ZyTriangle zt = it->element();
	//	for (int i = 0; i < 3; i++){
	//		//zt.points[1][0]��ʾ��������p��ʽ�ĵ�һ����
	//		if ((zt.points)[i][0] == point){
	//			int logo = 0;
	//			for (int k = 0; k < vectors.size(); k++){
	//				if (vectors[k] == (zt.points)[1][0]){
	//					logo = logo + 1;
	//					break;
	//				}
	//			}
	//			for (int k = 0; k < vectors.size(); k++){
	//				if (vectors[k] == (zt.points)[2][0]){
	//					logo = logo + 2;
	//					break;
	//				}
	//			}
	//			switch (logo)
	//			{
	//			case 0:
	//				vectors.push_back((zt.points)[1][0]);
	//				vectors.push_back((zt.points)[2][0]);
	//				break;
	//			case 1:
	//				vectors.push_back((zt.points)[1][0]);
	//				break;
	//			case 2:
	//				vectors.push_back((zt.points)[2][0]);
	//				break;
	//				//�����������logo=3���˳�
	//			default:
	//				break;
	//			}
	//		}
	//	}
	//	return vectors;
	//}
}

/*�жϵ�����һ���������ڲ�,point����q����ʽ�洢��,MPointsΪ������M��ʽ�ĵ�,Ϊ����ά�еĵ�,���Գɹ�*/
CV_INLINE DTNode<int, ZyTriangle>* zyWhichInTriangle(vector<DTNode<int, ZyTriangle>*>* zyDTreeLeaf, CvMat* point, CvMat* mPoint){
	//1. ͨ��point�ҵ�����ڽӵ�
	CvMat* minNeigPoint = zyWhichMinNeig(zyDTreeLeaf, point);
	for (int j = 0; j < zyDTreeLeaf->size(); j++){
		//2. ͨ��minNeigPoint����֪minNeigPoint�����������6����������
		DTNode<int, ZyTriangle>* it= zyDTreeLeaf->at(j);
		ZyTriangle zt = it->element();
		for (int i = 0; i < 3; i++){	
			CvMat* ab = cvCreateMat(3, 1, CV_32FC1);
			CvMat* ac = cvCreateMat(3, 1, CV_32FC1);
			CvMat* ap = cvCreateMat(3, 1, CV_32FC1);
			CvMat* res = cvCreateMat(3, 1, CV_32FC1);
			CvMat* res2 = cvCreateMat(3, 1, CV_32FC1);
			
			if ((zt.points)[i][0] == minNeigPoint){//�ж��Ƿ�������ΰ�������ڵĵ�
				//2.1 ����ÿ�������Σ�ͨ�������ķ���ȥ�ж����ĸ��������ڲ���return��������,����Ҫ�ж��������Ƿ��ڱ���
				

				cvSub(zt.points[1][1], zt.points[0][1], ab);//bug:a��b�ĵ���һ����
				cvSub(zt.points[2][1], zt.points[0][1], ac);
				cvSub(mPoint, zt.points[0][1], ap);
				//cvCrossProduct(ab, ac, res);//3x1 x 3x1
				//cvCrossProduct(ab, ap, res2);

				
				int d = check(ab, ac, ap);
				
			
				if ( d > 0){
 					return it;
				}
				if (d == 0 ){
					cout << "a:\n";
					printMat((zt.points)[0][1]);
					cout << "b:\n";
					printMat((zt.points)[1][1]);
					cout << "c:\n";
					printMat((zt.points)[2][1]);
					cout << "p:\n";
					printMat(mPoint);


					cout << "ab:\n";
					printMat(ab);
					cout << "ac:\n";
					printMat(ac);
					cout << "ap:\n";
					printMat(ap);

					cout << "res:\n";
					printMat(res);

					cout << "res2:\n";
					printMat(res2);

					cout << "�õ���ĳ����" << j <<"{";
					printMat(mPoint);
					cout <<		 "}";
				};
			}
		}
	}
	return NULL;
}
/*�ڲ�����,0:left 1:right*/
int pointLocLine(float x, float y, float k, float b){
	if (k*x + b - y > 0){
		if (k >= 0){
			return 1;
		}
		else{
			return 0;
		}
	}
	else{
		if (k >= 0){
			return 0;
		}
		else{
			return 1;
		}
	}

}
/*�ڲ���������*/

float vectorAngle(float ox,float oy, float dx, float dy, float ax, float ay){
	float k = (dx - ox) / (dy - oy);
	float b = dy - (k * dx);
	//1�棬0�٣������㴦�����ұߵ�ʱ��
	float adotb = (dx - ox) * (ax - ox) + (dy - oy) * (ay - oy);
	float absab = sqrt((dx - ox) * (dx - ox) + (dy - oy) * (dy - oy)) * sqrt((ax - ox) * (ax - ox) + (ay - oy) * (ay - oy));
	if (pointLocLine(ax, ay, k, b)){//������ұ�
		return 3.14 - acos(adotb / absab);
	}
	else{
		return acos(adotb / absab);
	};
}



/*�Ƿ���Ҫ��ת��,1��Ҫ��ת*/
CV_INLINE int isCicrle(CvMat* d, CvMat* a, CvMat* b, CvMat* c){ //debug by zhouyueyue 
	//1.����4x4�ľ���
	//1.�ж�a,b,c�еľ���˳����Ҫ������ʱ�뷽��,��d��ΪԲ��	

	float C[4][4] = {0};
	C[0][0] = 1 * 3;
	C[1][0] = (float)cvGetReal2D(d, 0, 0) * 3;
	C[2][0] = (float)cvGetReal2D(d, 1, 0) * 3;
	C[3][0] = (float)cvGetReal2D(d, 2, 0) * 3;


	//a
	C[0][1] = 1 * 3;
	C[1][1] = (float)cvGetReal2D(a, 0, 0) * 3;
	C[2][1] = (float)cvGetReal2D(a, 1, 0) * 3;
	C[3][1] = (float)cvGetReal2D(a, 2, 0) * 3;
	//b
	C[0][2] = 1 * 3;
	C[1][2] = (float)cvGetReal2D(b, 0, 0) * 3;
	C[2][2] = (float)cvGetReal2D(b, 1, 0) * 3;
	C[3][2] = (float)cvGetReal2D(b, 2, 0) * 3;
	//c
	C[0][3] = 1 * 3;
	C[1][3] = (float)cvGetReal2D(c, 0, 0) * 3;
	C[2][3] = (float)cvGetReal2D(c, 1, 0) * 3;
	C[3][3] = (float)cvGetReal2D(c, 2, 0) * 3;
	//design by zhouyueyue
	float kab, kac, kbc, abb, acb, bcb;
	float ax = C[1][1];
	float ay = C[2][1];
	float bx = C[1][2];
	float by = C[2][2];
	float cx = C[1][3];
	float cy = C[2][3];
	float dx = C[1][0];
	float dy = C[2][0];
	kab = (ay - by) / (ax - bx);
	abb = ay - (kab * ax);
	kac = (ay - cy) / (ax - cx);
	acb = ay - (kac * ax);
	kbc = (by - cy) / (bx - cx);
	bcb = by - (kbc * bx);

	if (pointLocLine(dx, dy, kab, abb) && pointLocLine(dx, dy, kbc, bcb)){
		//d,b,c,a
		//a
		C[0][1] = 1 * 3;
		C[1][1] = (float)cvGetReal2D(b, 0, 0) * 3;
		C[2][1] = (float)cvGetReal2D(b, 1, 0) * 3;
		C[3][1] = (float)cvGetReal2D(b, 2, 0) * 3;
		//b
		C[0][2] = 1 * 3;
		C[1][2] = (float)cvGetReal2D(c, 0, 0) * 3;
		C[2][2] = (float)cvGetReal2D(c, 1, 0) * 3;
		C[3][2] = (float)cvGetReal2D(c, 2, 0) * 3;
		//c
		C[0][3] = 1 * 3;
		C[1][3] = (float)cvGetReal2D(a, 0, 0) * 3;
		C[2][3] = (float)cvGetReal2D(a, 1, 0) * 3;
		C[3][3] = (float)cvGetReal2D(a, 2, 0) * 3;
	}
	if (!pointLocLine(dx, dy, kac, acb) && !pointLocLine(dx, dy, kbc, bcb)){
		//d,c,a,b
		
		C[0][1] = 1 * 3;
		C[1][1] = (float)cvGetReal2D(c, 0, 0) * 3;
		C[2][1] = (float)cvGetReal2D(c, 1, 0) * 3;
		C[3][1] = (float)cvGetReal2D(c, 2, 0) * 3;
	
		C[0][2] = 1 * 3;
		C[1][2] = (float)cvGetReal2D(a, 0, 0) * 3;
		C[2][2] = (float)cvGetReal2D(a, 1, 0) * 3;
		C[3][2] = (float)cvGetReal2D(a, 2, 0) * 3;
	
		C[0][3] = 1 * 3;
		C[1][3] = (float)cvGetReal2D(b, 0, 0) * 3;
		C[2][3] = (float)cvGetReal2D(b, 1, 0) * 3;
		C[3][3] = (float)cvGetReal2D(b, 2, 0) * 3;
	}
	if (pointLocLine(dx, dy, kac, acb) && !pointLocLine(dx, dy, kab, abb)){
		//d,a,b,c
	}

	CvMat M = cvMat(4, 4, CV_32FC1, C);
	double dou = cvDet(&M);
	if (dou > 0){
		return 1;
	}
	else{
		return 0;
	};
	return -1;
}
/*ͨ��һ���߷��ֹ��ߵ�����������*/
CV_INLINE vector<DTNode<int, ZyTriangle>*>* zyFindZyTriangle(vector<DTNode<int, ZyTriangle>*>* zyDTreeLeaf, CvMat* point1, CvMat* point2){
	vector<DTNode<int, ZyTriangle>*>* dts;
	for (int i = 0; i < zyDTreeLeaf->size(); i++){
		ZyTriangle* zt = &(zyDTreeLeaf->at(i)->element());
		int count = 0;
		for (int j = 0; j < 3; j++){
			if (zt->points[j][2] == point1 || zt->points[j][2] == point2){
				count++;
			}
		}
		if (count == 2){
			dts->push_back(zyDTreeLeaf->at(i));
		}
		if (dts->size() == 2){
			return dts;
		}
	}
	return dts;
}

/**/
CV_INLINE vector<CvMat**>* isLine(vector<DTNode<int, ZyTriangle>*>* dts, vector<DTNode<int, ZyTriangle>*>* zyDTreeLeaf, ZyTriangle* zt, CvMat* point){
	CvMat* ab = cvCreateMat(3, 1, CV_32FC1);
	CvMat* ac = cvCreateMat(3, 1, CV_32FC1);
	CvMat* bc = cvCreateMat(3, 1, CV_32FC1);
	vector<CvMat**>* vec = NULL;
	CvMat* aq = cvCreateMat(3, 1, CV_32FC1);
	CvMat* bq = cvCreateMat(3, 1, CV_32FC1);
	CvMat* cq = cvCreateMat(3, 1, CV_32FC1);
	cvSub(zt->points[1][2], zt->points[0][2], ab);
	cvSub(zt->points[2][2], zt->points[0][2], ac);
	cvSub(zt->points[2][2], zt->points[1][2], bc);
	
	cvSub(point, zt->points[0][2], aq);
	cvSub(point, zt->points[1][2], bq);
	cvSub(point, zt->points[2][2], cq);
	if (abs(CV_MAT_ELEM(*ab, float, 0, 0) * CV_MAT_ELEM(*aq, float, 1, 0)) == abs(CV_MAT_ELEM(*ab, float, 1, 0) * CV_MAT_ELEM(*aq, float, 0, 0))){
		dts = zyFindZyTriangle(zyDTreeLeaf, vec->at(0)[2], vec->at(1)[2]);
		vec->push_back(zt->points[1]);
		vec->push_back(zt->points[0]);
	}
	if (abs(CV_MAT_ELEM(*ac, float, 0, 0) * CV_MAT_ELEM(*cq, float, 1, 0)) == abs(CV_MAT_ELEM(*ac, float, 1, 0) * CV_MAT_ELEM(*cq, float, 0, 0))){
		dts = zyFindZyTriangle(zyDTreeLeaf, vec->at(0)[2], vec->at(1)[2]);
		vec->push_back(zt->points[2]);
		vec->push_back(zt->points[0]);
	}
	if (abs(CV_MAT_ELEM(*bc, float, 0, 0) * CV_MAT_ELEM(*bq, float, 1, 0)) == abs(CV_MAT_ELEM(*bc, float, 1, 0) * CV_MAT_ELEM(*bq, float, 0, 0))){
		dts = zyFindZyTriangle(zyDTreeLeaf, vec->at(0)[2], vec->at(1)[2]);
		vec->push_back(zt->points[2]);
		vec->push_back(zt->points[1]);
	}
	return vec;
}

/*���ֹ��ߵ�������*/
CV_INLINE DTNode<int, ZyTriangle>* zyFindCommonEdge(vector<DTNode<int, ZyTriangle>*>* triangles, DTNode<int, ZyTriangle>* src){
	/*printMat(src->element().points[0][2]);
	printMat(src->element().points[1][2]);
	printMat(src->element().points[2][2]);*/
	//for (int i = 0; i < triangles->size(); i++){
	//	ZyTriangle* zt = &(triangles->at(i)->element());
	//	cout << "{" << "\n";
	//	printMat(zt->points[0][2]);	
	//	printMat(zt->points[1][2]);
	//	printMat(zt->points[2][2]);
	//	cout << "}" << "\n";
	//}
 	for (int i = 0; i < triangles->size(); i++){
		if (triangles->at(i)->isLeaf()){
			ZyTriangle* zt = &(triangles->at(i)->element());
			/*printMat(zt->points[0][2]);
			printMat(zt->points[1][2]);
			printMat(zt->points[2][2]);*/
			int count = 0;
			for (int j = 0; j < 3; j++){
				if (zt->points[j][0] == (src->element().points)[1][0] || zt->points[j][0] == (src->element().points)[2][0]){
					count++;
				};
				//���������ε�����
				if (count == 2 && zt != &(src->element())){
					return triangles->at(i);
				}
			}
			
		}
	}

	return NULL;
}

