/* 
 * scenarioLoader.cpp
 * hog
 * 
 * Created by Renee Jansen on 5/2/2006
 *
 */

#include "stdafx.h"
#include <fstream>
using std::ifstream;
using std::ofstream;

#include "ScenarioLoader.h"
#include <assert.h>

/** 
 * Loads the experiments from the scenario file. 
 */
ScenarioLoader::ScenarioLoader(const char* fname)
{
	strncpy(scenName, fname, 1024);
  ifstream sfile(fname,std::ios::in);
  
  float ver;
  string first;
  sfile>>first;

  // Check if a version number is given
  if(first != "version"){
    ver = 0.0;
    sfile.seekg(0,std::ios::beg);
  }
  else{
    sfile>>ver;
  }

  int sizeX = 0, sizeY = 0; 
  int bucket;
  string map;  
  int xs, ys, xg, yg;
  double dist;

  // Read in & store experiments
  if (ver==0.0){
    while(sfile>>bucket>>map>>xs>>ys>>xg>>yg>>dist) {
      Experiment exp(xs,ys,xg,yg,bucket,dist,map);
      experiments.push_back(exp);
    }
  }
  else if(ver==1.0){
    while(sfile>>bucket>>map>>sizeX>>sizeY>>xs>>ys>>xg>>yg>>dist){
      Experiment exp(xs,ys,xg,yg,sizeX,sizeY,bucket,dist,map);
      experiments.push_back(exp);
    }
  }
  else{
    printf("Invalid version number.\n");
    //assert(0);
  }
}

void ScenarioLoader::Save(const char *fname)
{
//	strncpy(scenName, fname, 1024);
	ofstream ofile(fname);
	
	float ver = 1.0;
	ofile<<"version "<<ver<<std::endl;
	
	
	for (unsigned int x = 0; x < experiments.size(); x++)
	{
		ofile<<experiments[x].bucket<<"\t"<<experiments[x].map<<"\t"<<experiments[x].scaleX<<"\t";
		ofile<<experiments[x].scaleY<<"\t"<<experiments[x].startx<<"\t"<<experiments[x].starty<<"\t";
		ofile<<experiments[x].goalx<<"\t"<<experiments[x].goaly<<"\t"<<experiments[x].distance<<std::endl;
	}
}

void ScenarioLoader::AddExperiment(Experiment which)
{
	experiments.push_back(which);
}

