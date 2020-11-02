#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "BasicExcelVC6.hpp"
#include <windows.h>

using namespace std;
using namespace YExcel;
using namespace cv;
using namespace std;

static int leafCount = 0;
CvSubdiv2D * cvCreateSubdiv2D(int subdiv_type, int header_size,
int vtx_size, int quadedge_size, CvMemStorage * storage)
{
	
	if (!storage)
		CV_Error(CV_StsNullPtr, "");

	if (header_size < (int)sizeof(CvSubdiv2D) ||
		quadedge_size < (int)sizeof(CvQuadEdge2D) ||
		vtx_size < (int)sizeof(CvSubdiv2DPoint))
		CV_Error(CV_StsBadSize, "");

	return (CvSubdiv2D *)cvCreateGraph(subdiv_type, header_size,
		vtx_size, quadedge_size, storage);
}

static CvSubdiv2DPoint * cvSubdiv2DAddPoint(CvSubdiv2D * subdiv, CvPoint2D32f pt, int is_virtual)
{
	CvSubdiv2DPoint* subdiv_point = (CvSubdiv2DPoint*)cvSetNew((CvSet*)subdiv);
	if (subdiv_point)
	{
		memset(subdiv_point, 0, subdiv->elem_size);
		subdiv_point->pt = pt;
		subdiv_point->first = 0;
		subdiv_point->flags |= is_virtual ? CV_SUBDIV2D_VIRTUAL_POINT_FLAG : 0;
		subdiv_point->id = -1;
	}

	return subdiv_point;
}

static CvSubdiv2DEdge
cvSubdiv2DMakeEdge(CvSubdiv2D * subdiv)
{
	if (!subdiv)
		CV_Error(CV_StsNullPtr, "");
	CvQuadEdge2D* edge = (CvQuadEdge2D*)cvSetNew((CvSet*)subdiv->edges);
	memset(edge->pt, 0, sizeof(edge->pt));
	CvSubdiv2DEdge edgehandle = (CvSubdiv2DEdge)edge;

	edge->next[0] = edgehandle;
	edge->next[1] = edgehandle + 3;
	edge->next[2] = edgehandle + 2;
	edge->next[3] = edgehandle + 1;

	subdiv->quad_edges++;
	return edgehandle;
}


static void
cvSubdiv2DSetEdgePoints(CvSubdiv2DEdge edge,
CvSubdiv2DPoint * org_pt, CvSubdiv2DPoint * dst_pt)
{
	CvQuadEdge2D *quadedge = (CvQuadEdge2D *)(edge & ~3);

	if (!quadedge)
		CV_Error(CV_StsNullPtr, "");

	quadedge->pt[edge & 3] = org_pt;
	quadedge->pt[(edge + 2) & 3] = dst_pt;
}


CvSubdiv2DEdge  cvSubdiv2DRotateEdge(CvSubdiv2DEdge edge, int rotate)
{
	return  (edge & ~3) + ((edge + rotate) & 3);
}
CV_INLINE  CvSubdiv2DEdge  cvSubdiv2DSymEdge(CvSubdiv2DEdge edge)
{
	return edge ^ 2;
}
static void
cvSubdiv2DSplice(CvSubdiv2DEdge edgeA, CvSubdiv2DEdge edgeB)
{
	CvSubdiv2DEdge *a_next = &CV_SUBDIV2D_NEXT_EDGE(edgeA);
	CvSubdiv2DEdge *b_next = &CV_SUBDIV2D_NEXT_EDGE(edgeB);
	CvSubdiv2DEdge a_rot = cvSubdiv2DRotateEdge(*a_next, 1);
	CvSubdiv2DEdge b_rot = cvSubdiv2DRotateEdge(*b_next, 1);
	CvSubdiv2DEdge *a_rot_next = &CV_SUBDIV2D_NEXT_EDGE(a_rot);
	CvSubdiv2DEdge *b_rot_next = &CV_SUBDIV2D_NEXT_EDGE(b_rot);
	CvSubdiv2DEdge t;

	CV_SWAP(*a_next, *b_next, t);
	CV_SWAP(*a_rot_next, *b_rot_next, t);
}

