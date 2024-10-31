/******************************************************************
 *
 * @short	data import in OSM format
 *
 * project:	Trafalgar/Osm
 *
 * class:	TrImportOsmRel
 * superclass:	---
 * modul:	tr_import_osm_rel.cc
 *
 * system:	UNIX/LINUX
 * compiler:	gcc
 *
 * beginning:	2.2024
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



#include "tr_import_osm_rel.h"

#include "osm_load_rel.h"

#include <tr_prof_class_def.h>

Relation::Relation()
        : m_flags(0)
{
}

QDebug operator<<(QDebug dbg, const Relation & member)
{
        return dbg << "size: " << member.m_members.size() << " flags:" << HEX << member.m_flags;
}

// TODO: check use of complex use of 'inner' and 'outer' members
// outer inner inner outer...
// https://wiki.openstreetmap.org/wiki/Relation:multipolygon
int Relation::isMultiPolyRing()
{
	int count_out = 0;
	if(m_flags & FLAG_MULTI_POLY)
	{
		for (int i = 0; i < m_members.size(); ++i)
		{
			if(m_members[i].flags & REL_MEM_ROLE_OUT)
				count_out++;
		}
	}
	return count_out;
}

void Relation::resetPolyRing(QMap<uint64_t, Way_t> & waylist)
{
	if(!(m_flags & FLAG_MULTI_POLY))
		return;

	for (int i = 0; i < m_members.size(); ++i)
	{
		if(waylist.contains(m_members[i].id))
		{
			if(m_members[i].flags & REL_MEM_ROLE_OUT)
				waylist[m_members[i].id].type &= ~FLAG_FEATURE_AERA;
		}
	}
}

TrImportOsmRel::TrImportOsmRel()
	: m_id(0)
{
}

TrImportOsmRel::~TrImportOsmRel()
{
}

bool TrImportOsmRel::osmRelationRead(QXmlStreamReader & xml, QMap<uint64_t, Way_t> & waylist, Relation & rel)
		//QMap<uint64_t, Rel_t> & rellist)
{
	rel.m_flags = 0;
	rel.m_members.clear();
	QXmlStreamAttributes attrs = xml.attributes();
	m_tags.clear();

	readRelation(attrs, rel);

	while (!xml.atEnd())
	{
		xml.readNext();
		if(xml.isStartElement())
		{
			if(xml.name() == "osm")
			{
				TR_WRN << xml.name() << "not allowed";
			}
			if(xml.name() == "node")
			{
				TR_WRN << xml.name() << "not allowed";
			}
			if(xml.name() == "way")
			{
				TR_WRN << xml.name() << "not allowed";
			}
			if(xml.name() == "relation")
			{
				TR_WRN << xml.name() << "inside";
			}
			if(xml.name() == "tag")
			{
				QXmlStreamAttributes attrs = xml.attributes();
				readTag(attrs);
			}
			if(xml.name() == "member")
			{
				QXmlStreamAttributes attrs = xml.attributes();
				readMember(attrs, rel);
			}
		}
		if(xml.isEndElement())
		{
			if(xml.name() == "osm")
			{
				TR_WRN << xml.name() << "not allowed";
			}
			if(xml.name() == "member")
			{
				//TR_INF << xml.name();
				//closeMember();
			}
			if(xml.name() == "way")
			{
				TR_WRN << xml.name() << "not allowed";
				//TR_INF << "close way" << m_tags;
				//closeWay(world.m_name_map, world.act_name_idx);
			}
			if(xml.name() == "relation")
			{
				closeRelation(waylist, rel);
				return true;
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
	return false;
}

void TrImportOsmRel::readRelation(const QXmlStreamAttributes &attributes, Relation & rel)
{
	bool ok;

	rel.m_members.clear();
	m_tags.clear();
	int64_t id = attributes.value("id").toULongLong(&ok);
	if(ok)
	{
		m_id = id;
	}
}


void TrImportOsmRel::readMember(const QXmlStreamAttributes &attributes, Relation & rel)
{
	bool ok;

	QString type = attributes.value("type").toString();
	if(type != "way")
		return;

	RelMember_t member;
	member.flags = 0;
	//m_id
	member.id = attributes.value("ref").toULongLong(&ok);
	if(!ok)
	{
		member.id = -1;
		return;
	}

	QString role = attributes.value("role").toString();
	if(role == "outer")
		member.flags |= REL_MEM_ROLE_OUT;
	if(role == "inner")
		member.flags |= REL_MEM_ROLE_IN;
	//type='way' ref='25505835' role
	//TR_INF << type << id << role;
	rel.m_members.append(member);
}

void TrImportOsmRel::readTag(const QXmlStreamAttributes &attributes)
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

void TrImportOsmRel::closeRelation(QMap<uint64_t, Way_t> & waylist, Relation & rel)
{
        rel.m_flags = 0;

	if(m_tags.contains("type"))
	{
		if(m_tags["type"] == "multipolygon")
			rel.m_flags |= FLAG_MULTI_POLY;
		if(m_tags["type"] == "route")
			rel.m_flags |= FLAG_ROUTE;
	}

	if(rel.m_flags & FLAG_MULTI_POLY)
	{
		handleMultiPoly(waylist, rel);
	}
	if(rel.m_flags & FLAG_ROUTE)
	{
		// not now
	}

	// avoid tag double use
	m_tags.clear();
	//m_members.clear();
}

bool TrImportOsmRel::handleMultiPoly(QMap<uint64_t, Way_t> & waylist, Relation & rel)
{
	uint64_t type = 0;

	//TR_INF << m_tags << HEX << TYPE_BUILDING;
	if(m_tags.contains("building"))
	{
		type = getBuildingClass(m_tags["building"]);
		rel.m_flags |= (TYPE_BUILDING | type);
	}
	if(m_tags.contains("highway"))
	{
		if(m_tags["highway"] == "pedestrian")
			return false;
	}
	if(m_tags.contains("natural"))
	{
		type = getNaturalClass(m_tags["natural"]);
		rel.m_flags |= (TYPE_NATURAL | type);
	}
	if(m_tags.contains("landuse"))
	{
		type = getLanduseClass(m_tags["landuse"]);
		rel.m_flags |= (TYPE_LANDUSE | type);
	}
	// TODO: remove this - do it this in "tr_import_osm" for both filters
	for (int i = 0; i < rel.m_members.size(); ++i)
	{
		if(rel.m_flags & TYPE_BUILDING)
		{
			if(waylist.contains(rel.m_members[i].id))
			{
                //TR_INF << HEX << rel.m_flags << waylist[rel.m_members[i].id].type;
				waylist[rel.m_members[i].id].type = rel.m_flags;
				if(rel.m_members[i].flags & REL_MEM_ROLE_IN)
				{
					waylist[rel.m_members[i].id].type &= 0xfffffffffffffff0;
					waylist[rel.m_members[i].id].type |= 0x0000000000000005;
				}
			}
		}
		if(rel.m_flags & TYPE_NATURAL)
		{
			if(waylist.contains(rel.m_members[i].id))
			{
				//TR_INF << rel.m_members[i].id;
				waylist[rel.m_members[i].id].type = rel.m_flags;
				if(rel.m_members[i].flags & REL_MEM_ROLE_IN)
				{
					uint64_t cl = waylist[rel.m_members[i].id].type & 0x000000000000000ff;
					if(!cl)
						cl = 0x0000000000000003;
					waylist[rel.m_members[i].id].type &= 0xfffffffffffffff0;
					waylist[rel.m_members[i].id].type |= cl;
				}
			}
		}
	}
	return true;
}

/*
<tag k='addr:city' v='München' />
<tag k='addr:housenumber' v='150' />
<tag k='addr:postcode' v='80807' />
<tag k='addr:street' v='Frankfurter Ring' />
<tag k='entrance' v='yes' />
*/

