/**
   MiracleGrue - Model Generator for toolpathing. <http://www.grue.makerbot.com>
   Copyright (C) 2011 Far McKon <Far@makerbot.com>, Hugo Boyer (hugo@makerbot.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

*/

#include "gcoder.h"

using namespace mgl;
using namespace std;


// function that adds an s to a noun if count is more than 1
std::string plural(const char*noun, int count, const char* ending = "s")
{
	string s(noun);
	if (count>1)
	{
		return s+ending;
	}
	return s;
}

Vector2 unitVector(const Vector2& a, const Vector2& b)
{
	double dx = b.x - a.x;
	double dy = b.y - a.y;
	double length = sqrt(dx *dx + dy * dy);
	Vector2 r(dx/length , dy/length);
	return r;
}

//
// computes 2 positions (one before and one at the end of) the polygon and stores them in start and stop.
// These positions are aligned with the fisrt line and last line of the polygon.
// LeadIn is the distance between start and the first point of the polygon (along the first polygon line).
// LeadOut is the distance between the last point of the Polygon and stop (along the last polygon line).
void polygonLeadInAndLeadOut(const Polygon &polygon, double leadIn, double leadOut, Vector2 &start, Vector2 &end)
{
	size_t count =  polygon.size();
	assert(count >= 2);

	const Vector2 &a = polygon[0];	// first element
	const Vector2 &b = polygon[1];

	const Vector2 &c = polygon[count-2];
	const Vector2 &d = polygon[count-1]; // last element

	Vector2 ab = unitVector(a,b);
	Vector2 cd = unitVector(c,d);

	start.x = a.x - ab.x * leadIn;
	start.y = a.y - ab.y * leadIn;
	end.x   = d.x + cd.x * leadOut;
	end.y   = d.y + cd.y * leadOut;

}

void mgl::writeGcodeFile(const Configuration &config,
					const char* gcodeFilePath,
					const std::vector<SliceData> & slices)
{
	GCoder gcoder = GCoder();
	gcoder.loadData(config);
    std::ofstream gout(gcodeFilePath);
    gcoder.writeStartOfFile(gout);

    ProgressBar progress(slices.size());
    for(int i = 0;i < slices.size();i++){
        progress.tick();
        cout.flush();
        const SliceData &slice = slices[i];
        gcoder.writeSlice(gout, slice);
    }
    gcoder.writeGcodeEndOfFile(gout);
    gout.close();
}


void GCoder::writeSlice(ostream& ss, const SliceData& sliceData)
{

	GCoder &gcoder = *this;
	double layerZ = sliceData.z;
	unsigned int extruderCount = sliceData.extruderSlices.size();

	ss << "(Slice " << sliceData.sliceIndex << ", " << extruderCount << " " << plural("Extruder", extruderCount) << ")"<< endl;
	for(unsigned int extruderId = 0; extruderId < extruderCount; extruderId++)
	{
		ss << "(   Extruder " <<  extruderId << ")" << endl;
		const Polygons &loops = sliceData.extruderSlices[extruderId].loops;
		const Polygons &infills = sliceData.extruderSlices[extruderId].infills;

		double z = layerZ + gcoder.extruders[extruderId].nozzleZ;

		if (extruderCount > 0)
		{
			writeSwitchExtruder(ss, extruderId);
		}

		writePaths(ss, sliceData.sliceIndex, extruderId, z, loops);
		writePaths(ss, sliceData.sliceIndex, extruderId, z, infills);

		if (extruderCount > 0)
		{
			writeWipeExtruder(ss, extruderId);
		}
		extruderId ++;
	}
}

void GCoder::writeSwitchExtruder(ostream& ss, int extruderId) const
{
	ss << "( extruder " << extruderId << " )" << endl;
	ss << "( GSWITCH T" << extruderId << " )" << endl;
	ss << endl;
}

void GCoder::writeWipeExtruder(ostream& ss, int extruderId) const
{
	ss << "( GWIPE my extruder #" << extruderId << " )"<< endl;
	ss << endl;
}

void GCoder::writeGCodeConfig(std::ostream &ss) const
{

	ss << endl;
	ss << "(Makerbot Industries)" << endl;
	ss << "(This file contains digital fabrication directives in gcode format)"<< endl;
	ss << "(For your 3D printer)" << endl;
	ss << "(http://wiki.makerbot.com/gcode)" <<  endl;
	writeGcodeConfig(ss, "* ");
	ss << endl;
}

