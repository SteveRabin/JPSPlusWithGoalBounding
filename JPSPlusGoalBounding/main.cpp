//
//  File.cpp
//  MapAbstraction
//
//  Created by Nathan Sturtevant on 7/11/13.
//  Modified by Steve Rabin 12/15/14
//
//

#include "stdafx.h"
#include <vector>
#include <stdio.h>
#include <stdint.h>
#include <numeric>
#include <algorithm>
#include "ScenarioLoader.h"
#include "Timer.h"
#include "Entry.h"
#include <stdlib.h>

#include <windows.h>
#include <tchar.h> 
#include <strsafe.h>
#include <iostream>
#include <fstream>

void LoadMap(const char *fname, std::vector<bool> &map, int &w, int &h);

struct stats {
	std::vector<double> times;
	std::vector<xyLoc> path;
	std::vector<int> lengths;
	
	double GetTotalTime()
	{
		return std::accumulate(times.begin(), times.end(), 0.0);
	}
	double GetMaxTimestep()
	{
		return *std::max_element(times.begin(), times.end());
	}
	double Get20MoveTime()
	{
		for (unsigned int x = 0; x < lengths.size(); x++)
			if (lengths[x] >= 20)
				return std::accumulate(times.begin(), times.begin()+1+x, 0.0);
		return GetTotalTime();
	}
	double GetPathLength()
	{
		double len = 0;
		for (int x = 0; x < (int)path.size()-1; x++)
		{
			if (path[x].x == path[x+1].x || path[x].y == path[x+1].y)
			{
				len++;
			}
			else {
				len += 1.4142;
			}
		}
		return len;
	}
	bool ValidatePath(int width, int height, const std::vector<bool> &mapData)
	{
		for (int x = 0; x < (int)path.size()-1; x++)
		{
			if (abs(path[x].x - path[x+1].x) > 1)
				return false;
			if (abs(path[x].y - path[x+1].y) > 1)
				return false;
			if (!mapData[path[x].y*width+path[x].x])
				return false;
			if (!mapData[path[x+1].y*width+path[x+1].x])
				return false;
			if (path[x].x != path[x+1].x && path[x].y != path[x+1].y)
			{
				if (!mapData[path[x+1].y*width+path[x].x])
					return false;
				if (!mapData[path[x].y*width+path[x+1].x])
					return false;
			}
		}
		return true;
	}
};

