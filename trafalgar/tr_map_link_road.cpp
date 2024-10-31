/******************************************************************
 *
 * @short	road link class of trafalgar geometric selectable objects
 *
 * project:	Trafalgar lib
 *
 * class:	TrMapLinkRoad
 * superclass:	TrMapLink
 * modul:	tr_map_link_raod.cc
 * @version	0.2
 *
 * system:	UNIX/LINUX
 * compiler:	gcc
 *
 * beginning:	05.2024
 *
 * @author	Schmid Hubert(C) 2024-2024
 *
 * history:
 *
 ******************************************************************/

/* The trafalgar package is free software.  You may redistribute it
   and/or modify it under the terms of the GNU General Public License as
   published by the Free Software foundation; either version 2, or (at your
   option) any later version.

   The GNU trafalgar package is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with the trafalgar package; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin St., Fifth Floor,
   Boston, MA 02110-1301, USA. */

#include "tr_map_link_road.h"

// TODO: only for the name list?
#include "tr_map_net_road.h"
#include "tr_name_element.h"

//#include <QPainterPath>
#include <math.h>

#define SEGMENT_TO true
#define SEGMENT_FROM false

TrMapLinkRoad::TrMapLinkRoad()
	: TrMapLink()
	, m_lanes(1)
	, m_parking(0)
	, m_mm_calc_width(DEF_WITH_P)
	, m_pen_park(nullptr)
{
	//m_inst_mask = TR_MASK_DRAW;
}

TrMapLinkRoad::~TrMapLinkRoad()
{
}

QDebug operator<<(QDebug dbg, const TrMapLinkRoad& link)
{
	if((link.m_node_from == nullptr) || (link.m_node_to == nullptr))
		return dbg << link.getXmlName() << "----" << "----" <<
			"class " << HEX << link.m_type << link.m_one_way;
	return dbg << link.getXmlName() << " " << DEC <<
		link.m_node_from->getGeoId() << " - " <<
		link.m_node_to->getGeoId() << " class " << HEX <<
		link.m_type << " " << link.m_one_way << " lanes: " <<
		link.m_lanes << " parking " << link.m_parking;
}

QString TrMapLinkRoad::getXmlName() const
{
	return "map_link_road";
}


void TrMapLinkRoad::setLanes(uint32_t r_lanes)
{
	// TODO: dir?
	//TR_INF << r_lanes;

	if(r_lanes == 0)
		m_lanes = 1;
	//if(r_lanes > 0)
	else
        m_lanes = static_cast<uint8_t>(r_lanes);
}

uint8_t TrMapLinkRoad::getLanes()
{
	return m_lanes;
}

void TrMapLinkRoad::setParking(uint16_t park)
{
	//TR_INF << HEX << park;
	m_parking = park;
}

uint16_t TrMapLinkRoad::getParking()
{
	return m_parking;
}


bool TrMapLinkRoad::getSegment(TrGeoSegment & seg, bool dir, bool par)
{
	if(par)
	{
		if(m_par_line.size() < 2)
		{
			//TR_INF << "PAR " << m_par_line.size() << *this;
			return false;
		}
		if(!dir)
		{
			seg.setPoints(m_par_line[0], m_par_line[1]);
		}
		else
		{
			seg.setPoints(m_par_line[m_par_line.size() - 2],
					m_par_line[m_par_line.size() - 1]);
		}
		return true;
	}
	else
	{
		if(m_pline == nullptr)
		{
			if(dir)
				seg.setPoints(m_node_from->getPoint(), m_node_to->getPoint());
			else
				seg.setPoints(m_node_to->getPoint(), m_node_from->getPoint());
			//TR_INF << "3333" << seg;
			return true;
		}
		if(dir)
		{
			seg.setPoints(m_node_to->getPoint(), m_pline->getLastPoint());
		}
		else
		{
			seg.setPoints(m_pline->getFirstPoint(), m_node_from->getPoint());
		}
	}
	return true;
}