void GCoder::writeMachineInitialization(std::ostream &ss) const
{
	ss <<  "G21 (set units to mm)" << endl;
	ss <<  "G90 (absolute positioning mode)" << endl;

	const GCoder &gcoder = *this;
	int toolHeadId = 0;
	if (gcoder.extruders.size() > 1)
	{
		for (std::vector<ToolHead>::const_iterator i= gcoder.extruders.begin(); i!=gcoder.extruders.end(); i++)
		{
			ToolHead t = *i;
			int coordinateSystemNb = toolHeadId +1;
			ss << "G10 P" << coordinateSystemNb << " X" <<  t.coordinateSystemOffsetX << " Y0 Z-0.3" << endl;
			toolHeadId ++;
		}
	}
	ss << endl;
}


void GCoder::writeExtrudersInitialization(std::ostream &ss) const
{
	const GCoder &gcoder = *this;
	string plural = "";
	if(gcoder.extruders.size()>1) plural = "s";
	ss << "(setup extruder" << plural <<")" <<endl;
	int toolHeadId = 0;
	for (std::vector<ToolHead>::const_iterator i= gcoder.extruders.begin(); i!=gcoder.extruders.end(); i++)
	{
		ToolHead t = *i;
		ss << "M103 T" << toolHeadId << " (Make sure motor for extruder " << toolHeadId << " is stopped)" << endl;
		ss << "M108 R" << t.defaultExtrusionSpeed << " T" << toolHeadId << " (set extruder " <<  toolHeadId << " speed to the default " << t.defaultExtrusionSpeed << " RPM)" << endl;
		ss << "M104 S" << t.extrusionTemperature  << " T" << toolHeadId << " (set temperature of extruder " << toolHeadId <<  " to "  << t.extrusionTemperature << " degrees Celsius)" << endl;
		ss << endl;
		toolHeadId ++;
	}
	ss << endl;
}


void GCoder::writePlatformInitialization(std::ostream &ss) const
{

	double t = platform.temperature;
	ss << "M109 S" << t << " T0 (heat the build-platform to "  << t << " Celsius)" << endl;
	ss << endl;

}


void GCoder::writeHomingSequence(std::ostream &ss) const
{
	const GCoder &gcoder = *this;

	ss << endl;
	ss << "(go to home position)" << endl;

	if(gcoder.homing.zMaxHoming)
		ss << "G162 Z F800 (home Z axis maximum)" << endl;
	else
		ss << "G161 Z F800 (home Z axis minimum)" << endl;

	ss << "G92 Z5 (set Z to 5)" << endl;
	ss << "G1 Z0.0 (move Z down 0)" << endl;

	if(gcoder.homing.zMaxHoming)
		ss << "G162 Z F100 (home Z axis maximum)" << endl;
	else
		ss << "G161 Z F100 (home Z axis minimum)" << endl;

	if(gcoder.homing.xyMaxHoming)
		ss << "G162 X Y F2500 (home XY axes maximum)" << endl;
	else
		ss << "G161 X Y F2500 (home XY axes minimum)" << endl;

	ss << "M132 X Y Z A B (Recall stored home offsets for XYZAB axis)" << endl;
	if (gcoder.extruders.size() > 1)
		ss << "G54 (first work coordinate system)" << endl;
	ss << endl;
}

void GCoder::writeWarmupSequence(std::ostream &ss)
{
	GCoder &gcoder = *this;
	ss << endl;
	gcoder.extruders[0].g1(ss, 	gcoder.platform.waitingPositionX,
				gcoder.platform.waitingPositionY,
				gcoder.platform.waitingPositionZ,
				gcoder.extruders[0].fastFeedRate,
				"go to waiting position" );

	for (int i=0; i< gcoder.extruders.size(); i++)
	{
		ss << "M6 T" << i << " (wait for tool " << i<<" to reach temperature)" << endl;
	}
	ss << "(note: the heated build platform temperature is tied to tool 0 for now)" << endl;
	ss << endl;
	ss << endl;
}

void GCoder::writeStartOfFile(std::ostream &gout)
{
	writeGCodeConfig(gout);
	writeMachineInitialization(gout);
	writeExtrudersInitialization(gout);
	writePlatformInitialization(gout);
	writeHomingSequence(gout);
	writeWarmupSequence(gout);
	writeAnchor(gout);
}

void GCoder::writeGcodeEndOfFile(std::ostream &ss) const
{
	for (int i=0; i< extruders.size(); i++)
	{
		ss << "M104 S0 T" << i << " (set extruder temperature to 0)" << endl;
		ss << "M109 S0 T" << i << " (set heated-build-platform id tied an extrusion tool)" << endl;
	}

	if(homing.zMaxHoming)
		ss << "G162 Z F500 (home Z axis maximum)" << endl;
	ss << "(That's all folks!)" << endl;
}

