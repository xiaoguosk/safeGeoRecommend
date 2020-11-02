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
	printf("\nThis program demostrates iterative construction of\n"//������������delaunay�ʷֺ�voronoiϸ�ֵĵ�������  
		"delaunay triangulation and voronoi tesselation.\n"
		"It draws a random set of points in an image and then delaunay triangulates them.\n"//��ͼ���ϻ���һЩ����㣬Ȼ�����delaunay�����ʷ�  
		"Usage: \n"
		"./delaunay \n"
		"\nThis program builds the traingulation interactively, you may stop this process by\n"
		"hitting any key.\n");//�������������ʷ֣������ֹͣ���������  
}


static CvSubdiv2D* init_delaunay(CvMemStorage* storage,//��ʼ�������ʷֽṹ��Ϊ����䵥Ԫ  
	CvRect rect)
{
	CvSubdiv2D* subdiv;//�����ʷֵ����ݵ�Ԫ  

	subdiv = cvCreateSubdiv2D(CV_SEQ_KIND_SUBDIV2D, sizeof(*subdiv),
		sizeof(CvSubdiv2DPoint),
		sizeof(CvQuadEdge2D),
		storage);
	cvInitSubdivDelaunay2D(subdiv, rect);

	return subdiv;
}


static void draw_subdiv_point(IplImage* img, CvPoint2D32f fp, CvScalar color)//���������ʷֵĶ���  
{
	cvCircle(img, cvPoint(cvRound(fp.x), cvRound(fp.y)), 5, color, CV_FILLED, 8, 0);
}


