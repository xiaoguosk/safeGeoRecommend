#include "DT.h"
#include "md5.h"
#include <map>
#include "BasicExcelVC6.hpp"
#include "AES.h"
#include "aes_encryptor.h"
#include "计时.h"
#include "TimerCounter.h"
using namespace YExcel;
static CvMat* queryRNN(vector<DTNode<int, ZyTriangle>*>* zyDTreeLeaf, CvMat* qPoint, CvMat* pPoint){
	//1. 先用pPoint找出包含其所在的三角形的点集，不包含pPoint
	vector<CvMat*> vec;
	for (int i = 0; i < zyDTreeLeaf->size(); i++){
		ZyTriangle* zt = &(zyDTreeLeaf->at(i)->element());
		for (int j = 0; j < 3; j++){
			if (CvCompare(zt->points[j][0], pPoint)){
				if (j == 0){
					vec.push_back(zt->points[1][0]);
					vec.push_back(zt->points[2][0]);
				}
				if (j == 1){
					vec.push_back(zt->points[0][0]);
					vec.push_back(zt->points[2][0]);
				}
				if (j == 2){
					vec.push_back(zt->points[0][0]);
					vec.push_back(zt->points[1][0]);
				}
			}
		}
	}
	//2. 遍历vec,找出最近邻
	CvMat* dstTemp1 = cvCreateMat(3, 1, CV_32FC1);
	CvMat* dstTemp2 = cvCreateMat(1, 1, CV_32FC1);
	CvMat* res = CvPoint2CvMatOfP(cvPoint2D32f(99999, 99999));
	for (int i = 0; i < vec.size(); i++){
		cvSub(res, vec[i], dstTemp1);
		cvGEMM(dstTemp1, qPoint, 1, dstTemp1, 0, dstTemp2, 1);//1*3 * 3*1
		float f = CV_MAT_ELEM(*dstTemp2, float, 0, 0);
		if (CV_MAT_ELEM(*dstTemp2, float, 0, 0) <= 0){
			res = vec[i];
		}; 
	}
	return res;
}
static void initData(vector<CvPoint2D32f>& initPoints){
	//1.读取文件中的点,路径为D://opencvdatas.xlsx
	BasicExcel e;
	// Load a workbook with one sheet, display its contents and save into another file.
	e.Load("opencvdatas.xls");
	BasicExcelWorksheet* sheet1 = e.GetWorksheet("Sheet5");
	if (sheet1)
	{
		size_t maxRows = sheet1->GetTotalRows();//行
		size_t maxCols = sheet1->GetTotalCols();//列
		
	
		for (size_t r = 0; r < maxRows; ++r)
		{
			CvPoint2D32f point = cvPoint2D32f(0, 0);
			for (size_t c = 0; c < maxCols; ++c)
			{
				BasicExcelCell* cell = sheet1->Cell(r, c);
				switch (cell->Type())
				{
				case BasicExcelCell::UNDEFINED:
					printf("          ");
					if (c == 0){
						
						point.x = cell->GetInteger();
					}
					else{
						point.y = cell->GetInteger();
					}
					break;

				case BasicExcelCell::INT:
					printf("%10d", cell->GetInteger());
					if (c == 0){
						point.x = cell->GetInteger();
					}
					else{
						point.y = cell->GetInteger();
					}
					break;

				case BasicExcelCell::DOUBLE:
					if (c == 0){
						point.x = (float)((int)(cell->GetDouble() * 1000)) / 1000;
					}
					else{
						point.y = (float)(int)(cell->GetDouble() * 1000) / 1000;
					}
					break;

				case BasicExcelCell::STRING:
					if (c == 0){
						string s = cell->GetString();
						point.x = atof(s.c_str()) * 10;
					}
					else{
						string s = cell->GetString();
						point.y = atof(s.c_str()) * 10;
					}
					break;

				case BasicExcelCell::WSTRING:
					wprintf(L"%10s", cell->GetWString());
					break;
				}
			
			}
			initPoints.push_back(point);
		}
		
	}
}

