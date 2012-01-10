/**
   MiracleGrue - Model Generator for toolpathing. <http://www.grue.makerbot.com>
   Copyright (C) 2011 Far McKon <Far@makerbot.com>, Hugo Boyer (hugo@makerbot.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

*/

#define OMPFF // openMP mulitithreading extensions This Fu packs a ompff!


#include <iostream>
#include <string>

#include <stdlib.h>
#include "mgl/abstractable.h"
#include "mgl/meshy.h"
#include "Configuration.h"
#include "GCoderOperation.h"
#include "FileWriterOperation.h"

using namespace std;
using namespace mgl;


double numberFromCharEqualsStr(const std::string& str)
{
	string nb = str.substr(2, str.length()-2);
	double val = atof(nb.c_str());
	return val;
}

void parseArgs(int argc, char *argv[], string &modelFile, string &configFileName,
		double & firstLayerZ, double & layerH, double & layerW, double & tubeSpacing, double &angle)
{
	modelFile = argv[argc-1];
    for(int i = 1;i < argc - 1;i++){
        string str = argv[i];
        cout << i << " " << str << endl;
        if(str.find("f=") != string::npos)
            firstLayerZ = numberFromCharEqualsStr(str);

        if(str.find("h=") != string::npos)
            layerH = numberFromCharEqualsStr(str);

        if(str.find("w=") != string::npos)
            layerW = numberFromCharEqualsStr(str);

        if(str.find("t=") != string::npos)
            tubeSpacing = numberFromCharEqualsStr(str);

        if(str.find("c=") != string::npos)
        	configFileName = str.substr(2, str.length()-2);

        if(str.find("a=") != string::npos)
            angle = numberFromCharEqualsStr(str);
    }
}


int preConditionsOrShowUsage(int argc, char *argv[])
{
	if (argc < 2)
	{
		cout << endl;
		cout << "*-------------------------------------*" << endl;
		cout << argv[0]<< endl;
		cout << "It is pitch black. You are likely to be eaten by a grue." << endl;
		cout << "Find the light: try " << argv[0] << " [FILE]" << endl;
		return (-1);
	}
	return 0;
}

PathData * createPathFromTubes(const std::vector<Segment> &tubes, Scalar z)
{
	// paths R us
	PathData *pathData = new PathData(z);
	pathData->paths.push_back(ExtruderPaths());

	ExtruderPaths& paths = pathData->paths[0];
	size_t tubeCount = tubes.size();
	for (int i=0; i< tubeCount; i++)
	{
		const Segment &segment = tubes[i];

		cout << "SEGMENT " << i << "/" << tubeCount << endl;
		paths.push_back(Polygon());
		Polygon &poly = paths[paths.size()-1];

		Point2D p0 (segment.a.x, segment.a.y);
		Point2D p1 (segment.b.x, segment.b.y);

		poly.push_back(p0);
		poly.push_back(p1);

	}
	return pathData;
}

PathData *createPathsForSlice(const TubesInSlice& tubesInSlice)
{
	PathData *pathData = new PathData(tubesInSlice.z);
	pathData->paths.push_back(ExtruderPaths());
	ExtruderPaths& paths = pathData->paths[0];

	// outline loops
	for(int i=0; i < tubesInSlice.outlines.size(); i++)
	{
		const std::vector<Segment> &loop = tubesInSlice.outlines[i];
		paths.push_back(Polygon());
		Polygon &poly = paths[paths.size()-1];
		poly.reserve(loop.size());
		for(int j=0; j< loop.size(); j++)
		{
			const Segment &line = loop[j];
			Point2D p(line.a.x, line.a.y);
			poly.push_back(p);

			if(j == loop.size()-1)
			{
				Point2D p(line.b.x, line.b.y);
				poly.push_back(p);
			}
		}
	}

	// infills
	const std::vector<Segment> &tubes = tubesInSlice.infill;
	size_t tubeCount = tubes.size();
	for (int i=0; i< tubeCount; i++)
	{
		const Segment &segment = tubes[i];

		cout << "SEGMENT " << i << "/" << tubeCount << endl;
		paths.push_back(Polygon());
		Polygon &poly = paths[paths.size()-1];

		Point2D p0 (segment.a.x, segment.a.y);
		Point2D p1 (segment.b.x, segment.b.y);

		poly.push_back(p0);
		poly.push_back(p1);

	}
	return pathData;
}

