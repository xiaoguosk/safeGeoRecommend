// From the software distribution accompanying the textbook
// "A Practical Introduction to Data Structures and Algorithm Analysis,
// Third Edition (C++)" by Clifford A. Shaffer.
// Source code Copyright (C) 2007-2011 by Clifford A. Shaffer.

// Include this file to access Graph representation implemented using an
// Adjacency Matrix.

#include <stdio.h>
#include <ctype.h>

// Implementation for the adjacency matrix representation
class subdiv{
	CvSubdiv2DPoint *cvSubdivDelaunay2DInsert(CvSubdiv2D * subdiv, CvPoint2D32f pt)
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

			curr_point = cvSubdiv2DAddPoint(subdiv, pt, 0);//获得插入点，类型为CvSubdiv2DPoint
			base_edge = cvSubdiv2DMakeEdge(subdiv);
			first_point = cvSubdiv2DEdgeOrg(curr_edge);
			cvSubdiv2DSetEdgePoints(base_edge, first_point, curr_point);//给边设置起点和终点
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
					curr_dst->pt, curr_point->pt) < 0)//判定点是否在园内，在圆内则为非法边，交换，edge ^ 2即交换边
				{
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
};