//test
int  main(int argc, char** argv){
	//CvMat* cvpoint = CvPoint2CvMatS(cvPoint2D32f(0, -20));
	srand((unsigned)time(NULL));
	TimerCounter timer;;
	//
	//CvMat* cvpoint1 = CvPoint2CvMatS(cvPoint2D32f(10, 0));
	//CvMat* cvpoint2 = CvPoint2CvMatS(cvPoint2D32f(0, 10));
	//CvMat* cvpoint3 = CvPoint2CvMatS(cvPoint2D32f(-10, -10));

	//int i = isCicrle(cvpoint, cvpoint1, cvpoint2, cvpoint3);
	//system("pause");


	vector<DTNode<int, ZyTriangle>*>* mZyTriangle = new vector<DTNode<int, ZyTriangle>*>();//存储D树叶节点的结构
	//1. 本地用户初始化操作即生成加密的数据以便交给服务器
	//1.1. 从geo.xls里面得到若干点，这里是明文形式，测试数据自己写几个
	CvRect rect = { 0, 0, 600, 600 };
	vector<CvPoint2D32f> initPoints;
	initData(initPoints);// initPoints为明文的初始化点

	/*画图*/


	/*old code start*/
	//for (int i = 0; i < 5; i++)
	//{
	//	CvPoint2D32f fp = cvPoint2D32f((float)(rand() % (rect.width - 10)),//使点约束在距离边框10像素之内。  
	//		(float)(rand() % (rect.height - 10)));
	//	initPoints.push_back(fp);
	//	printHelper(fp.x, fp.y);
	//}
	/*old code end*/

	//1.2. 以加密的形式形成这若干点的三角剖分
	//1.2.1. 初始化三角剖分，使所有的点都在这个三角形内部,以p点形式保存，还得保存M形式
	float big_coord = 3.f * MAX(rect.width, rect.height);
	CvMat *ppA, *ppB, *ppC;
	CvMat *mmA, *mmB, *mmC;
	CvMat *ssA, *ssB, *ssC;
	float rx = (float)rect.x;
	float ry = (float)rect.y;

	ssA = CvPoint2CvMatS(cvPoint2D32f(rx + big_coord, ry));
	ssB = CvPoint2CvMatS(cvPoint2D32f(rx, ry + big_coord));
	ssC = CvPoint2CvMatS(cvPoint2D32f(rx - big_coord, ry - big_coord));

	ppA = CvPoint2CvMatOfP(cvPoint2D32f(rx + big_coord, ry));//(1800,0)
	//printHelper(rx + big_coord, ry);
	ppB = CvPoint2CvMatOfP(cvPoint2D32f(rx, ry + big_coord));//(0,1800)
	ppC = CvPoint2CvMatOfP(cvPoint2D32f(rx - big_coord, ry - big_coord));//(-1800,-1800)


	mmA = CvPoint2CvMatOfM(cvPoint2D32f(rx + big_coord, ry));
	mmB = CvPoint2CvMatOfM(cvPoint2D32f(rx, ry + big_coord));
	mmC = CvPoint2CvMatOfM(cvPoint2D32f(rx - big_coord, ry - big_coord));


	DTNode<int, ZyTriangle> dt(1, ZyTriangle(ppA, ppB, ppC, mmA, mmB, mmC, ssA, ssB, ssC));//key随意
	ZyTriangle zt = (dt.element());
	DT<int, ZyTriangle> dTree(&dt);
	mZyTriangle->push_back(&dt);
	vector<CvMat*> pointAddrs;
	BasicExcel be;

	be.Load("statistics_query.xls");
	BasicExcelWorksheet* sheet = be.GetWorksheet("Sheet4");
	if (sheet){
		sheet->Cell(0, 0)->SetString("count");
		//sheet->Cell(0, 0)->SetString("time");
	/*	sheet->Cell(0, 2)->SetString("start");
		sheet->Cell(0, 3)->SetString("end");*/
	}
	FILE *f;
	f= fopen("a.csv", "wb");
	//1.2.2（关键步骤）. 开始遍历每一个点,开始插入点是以q点形式保存

	cout <<initPoints.size();
	for (int i = 0; i < initPoints.size(); i++){
		timer.Start();
		//计时开始
		cout << i << "\n";
		long start = GetTickCount();

		//1.2.2.1. 先以q点形式加密，通过zyWhichMinNeig()函数判断在哪一个三角形，返回三角形的指针
		CvMat* ssQ = CvPoint2CvMatS(initPoints[i]);
		CvMat* mmQ = CvPoint2CvMatOfM(initPoints[i]);
		CvMat* qqQ = CvPoint2CvMatOfQ(initPoints[i]);
		CvMat* ppQ = CvPoint2CvMatOfP(initPoints[i]);//q点的p形式
		pointAddrs.push_back(ppQ);//存储q点形式加密方便RKNN静态查询
	

		DTNode<int, ZyTriangle>* dt = zyWhichInTriangle(mZyTriangle, qqQ, mmQ);//dt为找到的三角形所在D树的节点
		DTNode<int, ZyTriangle>* newDt1, *newDt2, *newDt3, *newDt4;
		if (dt != NULL){
			ZyTriangle* zt = &(dt->element());

			newDt1 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, (zt->points)//
				[0][0], (zt->points)[1][0], mmQ, (zt->points)
				[0][1], (zt->points)
				[1][1], ssQ, (zt->points)
				[0][2], (zt->points)
				[1][2]));
			newDt2 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, (zt->points)
				[1][0], (zt->points)[2][0], mmQ, (zt->points)
				[1][1], (zt->points)
				[2][1], ssQ, (zt->points)
				[1][2], (zt->points)
				[2][2]));
			newDt3 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, (zt->points)
				[2][0], (zt->points)[0][0], mmQ, (zt->points)
				[2][1], (zt->points)
				[0][1], ssQ, (zt->points)
				[2][2], (zt->points)
				[0][2]));
			dTree.insertChildsHelper(dt, newDt1, newDt2, newDt3);
			//1.2.3（关键步骤）. 对取出的三角形进行局部剖分，循环，递归每个小三角形，递归函数为zyToggleEdge()
			dTree.zyToggleEdge(mZyTriangle, newDt1);
			dTree.zyToggleEdge(mZyTriangle, newDt2);
			dTree.zyToggleEdge(mZyTriangle, newDt3);
			//1.3. 根据形 树，取所有子节点（以P点形式保存）

			//1.4. 输出统计数据
			//test;
			//CvPoint2D32f cp = cvPoint2D32f(345.850006, -1174.42004);
			//DTNode<int, ZyTriangle>* dt1 = zyWhichInTriangle(mZyTriangle, CvPoint2CvMatOfQ(cp), CvPoint2CvMatOfM(cp));
			/*if (sheet){
				sheet->Cell(i + 1, 0)->SetInteger(leafCount);
				sheet->Cell(i + 1, 2)->SetDouble(start);
				long end = GetTickCount();
				sheet->Cell(i + 1, 3)->SetDouble(end);
				sheet->Cell(i + 1, 1)->SetDouble(end - start);
			}*/
			
		}
		timer.Stop();
		fprintf(f, "%d,%lf,%d\r\n", i, timer.dbTime, leafCount);
		leafCount = 1;
	}
	fclose(f);



	MD5 md5;
	
	string s = ase128_random_key();
	string y = ase128_random_key();
	string z = ase128_random_key();
	AesEncryptor sEncryptor(string2Uchar(s)), yEncryptor(string2Uchar(y)), zEncryptor(string2Uchar(z));
	
	std::hash<std::string> str_hash;
	//遍历每一个关键字，这里的关键字即所有的点
	int keywordSize = pointAddrs.size();
	//string K[1000][50];//定死500
	map<string, string> A, T;
	int ctr = 1;
	for (int i = 0; i < pointAddrs.size(); i++){
		//通过关键字找出文档即点的所有邻接点
		CvMat* wi = pointAddrs[i];
		string swi = mat2String(wi);
		vector<CvMat*> Dwi = zyFindAllNeig(mZyTriangle, pointAddrs[i]);
	
		string Ki0= ase128_random_key();
		string prevK = Ki0;
		//AesEncryptor Ki0Encryptor(string2Uchar(K[i][0]));
		for (int j = 1; j <= Dwi.size(); j++){
			//得到Nij节点
			string Nij = "";
			CvMat* Dij = Dwi[j - 1];
			md5.update(mat2String(Dij));
			string ID_Dij = md5.toString();
			
			string Kij = ase128_random_key();
			string aesCtr = sEncryptor.EncryptString(int2str(ctr)).substr(0,32);
			
			AesEncryptor KijEncryptor(string2Uchar(prevK));
		
			//AesEncryptor Kij2Encyrptor(string2Uchar(prevK));
			prevK = Kij;
			
			ctr = ctr + 1;
			string lastEle = "1.1";
			if (j != Dwi.size()){
				lastEle = int2str(ctr);
			}
			Nij = ID_Dij + Kij + sEncryptor.EncryptString(lastEle).substr(0, 32);
			if (Nij.size() != 80){
				cout << 80;
			}
			string cipherText = KijEncryptor.EncryptString(Nij);
			//string result = Kij2Encyrptor.DecryptString(cipherText);
			A[aesCtr] = cipherText;

			if (j == 1){
				string value = or(aesCtr + Ki0, yEncryptor.EncryptString(swi));
				if (value == ""){
					cout << "";
				}
				T[zEncryptor.EncryptString(swi)] = value;
			}
			
		}
	}

	FILE *f1;
	f1 = fopen("b.csv", "wb");
	//搜索过程，input:关键字即p形式的加密点,output：RKnn
	for (int i = 0; i < pointAddrs.size(); i++){
		//高精度计时 时间;
		//时间.开始();

timer.Start();
		int count = 0;
		CvMat* keywordPoint = pointAddrs[i];
		string PIzw = zEncryptor.EncryptString(mat2String(keywordPoint));
		string Fyw = yEncryptor.EncryptString(mat2String(keywordPoint));
		string o = T[PIzw];
		if (o != ""){
			string res = or(o, Fyw);
			string key = res.substr(res.length() - 16, 16);

			string index = res.substr(0, res.length() - 16);
			string Nij = A[index];
			while (Nij != ""){
				unsigned char* rand = string2Uchar(key);
				unsigned char* rand1 = string2Uchar(key);
				unsigned char* rand2 = string2Uchar(key);
				AesEncryptor KijEncryptor(string2Uchar(key));
				string mNij = KijEncryptor.DecryptString(Nij);
				count++;
				if (mNij.size() != 80){
					break;
				}
				string id = mNij.substr(0, 32);
				key = mNij.substr(32, 16);
				index = mNij.substr(48, 32);
				Nij = A[index];
			}
		}
		timer.Stop();
		//时间.结束();
		/*if (sheet){
			sheet->Cell(i + 1, 0)->SetInteger(end - start);
		}*/
		fprintf(f1, "%d,%d,%lf\n", i, count, timer.dbTime);
		//fprintf(f1, "%d %s %d %s %lld %s %lld %s %lld %s\r\n", i, ",", count, ",", (double)时间.开始时间.QuadPart, ",", (double)时间.结束时间.QuadPart, ",", (double)时间.结束时间.QuadPart - (double)时间.开始时间.QuadPart, "ms");
		//fprintf(f1, "%d %s %d %s %lld %s %lld %s %lld %s\r\n", i, ",", count, ",", (double)时间.开始时间.QuadPart, ",", (double)时间.结束时间.QuadPart, ",", (double)时间.结束时间.QuadPart - (double)时间.开始时间.QuadPart, "ms");
		
	
	}
	fclose(f1);
	//system("pause");
}

