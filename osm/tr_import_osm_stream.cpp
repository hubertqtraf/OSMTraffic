/******************************************************************
 *
 * @short	data import in OSM format
 *
 * project:	Trafalgar/Osm
 *
 * class:	TrImportOsmStream
 * superclass:	QXmlDefaultHandler
 * modul:	tr_import_osm_stream.cc
 *
 * system:	UNIX/LINUX
 * compiler:	gcc
 *
 * beginning:	1.2024
 *
 * @author	Schmid Hubert (C)2024-2024
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



#include "tr_import_osm_stream.h"
//#include "tr_import_osm_rel.h"

#include <tr_prof_class_def.h>

TrImportOsmStream::TrImportOsmStream(const QString & name)
	: TrGeoObject()
	, m_id(0)
	, m_point{0.0,0.0}
{
	m_filename = name;
}

TrImportOsmStream::TrImportOsmStream()
	: TrGeoObject()
	, m_id(0)
{
}

TrImportOsmStream::~TrImportOsmStream()
{
	//m_poly_points.clear();
}

bool TrImportOsmStream::osmRead(World_t & world)
{
	QFile file(m_filename);
	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		// TODO: cleanup
		return false;
	}

	QXmlStreamReader xml;

	xml.setDevice(&file);

	while (!xml.atEnd())
	{
		xml.readNext();
		if(xml.isStartElement())
		{
			if(xml.name() == "osm")
			{
				QXmlStreamAttributes attrs = xml.attributes();
				//readTrackPoint(attrs);
				world.act_name_idx = 1;
			}
			if(xml.name() == "node")
			{
				QXmlStreamAttributes attrs = xml.attributes();
				readNodePoint(attrs);
			}
			if(xml.name() == "way")
			{
				QXmlStreamAttributes attrs = xml.attributes();
				//TR_INF << m_tags;
				readWay(attrs);
				//m_tags.clear();
			}
			if(xml.name() == "relation")
			{
				TrImportOsmRel rel_read;
				Relation rel;
				if(rel_read.osmRelationRead(xml, m_waylist, rel))
				{
					// for "multipolygon" -> "face" object
					//TR_INF << "MULTI_POLY: " << rel.isMultiPolyRing();
					if(rel.isMultiPolyRing() > 1)
					{
						rel.resetPolyRing(m_waylist);
						addRelation(rel);
					}
				}
			}
			if(xml.name() == "tag")
			{
				QXmlStreamAttributes attrs = xml.attributes();
				readTag(attrs);
			}
			if(xml.name() == "nd")
			{
				bool ok;
				QXmlStreamAttributes attrs = xml.attributes();
				uint64_t ref = attrs.value("ref").toULongLong(&ok);
				if(ref > 0)
					m_way_reflist.append(ref);
			}
		}
		if(xml.isEndElement())
		{
			if(xml.name() == "osm")
			{
				closeOsm(world);
			}
			if(xml.name() == "node")
			{
				closeNode(world.m_name_map, world.act_name_idx, world.m_point_name_map);
			}
			if(xml.name() == "way")
			{
				//TR_INF << "close way" << m_tags;
				closeWay(world.m_name_map, world.act_name_idx);
			}
			if(xml.name() == "relation")
			{
				//closeRelation();
			}
			if(xml.name() == "tag")
			{
			}
			if(xml.name() == "nd")
			{
			}
		}
	}
	if (xml.hasError())
	{
		TR_ERR << xml.errorString();
		// do error handling
		return false;
	}
	this->setSurroundingRect();
	return true;
}

QMap<uint64_t, Point_t> & TrImportOsmStream::getNodeMap()
{
	return m_nodelist;
}

QVector<Rel_t> & TrImportOsmStream::getRelationList()
{
	return m_rellist;
}

bool TrImportOsmStream::init(const TrZoomMap& zoom, uint64_t contr, TrGeoObject* obj)
{
	return false;
}

bool TrImportOsmStream::setSurroundingRect()
{
	return false;
}


void TrImportOsmStream::readTag(const QXmlStreamAttributes &attributes)
{
	QString key = attributes.value("k").toString();
	QString value = attributes.value("v").toString();
	if(m_tags.contains(key))
	{
		TR_WRN << "double use: " << key << value;
	}
	else
	{
		m_tags[key] = value;
	}
}

void TrImportOsmStream::readNodePoint(const QXmlStreamAttributes &attributes)
{
	bool ok;

	//<node id="134" version="1" lon="11.50333" lat="48.12541"/>
	m_id = attributes.value("id").toULongLong(&ok);
	if(!ok)
		TR_WRN << "integer fault" << attributes.value("id");
	m_point.x = attributes.value("lon").toDouble(&ok) * TR_COOR_FACTOR;
	if(!ok)
		TR_WRN << "float fault" << attributes.value("lon");
	m_point.y = attributes.value("lat").toDouble(&ok) * TR_COOR_FACTOR;
	if(!ok)
		TR_WRN << "float fault" << attributes.value("lat");
}

void TrImportOsmStream::closeNode(QMap<QString, name_set> & name_map,
		uint64_t & act_id, QMap<uint64_t, QString> & n_map)
{
	Point_t point;

    point.x = static_cast<int32_t>(m_point.x * 100.0);
    point.y = static_cast<int32_t>(m_point.y * 100.0);
	point.id = m_id;

	point.pt_type = 0;
	if(m_tags.contains("highway"))
	{
		uint64_t code = TrImportOsmRel::getHighwayPointClass(m_tags["highway"]);
		if(code)
		{
			point.pt_type = code;
		}
	}
	if(m_tags.contains("railway"))
	{
		uint64_t code = TrImportOsmRel::getRailwayPointClass(m_tags["railway"]);
		if(code)
		{
			point.pt_type = code;
		}
	}
	if(m_tags.contains("natural"))
	{
		uint64_t code = TrImportOsmRel::getNaturalClass(m_tags["natural"]);
		if(code)
		{
			point.pt_type = code;
		}
	}
	if(m_tags.contains("amenity"))
	{
		uint64_t code = TrImportOsmRel::getAmenityClass(m_tags["amenity"]);
		if(code)
		{
			point.pt_type = code;
			//act_world->node_flags & (TYPE_FILTER | 0xffffffff00000000);
			//TR_INF << ">> amenity" << HEX << code << point.pt_type;
		}
	}
	if(m_tags.contains("shop"))
	{
		uint64_t code = TrImportOsmRel::getShopClass(m_tags["shop"]);
		if(code)
		{
			point.pt_type = code;
		}
	}
	if(m_tags.contains("barrier"))
	{
		uint64_t code = TrImportOsmRel::getBarrierClass(m_tags["barrier"]);
		if(code)
		{
			// TODO: dummy!
			//way.type = TYPE_ROAD | 12;
		}
	}
	//<tag k='tourism' v='artwork' />
	//<tag k='healthcare' v='pharmacy' />
	//<tag k='leisure' v='fitness_centre' />
	if(m_tags.contains("name"))
	{
		if(!name_map.contains(m_tags["name"]))
		{
			name_set set;
			set.id = act_id;
			set.number = 1;
			name_map[m_tags["name"]] = set;
			// TODO: use of point.id -> crash!
			point.id = act_id;
            //TR_INF << "POI: " << point.id << m_tags["name"];
			n_map[point.id] = m_tags["name"];
			act_id++;
		}
		else
		{
			name_map[m_tags["name"]].number++;
			point.id = name_map[m_tags["name"]].id;
		}
	}

	point.pt_data = 0;

	if(m_id > 0)
	{
		if(m_nodelist.contains(m_id))
		{
			TR_WRN << "double use:" << m_id;
		}
		else
		{
			m_nodelist[m_id] = point;
		}
	}

	m_id = 0;
	m_point.x = 0;
	m_point.y = 0;

	m_tags.clear();
}

void TrImportOsmStream::readWay(const QXmlStreamAttributes &attributes)
{
	bool ok;
	m_way_reflist.clear();
	m_id = attributes.value("id").toULongLong(&ok);
	if(!ok)
		TR_WRN << "integer fault" << attributes.value("id");

	//TR_INF << m_id << ok;
}

void TrImportOsmStream::closeWay(QMap<QString, name_set> & name_map, uint64_t & act_id)
{
	Way_t way;
	way.id = m_id;
	way.name_id = 0;
	way.nd_id = nullptr;
	way.n_nd_id = 0;
	way.type = 0;
	// uint16_t type;
	// 1 road, 2 rail, ...
	// uint16_t data;
	// data: class 0x001f , dir << 9 (0-2)
	way.lanes = 0;
	// lanes: lanes 7, lanesF << 16 lanesB << 24
	way.parking = 0;
	way.width = 0;

	// TODO: tmp...
	//way.type = TYPE_ROAD | 1;
	//rd_class = osm2_world.ways[i].type & 0x000000000000000f;

	if(m_tags.contains("highway"))
	{
		uint64_t code = getClass(m_tags["highway"]);
		if(code)
		{
			way.type = TYPE_ROAD | code;
		}
		//TR_INF << HEX << code;
	}

    if(m_tags.contains("railway"))
    {
        uint64_t code = TrImportOsmRel::getRailWayClass(m_tags["railway"]);
        //TR_INF << "## railway ##" << m_tags["railway"] << HEX << code;
        if(code)
        {
            way.type = TYPE_RAIL | code;
        }
    }

	if(m_tags.contains("barrier"))
	{
		uint64_t code = TrImportOsmRel::getBarrierClass(m_tags["barrier"]);
		if(code)
		{
			// TODO: dummy!
			way.type = TYPE_ROAD | 12;
		}
	}

	if(m_tags.contains("building"))
	{
		// TODO: use: 'building:part'?
		uint64_t code = TrImportOsmRel::getBuildingClass(m_tags["building"]);
		if(code)
		{
			way.type = TYPE_BUILDING | code;
		}
	}

	if(m_tags.contains("landuse"))
	{
		uint64_t code = TrImportOsmRel::getLanduseClass(m_tags["landuse"]);
		if(code)
		{
			way.type = TYPE_LANDUSE | code;
		}
	}

	if(m_tags.contains("natural"))
	{
		uint64_t code = TrImportOsmRel::getNaturalClass(m_tags["natural"]);
		if(code)
		{
			way.type = TYPE_NATURAL | code;
		}
	}

	if(m_tags.contains("waterway"))
	{
		//TR_INF << m_tags["waterway"];
	}

	if(m_tags.contains("oneway"))
	{
		uint64_t one = getDir(m_tags["oneway"]) << 32;
        if(!((way.type >> 32) & 0x03))
            way.type |= one;
		//world_->way_flags |= one;
	}

    if(m_tags.contains("junction"))
    {
        uint64_t one = getDir(m_tags["junction"]) << 32;
        if(!((way.type >> 32) & 0x03))
            way.type |= one;
    }

	if(m_tags.contains("width"))
	{
		way.width = 0;
		bool ok = false;
		double width = m_tags["width"].toDouble(&ok);
		if(ok)
		{
            way.width = static_cast<uint32_t>(width * 1000);
		}
		//TR_INF << m_tags["width"] << way.width;
	}

	if(m_tags.contains("name"))
	{
		if(!name_map.contains(m_tags["name"]))
		{
			name_set set;
			set.id = act_id;
			set.number = 1;
			name_map[m_tags["name"]] = set;
			way.name_id = act_id;

            //TR_INF << "Way: " << way.name_id << m_tags["name"];
			act_id++;
		}
		else
		{
			name_map[m_tags["name"]].number++;
			way.name_id = name_map[m_tags["name"]].id;
		}
	}
	if(m_tags.contains("name:en"))
	{
		//TR_INF << "E" << m_tags["name:en"];
	}

	if(m_tags.contains("power"))
	{
		//TR_INF << "P" << m_tags["power"];
		// TODO: create network...
		// 'line' -> way
		// 'portal' -> way (node?)
	}

	// exp: <tag k='parking:left:orientation' v='parallel' />
	uint64_t park_code = getParking();
	if(park_code)
	{
		//TR_INF << "PARK" << HEX << park_code << (park_code >> 20) << m_tags["name"];
		way.parking = (park_code >> 20);
	}
	way.lanes = getLanes();
	if(!way.lanes)
		way.lanes = 1;

	way.nd_id = (uint64_t*)malloc(sizeof(uint64_t) * m_way_reflist.size());
	memset(way.nd_id, 0x00, sizeof(uint64_t) * m_way_reflist.size());
	for(int i = 0; i<m_way_reflist.size(); i++)
	{
		way.nd_id[i] = m_way_reflist[i];
	}
	way.n_nd_id = m_way_reflist.size();
	//memcpy(act_way->nd_id, world->tmp_id_buf,
	//sizeof(uint64_t) * act_way->n_nd_id);

	if(m_id > 0)
	{
		if(m_waylist.contains(m_id))
		{
			TR_WRN << "double use:" << m_id;
		}
		else
		{
			m_waylist[m_id] = way;
		}
	}

	m_id = 0;
	m_tags.clear();
}

void TrImportOsmStream::closeOsm(World_t & world)
{
#ifdef TESTX
	world.info.way.count = m_waylist.size();
	if((world.ways = (Way_t *)malloc(sizeof(Way_t) * world.info.way.count)) == nullptr)
    {
        return;
    }
    memset(world.ways, 0x00, sizeof(Way_t) * world.info.way.count);
#else
    world.way_count = m_waylist.size();
    if((world.ways = (Way_t *)malloc(sizeof(Way_t) * world.way_count)) == nullptr)
    {
		return;
	}
    memset(world.ways, 0x00, sizeof(Way_t) * world.way_count);
#endif

	int index = 0;
	for (auto i = m_waylist.cbegin(), end = m_waylist.cend(); i != end; ++i)
	{
		Way_t way = i.value();
		//memcpy(world.ways + (index * sizeof(Way_t)), &way, sizeof(Way_t));
		world.ways[index].id = way.id;
		world.ways[index].type = way.type;
		world.ways[index].nd_id = way.nd_id;
		world.ways[index].n_nd_id = way.n_nd_id;
		world.ways[index].lanes = way.lanes;
		world.ways[index].width = way.width;
		world.ways[index].name_id = way.name_id;
		world.ways[index].parking = way.parking;
		index++;
	}
#ifdef TESTX
	world.info.node.count = m_nodelist.size();
	if((world.nodes = (Point_t *)malloc(sizeof(Point_t) * world.info.node.count)) == nullptr)
	{
		return;
	}
	memset(world.nodes, 0x00, sizeof(Point_t) * world.info.node.count);
#else
    world.node_count = m_nodelist.size();
    if((world.nodes = (Point_t *)malloc(sizeof(Point_t) * world.node_count)) == nullptr)
    {
        return;
    }
    memset(world.nodes, 0x00, sizeof(Point_t) * world.node_count);
#endif
	index = 0;
	for (auto i = m_nodelist.cbegin(), end = m_nodelist.cend(); i != end; ++i)
	{
		Point_t node = i.value();
		world.nodes[index].id = node.id;
		world.nodes[index].x = node.x;
		world.nodes[index].y = node.y;
		index++;
	}

	TR_INF << "nodes:" << m_nodelist.size() << "ways:" << m_waylist.size();
}

void TrImportOsmStream::setNodeIds(World_t & world)
{
	int index = 0;
	for (auto i = m_nodelist.cbegin(), end = m_nodelist.cend(); i != end; ++i)
	{
		world.nodes[index].id = i.key();
		index++;
	}
}

void TrImportOsmStream::addRelation(const Relation & rel)
{
	Rel_t crel;
	crel.flags = rel.m_flags;
	crel.r_count = rel.m_members.size();
	crel.id = 0;
    if((crel.members = (RelMember_t *)malloc(sizeof(RelMember_t) * crel.r_count)) == nullptr)
		return;
	for(size_t i = 0; i<crel.r_count; i++)
	{
		crel.members[i] = rel.m_members[i];
	}
	m_rellist.append(crel);
}

uint64_t TrImportOsmStream::getClass(const QString & value)
{
	uint64_t ret = 0;

	if(value == "service")
		return 0x0000000000000009;
	if(value == "escape")
		return 0x0000000000000009;
	if(value == "bus_guideway")
		return 0x0000000000000009;

	if(value == "track")
		return 0x000000000000000A;

	if(value == "path")
		return 0x000000000000000B;
	if(value == "bridleway")
		return 0x000000000000000B;
	if(value == "footway")
		return 0x000000000000000F;
	if(value == "cycleway")
		return 0x000000000000000C;
	if(value == "steps")
		return 0x000000000000000D;

	if(value == "pedestrian")
		return 0x000000000000000E;

	if(value.endsWith("_link"))
	{
		ret |= FLAG_RAMP;
	}
	if(value.startsWith("motorway"))
		return (ret | 0x0000000000000001);

	if(value.startsWith("trunk"))
		return (ret | 0x0000000000000002);

	if(value.startsWith("primary"))
		return (ret | 0x0000000000000003);

	if(value.startsWith("secondary"))
		return (ret | 0x0000000000000004);

	if(value.startsWith("tertiary"))
		return (ret | 0x0000000000000005);

	if(value.startsWith("unclassified"))
		return (ret | 0x0000000000000006);

	if(value.startsWith("residential"))
		return (ret | 0x0000000000000007);

	if(value.startsWith("living_street"))
		return (ret | 0x0000000000000008);

	return 0;
}

uint64_t TrImportOsmStream::getDir(const QString & value)
{
	if(value == "1")
		return(1);

	if(value == "true")
		return(1);

	if(value == "yes")
		return(1);

	if(value == "-1")
		return(2);

	if(value == "roundabout")
		return(1);

	if(value == "mini_roundabout")
		return(1);

	//return 1;
	return(0);              // both directions
}

uint64_t TrImportOsmStream::getLanes()
{
	uint64_t ret = 0;
	uint64_t lane = 0;
	bool ok = false;

	if(m_tags.contains("lanes"))
	{
		lane = m_tags["lanes"].toULongLong(&ok);
		if(ok)
			ret |= (lane & 0x0000000000000007);
		//m_id = attributes.value("id").toULongLong(&ok);
	}
	if(m_tags.contains("lanes:forward"))
	{
		lane = m_tags["lanes:forward"].toULongLong(&ok);
		if(ok)
			ret |= (lane << 16);
	}
	if(m_tags.contains("lanes:backward"))
	{
		lane = m_tags["lanes:backward"].toULongLong(&ok);
		if(ok)
			ret |= (lane << 24);
	}
	// TODO:
	//"lanes:bus"
	//"lanes:psv"
	//"lanes:taxi"
	//"lanes:hov"

	// TODO: turn-lane etc.

	return ret;
}

bool TrImportOsmStream::parkingMode(const QString & part, uint64_t & code)
{
	//TR_INF << part;
	if(part == "both")
	{
		code |= 0x0000000000000001;
		code |= 0x0000000100000000;
		return true;
	}
	if(part == "left")
	{
		code |= 0x0000000100000000;
		return true;
	}
	if(part == "right")
	{
		code |= 0x0000000000000001;
		return true;
	}
	return false;
}

uint64_t TrImportOsmStream::parkingRes(const QString & value, uint64_t & code)
{
	uint64_t ret = 0;
	if(value == "no")
	{
		if(code & 0x0000000000000001)
			ret |= V_PARK_NO_R;
		if(code & 0x0000000100000000)
			ret |= V_PARK_NO_L;
	}
	if(value == "no_parking")
	{
		if(code & 0x0000000000000001)
			ret |= V_PARK_NO_R;
		if(code & 0x0000000100000000)
			ret |= V_PARK_NO_L;
	}
	if(value == "no_stopping")
	{
		if(code & 0x0000000000000001)
			ret |= V_PARK_NO_R;
		if(code & 0x0000000100000000)
			ret |= V_PARK_NO_L;
	}
	return ret;
}

uint64_t TrImportOsmStream::parkingTest(const QString & part, const QString & value, uint64_t & code)
{
	uint64_t ret = 0;

	if(part != "orientation")
		return ret;

	if(value == "parallel")
	{
		if(code & 0x0000000000000001)
			ret |= V_PARK_PARALLEL_R;
		if(code & 0x0000000100000000)
			ret |= V_PARK_PARALLEL_L;
	}
	if(value == "diagonal")
	{
		if(code & 0x0000000000000001)
			ret |= V_PARK_DIAGONAL_R;
		if(code & 0x0000000100000000)
			ret |= V_PARK_DIAGONAL_L;
	}
	if(value == "perpendicular")
	{
		if(code & 0x0000000000000001)
			ret |= V_PARK_PERPENDI_R;
		if(code & 0x0000000100000000)
			ret |= V_PARK_PERPENDI_L;
	}
	if(!ret)
		return ret;
	if(code & 0x0000000000000001)
		ret |= FLAG_PARKING_R;
	if(code & 0x0000000100000000)
		ret |= FLAG_PARKING_L;
	return ret;
}

// act_way->parking = (world->way_parking >> 20);
// exp: <tag k='parking:left:orientation' v='parallel' />
uint64_t TrImportOsmStream::getParking()
{
	QMapIterator<QString, QString> i(m_tags);
	uint64_t ret = 0;
	uint64_t mode = 0;
	while (i.hasNext())
	{
		i.next();
		if(i.key().startsWith("parking"))
		{
			QStringList plist = i.key().split(':');
			if(plist.size() > 1)
			{
				mode = 0;
				if(parkingMode(plist[1], mode))
				{
					if(plist.size() > 2)
					{
						if(plist[2] == "restriction")
						{
							ret |= parkingRes(i.value(), mode);
						}
						ret |= parkingTest(plist[2], i.value(), mode);
					}
					else
					{
						// "no", "no_parking", "no_stopping"
						if(i.value().startsWith("no"))
						{
							if(mode & 0x0000000000000001)
								ret |= V_PARK_NO_R;
							if(mode & 0x0000000100000000)
								ret |= V_PARK_NO_L;
						}
					}
				}
				else
				{
				}
			}
			//TR_INF << plist << i.value() << HEX << mode << ret;
		}
	}
	return ret;
}

QString TrImportOsmStream::errorString() const
{
    return QString();
}
