/*
 * Map.h
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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include <stdint.h>
#endif

#ifndef UINT32_MAX
#define UINT32_MAX        4294967295U
#endif

#ifndef MAP_H
#define MAP_H

static const double ONE = 1.0f;
static const double TWO = 2.0f;
static const double ROOT_TWO = 1.414213562f;//or 1.5f if desired
static const double ONE_OVER_ROOT_TWO = 1.0/ROOT_TWO;//0.707106781f;

#include <stdint.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <string.h>

//#define uint32_t unsigned int
//#define int32_t int

/**
* Terrain is simple here, either out-of-bounds or passable.
 * In reality a much more complex mode could be used for passibility,
 * but this suffices for now.
 */

enum tTerrain {
    kOutOfBounds,
    kGround
};

/**
*
 * Map
 *
 * This code is simplified from a more complex map structure.
 * This map class 
 *
 */
class Map {
public:
    Map(uint32_t width, uint32_t height);
    Map(const char *filename);
    Map(Map *);
    Map(FILE *);
    ~Map();
    void Load(const char *filename);
    void Load(FILE *f);
    void Scale(uint32_t newWidth, uint32_t newHeight);
    void Save(const char *filename);
    void Save(FILE *f);
    const char *GetMapName();
    
    /** return the width of the map */
    inline uint32_t GetMapWidth() const { return width; }
    /** return the height of the map */
    inline uint32_t GetMapHeight() const { return height; }
    
    tTerrain GetTerrainType(uint32_t x, uint32_t y);
    void SetTerrainType(uint32_t x, uint32_t y, tTerrain);
    void SetTerrainType(int32_t x1, int32_t y1,
                        int32_t x2, int32_t y2, tTerrain);
    void SetTerrainType(int32_t x, int32_t y, int32_t radius, tTerrain);
private:
        /** Given an x/y coordinate, compute the index in the land vector */
        inline int GetIndex(uint32_t x, uint32_t y)
    {
            if (x >= width) return -1;
            if (y >= height) return -1;
            return y*width+x;
    }
    /** Given a location in the land vector, get the x/y coordinate */
    inline void GetCoordinate(int loc, uint32_t &x, uint32_t &y)
    {
        y = loc/width;
        x = loc%width;
    }
    
    void LoadOctile(FILE *f, int height, int width);
    void SaveOctile(FILE *f);
    void DrawLandQuickly();
    uint32_t width, height;
    std::vector<int> land;
    char map_name[128];
    bool updated;
};

#endif