//int main(int argc, char** argv)
//{
//	vector<DTNode<int, ZyTriangle>*>* mZyTriangle = new vector<DTNode<int, ZyTriangle>*>();//存储D树叶节点的结构
//	//1. 本地用户初始化操作即生成加密的数据以便交给服务器
//	//1.1. 从geo.xls里面得到若干点，这里是明文形式，测试数据自己写几个
//	CvRect rect = { 0, 0, 600, 600 };
//	vector<CvPoint2D32f> initPoints;
//	initData(initPoints);// initPoints为明文的初始化点
//	
//	/*画图*/
//	
//
//	/*old code start*/
//	//for (int i = 0; i < 5; i++)
//	//{
//	//	CvPoint2D32f fp = cvPoint2D32f((float)(rand() % (rect.width - 10)),//使点约束在距离边框10像素之内。  
//	//		(float)(rand() % (rect.height - 10)));
//	//	initPoints.push_back(fp);
//	//	printHelper(fp.x, fp.y);
//	//}
//	/*old code end*/
//
//	//1.2. 以加密的形式形成这若干点的三角剖分
//	//1.2.1. 初始化三角剖分，使所有的点都在这个三角形内部,以p点形式保存，还得保存M形式
//	float big_coord = 3.f * MAX(rect.width, rect.height);
//	CvMat *ppA, *ppB, *ppC;
//	CvMat *mmA, *mmB, *mmC;
//	CvMat *ssA, *ssB, *ssC;
//	float rx = (float)rect.x;
//	float ry = (float)rect.y;
//	
//	ssA = CvPoint2CvMatS(cvPoint2D32f(rx + big_coord, ry));
//	ssB = CvPoint2CvMatS(cvPoint2D32f(rx, ry + big_coord));
//	ssC = CvPoint2CvMatS(cvPoint2D32f(rx - big_coord, ry - big_coord));
//
//	ppA = CvPoint2CvMatOfP(cvPoint2D32f(rx + big_coord, ry));//(1800,0)
//	//printHelper(rx + big_coord, ry);
//	ppB = CvPoint2CvMatOfP(cvPoint2D32f(rx, ry + big_coord));//(0,1800)
//	ppC = CvPoint2CvMatOfP(cvPoint2D32f(rx - big_coord, ry - big_coord));//(-1800,-1800)
//
//
//	mmA = CvPoint2CvMatOfM(cvPoint2D32f(rx + big_coord, ry));
//	mmB = CvPoint2CvMatOfM(cvPoint2D32f(rx, ry + big_coord));
//	mmC = CvPoint2CvMatOfM(cvPoint2D32f(rx - big_coord, ry - big_coord));
//	
//
//	DTNode<int, ZyTriangle> dt(1, ZyTriangle(ppA, ppB, ppC, mmA, mmB, mmC, ssA, ssB, ssC));//key随意
//	ZyTriangle zt = (dt.element());
//	DT<int, ZyTriangle> dTree(&dt);
//	mZyTriangle->push_back(&dt);
//	vector<CvMat*> pointAddrs;
//
//
//
//
//	
//	BasicExcel be;
//
//	be.Load("opencvdatas.xls");
//	BasicExcelWorksheet* sheet = be.GetWorksheet("Sheet1");
//	if (sheet){
//		sheet->Cell(0, 0)->SetString("count");
//		sheet->Cell(0, 1)->SetString("time");
//		sheet->Cell(0, 2)->SetString("start");
//		sheet->Cell(0, 3)->SetString("end");
//	}
//
//	//1.2.2（关键步骤）. 开始遍历每一个点,开始插入点是以q点形式保存
//	for (int i = 0; i < initPoints.size(); i++){
//		//计时开始
//		long start = GetTickCount();
//		
//		//1.2.2.1. 先以q点形式加密，通过zyWhichMinNeig()函数判断在哪一个三角形，返回三角形的指针
//		CvMat* ssQ = CvPoint2CvMatS(initPoints[i]);
//		CvMat* mmQ = CvPoint2CvMatOfM(initPoints[i]);
//		CvMat* qqQ = CvPoint2CvMatOfQ(initPoints[i]);
//		CvMat* ppQ = CvPoint2CvMatOfP(initPoints[i]);//q点的p形式
//		pointAddrs.push_back(qqQ);//存储q点形式加密方便RKNN静态查询
//
//		
//  	    DTNode<int, ZyTriangle>* dt = zyWhichInTriangle(mZyTriangle, qqQ, mmQ);//dt为找到的三角形所在D树的节点
//		DTNode<int, ZyTriangle>* newDt1, *newDt2, *newDt3, *newDt4;
//        if (dt != NULL){
//			ZyTriangle* zt = &(dt->element());
//			
//			//vector<CvMat**>* vec;
//			//vector<DTNode<int, ZyTriangle>*>* dts = new vector<DTNode<int, ZyTriangle>*>();
//			//if ((vec = isLine(dts, mZyTriangle, zt, ssQ)) == NULL){//共边就不需要转动边
//			//	// 找到共边的两个三角形，对于一个三角形检查不属于共边的点，于是与插入点和该点与返回的共边点组成三角形
//			//	DTNode<int, ZyTriangle>* dt1 = dts->at(0);
//			//	ZyTriangle* zt1 = &(dt1->element());
//			//	for (int k = 0; k < 3; k++){
//			//		if (zt1->points[k][0] != vec->at(0)[0] && zt1->points[k][0] != vec->at(1)[0]){
//			//			newDt1 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, vec->at(0)[0], zt1->points[k][0],
//			//				mmQ, vec->at(0)[1], zt1->points[k][1],
//			//				ssQ, vec->at(0)[2],
//			//				zt1->points[k][2]));
//			//			newDt2 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, vec->at(1)[0], zt1->points[k][0],
//			//				mmQ, vec->at(1)[1], zt1->points[k][1],
//			//				ssQ, vec->at(1)[2],
//			//				zt1->points[k][2]));
//			//			dTree.insertChildsHelper(dt1, newDt1, newDt2);
//			//		}
//			//	}
//			//
//			//	DTNode<int, ZyTriangle>* dt2 = dts->at(1);
//			//	ZyTriangle* zt2 = &(dt2->element());
//			//	for (int k = 0; k < 3; k++){
//			//		if (zt2->points[k][0] != vec->at(0)[0] && zt2->points[k][0] != vec->at(1)[0]){
//			//			newDt3 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, vec->at(0)[0], zt2->points[k][0],
//			//				mmQ, vec->at(0)[1], zt2->points[k][1],
//			//				ssQ, vec->at(0)[2],
//			//				zt2->points[k][2]));
//			//			newDt4 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, vec->at(1)[0], zt2->points[k][0],
//			//				mmQ, vec->at(1)[1], zt2->points[k][1],
//			//				ssQ, vec->at(1)[2],
//			//				zt2->points[k][2]));
//			//			dTree.insertChildsHelper(dt1, newDt3, newDt4);
//			//		}
//			//	}
//			//	
//			//	continue;
//			//};
//
//
//			//CvMat* points[3][2] = zt->points;
//			newDt1 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, (zt->points)//
//				[0][0], (zt->points)[1][0], mmQ, (zt->points)
//				[0][1], (zt->points)
//				[1][1], ssQ, (zt->points)
//				[0][2], (zt->points)
//				[1][2]));
//			newDt2 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, (zt->points)
//				[1][0], (zt->points)[2][0], mmQ, (zt->points)
//				[1][1], (zt->points)
//				[2][1], ssQ, (zt->points)
//				[1][2], (zt->points)
//				[2][2]));
//			newDt3 = new DTNode<int, ZyTriangle>(1, ZyTriangle(ppQ, (zt->points)
//				[2][0], (zt->points)[0][0], mmQ, (zt->points)
//				[2][1], (zt->points)
//				[0][1], ssQ, (zt->points)
//				[2][2], (zt->points)
//				[0][2]));
//			dTree.insertChildsHelper(dt, newDt1, newDt2, newDt3);
//			//1.2.3（关键步骤）. 对取出的三角形进行局部剖分，循环，递归每个小三角形，递归函数为zyToggleEdge()
//			dTree.zyToggleEdge(mZyTriangle, newDt1);
//			dTree.zyToggleEdge(mZyTriangle, newDt2);
//			dTree.zyToggleEdge(mZyTriangle, newDt3);
//			//1.3. 根据形 树，取所有子节点（以P点形式保存）
//			
//			//1.4. 输出统计数据
//			//test;
//			//CvPoint2D32f cp = cvPoint2D32f(345.850006, -1174.42004);
//			//DTNode<int, ZyTriangle>* dt1 = zyWhichInTriangle(mZyTriangle, CvPoint2CvMatOfQ(cp), CvPoint2CvMatOfM(cp));
//			if (sheet){
//				sheet->Cell(i + 1, 0)->SetInteger(leafCount);
//				sheet->Cell(i + 1, 2)->SetDouble(start);
//				long end = GetTickCount();
//				sheet->Cell(i + 1, 3)->SetDouble(end);
//				sheet->Cell(i + 1, 1)->SetDouble(end - start);
//			}
//			leafCount = 0;
//		}
//	}
//	//be.SaveAs("statistics.xls");
//
//
//
//	//静态RKNN查询		
//	//定义一些静态参数
//	AES aes;
//	MD5 md5;
//	int8_t* s; int8_t* y; int8_t* z;
//	aes.ase128_random_key(s);
//	aes.ase128_random_key(y);
//	aes.ase128_random_key(z);
//	std::hash<std::string> str_hash;
//	//遍历每一个关键字，这里的关键字即所有的点
//	int keywordSize = pointAddrs.size();
//	int8_t* K[500][500];//定死500
//	map<string, string> A, T;
//	int ctr = 1;
//	for (int i = 0; i < pointAddrs.size(); i++){
//		//通过关键字找出文档即点的所有邻接点
//		CvMat* wi = pointAddrs[i];
//		string swi = mat2String(wi);
//		vector<CvMat*> Dwi = zyFindAllNeig(mZyTriangle, pointAddrs[i]);
//		int8_t* Ki0;
//		aes.ase128_random_key(Ki0);
//		K[i][0] = Ki0;
//		for (int j = 0; j < Dwi.size(); j++){
//			//得到Nij节点
//			string Nij = "";
//			CvMat* Dij = Dwi[j];
//			md5.update(mat2String(Dij));
//			string ID_Dij = md5.toString();
//			int8_t* Kij;
//			aes.ase128_random_key(Kij);
//			K[i][j] = Kij;
//			string sKij = aes.to_string(Kij);
//			string sctr = int2str(ctr);
//			Nij = ID_Dij + sKij + sctr;
//			//这个to_int8_t有待考虑
//			string cipherText = aes.encrypt(Nij, K[i][j - 1]);
//			string aesCtr = aes.encrypt(sctr, s);
//			A[aesCtr] = cipherText;
//			ctr = ctr + 1;
//			if (j == 1){
//				string value = or(aesCtr + aes.to_string(K[i][0]), aes.encrypt(swi, y));
//				T[aes.encrypt(swi, z)] = value;
//			}
//		}
//	}
//
//
//
//	//测试代码
//	CvMat* qqQ = CvPoint2CvMatOfQ(initPoints[0]);
//	CvMat* ppQ = CvPoint2CvMatOfP(initPoints[0]);
//	CvMat* res = queryRNN(mZyTriangle, qqQ, ppQ);//返回
//	
//	CvMat M = cvMat(3, 3, CV_32FC1, B);
//	CvMat MT = cvMat(3, 3, CV_32FC1, B);
//	cvTranspose(&M, &MT);
//	CvMat* MI = cvCreateMat(3, 3, CV_32FC1);
//	CvMat* matrix = cvCreateMat(3, 1, CV_32FC1);
//	cvInvert(&MT, MI);
//
//	cvGEMM(MI, res, 1, res, 0, matrix, 1);//3 * 3 * 3 * 1
//	
//	float p1 = CV_MAT_ELEM(*matrix, float, 0, 0);
//	float p2 = CV_MAT_ELEM(*matrix, float, 1, 0);
//	float p3 = CV_MAT_ELEM(*matrix, float, 2, 0);
//
//	system("pause");
//}

#ifdef _EiC  
main(1, "delaunay.c");
#endif  