int main(int argc, char *argv[], char *envp[])
{
	// design by contract ;-)
	int checks = preConditionsOrShowUsage(argc, argv);
	if(checks != 0)
	{

		return checks;
	}

	string modelFile;
	double firstLayerZ = 0.11;
	double layerH = 0.35;
	double layerW = 0.583333;
	double tubeSpacing = 0.5;
	double angle = M_PI *0.5;
	string configFileName = "miracle.config";

    parseArgs(argc, argv, modelFile,  configFileName, firstLayerZ, layerH, layerW, tubeSpacing, angle);

    Configuration config;
    config.readFromFile(configFileName.c_str());

    cout << config.asJson() << endl;

	MyComputer computer;
	cout << endl;
	cout << endl;
	cout << "behold!" << endl;
	cout << "Materialization of \"" << modelFile << "\" has begun at " << computer.clock.now() << endl;

	// std::string modelFile = models[i];
	cout << "firstLayerZ (f) = " << firstLayerZ << endl;
	cout << "layerH (h) = " << layerH << endl;
	cout << "layerW (w) = " << layerW << endl;
	cout << "tubeSpacing (t) = " << tubeSpacing  << endl;
	cout << endl;
	std::string stlFiles = computer.fileSystem.removeExtension(computer.fileSystem.ExtractFilename(modelFile));
	stlFiles += "_";

	std::string scadFile = "."; // outDir;
	scadFile += computer.fileSystem.getPathSeparatorCharacter();
	scadFile += computer.fileSystem.ChangeExtension(computer.fileSystem.ExtractFilename(modelFile), ".scad" );

	std::string gcodeFile = ".";
	gcodeFile += computer.fileSystem.getPathSeparatorCharacter();
	gcodeFile += computer.fileSystem.ChangeExtension(computer.fileSystem.ExtractFilename(modelFile), ".gcode" );
	config["FileWriterOperation"]["filename"] = gcodeFile;
	config["FileWriterOperation"]["format"]= ".gcode";
	cout << endl << endl;
	cout << modelFile << " to \"" << gcodeFile << "\" and \"" << scadFile << "\"" << endl;

	// sliceAndScad(modelFile.c_str(), firstLayerZ, layerH, layerW, tubeSpacing, stlPrefix.c_str(), scadFile.c_str());
	Meshy mesh(config["slicer"]["firstLayerZ"].asDouble(), config["slicer"]["layerH"].asDouble()); // 0.35
	loadMeshyFromStl(mesh, modelFile.c_str());

	std::vector< TubesInSlice >  allTubes;
	sliceAndPath(mesh,
			config["slicer"]["layerW"].asDouble(),
			config["slicer"]["tubeSpacing"].asDouble(),
			config["slicer"]["angle"].asDouble(),
			scadFile.c_str(),
			allTubes); //paths);


	vector<DataEnvelope*> paths;
	//for(std::vector<DataEnvelope*>::iterator it = envelopes.begin(); it != envelopes.end(); it++)

	for (int i=0; i< allTubes.size(); i++)
	{
		// i is the slice index
		TubesInSlice &tubesInSlice = allTubes[i];
		PathData *data = createPathsForSlice(tubesInSlice);
		paths.push_back(data);
	}

	GCoderOperation &tooler = *new GCoderOperation();
	FileWriterOperation &fileWriter = *new FileWriterOperation();
	vector<Operation*> empty;
	vector<Operation*> toolerOutputs;
	toolerOutputs.push_back(&fileWriter);
	tooler.init(config,  toolerOutputs);
	fileWriter.init(config, empty);
	tooler.start();

	for(int i=0; i< paths.size(); i++)
	{
		DataEnvelope *envelope = paths[i];
		tooler.accept(*envelope);
	}

	/// 8) Send a finish signal to the first operation in the Operation Graph
	/// that call to finish will propagate down the graph automatically
	tooler.finish();

	//9) De-init (for safety)
	tooler.deinit();
	fileWriter.deinit();

	delete &tooler;
	delete &fileWriter;
	cout << "Sliced until " << computer.clock.now() << endl;
	cout << endl;
}