/*
 * Map.cpp
 *
 * Copyright (c) 2007, Nathan Sturtevant
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of Alberta nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NATHAN STURTEVANT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include "stdafx.h"
#include "Map.h"
#include <stack>
#include <stdlib.h>

using namespace std;

static const bool verbose = false; 

/** 
* Map::Map()
* 
* \brief Create a new map of a particular size.
*
* A map is an array of tiles according to the height and width of the map.
*
* \param wide The new map width
* \param high The new map height
* \return none
*/
Map::Map(uint32_t wide, uint32_t high)
:width(wide), height(high)
{
    map_name[0] = 0;
    land.resize(height*width);
    for (uint32_t x = 0; x < land.size(); x++)
        land[x] = 1;
    
}

/** 
* Map::Map()
*
* \brief Create a new map by loading it from a file.
*
* Creates a new map and initializes it with the file passed to it.
*
* \param filename The path of a file containing a map.
* \return none
*/
Map::Map(const char *filename)
{
    Load(filename);
}

/** 
* Map::Map()
*
* \brief Create a new map by loading it from a file pointer.
*
* Creates a new map and initializes it with the file pointer passed to it.
*
* \param f A file pointer ready to read the beginning of a map within a file
*/
Map::Map(FILE *f)
{
    map_name[0] = 0;
    land.resize(0);
    Load(f);
}

Map::~Map()
{
}

/**
* Map::Scale()
 *
 * \brief Scale a map to the new size
 *
 * \param newWidth The new width of the map
 * \param newHeight The new height of the map
 */
void Map::Scale(uint32_t newWidth, uint32_t newHeight)
{
    std::vector<int> newLand;
    newLand.resize(newWidth*newHeight);
    for (uint32_t x = 0; x < newWidth; x++)
    {
        for (uint32_t y = 0; y < newHeight; y++)
        {
            newLand[GetIndex(x, y)] = land[GetIndex((x*width)/newWidth,
                                                    (y*height)/newHeight)];
        }
    }
    land = newLand;
    width = newWidth;
    height = newHeight;
    updated = true;
    map_name[0] = 0;
}

/** 
* Map::Load()
*
* \brief Resets the current map by loading the file passed in.
*
* Resets the current map by loading the file passed in.
*
* \param filename The file to load
* \return none
*/
void Map::Load(const char *filename)
{
    land.resize(0);
    FILE *f = fopen(filename, "r");
    if (f)
    {
        Load(f);
        fclose(f);
        strncpy(map_name, filename, 128);
    }
    else {
        printf("Error! Can't open file %s\n", filename);
        width = height = 64;
        land.resize(height*width);
        
        updated = true;
        map_name[0] = 0;
    }
}

/** 
* Map::Load()
*
* \brief Resets the current map by loading the file from the pointer passed in.
*
* Resets the current map by loading the file from the pointer passed in.
*
* Loads an octile map according to the following format.
* The header includes 4 lines:
* type octile\n
* height <height>\n
* width <width>\n
* map\n
*
* \param f A (open) file pointer read to read from the file.
* \return none
*/
void Map::Load(FILE *f)
{
    land.resize(0);
    
    char format[32];
    // ADD ERROR HANDLING HERE
    int num = fscanf(f, "type %s\nheight %u\nwidth %u\nmap\n", format, &height, &width);
    if (num == 3)
    {
        if (strcmp(format, "octile") == 0)
        {
            printf("Loading octile %ux%u\n", width, height);
            LoadOctile(f, height, width);
        }
    }
}

/**
* Map::LoadOctile()
 *
 * \brief Helper function for loading an octile map
 * 
 * After the header has been ready, 
 * following header lines there is a "@" or an "O" for blocked land
 * and any other character for unblocked land.
 *
 * \param f and open file pointer
 * \param high The height of the map being loaded (number of lines)
 * \param wide The width of the map being loaded (column width)
 * \return none
 */
void Map::LoadOctile(FILE *f, int high, int wide)
{
    height = high;
    width = wide;
    updated = true;
    land.resize(high*wide);
    for (int y = 0; y < high; y++)
    {
        for (int x = 0; x < wide; x++)
        {
            char what;
            fscanf(f, "%c", &what);
            switch (toupper(what))
            {
                case '@':
                case 'O':
                    land[GetIndex(x, y)] = 0;
                    break;
                default:
                    land[GetIndex(x, y)] = 1;
                    break;
            }
            fscanf(f, "\n");
        }
    }
}

