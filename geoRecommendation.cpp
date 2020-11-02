#include <opencv2/imgproc/imgproc_c.h>  
#include <opencv2/legacy/legacy.hpp>  
#include "opencv2/highgui/highgui.hpp"  
#include<opencv2\opencv.hpp>  
#include<iostream>  
#include "BasicExcel.hpp"
#include <stdio.h>  
#include <time.h>
#include "grmat.h"
#include "md5.h"
#include <wmmintrin.h>
#include "AES.h"
using namespace std;
using namespace YExcel;
using namespace cv;
static void help(void)
{
	printf("\nThis program demostrates iterative construction of\n"//这个程序阐述了delaunay剖分和voronoi细分的迭代构造  
		"delaunay triangulation and voronoi tesselation.\n"
		"It draws a random set of points in an image and then delaunay triangulates them.\n"//在图像上画出一些随机点，然后进行delaunay三角剖分  
		"Usage: \n"
		"./delaunay \n"
		"\nThis program builds the traingulation interactively, you may stop this process by\n"
		"hitting any key.\n");//迭代构造三角剖分，如果像停止，则按任意键  
}


static CvSubdiv2D* init_delaunay(CvMemStorage* storage,//初始化三角剖分结构，为其分配单元  
	CvRect rect)
{
	CvSubdiv2D* subdiv;//三角剖分的数据单元  

	subdiv = cvCreateSubdiv2D(CV_SEQ_KIND_SUBDIV2D, sizeof(*subdiv),
		sizeof(CvSubdiv2DPoint),
		sizeof(CvQuadEdge2D),
		storage);
	cvInitSubdivDelaunay2D(subdiv, rect);

	return subdiv;
}


static void draw_subdiv_point(IplImage* img, CvPoint2D32f fp, CvScalar color)//画出三角剖分的顶点  
{
	cvCircle(img, cvPoint(cvRound(fp.x), cvRound(fp.y)), 5, color, CV_FILLED, 8, 0);
}


static void draw_subdiv_edge(IplImage* img, CvSubdiv2DEdge edge, CvScalar color)//画出三角剖分的边  
{
	CvSubdiv2DPoint* org_pt;//源顶点  
	CvSubdiv2DPoint* dst_pt;//目地顶点  
	CvPoint2D32f org;
	CvPoint2D32f dst;
	CvPoint iorg, idst;

	org_pt = cvSubdiv2DEdgeOrg(edge);//通过边获取顶点  
	dst_pt = cvSubdiv2DEdgeDst(edge);

	if (org_pt && dst_pt)//如果两个端点不为空  
	{
		org = org_pt->pt;
		dst = dst_pt->pt;
		cout << "(" << org.x << "," << org.y << ")->" << "(" << dst.x << "," << dst.y << ")" << "\n";
		iorg = cvPoint(cvRound(org.x), cvRound(org.y));
		idst = cvPoint(cvRound(dst.x), cvRound(dst.y));

		cvLine(img, iorg, idst, color, 1, CV_AA, 0);
	}
}


static void draw_subdiv(IplImage* img, CvSubdiv2D* subdiv,
	CvScalar delaunay_color, CvScalar voronoi_color)//画出剖分和细分  
{
	CvSeqReader  reader;
	int i, total = subdiv->edges->total;//边的数量  
	int elem_size = subdiv->edges->elem_size;//边的大小  
	cout << typeid(subdiv->edges).name() << endl;

	cvStartReadSeq((CvSeq*)(subdiv->edges), &reader, 0);//使用CvSeqReader遍历Delaunay或者Voronoi边  

	for (i = 0; i < total; i++)
	{
		CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);

		if (CV_IS_SET_ELEM(edge))
		{
			// draw_subdiv_edge( img, (CvSubdiv2DEdge)edge + 1, voronoi_color );  
			draw_subdiv_edge(img, (CvSubdiv2DEdge)edge, delaunay_color);
		}

		CV_NEXT_SEQ_ELEM(elem_size, reader);
	}

}