bool TrMapLinkRoad::getParSegment(const TrZoomMap & zoom_ref, poly_add & add, bool dir)
{
	if(m_par_line.size() < 2)
		return false;
	if(dir)
	{
		TrGeoSegment seg(m_par_line[0], m_par_line[1]);
		//TR_INF << "1 " << seg;
		seg.getSegmentData(zoom_ref, add, m_par_line[0].x);
	}
	else
	{
		TrGeoSegment seg(m_par_line[m_par_line.size()-2], m_par_line[m_par_line.size()-1]);
		//TR_INF << "2 " << seg;
		seg.getSegmentData(zoom_ref, add, m_par_line[m_par_line.size()-1].x);
	}
	return true;
}


TrMapLinkRoad * TrMapLinkRoad::getNextLink(TrMapNode & node, int n, bool dir, double & ang)
{
	TrGeoObject * obj = node.getConLink(n, dir, ang);
	if(obj == nullptr)
		return nullptr;
	TrMapLinkRoad * next = dynamic_cast<TrMapLinkRoad*>(obj);
	return next;
}

// TODO: check ramps!
// TODO: same code in the node class? (fork, etc.)
// TODO: to remove -> to node class???
int TrMapLinkRoad::moveCheck(TrMapNode & node)
{
	double ang = 10.0;
	//double ang_in = ang;
	int ret = 0;

	int count_one_in = 0;
	for (int i = 0; i < node.getIn(false); ++i)
	{
		TrMapLinkRoad * next = getNextLink(node, i, TR_NODE_IN, ang);
		if(next != nullptr)
		{
			if(next->getOneWay() & TR_LINK_DIR_ONEWAY)
				count_one_in++;
			if(next == this)
			{
				//TR_INF << "ang: " << ang;
				//ang_in = ang;
			}
			//TR_INF << "IN  " << node.getIn(false) << "# " << count_one_in << " #" << *next;
		}
		else
		{
			//TR_INF << "--border-- " << *this;
			return 0;
		}
	}
	int count_one_out = 0;
	int count_class_out = 0;
	for (int i = 0; i < node.getOut(false); ++i)
	{
		TrMapLinkRoad * next = getNextLink(node, i, TR_NODE_OUT, ang);
		if(next != nullptr)
		{
			if(next->getOneWay() & TR_LINK_DIR_ONEWAY)
			{
				count_one_out++;
				if((next->m_type & 0x00ff) < 9)
					count_class_out++;
			}
			else
			{
				//TR_INF << ang << ang_in;
			}
			//TR_INF << "OUT " << node.getOut(false) << "& " << count_one_out << " &" << *next;
		}
		else
		{
			//TR_INF << "++border++ " << *this;
			return 0;
		}
	}
	if((count_one_in == 1) && (node.getIn(false) == 1) &&
			(count_one_out == 1) && (node.getOut(false) == 1))
	{
		return 1;
	}
	if((count_one_in == 1) && (node.getIn(false) == 1) &&
			(count_one_out == 2) && (node.getOut(false) == 2))
	{
		return 2;
	}
	if((count_one_in == 2) && (node.getIn(false) == 2) &&
			(count_one_out == 1) && (node.getOut(false) == 1))
	{
		return 3;
	}
	if(count_class_out == 1)
		return 4;
	return ret;
}

void TrMapLinkRoad::triCross(const TrZoomMap & zoom_ref, TrMapNode & node)
{
	TrMapLink * link_1 = dynamic_cast<TrMapLink *>(node.getSingleElement(1));
	TrMapLink * link_2 = dynamic_cast<TrMapLink *>(node.getSingleElement(2));
	TrMapLink * link_3 = dynamic_cast<TrMapLink *>(node.getSingleElement(3));
	if((link_1 == nullptr) || (link_2 == nullptr) || (link_3 == nullptr))
		return;
    TrGeoSegment seg1;
	link_1->getSegment(seg1, false, true);
	double ang1 = seg1.getAngle(zoom_ref);
    //link_1->getSegment(*TrMapNet::ms_seg_1, false, true);
	TrGeoSegment seg2;
	link_2->getSegment(seg2, true, true);
	double ang2 = seg2.getAngle(zoom_ref);
	if(fabs(ang1 - ang2) > 0.1)
        return;
	m_node_to->setPoint(seg1.getFirstPoint());
    //link_2->getSegment(*TrMapNet::ms_seg_2, true, true);
	TrGeoSegment seg3;
	link_3->getSegment(seg3, true, true);
	//link_3->getSegment(*TrMapNet::ms_seg_1, true, false);
}