uint64_t TrImportOsmRel::getBarrierClass(const QString & value)
{
	if(value == "guard_rail")
		return 1;

	//TR_INF << value;
	if(value == "gate")
		return (FLAG_FEATURE_NODE | 2);
	if(value == "bollard")
		return (FLAG_FEATURE_NODE | 3);
	if(value == "entrance")
		return (FLAG_FEATURE_NODE | 2);
	return 0;
}

uint64_t TrImportOsmRel::getBuildingClass(const QString & value)
{
	if(value == "yes")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "house")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "bungalow")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "terrace")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "detached")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "semidetached_house")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "residential")
		return (BUILDING_APAR | FLAG_FEATURE_AERA);
	if(value == "apartments")
		return (BUILDING_APAR | FLAG_FEATURE_AERA);
	if(value == "dormitory")
		return (BUILDING_APAR | FLAG_FEATURE_AERA);
	if(value == "retail")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "office")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "commercial")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "warehouse")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "roof")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "bridge")
		return (BUILDING_SERVICE | FLAG_FEATURE_AERA);
	if(value == "kiosk")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "church")
		return (BUILDING_REL | FLAG_FEATURE_AERA);
	if(value == "chapel")
		return (BUILDING_REL | FLAG_FEATURE_AERA);
	if(value == "public")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "civic")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "school")
		return (BUILDING_LEARN | FLAG_FEATURE_AERA);
	if(value == "hospital")
		return (BUILDING_MEDI | FLAG_FEATURE_AERA);
	if(value == "fire_station")
		return (BUILDING_SERVICE | FLAG_FEATURE_AERA);
	if(value == "kindergarten")
		return (BUILDING_LEARN | FLAG_FEATURE_AERA);
	if(value == "industrial")
		return (BUILDING_INDUST | FLAG_FEATURE_AERA);
	if(value == "farm")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "farm_auxiliary")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "barn")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "cowshed")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "stable")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
    if(value == "allotment_house")
        return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
    if(value == "hut")
        return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
    if(value == "sports_centre")
        return (BUILDING_SERVICE | FLAG_FEATURE_AERA);
    if(value == "greenhouse")
    {
        return (BUILDING_INDUST | FLAG_FEATURE_AERA);
    }
    if(value == "garage")
		return (BUILDING_CAR | FLAG_FEATURE_AERA);
	if(value == "garages")
		return (BUILDING_CAR | FLAG_FEATURE_AERA);
	if(value == "carport")
		return (BUILDING_CAR | FLAG_FEATURE_AERA);
	if(value == "parking")
		return (BUILDING_CAR | FLAG_FEATURE_AERA);
	if(value == "hotel")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "government")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "museum")
		return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
	if(value == "university")
		return (BUILDING_LEARN | FLAG_FEATURE_AERA);
	if(value == "ruins")		// TODO: new building class?
		return (BUILDING_RUINS | FLAG_FEATURE_AERA);
	if(value == "shed")
		return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
	if(value == "guardhouse")
		return (BUILDING_SERVICE | FLAG_FEATURE_AERA);
	if(value == "service")
		return (BUILDING_SERVICE | FLAG_FEATURE_AERA);
    if(value == "chalet")
    {
        return (BUILDING_HOUSE | FLAG_FEATURE_AERA);
    }
    if(value == "sports_hall")
    {
        return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
    }
    if(value == "clinic")
    {
        return (BUILDING_PUBLIC | FLAG_FEATURE_AERA);
    }
    if(value == "water_tower")
    {
        return (BUILDING_SERVICE | FLAG_FEATURE_AERA);
    }
    return (0x0007 | FLAG_FEATURE_AERA);
}