void cvInitSubdivDelaunay2D(CvSubdiv2D * subdiv, CvRect rect)
{
	float big_coord = 3.f * MAX(rect.width, rect.height);
	CvPoint2D32f ppA, ppB, ppC;
	CvSubdiv2DPoint *pA, *pB, *pC;
	CvSubdiv2DEdge edge_AB, edge_BC, edge_CA;
	float rx = (float)rect.x;
	float ry = (float)rect.y;

	if (!subdiv)
		CV_Error(CV_StsNullPtr, "");

	cvClearSet((CvSet *)(subdiv->edges));
	cvClearSet((CvSet *)subdiv);

	subdiv->quad_edges = 0;
	subdiv->recent_edge = 0;
	subdiv->is_geometry_valid = 0;

	subdiv->topleft = cvPoint2D32f(rx, ry);
	subdiv->bottomright = cvPoint2D32f(rx + rect.width, ry + rect.height);

	ppA = cvPoint2D32f(rx + big_coord, ry);
	ppB = cvPoint2D32f(rx, ry + big_coord);
	ppC = cvPoint2D32f(rx - big_coord, ry - big_coord);

	pA = cvSubdiv2DAddPoint(subdiv, ppA, 0);
	pB = cvSubdiv2DAddPoint(subdiv, ppB, 0);
	pC = cvSubdiv2DAddPoint(subdiv, ppC, 0);

	edge_AB = cvSubdiv2DMakeEdge(subdiv);
	edge_BC = cvSubdiv2DMakeEdge(subdiv);
	edge_CA = cvSubdiv2DMakeEdge(subdiv);

	cvSubdiv2DSetEdgePoints(edge_AB, pA, pB);
	cvSubdiv2DSetEdgePoints(edge_BC, pB, pC);
	cvSubdiv2DSetEdgePoints(edge_CA, pC, pA);

	cvSubdiv2DSplice(edge_AB, cvSubdiv2DSymEdge(edge_CA));
	cvSubdiv2DSplice(edge_BC, cvSubdiv2DSymEdge(edge_AB));
	cvSubdiv2DSplice(edge_CA, cvSubdiv2DSymEdge(edge_BC));

	subdiv->recent_edge = edge_AB;
}
CV_INLINE  CvSubdiv2DPoint*  cvSubdiv2DEdgeOrg(CvSubdiv2DEdge edge)
{
	CvQuadEdge2D* e = (CvQuadEdge2D*)(edge & ~3);
	return (CvSubdiv2DPoint*)e->pt[edge & 3];
}
CV_INLINE  CvSubdiv2DPoint*  cvSubdiv2DEdgeDst(CvSubdiv2DEdge edge)
{
	CvQuadEdge2D* e = (CvQuadEdge2D*)(edge & ~3);
	return (CvSubdiv2DPoint*)e->pt[(edge + 2) & 3];

}
CV_INLINE  CvSubdiv2DEdge  cvSubdiv2DNextEdge(CvSubdiv2DEdge edge)
{
	return  CV_SUBDIV2D_NEXT_EDGE(edge);
}
CV_INLINE  CvSubdiv2DEdge  cvSubdiv2DGetEdge(CvSubdiv2DEdge edge, CvNextEdgeType type)
{
	CvQuadEdge2D* e = (CvQuadEdge2D*)(edge & ~3);
	edge = e->next[(edge + (int)type) & 3];
	return  (edge & ~3) + ((edge + ((int)type >> 4)) & 3);
}


CV_INLINE  double  cvTriangleArea(CvPoint2D32f a, CvPoint2D32f b, CvPoint2D32f c)
{
	return ((double)b.x - a.x) * ((double)c.y - a.y) - ((double)b.y - a.y) * ((double)c.x - a.x);
}

static int
icvIsRightOf(CvPoint2D32f& pt, CvSubdiv2DEdge edge)
{
	CvSubdiv2DPoint *org = cvSubdiv2DEdgeOrg(edge), *dst = cvSubdiv2DEdgeDst(edge);
	double cw_area = cvTriangleArea(pt, dst->pt, org->pt);

	return (cw_area > 0) - (cw_area < 0);
}