static void draw_subdiv_edge(IplImage* img, CvSubdiv2DEdge edge, CvScalar color)//���������ʷֵı�  
{
	CvSubdiv2DPoint* org_pt;//Դ����  
	CvSubdiv2DPoint* dst_pt;//Ŀ�ض���  
	CvPoint2D32f org;
	CvPoint2D32f dst;
	CvPoint iorg, idst;

	org_pt = cvSubdiv2DEdgeOrg(edge);//ͨ���߻�ȡ����  
	dst_pt = cvSubdiv2DEdgeDst(edge);

	if (org_pt && dst_pt)//��������˵㲻Ϊ��  
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
	CvScalar delaunay_color, CvScalar voronoi_color)//�����ʷֺ�ϸ��  
{
	CvSeqReader  reader;
	int i, total = subdiv->edges->total;//�ߵ�����  
	int elem_size = subdiv->edges->elem_size;//�ߵĴ�С  
	cout << typeid(subdiv->edges).name() << endl;

	cvStartReadSeq((CvSeq*)(subdiv->edges), &reader, 0);//ʹ��CvSeqReader����Delaunay����Voronoi��  

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


static void locate_point(CvSubdiv2D* subdiv, CvPoint2D32f fp, IplImage* img,//���������ʷֵı�  
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

//@author andme-��Ŀ�Ӿ�  
void dashLine(Mat &img, Point2d& pt1, Point2d& pt2, int n)//nΪ���߶���  
{
	Point sub = pt2 - pt1;
	for (int i = 0; i < 2 * n; i += 2)
	{
		line(img, Point(pt1.x + sub.x * i / (2 * n - 1), pt1.y + sub.y * i / (2 * n - 1)), Point(pt1.x + sub.x * (i + 1) / (2 * n - 1), pt1.y + sub.y * (i + 1) / (2 * n - 1)), Scalar(0, 255, 0), 2);
	}
}



//������ʽdraw_subdiv_facet( img, cvSubdiv2DRotateEdge( e, 1 ));  
static void draw_subdiv_facet(IplImage* img, CvSubdiv2DEdge edge)//����voronoi��   
{
	//cout<<edge<<endl;//edge����λ��ʾ��ʾ��������λ��ʾ�ķ���Եָ�롣  
	//cout<<(edge&3)<<endl;  
	CvSubdiv2DEdge t = edge;//�����ǰ�����ĵ�����ʽʱ��edgeΪeRot��  
	int i, count = 0;
	CvPoint* buf = 0;
	Point2d *buf1 = 0;

	// count number of edges in facet //���ڱߵļ���  
	do
	{
		count++;
		t = cvSubdiv2DGetEdge(t, CV_NEXT_AROUND_LEFT);
	} while (t != edge);//��������һ��voronoi��Ԫһ�ܣ�������vornonoi��Ե��ӵ�еı�Ե����  

	buf = (CvPoint*)malloc(count * sizeof(buf[0]));
	buf1 = (Point2d*)malloc(count*sizeof(buf1[0]));

	// gather points  
	t = edge;
	for (i = 0; i < count; i++)
	{
		CvSubdiv2DPoint* pt = cvSubdiv2DEdgeOrg(t);//��һ�λ�ȡeRot��Ե����ʼ��  
		if (!pt) break;//����ò�����Դ�㣬���˳�ѭ��  
		buf[i] = cvPoint(cvRound(pt->pt.x), cvRound(pt->pt.y));//���õ�ת��ΪcvPoint���͵㣬�洢��buf��  
		t = cvSubdiv2DGetEdge(t, CV_NEXT_AROUND_LEFT);//Ȼ������vornonoi��Ԫ������ת��  
	}

	if (i == count)//������еĵ㶼�洢�����ˡ�  
	{
		CvSubdiv2DPoint* pt = cvSubdiv2DEdgeDst(cvSubdiv2DRotateEdge(edge, 1));//����eRot����ת��ԵӦ����reversed e,��ôĿ�ĵ㣬����e��Դ�㡣  
		// cvFillConvexPoly( img, buf, count, CV_RGB(rand()&255,rand()&255,rand()&255), CV_AA, 0 );//���͹�����  
		for (i = 0; i<count; i++)
		{
			buf1[i].x = buf[i].x;
			buf1[i].y = buf[i].y;
		}
		Mat mat_img(img);

		cvPolyLine(img, &buf, &count, 1, 1, CV_RGB(0, 200, 0), 1, CV_AA, 0);//�����ߡ�  

		//for(int i=0;i<count-1;i++)  
		//{  
		//dashLine(mat_img,buf1[i],buf1[i+1],100);  
		//}  
		//dashLine(mat_img,buf1[i],buf1[0],100);  
		draw_subdiv_point(img, pt->pt, CV_RGB(255, 0, 0));//�ú�ɫ���������ʷֶ��㡣  
	}
	free(buf);
}
/**********************************************�ص㲿�֣����ʵ�ֱ������е�Delauany����Voronoi��*****************************/
static void paint_voronoi(CvSubdiv2D* subdiv, IplImage* img)//����voronoi��  
{
	CvSeqReader  reader;
	int i, total = subdiv->edges->total;//��Ե����  
	int elem_size = subdiv->edges->elem_size;//��Ե�Ĵ�С  

	cvCalcSubdivVoronoi2D(subdiv);

	cvStartReadSeq((CvSeq*)(subdiv->edges), &reader, 0);

	for (i = 0; i < total; i++)
	{
		CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);//��ȡ�ķ���Ե  

		if (CV_IS_SET_ELEM(edge))//�жϱ�Ե�Ƿ��ڱ�Ե����  
		{
			CvSubdiv2DEdge e = (CvSubdiv2DEdge)edge;//edge���ķ���Ե��ָ�룬��CvSubdiv2DEdge��λ��ʾ�ķ���Ե��ָ�롣  
			//cout<<(e&3)<<endl;//ͨ������e��2λ������ֵӦ������Ϊ0�ˣ��������Ե  
			// left  
			draw_subdiv_facet(img, cvSubdiv2DRotateEdge(e, 1));//eΪDelaunay�ߣ����Delaunay�߶�Ӧ��voronoi�ߣ���e����ת��Ե  

			// right  
			draw_subdiv_facet(img, cvSubdiv2DRotateEdge(e, 3));//�������ת��Ե  
		}

		CV_NEXT_SEQ_ELEM(elem_size, reader);//�ƶ�����һ��λ��  
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
	cout << "��λ��" << pt.x << " " << pt.y << endl;
	cvPutText(img, cp, pt, &font, cvScalar(0, 255, 0));
	if (pt_dst.y - pt_org.y>0)
	{
		cout << "��ͷ����" << endl;
	}
	else
	{
		cout << "��ͷ����" << endl;
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
	v[0] = min(a, min(b, c));//v[0]�ҵ��������Ⱥ�˳��0....N-1��NΪ��ĸ���������Сֵ  
	v[2] = max(a, max(b, c));//v[2]�洢���ֵ.  
	v[1] = a + b + c - v[0] - v[2];//v[1]Ϊ�м�ֵ  
	if (v[0] == -1) return false;

	vector<MyVec3i>::iterator iter = tri.begin();//��ʼʱΪ��  
	for (; iter != tri.end(); iter++)
	{
		Vec3i &check = iter->verticesIdx;//�����ǰ��ѹ��ĺʹ洢���ظ��ˣ���ֹͣ����false��  
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
	//��ʼ�����ӻ�����
	char win[] = "source";
	int i;
	CvRect rect = { 0, -1300, 600, 1300 };//ǰ���������������Ͻ�����
	CvMemStorage* storage;
	CvSubdiv2D* subdiv;
	IplImage* img;
	CvScalar active_facet_color, delaunay_color, voronoi_color, bkgnd_color, random_point_color;

	active_facet_color = CV_RGB(255, 0, 0);//��ɫ  
	delaunay_color = CV_RGB(0, 0, 0);//��ɫ  
	voronoi_color = CV_RGB(0, 180, 0);//��ɫ  
	bkgnd_color = CV_RGB(255, 255, 255);//��ɫ  
	random_point_color = CV_RGB(200, 0, 0);//��ɫ 

	img = cvCreateImage(cvSize(rect.width, rect.height), 8, 3);
	cvSet(img, bkgnd_color, 0);

	cvNamedWindow(win, 1);

	storage = cvCreateMemStorage(0);
	subdiv = init_delaunay(storage, rect);

	map<int, CvPoint2D32f> points;
	map<CvPoint2D32f,string> md5Points;//id(p)= md5(p) <-> p  to enc;
	Graphm graph(0);
	
	//1.��ȡ�ļ��еĵ�,·��ΪD://opencvdatas.xlsx
	BasicExcel e;
	// Load a workbook with one sheet, display its contents and save into another file.
	e.Load("opencvdatas.xls");
	BasicExcelWorksheet* sheet1 = e.GetWorksheet("Sheet1");
	if (sheet1)
	{
		size_t maxRows = sheet1->GetTotalRows();//��
		graph.Init(maxRows);//��ʼ��ͼ
		size_t maxCols = sheet1->GetTotalCols();//��
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
			points[r] = point;//�ֵ�ӳ�䣬������index�õ�point��
			//md5Points[point] = MD5(to_string(point.x) + to_string(point.y)).toString;
		}
	}
	//2.�γ������ʷ�
	for (int i = 0; i < 10; i++){
		CvPoint2D32f fp = points[i];
		locate_point(subdiv, fp, img, active_facet_color);
		cvShowImage(win, img);//ˢ����ʾ  
		if (cvWaitKey(100) >= 0)
			break;
		CvSubdiv2DPoint *pt = cvSubdivDelaunay2DInsert(subdiv, fp);//�������ʷ��в���õ㣬���Ըõ���������ʷ�  
		pt->id = i;//����id
		
		cvSet(img, bkgnd_color, 0);//����ͼ��ı�����ɫΪ��ɫ  
		draw_subdiv(img, subdiv, delaunay_color, voronoi_color);
		cvShowImage(win, img);

	}
	//3.�ҳ�ÿ������ڽӵ㣬������Ȩ�أ�Ȩ�صĴ�С�����ž���Ĵ�С��
	CvSeqReader reader;//����CvSeqReader����  
	int total = subdiv->edges->total;//�ߵ�����  
	int elem_size = subdiv->edges->elem_size;//�ߵĴ�С 
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
				CvSubdiv2DPoint* pt = cvSubdiv2DEdgeOrg(t);//��ȡt�ߵ�Դ��
				CvSubdiv2DPoint* ptDst = cvSubdiv2DEdgeDst(t);//��ȡt�ߵ��յ�
				if (!pt) break;
				ver3iStruct.verticesIdx[j] = pt->id;
				//����Ȩ��
				//if (pt->id == -1) isNeg = true;  
				t = cvSubdiv2DGetEdge(t, CV_NEXT_AROUND_LEFT);//��ȡ��һ����  
			}
			if (j != iPointNum) continue;
			if (isGoodTri(ver3iStruct, tri)){
				
			}

			CV_NEXT_SEQ_ELEM(elem_size, reader);
		}
	}
	map<int, map<int,int>> vnei;//�洢�ڽӵ�
	//4.��graph�ṹ��������
	//4.1.��vnei�ṹ��������
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
	//4.2���ڽӵ����Ȩ�ؼ���
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
		//��ʼ��
		for (int w = 0;  w < maps.size(); w++){
			weight[w] = w + 1; 
		}
		//�Ծ�����м��ܲ�������������֪�����嵽��λ��,����Ȩ��
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
	//��matrix[][]���м��ܣ�����ope��
	int** matrix = graph.getMatrix();
	map<string, map<string, int>> ope;//ope��
	for (int i = 0; i < 10; i++){
		map<string, int> maps;
		int8_t* temi = new int8_t[16];
		int2int128_t(i, temi);
		int8_t* temiCipher = new int8_t[16];
		a->aes128_self_aes(temi, K3, temiCipher);
		for (int j = 0; j < 10; j++){
			cout << matrix[i][j] << "\t";
			if (matrix[i][j] == 0){
				//�����0�������ڽӵ�
			}
			else{
				int8_t* temj = new int8_t[16];
				int2int128_t(j, temj);
				int8_t* temjCipher = new int8_t[16];
				a->aes128_self_aes(temj, K4, temjCipher);//�õ��������
				maps[a->to_string(temjCipher)] = matrix[i][j];
			}
		}
		cout << "\n";
		ope[a->to_string(temiCipher)] = maps;
	}
	//��ÿ�����Ӧ���ڽӵ���м��ܣ��γ�T��
	int8_t gk[16];//K4���ܵ�����
	int8_t w[16];//128λ������

	int8_t null[16] = {0};
	vector<int8_t*> ciphers;//�Ե���ܺ������

	int8_t kw1[16];//K1���ܵ�����

	map<string, vector<__m128i*>> T;

	for (unsigned int i = 0; i < 10; i++){
		//�ؼ�����������ʾ
		int8_t* kw = new int8_t[16];//K2���ܵ�����,��T��key
		int2int128_t(i, w);//��������ת128λ����
		a->aes128_self_aes(w, K2, kw);//��128λ���Ľ��м���
		a->aes128_self_aes(w, K1, kw1);//��128λ���Ľ��м���
		map<int, int>::iterator iter = vnei.at(i).begin();
		vector<__m128i*> vec;
		for (; iter != vnei.at(i).end(); iter++){
			__m128i* value = new __m128i[2];//K4���ܵ�������vi�����Ӽ�T��VALUE
			int2int128_t(iter->first, w);//iter->first:i of GK4(i) 
			a->aes128_self_aes(w, K4, gk);
			value[0] = _mm_loadu_si128((__m128i *) gk);
			value[1] = _mm_loadu_si128((__m128i *) w);

			value[0] = _mm_xor_si128(value[0], _mm_loadu_si128((__m128i *) kw1));
			value[1] = _mm_xor_si128(value[1], _mm_loadu_si128((__m128i *) null));//padding
			vec.push_back(value);
		}
		T[a->to_string(kw)] = vec;//��T���в���
	}
	//��ÿ����İ�������Ϣ���м���,������index����
	for (unsigned int i = 0; i < 10; i++){
		int2int128_t(i, w);//��������ת128λ����
		int8_t cipher[16];
		a->aes128_self_aes(w, K3, cipher);
		ciphers.push_back(cipher);
	}

	//�ͻ��˲��� token(K,W);
	int word = 1;//��˼���ҵ�һ������Ӧ����̾���
	int8_t fw[16];//k1 ��Ӧ������
	int8_t pw[16];//k2 ��Ӧ������
	int8_t tw[16];//k3 ��Ӧ�����ļ�OPE������

	int2int128_t(word, w);
	a->aes128_self_aes(w, K1, fw);

	a->aes128_self_aes(w, K2, pw);

	a->aes128_self_aes(w, K3, tw);


	int8_t computed_plain[16];

	int8_t cipher_index[16];
	__m128i val[2];
	int minWeight = 10000;
	
	int8_t min_cipher[16];

	// ���������� search
	int size = T[a->to_string(pw)].size();
	for (int i = 0; i < size; i++){
		__m128i* temp = T[a->to_string(pw)].at(i);//T����������û����
		val[0] = _mm_xor_si128(temp[0], _mm_loadu_si128((__m128i *) fw));
		val[1] = _mm_xor_si128(temp[1], _mm_loadu_si128((__m128i *) null));
		for (int j = 0; j < 16; j++){
			cipher_index[j] = val[0].m128i_i8[j];//�õ����������ļ�GK4(i)
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