// TODO: move point -> node class
bool TrMapLinkRoad::getMoveCrossPoint(const TrZoomMap & zoom_ref)
{
	double ang = getAngle(zoom_ref, true);
	//TR_INF << "~~~ " << ang << geoInvertAngle(ang) << *this;
	TrGeoObject * obj = m_node_to->getNextOutElement(geoInvertAngle(ang));
	if(obj == nullptr)
		return false;
	TrMapLinkRoad * next = dynamic_cast<TrMapLinkRoad*>(obj);
	if(next == nullptr)
		return false;
	if(m_node_to->checkTwoFork(true, true))
		return false;
	if(m_node_to->checkTwoFork(false, true))
		return false;
	// TODO: move point -> node class
	if((next->m_one_way & TR_LINK_DIR_ONEWAY) && (next->isAsDoubleLine()))
	{
		TrGeoSegment seg1;
		TrGeoSegment seg2;
		TrPoint pt;
		//TR_INF << *next;
		if(getSegment(seg1, true, true) == false)
			return false;
		if(next->getSegment(seg2, false, true) == false)
			return false;
		seg1.getCrossPoint(zoom_ref, pt, seg2);
		TrGeoSegment diff(m_node_to->getPoint(), pt);
		if(diff.getLength(zoom_ref) > 20.0)
		{
			return false;
		}
		//if(m_node_to->getViewOpt() & TR_POINT_MOVE)
		//	return false;
		m_node_to->setViewOpt(m_node_to->getViewOpt() | TR_POINT_MOVE | TR_POINT_MOVE_CROSS);
		// TODO tmp
		if(m_node_to->getShiftPoint(zoom_ref) == 1)
			m_node_to->setPoint(pt);
		else
			triCross(zoom_ref, *m_node_to);
		return true;
	}
	return false;
}

bool TrMapLinkRoad::moveBaseLine(const TrZoomMap & zoom_ref)
{
	//return false;
	if((!(m_one_way & TR_LINK_DIR_ONEWAY)) || (!(isAsDoubleLine())))
		return false;
	if((m_node_from == nullptr) || (m_node_to == nullptr))
		return false;
	if(m_par_line.size() < 2)
		return false;

	// TODO TMP remove to the node class - move node #1
	if(getMoveCrossPoint(zoom_ref))
	{
		// TODO: check return state
	}

	// TODO: tmp - move node #2
	if(!(m_node_to->getViewOpt() & (TR_POINT_MOVE | TR_POINT_MOVE_CROSS)))
	{
		m_node_to->setViewOpt(m_node_to->getViewOpt() | TR_POINT_MOVE);
		if(moveCheck(*m_node_to) && (m_node_to->getShiftPoint(zoom_ref) == 1))
			m_node_to->setPoint(m_par_line[m_par_line.size()-1]);
		else
		{
			triCross(zoom_ref, *m_node_to);
        }
	}
	// TODO end
	if(m_pline)
	{
		for(size_t i = 0; i < m_pline->getSize(); i++)
		{
			m_pline->setPoint(i, m_par_line[i+1]);
		}
		//m_pline->clearData();
		m_pline->init(zoom_ref, 0, nullptr);
	}
	return true;
}

void TrMapLinkRoad::setMoveParLine(const TrZoomMap & zoom_ref)
{
	if((m_one_way & TR_LINK_DIR_ONEWAY) && (isAsDoubleLine()))
	{
		int32_t test_width = (0 - (m_mm_calc_width >> 1));
		m_par_line.clear();
		initDoubleLine(zoom_ref, test_width);
	}
}


