/*
 * PrecomputeMap.h
 *
 * Copyright (c) 2014-2015, Steve Rabin
 * All rights reserved.
 *
 * An explanation of the JPS+ algorithm is contained in Chapter 14
 * of the book Game AI Pro 2, edited by Steve Rabin, CRC Press, 2015.
 * A presentation on Goal Bounding titled "JPS+: Over 100x Faster than A*"
 * can be found at www.gdcvault.com from the 2015 GDC AI Summit.
 * A copy of this code is on the website http://www.gameaipro.com.
 *
 * If you develop a way to improve this code or make it faster, please
 * contact steve.rabin@gmail.com and share your insights. I would
 * be equally eager to hear from anyone integrating this code or using
 * the Goal Bounding concept in a commercial application or game.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STEVE RABIN ``AS IS'' AND ANY
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

#pragma once
#include <vector>

enum ArrayDirections
{
	Down		= 0,
	DownRight	= 1,
	Right		= 2,
	UpRight		= 3,
	Up			= 4,
	UpLeft		= 5,
	Left		= 6,
	DownLeft	= 7,
	All			= 8
};

enum GoalBoundsEnum
{
	MinRow		= 0,
	MaxRow		= 1,
	MinCol		= 2,
	MaxCol		= 3
};

struct DistantJumpPoints
{
	short jumpDistance[8];
};

struct GoalBounds
{
	short bounds[8][4];
};

struct JumpDistancesAndGoalBounds
{
	unsigned char blockedDirectionBitfield;	// highest bit [DownLeft, Left, UpLeft, Up, UpRight, Right, DownRight, Down] lowest bit
	short jumpDistance[8];
	short bounds[8][4];
};

class PrecomputeMap
{
public:
	PrecomputeMap(int width, int height, std::vector<bool> map);
	~PrecomputeMap();

	DistantJumpPoints** CalculateMap();
	void SaveMap(const char *filename);
	void LoadMap(const char *filename);
	JumpDistancesAndGoalBounds** GetPreprocessedMap() { return m_jumpDistancesAndGoalBoundsMap; }
	void ReleaseMap() { if (m_mapCreated) DestroyArray(m_distantJumpPointMap); }

protected:
	bool m_mapCreated;
	int m_width;
	int m_height;
	std::vector<bool> m_map;
	unsigned char** m_jumpPointMap;
	DistantJumpPoints** m_distantJumpPointMap;
	GoalBounds** m_goalBoundsMap;
	JumpDistancesAndGoalBounds** m_jumpDistancesAndGoalBoundsMap;

	template <typename T> void InitArray(T**& t, int width, int height);
	template <typename T> void DestroyArray(T**& t);

	void CalculateJumpPointMap();
	void CalculateDistantJumpPointMap();
	void CalculateGoalBounding();
	bool IsJumpPoint(int r, int c, int rowDir, int colDir);
	bool IsEmpty(int r, int c);
	bool IsWall(int r, int c);

	enum BitfieldDirections
	{
		MovingDown = 1 << 0,
		MovingRight = 1 << 1,
		MovingUp = 1 << 2,
		MovingLeft = 1 << 3,
	};
};