/** 
* Map::Save()
*
* \brief Saves the current map out to the designated file.
*
* \param filename The filename to save the map in.
* \return none
*/
void Map::Save(const char *filename)
{
    FILE *f = fopen(filename, "w+");
    if (f)
    {
        Save(f);
        fclose(f);
    }
    else {
        printf("Error! Couldn't open file to save\n");
    }
}

/** 
* Map::Save()
*
* \brief Saves the current map out to the designated file.
*
* Saves the current map out to the designated file.
*
* \param f An open file pointer
* \return none
*/
void Map::Save(FILE *f)
{
    SaveOctile(f);
}

/**
* Map::SaveOctile()
 *
 * \brief Write out using the octile map format
 *
 * \param f An open file pointer
 * \return none
 */
void Map::SaveOctile(FILE *f)
{
    if (f)
    {
        fprintf(f, "type octile\nheight %u\nwidth %u\nmap\n", height, width);
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                switch (land[GetIndex(x, y)])
                {
                    case 0: fprintf(f, "@"); break;
                    default:
                    case 1: fprintf(f, "."); break;
                }
            }
            fprintf(f, "\n");
        }
    }
}

/**
* Map::GetMapName()
 *
 * \brief Returns the file name of the map, if available
 *
 * \return A pointer to a string containing the name of the map.
 */

const char *Map::GetMapName()
{
    if (map_name[0] == 0)
        return "";
    return map_name;
}


/**
* Map::GetTerrainType()
 *
 * \brief Get the terrain at a particular coordinate.
 *
 * \param x The x-coordinate to query
 * \param y The y-coordinate to query
 * \return The terrain at that coordinate
 */
tTerrain Map::GetTerrainType(uint32_t x, uint32_t y)
{
    int index = GetIndex(x, y);
    if (index == -1)
        return kOutOfBounds;
    if (land[index] == 0)
        return kOutOfBounds;
    return kGround;
}

/**
* Map::SetTerrainType()
 *
 * \brief Set the terrain at a particular coordinate.
 *
 * \param x The x-coordinate to set
 * \param y The y-coordinate to set
 * \param terrain The terrain for that coordinate
 * \return none
 */
void Map::SetTerrainType(uint32_t x, uint32_t y, tTerrain terrain)
{
    updated = true;
    int index = GetIndex(x, y);
    if (index == -1)
        return;
    if (terrain == kOutOfBounds)
        land[index] = 0;
    else
        land[index] = 1;
}

/**
* Map::SetTerrainType()
 *
 * \brief Set all the terrain between two points to be the same
 *
 * \param x1 The first x-coordinate to set
 * \param y1 The first y-coordinate to set
 * \param x2 The second x-coordinate to set
 * \param y2 The second y-coordinate to set
 * \param terrain The terrain for the line between the coordinates
 * \return none
 */
void Map::SetTerrainType(int32_t x1, int32_t y1,
                         int32_t x2, int32_t y2, tTerrain t)
{
    updated = true;
    //printf("---> (%d, %d) to (%d, %d)\n", x1, y1, x2, y2);
    double xdiff = (int)x1-(int)x2;
    double ydiff = (int)y1-(int)y2;
    double dist = sqrt(xdiff*xdiff + ydiff*ydiff);
    SetTerrainType(x1, y1, t);
    for (double c = 0; c < dist; c += 0.5)
    {
        double ratio = c/dist;
        double newx = (double)x1-xdiff*ratio;
        double newy = (double)y1-ydiff*ratio;
        SetTerrainType((uint32_t)newx, (uint32_t)newy, t);
    }
    SetTerrainType(x2, y2, t);
}

/**
* Map::SetTerrainType()
 *
 * \brief Set the terrain at a radius from a point to be the same
 *
 * \param x The center x-coordinate
 * \param y The center y-coordinate
 * \param radius The radius of the circle
 * \param terrain The terrain for the circle
 * \return none
 */
void Map::SetTerrainType(int32_t x, int32_t y, int32_t radius, tTerrain t)
{
    updated = true;
    for (int x2 = -radius; x2 <= radius; x2++)
    {
        for (int y2 = -radius; y2 <= radius; y2++)
        {
            if ((x+x2 >= 0) && (y+y2 >= 0))
                if ((x2*x2+y2*y2) < radius*radius)
                    SetTerrainType(x+x2, y+y2, t);
        }
    }
}