bool TrMapLinkRoad::init(const TrZoomMap & zoom_ref, uint64_t ctrl, TrGeoObject * base)
{
	bool ret = false;

	if((m_node_from == nullptr) || (m_node_to == nullptr))
	{
		TR_WRN << "node == nullptr";
		return false;
	}
	if(ctrl & TR_INIT_COLORS)
	{
		setParkingPen(m_parking >> 8, base);
	}
	ret = TrMapLink::init(zoom_ref, ctrl, base);

	// TODO: first test, use in other class?
	if((m_geo_active_pen != nullptr) && (ctrl & TR_INIT_COLORS))
	{
		m_pen_para.setColor(m_geo_active_pen->color());
		// TODO: '2' -> from static value?
		m_pen_para.setWidth(2);
	}
	/*if(ctrl & TR_INIT_COLORS)
	{
		setLinkPen(base);
			setParkingPen(m_parking >> 8, base);
	}*/

	if(ctrl & TR_INIT_GEOMETRY)
	{
		ctrl &= 0xff;
		if(ctrl == 10)
		{
		}

		// code for the angles
		if(ctrl == 11)
		{
			// TODO: is in node class -> to remove?
			// only one node is needed
			//m_node_to->setConnectionAngles(zoom_ref, TR_NODE_OUT);
			//m_node_to->setConnectionAngles(zoom_ref, TR_NODE_IN);
			return true;
		}

		// code for cleanup the polygon points
		if(ctrl == 29)
		{
			if((m_one_way & TR_LINK_DIR_ONEWAY) && (isAsDoubleLine()) && (m_pline != nullptr))
			{
				QList<TrGeoSegment> seg_list;
				TrGeoSegment seg(m_node_from->getPoint(), m_node_to->getPoint());
				//TR_INF << *this;
				seg.getSegList(seg_list, *m_pline);
				if(seg.managePolygon(zoom_ref, *m_pline, seg_list, m_mm_calc_width))
				{
					seg.getSegList(seg_list, *m_pline);
					//for(int i = 0; i < seg_list.size(); ++i)
					//	TR_INF << "2 a: " << seg_list[i].getAngle(zoom_ref) << "l: " << seg_list[i].getLength(zoom_ref);
				}
				if(m_pline->getSize())
					m_pline->init(zoom_ref, TR_INIT_GEOMETRY, nullptr);
				else
					m_pline = nullptr;
			}
		}

		// code to move the parallel line
		if(ctrl == 27)
		{
			setMoveParLine(zoom_ref);
		}
		// code to move the nodes of the oneway link
		if(ctrl == 30)
		{
			//if((m_one_way & TR_LINK_DIR_ONEWAY) && (isAsDoubleLine()))
			moveBaseLine(zoom_ref);
		}
		if(ctrl == 35)
		{
			if((getRdClass() & 0x000f) < 9)
				handleSmallElement(zoom_ref, 0.2, m_mm_calc_width / 1000.0); //8.0); //4.0); //1.0);
		}

		// code for double line
		if(ctrl == 14)
		{
			if(s_mask & TR_MASK_MORE_LINES)
				initDoubleLine(zoom_ref, m_mm_calc_width);
			return true;
		}

		// code for checking the ramps and set cross points
		if(ctrl == 12)
		{
			if(!checkRamps(zoom_ref, (s_mask & TR_MASK_SET_RAMPS) != TR_MASK_SET_RAMPS))
			{
				// TODO: init -> to reset the points
				//setCrossPoint(zoom_ref);
			}
			return true;
		}

		// TODO: create a function for switch to GB
		double w = m_lanes * LANE_WITH_P;

		if(m_one_way & TR_LINK_DIR_BWD)
		{
			w = m_lanes * LANE_WITH_M;
			m_mm_calc_width = DEF_WITH_M;	// default is DEF_WITH_P
		}

		// TODO: switch for loaded width...
		if(getWidth() != 0)		// with with form base class, loaded from file
		{
			//TR_INF << "L" << m_mm_load_width << "C" << w << HEX << m_rd_class;
			// id lane based width is positive
			if(w > 0)
				w = getWidth(); //m_mm_load_width;
			else
				w = 0-getWidth(); //m_mm_load_width;
		}

		if(isAsDoubleLine())
		{
			m_mm_calc_width = w;
		}

		if(ctrl == 3)		// clear data
		{
		}
		else
		{
			//m_pt_from = m_node_from->getPoint();
			//m_pt_to = m_node_to->getPoint();
		}
	}
	return ret;
	//return true;
}

