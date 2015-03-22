/*
 * PrecomputeMap.cpp
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

#include "stdafx.h"
#include "PrecomputeMap.h"
#include "DijkstraFloodfill.h"
#include "JPSPlus.h"
#include <fstream>

using std::ifstream;
using std::ofstream;

//#define FILE_FORMAT_ASCII
#define INVALID_GOAL_BOUNDS -1

PrecomputeMap::PrecomputeMap(int width, int height, std::vector<bool> map)
: m_mapCreated(false), m_width(width), m_height(height), m_map(map)
{
}

PrecomputeMap::~PrecomputeMap()
{
}

DistantJumpPoints** PrecomputeMap::CalculateMap()
{
	m_mapCreated = true;

	InitArray(m_jumpPointMap, m_width, m_height);
	CalculateJumpPointMap();

	InitArray(m_distantJumpPointMap, m_width, m_height);
	CalculateDistantJumpPointMap();

	// Destroy the m_jumpPointMap since it isn't needed for the search
	DestroyArray(m_jumpPointMap);

	// Calculate Goal Bounds
	//CalculateGoalBoundingDEPRECATED();
	CalculateGoalBounding();

	return m_distantJumpPointMap;
}

void PrecomputeMap::SaveMap(const char *filename)
{
#ifdef FILE_FORMAT_ASCII
	ofstream file(filename);

	for (int r = 0; r < m_height; r++)
	{
		for (int c = 0; c < m_width; c++)
		{
			DistantJumpPoints* jumpPoints = &m_distantJumpPointMap[r][c];

			for (int i = 0; i < 8; i++)
			{
				file << (int)jumpPoints->jumpDistance[i] << "\t";
			}
			for (int dir = 0; dir < 8; dir++)
			{
				for (int minMaxIndex = 0; minMaxIndex < 4; minMaxIndex++)
				{
					file << (int)m_goalBoundsMap[r][c].bounds[dir][minMaxIndex] << "\t";
				}
			}
			file << std::endl;
		}
		file << std::endl;
	}
#else
	ofstream file(filename, std::ios::out | std::ios::binary);

	for (int r = 0; r < m_height; r++)
	{
		for (int c = 0; c < m_width; c++)
		{
			if (IsWall(r, c))
			{
				// Don't save data if a wall
				continue;
			}

			// Save Jump Distances
			DistantJumpPoints* jumpPoints = &m_distantJumpPointMap[r][c];

			for (int i = 0; i < 8; i++)
			{
				file.write((char*)&jumpPoints->jumpDistance[i], 2);
			}

			// Save Goal Bounds
			for (int dir = 0; dir < 8; dir++)
			{
				if ((m_goalBoundsMap[r][c].bounds[dir][MinRow] > 
					 m_goalBoundsMap[r][c].bounds[dir][MaxRow]) &&
					(m_goalBoundsMap[r][c].bounds[dir][MinCol] > 
					 m_goalBoundsMap[r][c].bounds[dir][MaxCol]))
				{
					// Use INVALID_GOAL_BOUNDS to represent all four values for compression
					short value = INVALID_GOAL_BOUNDS;
					file.write((char*)&value, 2);
				}
				else
				{
					for (int minMaxIndex = 0; minMaxIndex < 4; minMaxIndex++)
					{
						short value = m_goalBoundsMap[r][c].bounds[dir][minMaxIndex];
						file.write((char*)&value, 2);
					}
				}
			}
		}
	}
#endif
}

void PrecomputeMap::LoadMap(const char *filename)
{
	m_mapCreated = true;

#ifdef FILE_FORMAT_ASCII
	ifstream file(filename, std::ios::in);

	InitArray(m_jumpDistancesAndGoalBoundsMap, m_width, m_height);

	for (int r = 0; r < m_height; r++)
	{
		for (int c = 0; c < m_width; c++)
		{
			JumpDistancesAndGoalBounds* map = &m_jumpDistancesAndGoalBoundsMap[r][c];
			map->blockedDirectionBitfield = 0;

			for (int i = 0; i < 8; i++)
			{
				file >> map->jumpDistance[i];
			}
			for (int i = 0; i < 8; i++)
			{
				// Detect invalid movement from jump distances
				// (jump distance of zero is invalid movement)
				if(map->jumpDistance[i] == 0)
				{
					map->blockedDirectionBitfield |= (1 << i);
				}
			}
			for (int dir = 0; dir < 8; dir++)
			{
				file >> m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MinRow];
				file >> m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MaxRow];
				file >> m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MinCol];
				file >> m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MaxCol];
			}

		}
	}
#else
	ifstream file(filename, std::ios::in | std::ios::binary);

	InitArray(m_jumpDistancesAndGoalBoundsMap, m_width, m_height);

	for (int r = 0; r < m_height; r++)
	{
		for (int c = 0; c < m_width; c++)
		{
			if (IsWall(r, c))
			{
				// Don't load data if a wall
				continue;
			}

			JumpDistancesAndGoalBounds* map = &m_jumpDistancesAndGoalBoundsMap[r][c];
			map->blockedDirectionBitfield = 0;

			// Load Jump Distances
			for (int i = 0; i < 8; i++)
			{
				file.read((char*)&map->jumpDistance[i], 2);
			}

			// Fabricate wall bitfield for each node
			for (int i = 0; i < 8; i++)
			{
				// Jump distance of zero is invalid movement and means a wall
				if(map->jumpDistance[i] == 0)
				{
					map->blockedDirectionBitfield |= (1 << i);
				}
			}

			// Load Goal Bounds
			for (int dir = 0; dir < 8; dir++)
			{
				short value;
				file.read((char*)&value, 2);

				if(value == INVALID_GOAL_BOUNDS)
				{
					m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MinRow] = m_height;
					m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MaxRow] = 0;
					m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MinCol] = m_width;
					m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MaxCol] = 0;
				}
				else
				{
					m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MinRow] = value;
					file.read((char*)&m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MaxRow], 2);
					file.read((char*)&m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MinCol], 2);
					file.read((char*)&m_jumpDistancesAndGoalBoundsMap[r][c].bounds[dir][MaxCol], 2);
				}
			}

		}
	}
#endif
}

template <typename T>
void PrecomputeMap::InitArray(T**& t, int width, int height)
{
	t = new T*[height];
	for (int i = 0; i < height; ++i)
	{
		t[i] = new T[width];
		memset(t[i], 0, sizeof(T)*width);
	}
}

template <typename T>
void PrecomputeMap::DestroyArray(T**& t)
{
	for (int i = 0; i < m_height; ++i)
		delete[] t[i];
	delete[] t;

	t = 0;
}

void PrecomputeMap::CalculateJumpPointMap()
{
	for (int r = 0; r < m_height; ++r)
	{
		for (int c = 0; c < m_width; ++c)
		{
			if (m_map[c + (r * m_width)])
			{
				if (IsJumpPoint(r, c, 1, 0))
				{
					m_jumpPointMap[r][c] |= MovingDown;
				}
				if (IsJumpPoint(r, c, -1, 0))
				{
					m_jumpPointMap[r][c] |= MovingUp;
				}
				if (IsJumpPoint(r, c, 0, 1))
				{
					m_jumpPointMap[r][c] |= MovingRight;
				}
				if (IsJumpPoint(r, c, 0, -1))
				{
					m_jumpPointMap[r][c] |= MovingLeft;
				}
			}
		}
	}
}

bool PrecomputeMap::IsJumpPoint(int r, int c, int rowDir, int colDir)
{
	return
		IsEmpty(r - rowDir, c - colDir) &&						// Parent not a wall (not necessary)
		((IsEmpty(r + colDir, c + rowDir) &&					// 1st forced neighbor
		IsWall(r - rowDir + colDir, c - colDir + rowDir)) ||	// 1st forced neighbor (continued)
		((IsEmpty(r - colDir, c - rowDir) &&					// 2nd forced neighbor
		IsWall(r - rowDir - colDir, c - colDir - rowDir))));	// 2nd forced neighbor (continued)
}

inline bool PrecomputeMap::IsEmpty(int r, int c)
{
	unsigned int colBoundsCheck = c;
	unsigned int rowBoundsCheck = r;
	if (colBoundsCheck < (unsigned int)m_width && rowBoundsCheck < (unsigned int)m_height)
	{
		return m_map[c + (r * m_width)];
	}
	else
	{
		return false;
	}
}

inline bool PrecomputeMap::IsWall(int r, int c)
{
	unsigned int colBoundsCheck = c;
	unsigned int rowBoundsCheck = r;
	if (colBoundsCheck < (unsigned int)m_width && rowBoundsCheck < (unsigned int)m_height)
	{
		return !m_map[c + (r * m_width)];
	}
	else
	{
		return true;
	}
}

void PrecomputeMap::CalculateDistantJumpPointMap()
{
	// Calculate distant jump points (Left and Right)
	for (int r = 0; r < m_height; ++r)
	{
		{
			int countMovingLeft = -1;
			bool jumpPointLastSeen = false;
			for (int c = 0; c < m_width; ++c)
			{
				if (IsWall(r, c))
				{
					countMovingLeft = -1;
					jumpPointLastSeen = false;
					m_distantJumpPointMap[r][c].jumpDistance[Left] = 0;
					continue;
				}

				countMovingLeft++;

				if (jumpPointLastSeen)
				{
					m_distantJumpPointMap[r][c].jumpDistance[Left] = countMovingLeft;
				}
				else // Wall last seen
				{
					m_distantJumpPointMap[r][c].jumpDistance[Left] = -countMovingLeft;
				}

				if ((m_jumpPointMap[r][c] & MovingLeft) > 0)
				{
					countMovingLeft = 0;
					jumpPointLastSeen = true;
				}
			}
		}

		{
			int countMovingRight = -1;
			bool jumpPointLastSeen = false;
			for (int c = m_width - 1; c >= 0; --c)
			{
				if (IsWall(r, c))
				{
					countMovingRight = -1;
					jumpPointLastSeen = false;
					m_distantJumpPointMap[r][c].jumpDistance[Right] = 0;
					continue;
				}

				countMovingRight++;

				if (jumpPointLastSeen)
				{
					m_distantJumpPointMap[r][c].jumpDistance[Right] = countMovingRight;
				}
				else // Wall last seen
				{
					m_distantJumpPointMap[r][c].jumpDistance[Right] = -countMovingRight;
				}

				if ((m_jumpPointMap[r][c] & MovingRight) > 0)
				{
					countMovingRight = 0;
					jumpPointLastSeen = true;
				}
			}
		}
	}

	// Calculate distant jump points (Up and Down)
	for (int c = 0; c < m_width; ++c)
	{
		{
			int countMovingUp = -1;
			bool jumpPointLastSeen = false;
			for (int r = 0; r < m_height; ++r)
			{
				if (IsWall(r, c))
				{
					countMovingUp = -1;
					jumpPointLastSeen = false;
					m_distantJumpPointMap[r][c].jumpDistance[Up] = 0;
					continue;
				}

				countMovingUp++;

				if (jumpPointLastSeen)
				{
					m_distantJumpPointMap[r][c].jumpDistance[Up] = countMovingUp;
				}
				else // Wall last seen
				{
					m_distantJumpPointMap[r][c].jumpDistance[Up] = -countMovingUp;
				}

				if ((m_jumpPointMap[r][c] & MovingUp) > 0)
				{
					countMovingUp = 0;
					jumpPointLastSeen = true;
				}
			}
		}

		{
			int countMovingDown = -1;
			bool jumpPointLastSeen = false;
			for (int r = m_height - 1; r >= 0; --r)
			{
				if (IsWall(r, c))
				{
					countMovingDown = -1;
					jumpPointLastSeen = false;
					m_distantJumpPointMap[r][c].jumpDistance[Down] = 0;
					continue;
				}

				countMovingDown++;

				if (jumpPointLastSeen)
				{
					m_distantJumpPointMap[r][c].jumpDistance[Down] = countMovingDown;
				}
				else // Wall last seen
				{
					m_distantJumpPointMap[r][c].jumpDistance[Down] = -countMovingDown;
				}

				if ((m_jumpPointMap[r][c] & MovingDown) > 0)
				{
					countMovingDown = 0;
					jumpPointLastSeen = true;
				}
			}
		}
	}

	// Calculate distant jump points (Diagonally UpLeft and UpRight)
	for (int r = 0; r < m_height; ++r)
	{
		for (int c = 0; c < m_width; ++c)
		{
			if (IsEmpty(r, c))
			{
				if (r == 0 || c == 0 || (IsWall(r - 1, c) || IsWall(r, c - 1) || IsWall(r - 1, c - 1)))
				{
					// Wall one away
					m_distantJumpPointMap[r][c].jumpDistance[UpLeft] = 0;
				}
				else if (IsEmpty(r - 1, c) && IsEmpty(r, c - 1) && 
					(m_distantJumpPointMap[r - 1][c - 1].jumpDistance[Up] > 0 ||
					 m_distantJumpPointMap[r - 1][c - 1].jumpDistance[Left] > 0))
				{
					// Diagonal one away
					m_distantJumpPointMap[r][c].jumpDistance[UpLeft] = 1;
				}
				else
				{
					// Increment from last
					int jumpDistance = m_distantJumpPointMap[r - 1][c - 1].jumpDistance[UpLeft];

					if (jumpDistance > 0)
					{
						m_distantJumpPointMap[r][c].jumpDistance[UpLeft] = 1 + jumpDistance;
					}
					else //if( jumpDistance <= 0 )
					{
						m_distantJumpPointMap[r][c].jumpDistance[UpLeft] = -1 + jumpDistance;
					}
				}


				if (r == 0 || c == m_width - 1 || (IsWall(r - 1, c) || IsWall(r, c + 1) || IsWall(r - 1, c + 1)))
				{
					// Wall one away
					m_distantJumpPointMap[r][c].jumpDistance[UpRight] = 0;
				}
				else if (IsEmpty(r - 1, c) && IsEmpty(r, c + 1) &&
					(m_distantJumpPointMap[r - 1][c + 1].jumpDistance[Up] > 0 ||
					 m_distantJumpPointMap[r - 1][c + 1].jumpDistance[Right] > 0))
				{
					// Diagonal one away
					m_distantJumpPointMap[r][c].jumpDistance[UpRight] = 1;
				}
				else
				{
					// Increment from last
					int jumpDistance = m_distantJumpPointMap[r - 1][c + 1].jumpDistance[UpRight];

					if (jumpDistance > 0)
					{
						m_distantJumpPointMap[r][c].jumpDistance[UpRight] = 1 + jumpDistance;
					}
					else //if( jumpDistance <= 0 )
					{
						m_distantJumpPointMap[r][c].jumpDistance[UpRight] = -1 + jumpDistance;
					}
				}
			}
		}
	}

	// Calculate distant jump points (Diagonally DownLeft and DownRight)
	for (int r = m_height - 1; r >= 0; --r)
	{
		for (int c = 0; c < m_width; ++c)
		{
			if (IsEmpty(r, c))
			{
				if (r == m_height - 1 || c == 0 ||
					(IsWall(r + 1, c) || IsWall(r, c - 1) || IsWall(r + 1, c - 1)))
				{
					// Wall one away
					m_distantJumpPointMap[r][c].jumpDistance[DownLeft] = 0;
				}
				else if (IsEmpty(r + 1, c) && IsEmpty(r, c - 1) &&
					(m_distantJumpPointMap[r + 1][c - 1].jumpDistance[Down] > 0 ||
					 m_distantJumpPointMap[r + 1][c - 1].jumpDistance[Left] > 0))
				{
					// Diagonal one away
					m_distantJumpPointMap[r][c].jumpDistance[DownLeft] = 1;
				}
				else
				{
					// Increment from last
					int jumpDistance = m_distantJumpPointMap[r + 1][c - 1].jumpDistance[DownLeft];

					if (jumpDistance > 0)
					{
						m_distantJumpPointMap[r][c].jumpDistance[DownLeft] = 1 + jumpDistance;
					}
					else //if( jumpDistance <= 0 )
					{
						m_distantJumpPointMap[r][c].jumpDistance[DownLeft] = -1 + jumpDistance;
					}
				}


				if (r == m_height - 1 || c == m_width - 1 || (IsWall(r + 1, c) || IsWall(r, c + 1) || IsWall(r + 1, c + 1)))
				{
					// Wall one away
					m_distantJumpPointMap[r][c].jumpDistance[DownRight] = 0;
				}
				else if (IsEmpty(r + 1, c) && IsEmpty(r, c + 1) &&
					(m_distantJumpPointMap[r + 1][c + 1].jumpDistance[Down] > 0 ||
					 m_distantJumpPointMap[r + 1][c + 1].jumpDistance[Right] > 0))
				{
					// Diagonal one away
					m_distantJumpPointMap[r][c].jumpDistance[DownRight] = 1;
				}
				else
				{
					// Increment from last
					int jumpDistance = m_distantJumpPointMap[r + 1][c + 1].jumpDistance[DownRight];

					if (jumpDistance > 0)
					{
						m_distantJumpPointMap[r][c].jumpDistance[DownRight] = 1 + jumpDistance;
					}
					else //if( jumpDistance <= 0 )
					{
						m_distantJumpPointMap[r][c].jumpDistance[DownRight] = -1 + jumpDistance;
					}
				}
			}
		}
	}
}

void PrecomputeMap::CalculateGoalBounding()
{
	printf("Goal Bounding Preprocessing\n");

	DijkstraFloodfill* dijkstra = new DijkstraFloodfill(m_width, m_height, m_map, m_distantJumpPointMap);

	InitArray(m_goalBoundsMap, m_width, m_height);
	for (int r = 0; r < m_height; ++r)
	{
		for (int c = 0; c < m_width; ++c)
		{
			for (int dir = 0; dir < 8; ++dir)
			{
				m_goalBoundsMap[r][c].bounds[dir][MinRow] = m_height;
				m_goalBoundsMap[r][c].bounds[dir][MaxRow] = 0;
				m_goalBoundsMap[r][c].bounds[dir][MinCol] = m_width;
				m_goalBoundsMap[r][c].bounds[dir][MaxCol] = 0;
			}
		}
	}

	for (int startRow = 0; startRow < m_height; ++startRow)
	{
		printf("Row: %d\n", startRow);

		for (int startCol = 0; startCol < m_width; ++startCol)
		{
			if (IsWall(startRow, startCol))
			{
				continue;
			}

			dijkstra->Flood(startRow, startCol);
			int currentIteration = dijkstra->GetCurrentInteration();

			for (int r = 0; r < m_height; ++r)
			{
				for (int c = 0; c < m_width; ++c)
				{
					if (IsWall(r, c))
					{
						continue;
					}

					int iteration = dijkstra->m_mapNodes[r][c].m_iteration;
					unsigned char status = dijkstra->m_mapNodes[r][c].m_listStatus;
					int dir = dijkstra->m_mapNodes[r][c].m_directionFromStart;

					if (iteration == currentIteration && 
						status == PathfindingNode::OnClosed &&
						dir >= 0 && dir <= 7)
					{
						int row = dijkstra->m_mapNodes[r][c].m_row;
						int col = dijkstra->m_mapNodes[r][c].m_col;

						if (m_goalBoundsMap[startRow][startCol].bounds[dir][MinRow] > row)
						{ 
							m_goalBoundsMap[startRow][startCol].bounds[dir][MinRow] = row; 
						}
						if (m_goalBoundsMap[startRow][startCol].bounds[dir][MaxRow] < row)
						{ 
							m_goalBoundsMap[startRow][startCol].bounds[dir][MaxRow] = row; 
						}
						if (m_goalBoundsMap[startRow][startCol].bounds[dir][MinCol] > col)
						{ 
							m_goalBoundsMap[startRow][startCol].bounds[dir][MinCol] = col; 
						}
						if (m_goalBoundsMap[startRow][startCol].bounds[dir][MaxCol] < col)
						{ 
							m_goalBoundsMap[startRow][startCol].bounds[dir][MaxCol] = col; 
						}
					}
				}
			}
		}
	}

	delete dijkstra;
}