static void locate_point(CvSubdiv2D* subdiv, CvPoint2D32f fp, IplImage* img,//遍历三角剖分的边  
	CvScalar active_color)
{
	CvSubdiv2DEdge e;
	CvSubdiv2DEdge e0 = 0;
	CvSubdiv2DPoint* p = 0;

	cvSubdiv2DLocate(subdiv, fp, &e0, &p);

	if (e0)
	{
		e = e0;
		do
		{
			draw_subdiv_edge(img, e, active_color);
			e = cvSubdiv2DGetEdge(e, CV_NEXT_AROUND_LEFT);
		} while (e != e0);
	}

	draw_subdiv_point(img, fp, active_color);
}

//@author andme-单目视觉  
void dashLine(Mat &img, Point2d& pt1, Point2d& pt2, int n)//n为虚线段数  
{
	Point sub = pt2 - pt1;
	for (int i = 0; i < 2 * n; i += 2)
	{
		line(img, Point(pt1.x + sub.x * i / (2 * n - 1), pt1.y + sub.y * i / (2 * n - 1)), Point(pt1.x + sub.x * (i + 1) / (2 * n - 1), pt1.y + sub.y * (i + 1) / (2 * n - 1)), Scalar(0, 255, 0), 2);
	}
}



//调用形式draw_subdiv_facet( img, cvSubdiv2DRotateEdge( e, 1 ));  
static void draw_subdiv_facet(IplImage* img, CvSubdiv2DEdge edge)//画出voronoi面   
{
	//cout<<edge<<endl;//edge低两位表示表示索引，高位表示四方边缘指针。  
	//cout<<(edge&3)<<endl;  
	CvSubdiv2DEdge t = edge;//当我们按上面的调用形式时，edge为eRot。  
	int i, count = 0;
	CvPoint* buf = 0;
	Point2d *buf1 = 0;

	// count number of edges in facet //面内边的计数  
	do
	{
		count++;
		t = cvSubdiv2DGetEdge(t, CV_NEXT_AROUND_LEFT);
	} while (t != edge);//我们绕着一个voronoi单元一周，遍历该vornonoi边缘所拥有的边缘数。  

	buf = (CvPoint*)malloc(count * sizeof(buf[0]));
	buf1 = (Point2d*)malloc(count*sizeof(buf1[0]));

	// gather points  
	t = edge;
	for (i = 0; i < count; i++)
	{
		CvSubdiv2DPoint* pt = cvSubdiv2DEdgeOrg(t);//第一次获取eRot边缘的起始点  
		if (!pt) break;//如果得不到该源点，则退出循环  
		buf[i] = cvPoint(cvRound(pt->pt.x), cvRound(pt->pt.y));//将该点转换为cvPoint类型点，存储在buf中  
		t = cvSubdiv2DGetEdge(t, CV_NEXT_AROUND_LEFT);//然后绕着vornonoi单元，左旋转。  
	}

	if (i == count)//如果所有的点都存储起来了。  
	{
		CvSubdiv2DPoint* pt = cvSubdiv2DEdgeDst(cvSubdiv2DRotateEdge(edge, 1));//这里eRot的旋转边缘应该是reversed e,那么目的点，就是e的源点。  
		// cvFillConvexPoly( img, buf, count, CV_RGB(rand()&255,rand()&255,rand()&255), CV_AA, 0 );//填充凸多边形  
		for (i = 0; i<count; i++)
		{
			buf1[i].x = buf[i].x;
			buf1[i].y = buf[i].y;
		}
		Mat mat_img(img);

		cvPolyLine(img, &buf, &count, 1, 1, CV_RGB(0, 200, 0), 1, CV_AA, 0);//画出线。  

		//for(int i=0;i<count-1;i++)  
		//{  
		//dashLine(mat_img,buf1[i],buf1[i+1],100);  
		//}  
		//dashLine(mat_img,buf1[i],buf1[0],100);  
		draw_subdiv_point(img, pt->pt, CV_RGB(255, 0, 0));//用黑色画出画出剖分顶点。  
	}
	free(buf);
}
/**********************************************重点部分：如何实现变量所有的Delauany或者Voronoi边*****************************/
static void paint_voronoi(CvSubdiv2D* subdiv, IplImage* img)//画出voronoi面  
{
	CvSeqReader  reader;
	int i, total = subdiv->edges->total;//边缘总数  
	int elem_size = subdiv->edges->elem_size;//边缘的大小  

	cvCalcSubdivVoronoi2D(subdiv);

	cvStartReadSeq((CvSeq*)(subdiv->edges), &reader, 0);

	for (i = 0; i < total; i++)
	{
		CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);//获取四方边缘  

		if (CV_IS_SET_ELEM(edge))//判断边缘是否在边缘集中  
		{
			CvSubdiv2DEdge e = (CvSubdiv2DEdge)edge;//edge是四方边缘的指针，而CvSubdiv2DEdge高位表示四方边缘的指针。  
			//cout<<(e&3)<<endl;//通过测试e低2位即索引值应该设置为0了，即输入边缘  
			// left  
			draw_subdiv_facet(img, cvSubdiv2DRotateEdge(e, 1));//e为Delaunay边，获得Delaunay边对应的voronoi边，即e的旋转边缘  

			// right  
			draw_subdiv_facet(img, cvSubdiv2DRotateEdge(e, 3));//反向的旋转边缘  
		}

		CV_NEXT_SEQ_ELEM(elem_size, reader);//移动到下一个位置  
	}
}
/*************************************************************************************/
void  draw_edge(CvSubdiv2DEdge e0, IplImage *img, const char *cp)
{
	CvSubdiv2DPoint *point1_org = cvSubdiv2DEdgeOrg(e0);
	CvSubdiv2DPoint *point1_dst = cvSubdiv2DEdgeDst(e0);
	CvPoint pt_org = cvPointFrom32f(point1_org->pt);
	CvPoint pt_dst = cvPointFrom32f(point1_dst->pt);
	CvPoint pt;
	pt.x = (pt_org.x + pt_dst.x) / 2;
	pt.y = (pt_org.y + pt_dst.y) / 2;
	CvFont font;
	cvInitFont(&font, CV_FONT_ITALIC, 1, 1, 0, 2, 8);
	cvLine(img, pt_org, pt_dst, cvScalar(255, 0, 0), 2, 8);
	cvCircle(img, pt, 6, cvScalar(0, 0, 0), 2, CV_AA);
	cout << "点位于" << pt.x << " " << pt.y << endl;
	cvPutText(img, cp, pt, &font, cvScalar(0, 255, 0));
	if (pt_dst.y - pt_org.y>0)
	{
		cout << "箭头朝下" << endl;
	}
	else
	{
		cout << "箭头朝上" << endl;
	}

}
struct MyVec3i
{
	Vec3i verticesIdx;
	int weight[];
};
bool isGoodTri(MyVec3i &mv, vector<MyVec3i> & tri)
{
	Vec3i v= mv.verticesIdx;
	int a = v[0], b = v[1], c = v[2];
	v[0] = min(a, min(b, c));//v[0]找到点插入的先后顺序（0....N-1，N为点的个数）的最小值  
	v[2] = max(a, max(b, c));//v[2]存储最大值.  
	v[1] = a + b + c - v[0] - v[2];//v[1]为中间值  
	if (v[0] == -1) return false;

	vector<MyVec3i>::iterator iter = tri.begin();//开始时为空  
	for (; iter != tri.end(); iter++)
	{
		Vec3i &check = iter->verticesIdx;//如果当前待压入的和存储的重复了，则停止返回false。  
		if (check[0] == v[0] &&
			check[1] == v[1] &&
			check[2] == v[2])
		{
			break;
		}
	}
	if (iter == tri.end())
	{
		tri.push_back(mv);
		return true;
	}
	return false;
}
float disCal(float x1, float y1, float x2, float y2){
	return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}