void TrMapLinkRoad::getNodePoints(TrPoint & pt1, TrPoint & pt2)
{
	pt1 = m_pt_from;
	pt2 = m_pt_to;

	if(m_one_way & TR_LINK_DIR_BWD)
	{
		TrPoint pt_switch = pt2;
			pt2 = pt1;
			pt1 = pt_switch;
	}
}

void TrMapLinkRoad::initDoubleLineWidth(const TrZoomMap & zoom_ref)
{
	initDoubleLine(zoom_ref, m_mm_calc_width);
}

// the 'initDoubleLine->getLength' function needes the length
// of the segment for all links, not usable for routing
void TrMapLinkRoad::initDoubleLine(const TrZoomMap & zoom_ref, int32_t width)
{
	TrPoint pt1;
	TrPoint pt2;
	getNodePoints(pt1, pt2);

	m_par_line.clear();

	if(m_pline)
	{
		if(m_pline->getSize() == 0)
		{
			TR_WRN << "no size" << *this;
			return;
		}

		//TR_MSG << "with pline";

		TrGeoPolygon::setInfoSect(zoom_ref, m_seg_from,
			pt1, m_pline->getFirstPoint());
		m_seg_from.sl.flags  &= ~(DIR_INIT);

		TrGeoPolygon::calcParPoint(zoom_ref, pt1, m_seg_from.sl, width);
		m_par_line.append(pt1);

		TrGeoPolygon::setInfoSect(zoom_ref, m_seg_to,
			m_pline->getLastPoint(), pt2);
		m_pline->parallel(&m_par_line, zoom_ref, m_seg_from.sl, m_seg_to.sl, width);

		TrGeoPolygon::calcParPoint(zoom_ref, pt2, m_seg_to.sl, width);
		m_par_line.append(pt2);
		//setCrossPoint(zoom_ref);
	}
	else
	{
		//TR_MSG << "2 points";

		TrGeoPolygon::setInfoSect(zoom_ref, m_seg_from, pt1, pt2);
		m_seg_from.sl.flags  &= ~(DIR_INIT);

		TrGeoPolygon::calcParPoint(zoom_ref, pt1, m_seg_from.sl, width);
		TrGeoPolygon::calcParPoint(zoom_ref, pt2, m_seg_from.sl, width);
		m_par_line.append(pt1);
		m_par_line.append(pt2);

		m_seg_to = m_seg_from;
	}
}


TrGeoObject * TrMapLinkRoad::getSegmentWithParm(TrGeoSegment & segment, int64_t nd_id, bool dir)
{
	if(!(isAsDoubleLine()))
		return nullptr;

	if(m_one_way & TR_LINK_DIR_ONEWAY)
	{
		if(dir)
		{
			if(nd_id == getNodeTo())
			{
				//TR_INF << "TO" << *this;
				getSegment(segment, true, false);
			}
			else
			{
				//TR_INF << "not TO" << *this;
				getSegment(segment, false, true);
			}
		}
		else
		{
			if(nd_id == getNodeFrom())
			{
				//TR_INF << "FROM" << *this;
				getSegment(segment, false, false);
				//TrMapNet::ms_seg_1->setPoints(segment);
			}
			else
			{
				//TR_INF << "not FROM" << *this;
				getSegment(segment, true, true);
			}
		}
		return this;
	}

	TrMapLink * parallel = this;
	if(dir)
	{
		if(nd_id == getNodeTo())
			parallel = getParallelLink();
	}
	else
	{
		if(nd_id == getNodeFrom())
			parallel = getParallelLink();
	}
	if(parallel == nullptr)
		return nullptr;

	// if not oneway -> parallel line is allways true
	if(parallel->getOneWay() & TR_LINK_DIR_BWD)
		parallel->getSegment(segment, dir, true);
	else
		parallel->getSegment(segment, !dir, true);
	return parallel;
}