// TODO:
// <tag k='craft' v='shoemaker; locksmith' />
//<tag k='railway' v='stop' />

uint64_t TrImportOsmRel::getHighwayPointClass(const QString & value)
{
	if(value == "level_crossing")
		return (1 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "crossing")
		return (1 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "street_lamp")
		return 2;
	if(value == "traffic_signals")
		return (7 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "bus_stop")
		return (8 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "turning_circle")
		return (6 | FLAG_FEATURE_NODE | TYPE_ROAD);
	return 0;
}

uint64_t TrImportOsmRel::getRailwayPointClass(const QString & value)
{
	if(value == "milestone")
		return 1;
	if(value == "switch")
		return 1;
	if(value == "level_crossing")
		return 1;
	if(value == "crossing")
		return 1;
	if(value == "railway_crossing")
		return 1;
	if(value == "tram_crossing")
		return 1;
	if(value == "tram_level_crossing")
		return 1;
	if(value == "buffer_stop")
		return 1;
	if(value == "stop")
		return (8 | FLAG_FEATURE_NODE | TYPE_RAIL);
	if(value == "tram_stop")
		return (8 | FLAG_FEATURE_NODE | TYPE_RAIL);
	if(value == "station")
		return (8 | FLAG_FEATURE_NODE | TYPE_RAIL);
	if(value == "signal")
		return (7 | FLAG_FEATURE_NODE | TYPE_RAIL);
	return 0;
}

//<tag k='natural' v='tree' />
/*uint64_t TrImportOsmRel::getNaturalClass(const QString & value)
{
	if(value == "tree")
		return (1 | FLAG_FEATURE_NODE | TYPE_NATURAL);
	return 0;
}*/