template <class T>
int getArrayLen(T& array)

{
	return (sizeof(array) / sizeof(array[0]));
}

static void int2int128_t(int a, int8_t* i8){
	char buff[4];
	buff[0] = a >> 24;

	buff[1] = a >> 16;

	buff[2] = a >> 8;

	buff[3] = a;

	i8[15] = buff[3];
	i8[14] = buff[2];
	i8[13] = buff[1];
	i8[12] = buff[0];
	for (int i = 0; i < 12; i++){
		i8[i] = 0;
	}
}
/**/
static void run(int8_t* K1, int8_t* K2, int8_t* K3, int8_t* K4)
{
	//初始化可视化数据
	char win[] = "source";
	int i;
	CvRect rect = { 0, -1300, 600, 1300 };//前面两个参数是左上角坐标
	CvMemStorage* storage;
	CvSubdiv2D* subdiv;
	IplImage* img;
	CvScalar active_facet_color, delaunay_color, voronoi_color, bkgnd_color, random_point_color;

	active_facet_color = CV_RGB(255, 0, 0);//红色  
	delaunay_color = CV_RGB(0, 0, 0);//黑色  
	voronoi_color = CV_RGB(0, 180, 0);//绿色  
	bkgnd_color = CV_RGB(255, 255, 255);//白色  
	random_point_color = CV_RGB(200, 0, 0);//绿色 

	img = cvCreateImage(cvSize(rect.width, rect.height), 8, 3);
	cvSet(img, bkgnd_color, 0);

	cvNamedWindow(win, 1);

	storage = cvCreateMemStorage(0);
	subdiv = init_delaunay(storage, rect);

	map<int, CvPoint2D32f> points;
	map<CvPoint2D32f,string> md5Points;//id(p)= md5(p) <-> p  to enc;
	Graphm graph(0);
	
	//1.读取文件中的点,路径为D://opencvdatas.xlsx
	BasicExcel e;
	// Load a workbook with one sheet, display its contents and save into another file.
	e.Load("opencvdatas.xls");
	BasicExcelWorksheet* sheet1 = e.GetWorksheet("Sheet1");
	if (sheet1)
	{
		size_t maxRows = sheet1->GetTotalRows();//行
		graph.Init(maxRows);//初始化图
		size_t maxCols = sheet1->GetTotalCols();//列
		for (size_t r = 0; r < maxRows; ++r)
		{
			printf("%10d", r + 1);
			CvPoint2D32f point = cvPoint2D32f(0, 0);
			for (size_t c = 0; c < maxCols; ++c)

			{
				BasicExcelCell* cell = sheet1->Cell(r, c);
				switch (cell->Type())
				{
				case BasicExcelCell::UNDEFINED:
					printf("          ");
					break;

				case BasicExcelCell::INT:
					printf("%10d", cell->GetInteger());
					break;

				case BasicExcelCell::DOUBLE:
					if (c == 0){
						point.x = cell->GetDouble() * 10;
					}
					else{
						point.y = cell->GetDouble() * 10;
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
		/*	string a;
			float b = point.x;
			float c = point.y;
			
			a = b;*/
			points[r] = point;//字典映射，输入点的index得到point点
			//md5Points[point] = MD5(to_string(point.x) + to_string(point.y)).toString;
		}
	}
	//2.形成三角剖分
	for (int i = 0; i < 10; i++){
		CvPoint2D32f fp = points[i];
		locate_point(subdiv, fp, img, active_facet_color);
		cvShowImage(win, img);//刷新显示  
		if (cvWaitKey(100) >= 0)
			break;
		CvSubdiv2DPoint *pt = cvSubdivDelaunay2DInsert(subdiv, fp);//向三角剖分中插入该点，即对该点进行三角剖分  
		pt->id = i;//给点id
		
		cvSet(img, bkgnd_color, 0);//设置图像的背景颜色为白色  
		draw_subdiv(img, subdiv, delaunay_color, voronoi_color);
		cvShowImage(win, img);

	}
	//3.找出每个点的邻接点，并给出权重（权重的大小代表着距离的大小）
	CvSeqReader reader;//利用CvSeqReader遍历  
	int total = subdiv->edges->total;//边的总数  
	int elem_size = subdiv->edges->elem_size;//边的大小 
	cvStartReadSeq((CvSeq*)(subdiv->edges), &reader, 0);
	MyVec3i ver3iStruct;
	vector<MyVec3i> tri;
	for (int i = 0; i < total; i++)
	{
		CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);

		if (CV_IS_SET_ELEM(edge))
		{
			CvSubdiv2DEdge t = (CvSubdiv2DEdge)edge;
			int iPointNum = 3;
			Scalar color = CV_RGB(rand() & 255, rand() & 255, rand() & 255);
			//Scalar color=CV_RGB(255,0,0);  
			//bool isNeg = false;  
			int j;
			float maxDis = 0;
			float minDis = 0;
			int minDisFlag = 0;
			int maxDisFlag = 0;
			for (j = 0; j < iPointNum; j++)
			{
				CvSubdiv2DPoint* pt = cvSubdiv2DEdgeOrg(t);//获取t边的源点
				CvSubdiv2DPoint* ptDst = cvSubdiv2DEdgeDst(t);//获取t边的终点
				if (!pt) break;
				ver3iStruct.verticesIdx[j] = pt->id;
				//计算权重
				//if (pt->id == -1) isNeg = true;  
				t = cvSubdiv2DGetEdge(t, CV_NEXT_AROUND_LEFT);//获取下一条边  
			}
			if (j != iPointNum) continue;
			if (isGoodTri(ver3iStruct, tri)){
				
			}

			CV_NEXT_SEQ_ELEM(elem_size, reader);
		}
	}
	map<int, map<int,int>> vnei;//存储邻接点
	//4.对graph结构进行输入
	//4.1.对vnei结构进行输入
	for (int m = 0; m < 10; m++){
		map<int, int> maps;
		for (int n = 0; n < tri.size(); n++){ 
			for (int k = 0; k < 3; k++){
				if (m == tri.at(n).verticesIdx[k]){
					switch (k)
					{
					case 0:
						maps[tri.at(n).verticesIdx[1]] = tri.at(n).verticesIdx[1];
						maps[tri.at(n).verticesIdx[2]] = tri.at(n).verticesIdx[2];
						break;

					case 1:
						maps[tri.at(n).verticesIdx[0]] = tri.at(n).verticesIdx[0];
						maps[tri.at(n).verticesIdx[2]] = tri.at(n).verticesIdx[2];
						break;

					case 2:
						maps[tri.at(n).verticesIdx[0]] = tri.at(n).verticesIdx[0];
						maps[tri.at(n).verticesIdx[1]] = tri.at(n).verticesIdx[1];
						break;
					}
				}
			}
		}
		vnei[m] = maps;
	}
	AES* a = new AES();
	//4.2对邻接点进行权重计算
	map<int, vector<int>> mapgraph;
	for (int m = 0; m < 10; m++){
		map<int, int> maps = vnei[m];
		/*int size = maps.size();*/
		float *diss = new float[maps.size()];
		float *weight = new float[maps.size()];
		int n = 0;
		map<int, int>::iterator iter = maps.begin();
		for (;iter != maps.end(); iter++){	
			diss[n] = (disCal(points[m].x, points[iter->first].x, points[m].y, points[iter->first].y));
			n++;
		}
		//初始化
		for (int w = 0;  w < maps.size(); w++){
			weight[w] = w + 1; 
		}
		//对距离进行加密操作，这里无需知道具体到的位置,分配权重
		for (int n = 0; n < maps.size(); n++){
			for (int k = n; k > 0; k--){
				if (diss[n] < diss[k - 1]){
					cout<<--weight[n]<<"\t";
					cout<<++weight[k - 1]<<"\n";
				}
				else{
					break;
				}
			}
		}
		int k = 0;
		iter = maps.begin();
		for (;iter != maps.end(); iter++){
			graph.setEdge(m, iter->first, weight[k]);
			cout <<weight[k]<<"\t"<<diss[k]<<"\n";
			k++;
		}

	}
	//对matrix[][]进行加密（加密ope）
	int** matrix = graph.getMatrix();
	map<string, map<string, int>> ope;//ope表
	for (int i = 0; i < 10; i++){
		map<string, int> maps;
		int8_t* temi = new int8_t[16];
		int2int128_t(i, temi);
		int8_t* temiCipher = new int8_t[16];
		a->aes128_self_aes(temi, K3, temiCipher);
		for (int j = 0; j < 10; j++){
			cout << matrix[i][j] << "\t";
			if (matrix[i][j] == 0){
				//如果是0，则不是邻接点
			}
			else{
				int8_t* temj = new int8_t[16];
				int2int128_t(j, temj);
				int8_t* temjCipher = new int8_t[16];
				a->aes128_self_aes(temj, K4, temjCipher);//得到点的密文
				maps[a->to_string(temjCipher)] = matrix[i][j];
			}
		}
		cout << "\n";
		ope[a->to_string(temiCipher)] = maps;
	}
	//对每个点对应的邻接点进行加密，形成T表
	int8_t gk[16];//K4加密的密文
	int8_t w[16];//128位的明文

	int8_t null[16] = {0};
	vector<int8_t*> ciphers;//对点加密后的密文

	int8_t kw1[16];//K1加密的密文

	map<string, vector<__m128i*>> T;

	for (unsigned int i = 0; i < 10; i++){
		//关键字用索引表示
		int8_t* kw = new int8_t[16];//K2加密的密文,即T的key
		int2int128_t(i, w);//索引整形转128位明文
		a->aes128_self_aes(w, K2, kw);//对128位明文进行加密
		a->aes128_self_aes(w, K1, kw1);//对128位明文进行加密
		map<int, int>::iterator iter = vnei.at(i).begin();
		vector<__m128i*> vec;
		for (; iter != vnei.at(i).end(); iter++){
			__m128i* value = new __m128i[2];//K4加密的密文与vi的链接即T的VALUE
			int2int128_t(iter->first, w);//iter->first:i of GK4(i) 
			a->aes128_self_aes(w, K4, gk);
			value[0] = _mm_loadu_si128((__m128i *) gk);
			value[1] = _mm_loadu_si128((__m128i *) w);

			value[0] = _mm_xor_si128(value[0], _mm_loadu_si128((__m128i *) kw1));
			value[1] = _mm_xor_si128(value[1], _mm_loadu_si128((__m128i *) null));//padding
			vec.push_back(value);
		}
		T[a->to_string(kw)] = vec;//对T进行操作
	}
	//对每个点的包含的信息进行加密,这里用index代替
	for (unsigned int i = 0; i < 10; i++){
		int2int128_t(i, w);//索引整形转128位明文
		int8_t cipher[16];
		a->aes128_self_aes(w, K3, cipher);
		ciphers.push_back(cipher);
	}

	//客户端操作 token(K,W);
	int word = 1;//意思查找第一点所对应的最短距离
	int8_t fw[16];//k1 对应的密文
	int8_t pw[16];//k2 对应的密文
	int8_t tw[16];//k3 对应的密文即OPE的索引

	int2int128_t(word, w);
	a->aes128_self_aes(w, K1, fw);

	a->aes128_self_aes(w, K2, pw);

	a->aes128_self_aes(w, K3, tw);


	int8_t computed_plain[16];

	int8_t cipher_index[16];
	__m128i val[2];
	int minWeight = 10000;
	
	int8_t min_cipher[16];

	// 服务器操作 search
	int size = T[a->to_string(pw)].size();
	for (int i = 0; i < size; i++){
		__m128i* temp = T[a->to_string(pw)].at(i);//T表按索引操作没问题
		val[0] = _mm_xor_si128(temp[0], _mm_loadu_si128((__m128i *) fw));
		val[1] = _mm_xor_si128(temp[1], _mm_loadu_si128((__m128i *) null));
		for (int j = 0; j < 16; j++){
			cipher_index[j] = val[0].m128i_i8[j];//得到索引的密文即GK4(i)
		}
		
		int tempWeight = ope[a->to_string(tw)][a->to_string(cipher_index)];
		if (tempWeight < minWeight){
			minWeight = tempWeight;
			for (int k = 0; k < 16; k++){
				min_cipher[k] = cipher_index[k];
			}
			a->aes128_self_unaes(min_cipher, K4, computed_plain);
		}
		//a->aes128_self_unaes(cipher_index, K4, computed_plain);
	}

	a->aes128_self_unaes(min_cipher, K4, computed_plain);
	cout << computed_plain[15];
}

int main(int argc, char** argv)
{
	(void)argc; (void)argv;
	AES* a = new AES();
	int8_t K1[16];
	//int8_t plain[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
	//int8_t enc_key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	//int8_t cipher[] = { 0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb, 0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32 };
	//int8_t computed_cipher[16];
	//int8_t computed_plain[16];
	int8_t K2[16];
	int8_t K3[16];
	int8_t K4[16];
	a->ase128_random_key(K1);
	a->ase128_random_key(K2);
	a->ase128_random_key(K3);
	a->ase128_random_key(K4);
	help();
	run(K1,K2,K3,K4);


	//int2int128_t(2000, K1);
	system("pause");
}

#ifdef _EiC  
main(1, "delaunay.c");
#endif  