bool TrMapLinkRoad::setRamp(const TrZoomMap & zoom_ref, bool dir)
{
	if((m_node_from == nullptr) || (m_node_to == nullptr))
	{
		TR_ERR << "nullptr";
		return -1;
	}
	TrMapNode * nd = m_node_to;
	if(dir)
		nd = m_node_from;

	// only for onway links
	if(!nd->checkTwoFork(dir, true))
	{
		return false;
	}

	TrPoint t_pt = {0.0, 0.0};

	double ang1 = 10.0;
	double ang2 = 10.0;
	TrMapLink * link1 = dynamic_cast<TrMapLink *>(nd->getElement(0, dir, ang1));
	TrMapLink * link2 = dynamic_cast<TrMapLink *>(nd->getElement(1, dir, ang2));

	if((link1 == nullptr) || (link2 == nullptr))
	{
		return false;
	}

	if(TrMapNode::switchLinksByAngle(ang2, ang1, dir))
	{
		TrMapLink * cp = link1;
		link1 = link2;
		link2 = cp;
	}

	bool dir1 = dir;
	bool dir2 = dir;
	if(link1->getOneWay() & TR_LINK_DIR_BWD)
		dir1 = !dir;
	if(link2->getOneWay() & TR_LINK_DIR_BWD)
		dir2 = !dir;

	if(link2->getParPoint(!dir2, t_pt))
	{
		if(link1->isAsDoubleLine() && link2->isAsDoubleLine())
		{
			link1->setCrossingPoint(t_pt, !dir1);
			TrMapLinkRoad * link = dynamic_cast<TrMapLinkRoad *>(link1);
			if(link != nullptr)
				link->initDoubleLineWidth(zoom_ref);
		}
	}
	return true;
}

//#define SEGMENT_FROM false
// TODO: do check the ramps on nodes?
// TODO: to remove - only reset the points
bool TrMapLinkRoad::checkRamps(const TrZoomMap & zoom_ref, bool do_reset)
{
	//return false;
	// for double line:  if(((rd_class & 0x0f) >= 9) || (rd_class <= 0) || (!(mask & TR_MASK_MORE_LINES )))
	//TR_MSG << "do reset: " << do_reset;

	if((m_node_from == nullptr) || (m_node_to == nullptr))
	{
		TR_ERR << "nullptr";
		return false;
	}

	if(do_reset)
	{
		init(zoom_ref, 0);
		return false;
	}
	return false;
}

bool TrMapLinkRoad::getParPoint(bool first, TrPoint & pt)
{
	if(m_par_line.size() < 2)
		return false;

	if(first)
		pt = m_par_line[0];
	else
		pt = m_par_line[m_par_line.size()-1];
	return true;
}

bool TrMapLinkRoad::setParPoint(bool first, TrPoint & pt)
{
	if(m_par_line.size() < 2)
		return false;
	if(first)
		m_par_line[0] = pt;
	else
		m_par_line[m_par_line.size()-1] = pt;
	return true;
}

void TrMapLinkRoad::setParkingPen(uint16_t type, TrGeoObject * base)
{
	TrMapList * list = dynamic_cast<TrMapList *>(base);
	if(list == nullptr)
		return;

	//V_PARK_PARALLEL_L   0x0000000100000000
	//V_PARK_DIAGONAL_L   0x0000000200000000
	//V_PARK_PERPENDI_L   0x0000000400000000
	// FLAG_PARKING       0x0000000000100000
	// V_PARK_PARALLEL_R  0x0000010000000000
	// V_PARK_DIAGONAL_R  0x0000020000000000
	// V_PARK_PERPENDI_R  0x0000040000000000

	// FLAG_PARKING_R     0x0000000000200000
	// FLAG_PARKING_L     0x0000000000400000
	// FLAG_PARKING_B     0x0000000000800000
	//                           102006

	if(type & 0x0001)	// parking_par
		m_pen_park = list->getObjectPen(0x2002);
	if(type & 0x0002)	// parking_dia
		m_pen_park = list->getObjectPen(0x2004);
	if(type & 0x0004)	// parking_per
		m_pen_park = list->getObjectPen(0x2003);
	if(type & 0x0030)	// parking_no
		m_pen_park = list->getObjectPen(0x2001);
	if(m_pen_park != nullptr)
	{
		//TR_INF << HEX << type;
		m_pen_park->setStyle(Qt::DotLine);
        m_pen_park->setWidth(4);
	}
}