int _tmain(int argc, char* argv[])
{
	double allTestsTotalTime = 0;

	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;

	StringCchCopy(szDir, MAX_PATH, TEXT("Maps\\*"));
	hFind = FindFirstFile(szDir, &ffd);

	do
	{
		char filename[2048];
		std::vector<xyLoc> thePath;
		std::vector<bool> mapData;
		int width, height;
		bool pre = false;
		bool run = true;
		bool silenceIndividualTests = false;
		
		char mapFilename[2048] = "\0";
		char mapScenarioFilename[2048] = "\0";
		char mapPreprocessedFilename[2048] = "\0";
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}
		else
		{			
			int filenameLength = 0;
			while(ffd.cFileName[++filenameLength] != 0) {}
			
			if (ffd.cFileName[filenameLength-3] == 'm' &&
				ffd.cFileName[filenameLength-2] == 'a' &&
				ffd.cFileName[filenameLength-1] == 'p')
			{
				char baseFilename[2048];
				for(int i=0; i<filenameLength; i++)
				{
					baseFilename[i] = ffd.cFileName[i];
				}
				baseFilename[filenameLength] = '\0';

				sprintf(mapFilename, "Maps\\%s", baseFilename);
				sprintf(mapScenarioFilename, "Maps\\%s.scen", baseFilename);
				sprintf(mapPreprocessedFilename, "Maps\\%s.pre", baseFilename);

				std::ifstream ifile(mapPreprocessedFilename);
				pre = !ifile;
			}
			else
			{
				continue;
			}
		}

		LoadMap(mapFilename, mapData, width, height);

		if (pre)
		{
			printf("Begin preprocessing map: %s\n", mapFilename);
			PreprocessMap(mapData, width, height, mapPreprocessedFilename);
			printf("Done preprocessing map: %s\n", mapFilename);
		}
	
		if (!run)
		{
			continue;
		}
	
		void *reference = PrepareForSearch(mapData, width, height, mapPreprocessedFilename);
	
		ScenarioLoader scen(mapScenarioFilename);

		Timer t;
		std::vector<stats> experimentStats;
		for (int x = 0; x < scen.GetNumExperiments(); x++)
		{
			thePath.resize(0);
			experimentStats.resize(x+1);
			bool done;
			do {
				xyLoc s, g;
				s.x = scen.GetNthExperiment(x).GetStartX();
				s.y = scen.GetNthExperiment(x).GetStartY();
				g.x = scen.GetNthExperiment(x).GetGoalX();
				g.y = scen.GetNthExperiment(x).GetGoalY();
			
				if(s.x == g.x && s.y == g.y)
				{
					done = true;
				}
				else
				{
					t.StartTimer();
					done = GetPath(reference, s, g, thePath);
					t.EndTimer();
			
					if(thePath.size() > 0)
					{
						experimentStats[x].times.push_back(t.GetElapsedTime());
						experimentStats[x].lengths.push_back(thePath.size());
						for (unsigned int t = experimentStats[x].path.size(); t < thePath.size(); t++)
							experimentStats[x].path.push_back(thePath[t]);
					}
				}
			} while (done == false);
		
		}
		thePath.clear();
		delete reference;
	
		double totalTime = 0;
		bool invalid = false;
		bool suboptimal = false;
		for (unsigned int x = 0; x < experimentStats.size(); x++)
		{
			if(!silenceIndividualTests)
			{
				printf("GPPC\t%s\ttotal-time\t%f\tmax-time-step\t%f\ttime-20-moves\t%f\ttotal-len\t%f\tsubopt\t%f\t", mapScenarioFilename,
					   experimentStats[x].GetTotalTime(), experimentStats[x].GetMaxTimestep(), experimentStats[x].Get20MoveTime(),
					   experimentStats[x].GetPathLength(), experimentStats[x].GetPathLength()/scen.GetNthExperiment(x).GetDistance());
			}

			if (experimentStats[x].path.size() != 0 &&
				(experimentStats[x].ValidatePath(width, height, mapData) &&
				 scen.GetNthExperiment(x).GetStartX() == experimentStats[x].path[0].x &&
				 scen.GetNthExperiment(x).GetStartY() == experimentStats[x].path[0].y &&
				 scen.GetNthExperiment(x).GetGoalX() == experimentStats[x].path.back().x &&
				 scen.GetNthExperiment(x).GetGoalY() == experimentStats[x].path.back().y))
			{
				if(!silenceIndividualTests)
					printf("valid\n");
			}
			else
			{
				invalid = true;
				if(!silenceIndividualTests)
					printf("invalid\n");
			}
			if (experimentStats[x].GetPathLength() / scen.GetNthExperiment(x).GetDistance() > 1.000005f)
			{
				suboptimal = true;
			}

			totalTime += experimentStats[x].GetTotalTime();
			allTestsTotalTime += experimentStats[x].GetTotalTime();
		}

		printf("Total map time: %f,\t%s", totalTime, mapFilename);
		if (invalid) { printf(",\tINVALID"); }
		if (suboptimal) { printf(",\tSUBOPTIMAL"); }
		printf("\n");

		for (unsigned int x = 0; x < experimentStats.size(); x++)
		{
			experimentStats[x].path.clear();
			experimentStats[x].times.clear();
			experimentStats[x].lengths.clear();
		}
		experimentStats.clear();
		scen.Clear();
		mapData.clear();

	} while (FindNextFile(hFind, &ffd) != 0);

	printf("All tests total time: %f", allTestsTotalTime);

	while(true) {}

	return 0;
}

void LoadMap(const char *fname, std::vector<bool> &map, int &width, int &height)
{
	FILE *f;
	f = fopen(fname, "r");
	if (f)
	{
		fscanf(f, "type octile\nheight %d\nwidth %d\nmap\n", &height, &width);
		map.resize(height*width);
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				char c;
				do {
					fscanf(f, "%c", &c);
				} while (isspace(c));
				map[y*width+x] = (c == '.' || c == 'G' || c == 'S');
			}
		}
		fclose(f);
	}
}