void GCoder::writeAnchor(std::ostream &ss)
{
	GCoder &gcoder = *this;
	ss << "(Create Anchor)" << endl;
	ss << "G1 Z0.6 F300    (Position Height)" << endl;
	ss << "M108 R4.0   (Set Extruder Speed)" << endl;
	ss << "M101        (Start Extruder)" << endl;
	ss << "G4 P1600" << endl;

	gcoder.extruders[0].g1( ss,
							gcoder.platform.waitingPositionX,
							gcoder.platform.waitingPositionY,
							0.6,
							gcoder.extruders[0].slowFeedRate,
							NULL );

	double dx = gcoder.platform.waitingPositionX - 3.0;
	double dy = gcoder.platform.waitingPositionY - 0.0;

	gcoder.extruders[0].g1(ss, dx, dy, 0.6, 0.2 * gcoder.extruders[0].slowFeedRate , NULL);
	ss << endl;
}

void GCoder::loadData(const Configuration& conf)
{

	programName = conf.root["programName"].asString();
	versionStr  = conf.root["versionStr"].asString();
	machineName = conf.root["machineName"].asString();
	firmware    = conf.root["firmware"].asString();		// firmware revision
	gcodeFilename = conf.root["gcodeFilename"].asString();

	homing.xyMaxHoming = conf.root["homing"]["xyMaxHoming"].asBool();
	homing.zMaxHoming  = conf.root["homing"]["zMaxHoming" ].asBool();

	//fastFeed = conf.root["firmware"].asString();
	scalingFactor = conf.root["scalingFactor"].asDouble();

	platform.temperature = conf.root["platform"]["temperature"].asDouble();
	platform.automated   = conf.root["platform"]["automated"].asBool();
	platform.waitingPositionX = conf.root["platform"]["waitingPositionX"].asDouble();
	platform.waitingPositionY = conf.root["platform"]["waitingPositionY"].asDouble();
	platform.waitingPositionZ = conf.root["platform"]["waitingPositionZ"].asDouble();

	outline.enabled  = conf.root["outline"]["enabled"].asBool();
	outline.distance = conf.root["outline"]["distance"].asDouble();

	assert(conf.root["extruders"].size() >= 1);

	unsigned int extruderCount = conf.root["extruders"].size();
	for(int i=0; i < extruderCount; i++)
	{
		ToolHead toolHead;
		toolHead.coordinateSystemOffsetX = conf.root["extruders"][i]["waitingPositionZ"].asDouble();//(0
		toolHead.extrusionTemperature = conf.root["extruders"][i]["extrusionTemperature"].asDouble();//(220),
		toolHead.defaultExtrusionSpeed = conf.root["extruders"][i]["defaultExtrusionSpeed"].asDouble();//(3),
		toolHead.slowFeedRate = conf.root["extruders"][i]["slowFeedRate"].asDouble();//(1080),
		toolHead.slowExtrusionSpeed = conf.root["extruders"][i]["slowExtrusionSpeed"].asDouble();//(1.0),
		toolHead.fastFeedRate = conf.root["extruders"][i]["fastFeedRate"].asDouble();//;(3000),
		toolHead.fastExtrusionSpeed = conf.root["extruders"][i]["fastExtrusionSpeed"].asDouble();//(2.682),
		toolHead.nozzleZ = conf.root["extruders"][i]["nozzleZ"].asDouble();
		toolHead.reversalExtrusionSpeed = conf.root["extruders"][i]["reversalExtrusionSpeed"].asDouble();

		toolHead.leadIn = conf.root["extruders"][i]["leadIn"].asDouble();
		toolHead.leadOut = conf.root["extruders"][i]["leadOut"].asDouble();

		extruders.push_back(toolHead);
	}

}

void GCoder::writePaths(std::ostream& ss,
							unsigned int sliceIndex,
							unsigned int extruderId,
							double z,
							const Polygons &paths)
{
	GCoder &gcoder = *this;
	// to each extruder its speed
	double pathFeedrate = gcoder.scalingFactor * gcoder.extruders[extruderId].slowFeedRate;
	double extrusionSpeed = gcoder.scalingFactor * gcoder.extruders[extruderId].fastExtrusionSpeed;
	double leadIn = gcoder.extruders[extruderId].leadIn;
	double leadOut = gcoder.extruders[extruderId].leadOut;

	// moving all up. This is the first move for every new layer
	gcoder.extruders[extruderId].g1(ss,
									gcoder.extruders[extruderId].x,
									gcoder.extruders[extruderId].y,
									z,
									pathFeedrate, NULL);

	int path_counter = 0;
	for (Polygons::const_iterator pathIt = paths.begin() ; pathIt != paths.end();  pathIt ++)
	{

		path_counter ++; // one based, FTU (for the user!)
		const Polygon &polygon = *pathIt;
		ss << "(  slice " << sliceIndex << " , path " << path_counter << "/" << paths.size() << ", " << polygon.size() << " points, "  << " )" << endl;

		Vector2 start(0,0), stop(0,0);
		polygonLeadInAndLeadOut(polygon, leadIn, leadOut, start, stop);

		gcoder.extruders[extruderId].g1(ss, start.x, start.y, z, gcoder.extruders[extruderId].fastFeedRate, NULL);
		//ss << "(START!)" << endl;
		gcoder.extruders[extruderId].squirt(ss, polygon[0], extrusionSpeed);
		//ss << "(!START)" << endl;
		for(int i=1; i < polygon.size(); i++)
		{
			const Vector2 &p = polygon[i];
			gcoder.extruders[extruderId].g1(ss, p.x, p.y, z, pathFeedrate, NULL);
		}
		//ss << "(STOP!)" << endl;
		gcoder.extruders[extruderId].snort(ss, stop);
		//ss << "(!STOP)" << endl;
		ss << endl;
	}
}