// TODO: is not abstract, only for road link...
void TrMapLinkRoad::getParScreenLine(const TrZoomMap & zoom_ref, QVector<QPointF> & pointPairs)
{
	for(int i = 0; i<m_par_line.size(); i++)
	{
		TrPoint screenx = m_par_line[i];
		zoom_ref.setMovePoint(&screenx.x, &screenx.y);
		QPointF ptf;
		ptf.setX(screenx.x);
		ptf.setY(screenx.y);
		pointPairs.append(ptf);
	}
}

// show the parallel line as helping line
// TODO: GUI option?
void TrMapLinkRoad::drawParLine(const TrZoomMap & zoom_ref, QPainter * p, unsigned char mode)
{
	TrGeoSegment seg;
	if(m_par_line.size() < 2)
		return;
	for (int i = 1; i < m_par_line.size(); ++i)
	{
		seg.setPoints(m_par_line[i-1], m_par_line[i]);
		seg.setSurroundingRect();
		seg.draw(zoom_ref, p, 0);
	}
}

void TrMapLinkRoad::draw(const TrZoomMap & zoom_ref, QPainter * p, unsigned char mode)
{
	//if(m_parking)
	//	TR_INF << HEX << m_parking;
	if(!(m_inst_mask & TR_MASK_DRAW))
		return;
	//TR_INF << *this;
	if(this->clip(zoom_ref))
		return;
	if(m_geo_active_pen == nullptr)
	{
		//TR_WRN << "no active_pen -> exiting!" << HEX << m_rd_class;
		return;
	}

    if((m_parking & 0xff00) && (s_mask & TR_MASK_SHOW_PARKING) && (m_pen_park != nullptr))
    {
        if(m_one_way & TR_LINK_DIR_ONEWAY)
        {
            p->setPen(*m_pen_park);
            if(m_pline == nullptr)
            {
                QPolygon poly(2);
                getTwoLine(zoom_ref, poly);
                p->drawPolyline(poly);
            }
            else
            {
                if(m_one_way & TR_LINK_DIR_BWD)
                    m_pline->draw(zoom_ref, p, m_pt_to, m_pt_from, 2);
                else
                    m_pline->draw(zoom_ref, p, m_pt_from, m_pt_to, 2);
            }
        }
        // TODO: two pens for base and parallel line on oneway links?
        if(isAsDoubleLine() && (s_mask & TR_MASK_MORE_LINES ))
        {
            p->setPen(*m_pen_park);
            QVector<QPointF> vptf;
            getParScreenLine(zoom_ref, vptf);
            if(vptf.size() > 1)
            {
                p->drawPolyline(vptf);
            }
        }
    }
	// show the parallel line
	//drawParLine(zoom_ref, p, 0);
	// print base line
	// TODO: double code but more flexible
	if(m_pline == nullptr)
	{
		if(isAsDoubleLine() && (s_mask & TR_MASK_MORE_LINES ))
			p->setPen(*m_geo_active_pen);
		else
			p->setPen(m_pen_para);
		QPolygon poly(2);
		getTwoLine(zoom_ref, poly);
		p->drawPolyline(poly);
	}
	else
	{
		if(isAsDoubleLine() && (s_mask & TR_MASK_MORE_LINES ))
			m_pline->setActivePen(getActivePen());
		else
			m_pline->setActivePen(&m_pen_para);
		if(m_one_way & TR_LINK_DIR_BWD)
			m_pline->draw(zoom_ref, p, m_pt_to, m_pt_from, 2);
		else
			m_pline->draw(zoom_ref, p, m_pt_from, m_pt_to, 2);
	}

	// print parking part
	if(isAsDoubleLine() && (s_mask & TR_MASK_MORE_LINES ))
	{
		p->setPen(m_pen_para);
		QVector<QPointF> vptf;
		getParScreenLine(zoom_ref, vptf);
		if(vptf.size() > 1)
        {
			p->drawPolyline(vptf);

            QString rd_name = getElementName();
            if(rd_name.size())
            {
                //p->drawText((int)vptf.at(0).x(), (int)vptf.at(0).y(), rd_name);
            }
        }
    }
    if((s_mask & TR_MASK_SHOW_ROADNAME) /*&& (m_pline != nullptr)*/)
    {
        TrGeoSegment seg;
        TrPoint screen;

        QString rd_name = getElementName();
        if(rd_name.size())
        {
            TrMapNode * nd = getNodeFromRef();
            if(nd == nullptr)
                    return;
            screen = nd->getPoint();

            QPolygon poly(2);
            getTwoLine(zoom_ref, poly);
            if(m_pline != nullptr)
            {
                poly.clear();
                m_pline->getScreenPoints(zoom_ref, poly);
                if(poly.size() < 2)
                {
                    TrPoint pt = m_pt_to;
                    zoom_ref.setMovePoint(&pt.x,&pt.y);
                    poly << QPoint(static_cast<int>(pt.x),static_cast<int>(pt.y));
                }
            }
            TrNameElement * el = dynamic_cast<TrNameElement*>(getNameElement());
            if(el != nullptr)
                el->drawOnPolygon(p, poly);
        }
    }
}