CvSubdiv2DPointLocation cvSubdiv2DLocate(CvSubdiv2D * subdiv, CvPoint2D32f pt,
CvSubdiv2DEdge * _edge, CvSubdiv2DPoint ** _point)
{
	CvSubdiv2DPoint *point = 0;
	int right_of_curr = 0;

	if (!subdiv)
		CV_Error(CV_StsNullPtr, "");

	if (!CV_IS_SUBDIV2D(subdiv))
		CV_Error(CV_StsBadFlag, "");

	int i, max_edges = subdiv->quad_edges * 4;
	CvSubdiv2DEdge edge = subdiv->recent_edge;

	if (max_edges == 0)
		CV_Error(CV_StsBadSize, "");
	CV_Assert(edge != 0);

	if (pt.x < subdiv->topleft.x || pt.y < subdiv->topleft.y ||
		pt.x >= subdiv->bottomright.x || pt.y >= subdiv->bottomright.y)
		CV_Error(CV_StsOutOfRange, "");

	CvSubdiv2DPointLocation location = CV_PTLOC_ERROR;

	right_of_curr = icvIsRightOf(pt, edge);
	if (right_of_curr > 0)
	{
		edge = cvSubdiv2DSymEdge(edge);
		right_of_curr = -right_of_curr;
	}

	for (i = 0; i < max_edges; i++)
	{
		
		CvSubdiv2DEdge onext_edge = cvSubdiv2DNextEdge(edge);
		CvSubdiv2DEdge dprev_edge = cvSubdiv2DGetEdge(edge, CV_PREV_AROUND_DST);

		int right_of_onext = icvIsRightOf(pt, onext_edge);
		int right_of_dprev = icvIsRightOf(pt, dprev_edge);

		if (right_of_dprev > 0)
		{
			if (right_of_onext > 0 || (right_of_onext == 0 && right_of_curr == 0))
			{
				location = CV_PTLOC_INSIDE;
				goto exit;
			}
			else
			{
				right_of_curr = right_of_onext;
				edge = onext_edge;
			}
		}
		else
		{
			if (right_of_onext > 0)
			{
				if (right_of_dprev == 0 && right_of_curr == 0)
				{
					location = CV_PTLOC_INSIDE;
					goto exit;
				}
				else
				{
					right_of_curr = right_of_dprev;
					edge = dprev_edge;
				}
			}
			else if (right_of_curr == 0 &&
				icvIsRightOf(cvSubdiv2DEdgeDst(onext_edge)->pt, edge) >= 0)
			{
				edge = cvSubdiv2DSymEdge(edge);
			}
			else
			{
				right_of_curr = right_of_onext;
				edge = onext_edge;
			}
		}
	}
exit:

	subdiv->recent_edge = edge;

	if (location == CV_PTLOC_INSIDE)
	{
		double t1, t2, t3;
		CvPoint2D32f org_pt = cvSubdiv2DEdgeOrg(edge)->pt;
		CvPoint2D32f dst_pt = cvSubdiv2DEdgeDst(edge)->pt;

		t1 = fabs(pt.x - org_pt.x);
		t1 += fabs(pt.y - org_pt.y);
		t2 = fabs(pt.x - dst_pt.x);
		t2 += fabs(pt.y - dst_pt.y);
		t3 = fabs(org_pt.x - dst_pt.x);
		t3 += fabs(org_pt.y - dst_pt.y);

		if (t1 < FLT_EPSILON)
		{
			location = CV_PTLOC_VERTEX;
			point = cvSubdiv2DEdgeOrg(edge);
			edge = 0;
		}
		else if (t2 < FLT_EPSILON)
		{
			location = CV_PTLOC_VERTEX;
			point = cvSubdiv2DEdgeDst(edge);
			edge = 0;
		}
		else if ((t1 < t3 || t2 < t3) &&
			fabs(cvTriangleArea(pt, org_pt, dst_pt)) < FLT_EPSILON)
		{
			location = CV_PTLOC_ON_EDGE;
			point = 0;
		}
	}

	if (location == CV_PTLOC_ERROR)
	{
		edge = 0;
		point = 0;
	}

	if (_edge)
		*_edge = edge;
	if (_point)
		*_point = point;

	return location;
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

static void
cvSubdiv2DDeleteEdge(CvSubdiv2D * subdiv, CvSubdiv2DEdge edge)
{
	CvQuadEdge2D *quadedge = (CvQuadEdge2D *)(edge & ~3);

	if (!subdiv || !quadedge)
		CV_Error(CV_StsNullPtr, "");

	cvSubdiv2DSplice(edge, cvSubdiv2DGetEdge(edge, CV_PREV_AROUND_ORG));

	CvSubdiv2DEdge sym_edge = cvSubdiv2DSymEdge(edge);
	cvSubdiv2DSplice(sym_edge, cvSubdiv2DGetEdge(sym_edge, CV_PREV_AROUND_ORG));

	cvSetRemoveByPtr((CvSet*)(subdiv->edges), quadedge);
	subdiv->quad_edges--;
}

static CvSubdiv2DEdge
cvSubdiv2DConnectEdges(CvSubdiv2D * subdiv, CvSubdiv2DEdge edgeA, CvSubdiv2DEdge edgeB)
{
	if (!subdiv)
		CV_Error(CV_StsNullPtr, "");

	CvSubdiv2DEdge new_edge = cvSubdiv2DMakeEdge(subdiv);

	cvSubdiv2DSplice(new_edge, cvSubdiv2DGetEdge(edgeA, CV_NEXT_AROUND_LEFT));
	cvSubdiv2DSplice(cvSubdiv2DSymEdge(new_edge), edgeB);

	CvSubdiv2DPoint* dstA = cvSubdiv2DEdgeDst(edgeA);
	CvSubdiv2DPoint* orgB = cvSubdiv2DEdgeOrg(edgeB);
	cvSubdiv2DSetEdgePoints(new_edge, dstA, orgB);

	return new_edge;
}
CV_INLINE int
icvIsPtInCircle3(CvPoint2D32f pt, CvPoint2D32f a, CvPoint2D32f b, CvPoint2D32f c)
{
	const double eps = FLT_EPSILON*0.125;
	double val = ((double)a.x * a.x + (double)a.y * a.y) * cvTriangleArea(b, c, pt);
	val -= ((double)b.x * b.x + (double)b.y * b.y) * cvTriangleArea(a, c, pt);
	val += ((double)c.x * c.x + (double)c.y * c.y) * cvTriangleArea(a, b, pt);
	val -= ((double)pt.x * pt.x + (double)pt.y * pt.y) * cvTriangleArea(a, b, c);

	return val > eps ? 1 : val < -eps ? -1 : 0;
}
static void
cvSubdiv2DSwapEdges(CvSubdiv2DEdge edge)
{
	CvSubdiv2DEdge sym_edge = cvSubdiv2DSymEdge(edge);
	CvSubdiv2DEdge a = cvSubdiv2DGetEdge(edge, CV_PREV_AROUND_ORG);
	CvSubdiv2DEdge b = cvSubdiv2DGetEdge(sym_edge, CV_PREV_AROUND_ORG);
	CvSubdiv2DPoint *dstB, *dstA;

	cvSubdiv2DSplice(edge, a);
	cvSubdiv2DSplice(sym_edge, b);

	dstA = cvSubdiv2DEdgeDst(a);
	dstB = cvSubdiv2DEdgeDst(b);
	cvSubdiv2DSetEdgePoints(edge, dstA, dstB);

	cvSubdiv2DSplice(edge, cvSubdiv2DGetEdge(a, CV_NEXT_AROUND_LEFT));
	cvSubdiv2DSplice(sym_edge, cvSubdiv2DGetEdge(b, CV_NEXT_AROUND_LEFT));
}

CvSubdiv2DPoint * cvSubdivDelaunay2DInsert(CvSubdiv2D * subdiv, CvPoint2D32f pt)
{
	CvSubdiv2DPointLocation location = CV_PTLOC_ERROR;

	CvSubdiv2DPoint *curr_point = 0, *first_point = 0;
	CvSubdiv2DEdge curr_edge = 0, deleted_edge = 0, base_edge = 0;
	int i, max_edges;

	if (!subdiv)
		CV_Error(CV_StsNullPtr, "");

	if (!CV_IS_SUBDIV2D(subdiv))
		CV_Error(CV_StsBadFlag, "");

	location = cvSubdiv2DLocate(subdiv, pt, &curr_edge, &curr_point);

	switch (location)
	{
	case CV_PTLOC_ERROR:
		CV_Error(CV_StsBadSize, "");

	case CV_PTLOC_OUTSIDE_RECT:
		CV_Error(CV_StsOutOfRange, "");

	case CV_PTLOC_VERTEX:
		break;

	case CV_PTLOC_ON_EDGE:
		deleted_edge = curr_edge;
		subdiv->recent_edge = curr_edge = cvSubdiv2DGetEdge(curr_edge, CV_PREV_AROUND_ORG);
		cvSubdiv2DDeleteEdge(subdiv, deleted_edge);
		/* no break */

	case CV_PTLOC_INSIDE:

		assert(curr_edge != 0);
		subdiv->is_geometry_valid = 0;

		curr_point = cvSubdiv2DAddPoint(subdiv, pt, 0);
		base_edge = cvSubdiv2DMakeEdge(subdiv);
		first_point = cvSubdiv2DEdgeOrg(curr_edge);
		cvSubdiv2DSetEdgePoints(base_edge, first_point, curr_point);
		cvSubdiv2DSplice(base_edge, curr_edge);

		do
		{
			base_edge = cvSubdiv2DConnectEdges(subdiv, curr_edge,
				cvSubdiv2DSymEdge(base_edge));
			curr_edge = cvSubdiv2DGetEdge(base_edge, CV_PREV_AROUND_ORG);
		} while (cvSubdiv2DEdgeDst(curr_edge) != first_point);

		curr_edge = cvSubdiv2DGetEdge(base_edge, CV_PREV_AROUND_ORG);

		max_edges = subdiv->quad_edges * 4;

		for (i = 0; i < max_edges; i++)
		{
			CvSubdiv2DPoint *temp_dst = 0, *curr_org = 0, *curr_dst = 0;
			CvSubdiv2DEdge temp_edge = cvSubdiv2DGetEdge(curr_edge, CV_PREV_AROUND_ORG);

			temp_dst = cvSubdiv2DEdgeDst(temp_edge);
			curr_org = cvSubdiv2DEdgeOrg(curr_edge);
			curr_dst = cvSubdiv2DEdgeDst(curr_edge);
			
			if (icvIsRightOf(temp_dst->pt, curr_edge) > 0 &&
				icvIsPtInCircle3(curr_org->pt, temp_dst->pt,
				curr_dst->pt, curr_point->pt) < 0)
			{
				leafCount++;
				cvSubdiv2DSwapEdges(curr_edge);
				curr_edge = cvSubdiv2DGetEdge(curr_edge, CV_PREV_AROUND_ORG);
				
			}
			else if (curr_org == first_point)
			{
				break;
			}
			else
			{
				curr_edge = cvSubdiv2DGetEdge(cvSubdiv2DNextEdge(curr_edge),
					CV_PREV_AROUND_LEFT);
			}
		}
		break;
	default:
		CV_Error_(CV_StsError, ("cvSubdiv2DLocate returned invalid location = %d", location));
	}

	return curr_point;
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
static void draw_subdiv_point(IplImage* img, CvPoint2D32f fp, CvScalar color)//画出三角剖分的顶点  
{
	cvCircle(img, cvPoint(cvRound(fp.x), cvRound(fp.y)), 5, color, CV_FILLED, 8, 0);
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

static void run()
{
	//初始化可视化数据
	char win[] = "source";
	int i;
	CvRect rect = { 0, -1300, 600, 1300 };//前面两个参数是左上角坐标
	CvMemStorage* storage;
	CvSubdiv2D* subdiv;
	storage = cvCreateMemStorage(0);
	subdiv = init_delaunay(storage, rect);

	//1.读取文件中的点,路径为D://opencvdatas.xlsx
	BasicExcel e;
	// Load a workbook with one sheet, display its contents and save into another file.
	e.Load("opencvdatas.xls");

	vector<CvPoint2D32f> points;
	BasicExcelWorksheet* sheet1 = e.GetWorksheet("Sheet1");
	
	if (sheet1)
	{

		size_t maxRows = sheet1->GetTotalRows();//行

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
			points.push_back(point);
		}
	}
	//2.形成三角剖分
	for (int i = 0; i < points.size(); i++){
		long start = GetTickCount();
		CvPoint2D32f fp = points[i];

		if (cvWaitKey(100) >= 0)
			break;
		CvSubdiv2DPoint *pt = cvSubdivDelaunay2DInsert(subdiv, fp);//向三角剖分中插入该点，即对该点进行三角剖分  
		if (sheet1){
			sheet1->Cell(i, 0)->SetInteger(leafCount);
			sheet1->Cell(i + 1, 1)->SetDouble(GetTickCount() - start);
		}
		leafCount = 0;
	}
	e.SaveAs("statistics.xls");
}

int main(int argc, char** argv){
	run();
}