void ToolHead::g1(std::ostream &ss, double x, double y, double z, double feed, const char *comment = NULL)
{
	bool doX=false;
	bool doY=false;
	bool doZ=false;
	bool doFeed=false;


	if(this->x >= MUCH_LARGER_THAN_THE_BUILD_PLATFORM)
	{
		doX = true;
		doY = true;
		doZ = true;
		doFeed = true;
	}

	if(!mgl::sameSame(this->x, x))
	{
		doX = true;
	}
	if(!mgl::sameSame(this->y, y))
	{
		doY=true;
	}
	if(!mgl::sameSame(this->z, z))
	{
		doZ=true;
	}

	if(!mgl::sameSame(this->feed, feed))
	{
		doFeed=true;
	}

	g1Motion(ss, x,y,z,feed,comment,doX,doY,doZ,doFeed);
}

void ToolHead::squirt(std::ostream &ss, const Vector2 &lineStart, double extrusionSpeed)
{
	//ss << "M108 R" << this->fastExtrusionSpeed << endl;
	//ss << "M101 (squirt)" << endl;

	// int extruderSquirtDelay = 200;
	ss << "M108 R" << this->reversalExtrusionSpeed << " (squirt)" << endl;
	ss << "M101" << endl;
	g1(ss, lineStart.x, lineStart.y, z, fastFeedRate, NULL);
	ss << "M108 R" << extrusionSpeed << " (good to go)" << endl;
}

void ToolHead::snort(std::ostream &ss, const Vector2 &lineEnd)
{
	double reversalFeedRate = this->fastFeedRate;
	ss << "M108 R" << this->reversalExtrusionSpeed << "  (snort)" << endl;
	ss << "M102" << endl;
	g1(ss, lineEnd.x, lineEnd.y, z, reversalFeedRate, NULL);
	ss << "M103" << endl;
}



//// writes an extruder reversal gcode snippet
//void reverseExtrude(std::ostream &ss, double feedrate)
//{
//	ss << "M108 R" << feedrate << endl;
//	ss << "M102 (reverse)" << endl;
//}
void ToolHead::g1Motion(std::ostream &ss, double x, double y, double z, double feed, const char *comment, bool doX, bool doY, bool doZ, bool doFeed)
{

	// not do something is not an option
	#ifdef STRONG_CHECKING
		assert(doX || doY || doZ || doFeed);
	#endif

	assert(fabs(x) < 10000000);
	assert(fabs(y) < 10000000);
	assert(fabs(z) < 10000000);



	ss << "G1";
	if(doX) ss << " X" << x;
	if(doY) ss << " Y" << y;
	if(doZ) ss << " Z" << z;
	if(doFeed) ss << " F" << feed;
	if(comment) ss << " (" << comment << ")";

	ss << endl;

	// update state machine
	this->x = x;
	this->y = y;
	this->z = z;
	this->feed = feed;

	if(comment == NULL)
		this->comment = "";
	else
		this->comment = comment;

}


void GCoder::writeGcodeConfig(std::ostream &ss, const std::string indent) const
{
	MyComputer hal9000;

	ss << "(" << indent << "Generated by "<<  programName << " " << versionStr << ")"<< endl;
	ss << "(" << indent << hal9000.clock.now() <<  ")" << endl;

	ss << "(" << indent << "machine name: " << machineName << ")"<< endl;
	ss << "(" << indent << "firmware revision:" << firmware << ")" << endl;
	std::string plurial = extruders.size()? "":"s";
	ss << "(" << indent << extruders.size() << " extruder" << plurial << ")" << endl;
 	if (outline.enabled)
 	{
 		ss << "(" << indent << outline.distance << "mm outline" << ")" << endl;
 	}
 	else
 	{
 		ss << "(" << indent << "no outline" <<  ")"<< endl;
 	}
 	ss << endl;
}