bool TrMapLinkRoad::setSurroundingRect()
{
	if((m_node_to == nullptr) || (m_node_from == nullptr))
		return false;
	TrMapLink::setSurroundingRect();

	double rect[4];
	for(int i = 0; i<m_par_line.size(); i++)
	{
		//TR_MSG << m_par_line[i].x << m_par_line[i].y;
		rect[0] = rect[2] = m_par_line[i].x;
		rect[1] = rect[3] = m_par_line[i].y;

		// TODO: create more check for the parallel line calculation
		// get sometimes a huge rectangle (see westend file...)
		// workaround: do not the include the surround rectangle for parallel line calculation
		if(s_mask & TR_MASK_MORE_LINES)
			updateSurroundRect(rect, false);
	}
	return true;
}

#ifdef TR_SERIALIZATION
uint64_t TrMapLinkRoad::readXmlDescription(QXmlStreamReader & xml_in)
{
	uint64_t id = TR_NO_VALUE;

	QStringRef name = readXmlHeader(xml_in);

	QXmlStreamAttributes attr = xml_in.attributes();
	m_lanes = attr.value("", "lanes").toInt();
	int ramp = attr.value("", "ramp").toInt();
	bool ok = false;
	m_parking = attr.value("", "parking").toInt(&ok, 16);
	if(!ok)
	{
		if(attr.value("", "parking") != "")
			TR_WRN << "parking: " << attr.value("", "parking") << m_parking;
	}
	//if(m_parking)
	//	TR_INF << HEX << m_parking;

	while(!xml_in.atEnd())
	{
		QStringRef ref;

		xml_in.readNext();
		if(readDefStartElement(xml_in, ref))
		{
			//TR_INF << ref;
			if(ref == "link")
			{
				id = TrMapLink::readXmlData(xml_in, "link");
				// TODO: remove double ramp flag
				if(!(m_type & TR_LINK_RAMP_FLAG))
				{
					if(ramp)
						m_type |= TR_LINK_RAMP_FLAG;
				}
			}
		}
		else if(xml_in.isEndElement())
		{
			//TR_MSG << xml_in.name();

			if(xml_in.name() == "link")
			{
			}
			if(xml_in.name() == getXmlName())
			{
				return id;
			}
		}
	}
	return TR_NO_VALUE;
}


void TrMapLinkRoad::writeXmlDescription(QXmlStreamWriter & xml_out, uint64_t id)
{
	xml_out.writeStartElement(getXmlName());
	QString hex;
	if(m_parking)
	{
		hex.setNum(m_parking, 16);
		xml_out.writeAttribute("parking", "0x" + hex);
	}
	xml_out.writeAttribute("lanes", QVariant(m_lanes).toString());
	if(m_type & TR_LINK_RAMP_FLAG)
	{
		xml_out.writeAttribute("ramp", "1");
	}
	xml_out.writeStartElement("link");
	writeXmlLinkPart(xml_out, id);
	xml_out.writeEndElement();
	xml_out.writeEndElement();
}
#endif
