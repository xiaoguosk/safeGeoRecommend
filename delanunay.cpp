#include <opencv2/imgproc/imgproc_c.h>  
#include <opencv2/legacy/legacy.hpp>  
#include "opencv2/highgui/highgui.hpp"  
#include<opencv2\opencv.hpp>  
#include<iostream>  
#include <stdio.h>  
#include <time.h>
using namespace std;
using namespace cv;
#define GREY_COLOR  CV_RGB(200, 200, 200)//灰色  

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
	cvCircle(img, cvPoint(cvRound(fp.x), cvRound(fp.y)), 6, CV_RGB(0, 0, 0), 1, 8, 0);
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
		if (!((org.x == 1800 && org.y == 0)||(org.x == 0&&org.y == 1800) || (org.x == -1800&&org.y == -1800))){
			iorg = cvPoint(cvRound(org.x), cvRound(org.y));
			idst = cvPoint(cvRound(dst.x), cvRound(dst.y));

			cvLine(img, iorg, idst, color, 1, CV_AA, 0);
		}
		
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
struct edge
{
	Point2d pt1;
	Point2d pt2;
};
static vector<edge> pts;
#define SEGMENT  10
void dashLine(Mat &img, Point2d& pt1, Point2d& pt2, int n)//n为虚线段数  
{
	edge e;
	e.pt1 = pt1;
	e.pt2 = pt2;
	bool b = true;
	for (int i = 0; i < pts.size(); i++){
		if (((pt1.x == pts[i].pt1.x && pt1.y == pts[i].pt1.y && pt2.x == pts[i].pt2.x
			&& pt2.y == pts[i].pt2.y) || (pt2.x == pts[i].pt1.x && pt2.y == pts[i].pt1.y && pt1.x == pts[i].pt2.x
			&& pt1.y == pts[i].pt2.y))){
			b = false;
			break;
		}
	}
	
	if (b){
		Point sub = pt2 - pt1;

		float lineLen = sqrt((sub.x * sub.x + sub.y * sub.y));
		n = lineLen / SEGMENT;
		
		for (int i = 0; i < 2 * n; i += 2)
		{
			line(img, Point(pt1.x + sub.x * i / (2 * n - 1), pt1.y + sub.y * i / (2 * n - 1)), Point(pt1.x + sub.x * (i + 1) / (2 * n - 1), pt1.y + sub.y * (i + 1) / (2 * n - 1)), Scalar(0, 0, 0), 1.5);
		}
		pts.push_back(e);
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
		if (!pt) break;//如果得不到该源点，则退出循环 '
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

		
		//cvPolyLine(img, &buf, &count, 1, 1, CV_RGB(200, 200, 200), 1, 4, 0);//画出线。  

		for(int i=0;i<count-1;i = i + 2)  
		{  
			dashLine(mat_img,buf1[i],buf1[i+1],100);  
		}  
		

		draw_subdiv_point(img, pt->pt, GREY_COLOR);//用黑色画出画出剖分顶点。  
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
static void run(void){
	char win[] = "source";
	int i;
	CvRect rect = {0, 0, 600, 600 };
	CvMemStorage* storage;
	CvSubdiv2D* subdiv;
	IplImage* img;
	CvScalar active_facet_color, delaunay_color, voronoi_color, bkgnd_color, random_point_color;

	active_facet_color = CV_RGB(200, 200, 200);//灰色  
	delaunay_color = CV_RGB(0, 0, 0);//黑色  
	voronoi_color = CV_RGB(0, 180, 0);//绿色  
	bkgnd_color = CV_RGB(255, 255, 255);//白色  
	random_point_color = CV_RGB(200, 0, 0);//绿色 

	img = cvCreateImage(cvSize(rect.width, rect.height), 8, 3);
	cvSet(img, bkgnd_color, 0);

	cvNamedWindow(win, 1);

	storage = cvCreateMemStorage(0);
	subdiv = init_delaunay(storage, rect);

	printf("Delaunay triangulation will be build now interactively.\n"
		"To stop the process, press any key\n\n");

	vector<CvPoint2D32f> points;
	//srand((unsigned)time(0));
	for (i = 0; i < 8; i++)
	{
		double x = (float)(rand() % (590));
		double y = (float)(rand() % (590));
		cout << "x = " << x << "y = " << y;
		CvPoint2D32f fp = cvPoint2D32f(x, y);//使点约束在距离边框10像素之内。 
		points.push_back(fp);

		locate_point(subdiv, fp, img, active_facet_color);//定位点的位置，并画出点所在voronoi面的边。  
		cvShowImage(win, img);//刷新显示  

		if (cvWaitKey(100) >= 0)
			break;

		cvSubdivDelaunay2DInsert(subdiv, fp);//向三角剖分中插入该点，即对该点进行三角剖分  
		cvCalcSubdivVoronoi2D(subdiv);//计算Voronoi细分，有时候我们不需要  
		cvSet(img, bkgnd_color, 0);//设置图像的背景颜色为白色  
		draw_subdiv(img, subdiv, delaunay_color, voronoi_color);
		cvShowImage(win, img);

		//cout << "(" << cvSubdiv2DEdgeOrg(subdiv->recent_edge)->pt.x << "," << cvSubdiv2DEdgeOrg(subdiv->recent_edge)->pt.y << ")->" << "(" << cvSubdiv2DEdgeDst(subdiv->recent_edge)->pt.x << "," << cvSubdiv2DEdgeDst(subdiv->recent_edge)->pt.y << ")" << "\n";
		//cvWaitKey(); 	
		if (cvWaitKey(100) >= 0)
			break;
	}
	CvSet* cs = subdiv->edges;
	for (int i = 0; i < cs->total; i++){
		CvSubdiv2DEdge edge = (CvSubdiv2DEdge)cvGetSetElem(cs, i);
		if (edge != 0){
			CvPoint2D32f pointOrg = cvSubdiv2DEdgeOrg(edge)->pt;
			CvPoint2D32f pointDst = cvSubdiv2DEdgeDst(edge)->pt;
			cout << "(" << pointOrg.x << "," << pointOrg.y << ")->" << "(" << pointDst.x << ","
				<< pointDst.y << ")" << "\n";
		}

	}
	for (int i = 0; i<points.size(); i++)
		draw_subdiv_point(img, points[i], active_facet_color);
		cvShowImage(win, img);
		cvWaitKey();

	//  cvSet( img, bkgnd_color, 0 );//重新刷新画布，即设置背景颜色为白色  
	paint_voronoi(subdiv, img);//画出细分  
	char* filename2 = "E:\\研究生\\delanunay.pdf"; //图像名
	cvSaveImage(filename2, img);

	//CvPoint2D32f point1 = cvPoint2D32f(105, 358);//图像中心选择一点。  
	//draw_subdiv_point(img, point1, random_point_color);
	//
	//CvSubdiv2DEdge e;
	//CvSubdiv2DEdge e0 = 0;
	//CvSubdiv2DPoint* p = 0;
	////从三角剖分得到其存在的位置
	//CvSubdiv2DPointLocation loc = cvSubdiv2DLocate(subdiv, point1, &e0, &p);
	//const int r = 100;

	cvShowImage(win, img);//  
	cvWaitKey(0);


	cvReleaseMemStorage(&storage);
	cvReleaseImage(&img);
	cvDestroyWindow(win);
}
//
int main(int argc, char** argv)
{
	(void)argc; (void)argv;
	help();
	run();
	return 0;
}

#ifdef _EiC  
main(1, "delaunay.c");
#endif  