uint64_t TrImportOsmRel::getAmenityClass(const QString & value)
{
	//TR_INF << value;
	if(value == "bench")
		return (1);
	if(value == "waste_basket")
		return (1);
	if(value == "recycling")
		return (1);
	if(value == "fountain")
		return (1);
	if(value == "hunting_stand")
		return (1);
	if(value == "parking")
		return (2 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "bicycle_parking")
		return (2 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "parking_entrance")
		return (2 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "vending_machine")
		return (2 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "restaurant")
		return (1 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "cafe")
		return (1 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "bar")
		return (1 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "pub")
		return (1 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "fast_food")	// POI
		return (1 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "canteen")
		return (1 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "doctors")
		return (5 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "veterinary")
		return (5 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "dentist")
		return (5 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "pharmacy")
		return (5 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "post_box")
		return (6 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "kindergarten")
		return (6 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "place_of_worship")
		return (3 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "shelter")
		return (4 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "fuel")
		return (5 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "charging_station")
		return (6 | FLAG_FEATURE_NODE | TYPE_ROAD);
	if(value == "taxi")
		return (5 | FLAG_FEATURE_NODE | TYPE_ROAD);
	return 0;
}

uint64_t TrImportOsmRel::getShopClass(const QString & value)
{
	if(value == "vacant")
		return 1;
	//shop' v='kiosk
	if(value == "kiosk")
		return (1 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "supermarket")
		return (2 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "convenience")
		return (2 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "beverages")
		return (2 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "wine")
		return (2 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "bakery")
		return (2 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "butcher")
		return (2 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "florist")
		return (1 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "beauty")
		return (3 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "laundry")
		return (3 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "hairdresser")
		return (3 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "electronics")
		return (4 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "clothes")
		return (2 | FLAG_FEATURE_NODE | TYPE_PUBLIC);
	if(value == "car_repair")
		return (5 | FLAG_FEATURE_NODE | TYPE_ROAD);

	//"bicycle"
	return 0;
}

uint64_t TrImportOsmRel::getLanduseClass(const QString & value)
{
	if(value == "forest")
		return (LANDUSE_WOOD  | FLAG_FEATURE_AERA);
	if(value == "grass")            // secondary layer
		return (LANDUSE_GRASS | FLAG_FEATURE_AERA);
	if(value == "recreation_ground")
		return (LANDUSE_GRASS | FLAG_FEATURE_AERA);
	if(value == "meadow")
		return (LANDUSE_GRASS | FLAG_FEATURE_AERA);
	if(value == "village_green")
		return (LANDUSE_BUSHES | FLAG_FEATURE_AERA);
	if(value == "farmland")
		return (LANDUSE_BUSHES | FLAG_FEATURE_AERA);
	if(value == "commercial")       // base layer?
		return (LANDUSE_MIXED | FLAG_FEATURE_AERA);
	if(value == "residential")      // base layer
		return (LANDUSE_RESIDENTIAL | FLAG_FEATURE_AERA);
	if(value == "construction")     // secondary layer?
		return (LANDUSE_INDUSTRIAL | FLAG_FEATURE_AERA);
	return 0;
}

uint64_t TrImportOsmRel::getNaturalClass(const QString & value)
{
	if(value == "wood")
		//return (NATURAL_FOREST | FLAG_FEATURE_AERA);
		return (LANDUSE_WOOD  | FLAG_FEATURE_AERA);
	if(value == "grassland")
		return (LANDUSE_GRASS | FLAG_FEATURE_AERA);
	if(value == "water")
		return (NATURAL_WATER | FLAG_FEATURE_AERA);
	if(value == "shingle")
		return (NATURAL_WET | FLAG_FEATURE_AERA);

	if(value == "tree")	// POI
	{
		//TR_INF << "-----tree------";
		return (8 | FLAG_FEATURE_NODE | TYPE_POI_N_TREE | TYPE_NATURAL);
	}
	return 0;
}

uint64_t TrImportOsmRel::getWaterWayClass(const QString & value)
{
	if(value == "river")
		return (WATER_RIVER | FLAG_FEATURE_AERA);
	if(value == "stream")
		return (WATER_STREAM | FLAG_FEATURE_AERA); //FLAG_FEATURE_WAY);
	return 0;
}

uint64_t TrImportOsmRel::getRailWayClass(const QString & value)
{
    if(value == "rail")
        return 0x0000000000000080;
    if(value == "subway")
        return 0x0000000000000090;
    if(value == "light_rail")
        return 0x00000000000000A0;
    if(value == "tram")
        return 0x00000000000000A0;
    if(value == "narrow_gauge")
        return 0x00000000000000B0;
    return 0;
}