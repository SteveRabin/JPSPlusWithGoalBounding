/*
 * DijkstraFloodfill.cpp
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
#include "DijkstraFloodfill.h"

#define ONLY_VISIT_REASONABLE_NEIGHBORS	// 3% to 17% speed-up

#define FIXED_POINT_MULTIPLIER 100000
#define FIXED_POINT_SHIFT(x) ((x) * FIXED_POINT_MULTIPLIER)
#define FIXED_POINT_ONE FIXED_POINT_MULTIPLIER
#define FIXED_POINT_SQRT_2 141421

typedef const void (DijkstraFloodfill::*DijkstraFloodFunctionPointer)(DijkstraPathfindingNode * currentNode);

DijkstraFloodfill::DijkstraFloodfill(int width, int height, std::vector<bool> map, DistantJumpPoints** distantJumpPointMap)
: m_width(width), m_height(height), m_map(map)
{
	m_currentIteration = 1;

#ifdef USE_FAST_OPEN_LIST
	// Number of buckets
	const int numberOfBuckets = 300000;	// Must be no smaller than longest path divided by "division" 
	const int nodesInEachBucket = 1000;
	const int division = 10000;
	m_fastOpenList = new BucketPriorityQueue(numberOfBuckets, nodesInEachBucket, division);
#endif

	// Initialize nodes
	InitArray(m_mapNodes, m_width, m_height);
	for (int r = 0; r<m_height; r++)
	{
		for (int c = 0; c<m_width; c++)
		{
			DijkstraPathfindingNode& node = m_mapNodes[r][c];
			node.m_row = r;
			node.m_col = c;
			node.m_iteration = 0;
			node.m_listStatus = PathfindingNode::OnNone;
			node.m_blockedDirectionBitfield = 0;

			for (int i = 0; i < 8; i++)
			{
				// Detect invalid movement from jump distances
				// (jump distance of zero is invalid movement)
				if (distantJumpPointMap[r][c].jumpDistance[i] == 0)
				{
					node.m_blockedDirectionBitfield |= (1 << i);
				}
			}
		}
	}
}

DijkstraFloodfill::~DijkstraFloodfill()
{
	DestroyArray(m_mapNodes);

#ifdef USE_FAST_OPEN_LIST
	delete m_fastOpenList;
#endif
}

void DijkstraFloodfill::Flood(int r, int c)
{
	// Create 2048 entry function pointer lookup table
	// This greatly speeds up processing by up to 40% by eliminating calculations and conditionals
	#define CASE(x) &DijkstraFloodfill::Explore_ ## x ## ,
	static const DijkstraFloodFunctionPointer exploreDirectionsDijkstraFlood[2048] = 
	{ 
		#include "cases.h"
	};
	#undef CASE(x)

	m_currentIteration++;

#ifdef USE_FAST_OPEN_LIST
	m_fastOpenList->Reset();
#else
	m_openList.reset();
	m_openList.reserve(3000);
#endif

	if (IsEmpty(r, c))
	{	
		// Begin with starting node
		DijkstraPathfindingNode* node = &m_mapNodes[r][c];
		node->m_parent = NULL;
		node->m_directionFromStart = 255;
		node->m_directionFromParent = 255;
		node->m_givenCost = 0;
		node->m_iteration = m_currentIteration;
		node->m_listStatus = PathfindingNode::OnOpen;

		// Explore outward in all directions on the starting node
		Explore_AllDirectionsWithChecks(node);

		node->m_listStatus = PathfindingNode::OnClosed;
	}

#ifdef USE_FAST_OPEN_LIST
	while (!m_fastOpenList->Empty())
#else
	while (!m_openList.empty())
#endif
	{

#ifdef USE_FAST_OPEN_LIST
		DijkstraPathfindingNode* currentNode = m_fastOpenList->Pop();
#else
		DijkstraPathfindingNode* currentNode = m_openList.remove();
#endif

		// Explore nodes based on the parent and surrounding walls.
		// This must be in the search style of JPS+ in order to produce
		// the correct data for JPS+. If goal bounding is used for regular
		// A*, then this search would need to mimic regular A*.
		(this->*exploreDirectionsDijkstraFlood[(currentNode->m_blockedDirectionBitfield * 8) + 
			currentNode->m_directionFromParent])(currentNode);

		currentNode->m_listStatus = PathfindingNode::OnClosed;
	}
}

const void DijkstraFloodfill::Explore_AllDirectionsWithChecks(DijkstraPathfindingNode * currentNode)
{
	//DOWN, DOWNRIGHT, RIGHT, UPRIGHT, UP, UPLEFT, LEFT, DOWNLEFT
	static const int offsetRow[] = { 1, 1, 0, -1, -1, -1,  0,  1 };
	static const int offsetCol[] = { 0, 1, 1,  1,  0, -1, -1, -1 };

	for (int i = 0; i < 8; ++i)
	{
		unsigned int neighborRow = currentNode->m_row + offsetRow[i];

		// Out of grid bounds?
		if (neighborRow >= m_height)
			continue;

		unsigned int neighborCol = currentNode->m_col + offsetCol[i];

		// Out of grid bounds?
		if (neighborCol >= m_width)
			continue;

		// Valid tile - get the node
		DijkstraPathfindingNode* newSuccessor = &m_mapNodes[neighborRow][neighborCol];

		// Blocked?
		if (!m_map[neighborCol + (neighborRow * m_width)])
			continue;

		// Diagonal blocked?
		bool isDiagonal = (i & 0x1) == 1;
		if (isDiagonal && (!m_map[currentNode->m_col + ((currentNode->m_row + offsetRow[i]) * m_width)] ||
			!m_map[currentNode->m_col + offsetCol[i] + (currentNode->m_row * m_width)]))
		{
			continue;
		}

		unsigned int costToNextNode = isDiagonal ? FIXED_POINT_SQRT_2 : FIXED_POINT_ONE;
		ArrayDirections dir = (ArrayDirections)i;

		PushNewNode(newSuccessor, currentNode, dir, dir, costToNextNode);
	}
}

inline const void DijkstraFloodfill::Explore_Null(DijkstraPathfindingNode * currentNode)
{
	// Purposely does nothing
}

inline const void DijkstraFloodfill::Explore_D(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
}

inline const void DijkstraFloodfill::Explore_DR(DijkstraPathfindingNode * currentNode)
{
	SearchDownRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_R(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_UR(DijkstraPathfindingNode * currentNode)
{
	SearchUpRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_U(DijkstraPathfindingNode * currentNode)
{
	SearchUp(currentNode);
}

inline const void DijkstraFloodfill::Explore_UL(DijkstraPathfindingNode * currentNode)
{
	SearchUpLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_L(DijkstraPathfindingNode * currentNode)
{
	SearchLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_DL(DijkstraPathfindingNode * currentNode)
{
	SearchDownLeft(currentNode);
}

// Adjacent Doubles

inline const void DijkstraFloodfill::Explore_D_DR(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchDownRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_DR_R(DijkstraPathfindingNode * currentNode)
{
	SearchDownRight(currentNode);
	SearchRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_R_UR(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
	SearchUpRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_UR_U(DijkstraPathfindingNode * currentNode)
{
	SearchUpRight(currentNode);
	SearchUp(currentNode);
}

inline const void DijkstraFloodfill::Explore_U_UL(DijkstraPathfindingNode * currentNode)
{
	SearchUp(currentNode);
	SearchUpLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_UL_L(DijkstraPathfindingNode * currentNode)
{
	SearchUpLeft(currentNode);
	SearchLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_L_DL(DijkstraPathfindingNode * currentNode)
{
	SearchLeft(currentNode);
	SearchDownLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_DL_D(DijkstraPathfindingNode * currentNode)
{
	SearchDownLeft(currentNode);
	SearchDown(currentNode);
}

// Non-Adjacent Cardinal Doubles

inline const void DijkstraFloodfill::Explore_D_R(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_R_U(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
	SearchUp(currentNode);
}

inline const void DijkstraFloodfill::Explore_U_L(DijkstraPathfindingNode * currentNode)
{
	SearchUp(currentNode);
	SearchLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_L_D(DijkstraPathfindingNode * currentNode)
{
	SearchLeft(currentNode);
	SearchDown(currentNode);
}

inline const void DijkstraFloodfill::Explore_D_U(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchUp(currentNode);
}

inline const void DijkstraFloodfill::Explore_R_L(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
	SearchLeft(currentNode);
}

// Adjacent Triples

inline const void DijkstraFloodfill::Explore_D_DR_R(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchDownRight(currentNode);
	SearchRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_DR_R_UR(DijkstraPathfindingNode * currentNode)
{
	SearchDownRight(currentNode);
	SearchRight(currentNode);
	SearchUpRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_R_UR_U(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
	SearchUpRight(currentNode);
	SearchUp(currentNode);
}

inline const void DijkstraFloodfill::Explore_UR_U_UL(DijkstraPathfindingNode * currentNode)
{
	SearchUpRight(currentNode);
	SearchUp(currentNode);
	SearchUpLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_U_UL_L(DijkstraPathfindingNode * currentNode)
{
	SearchUp(currentNode);
	SearchUpLeft(currentNode);
	SearchLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_UL_L_DL(DijkstraPathfindingNode * currentNode)
{
	SearchUpLeft(currentNode);
	SearchLeft(currentNode);
	SearchDownLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_L_DL_D(DijkstraPathfindingNode * currentNode)
{
	SearchLeft(currentNode);
	SearchDownLeft(currentNode);
	SearchDown(currentNode);
}

inline const void DijkstraFloodfill::Explore_DL_D_DR(DijkstraPathfindingNode * currentNode)
{
	SearchDownLeft(currentNode);
	SearchDown(currentNode);
	SearchDownRight(currentNode);
}

// Non-Adjacent Cardinal Triples

inline const void DijkstraFloodfill::Explore_D_R_U(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchRight(currentNode);
	SearchUp(currentNode);
}

inline const void DijkstraFloodfill::Explore_R_U_L(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
	SearchUp(currentNode);
	SearchLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_U_L_D(DijkstraPathfindingNode * currentNode)
{
	SearchUp(currentNode);
	SearchLeft(currentNode);
	SearchDown(currentNode);
}

inline const void DijkstraFloodfill::Explore_L_D_R(DijkstraPathfindingNode * currentNode)
{
	SearchLeft(currentNode);
	SearchDown(currentNode);
	SearchRight(currentNode);
}

// Quads

inline const void DijkstraFloodfill::Explore_R_DR_D_L(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
	SearchDownRight(currentNode);
	SearchDown(currentNode);
	SearchLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_R_D_DL_L(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
	SearchDown(currentNode);
	SearchDownLeft(currentNode);
	SearchLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_U_UR_R_D(DijkstraPathfindingNode * currentNode)
{
	SearchUp(currentNode);
	SearchUpRight(currentNode);
	SearchRight(currentNode);
	SearchDown(currentNode);
}

inline const void DijkstraFloodfill::Explore_U_R_DR_D(DijkstraPathfindingNode * currentNode)
{
	SearchUp(currentNode);
	SearchRight(currentNode);
	SearchDownRight(currentNode);
	SearchDown(currentNode);
}

inline const void DijkstraFloodfill::Explore_L_UL_U_R(DijkstraPathfindingNode * currentNode)
{
	SearchLeft(currentNode);
	SearchUpLeft(currentNode);
	SearchUp(currentNode);
	SearchRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_L_U_UR_R(DijkstraPathfindingNode * currentNode)
{
	SearchLeft(currentNode);
	SearchUp(currentNode);
	SearchUpRight(currentNode);
	SearchRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_D_DL_L_U(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchDownLeft(currentNode);
	SearchLeft(currentNode);
	SearchUp(currentNode);
}

inline const void DijkstraFloodfill::Explore_D_L_UL_U(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchLeft(currentNode);
	SearchUpLeft(currentNode);
	SearchUp(currentNode);
}

// Quints

inline const void DijkstraFloodfill::Explore_R_DR_D_DL_L(DijkstraPathfindingNode * currentNode)
{
	SearchRight(currentNode);
	SearchDownRight(currentNode);
	SearchDown(currentNode);
	SearchDownLeft(currentNode);
	SearchLeft(currentNode);
}

inline const void DijkstraFloodfill::Explore_U_UR_R_DR_D(DijkstraPathfindingNode * currentNode)
{
	SearchUp(currentNode);
	SearchUpRight(currentNode);
	SearchRight(currentNode);
	SearchDownRight(currentNode);
	SearchDown(currentNode);
}

inline const void DijkstraFloodfill::Explore_L_UL_U_UR_R(DijkstraPathfindingNode * currentNode)
{
	SearchLeft(currentNode);
	SearchUpLeft(currentNode);
	SearchUp(currentNode);
	SearchUpRight(currentNode);
	SearchRight(currentNode);
}

inline const void DijkstraFloodfill::Explore_D_DL_L_UL_U(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchDownLeft(currentNode);
	SearchLeft(currentNode);
	SearchUpLeft(currentNode);
	SearchUp(currentNode);
}

inline const void DijkstraFloodfill::Explore_AllDirections(DijkstraPathfindingNode * currentNode)
{
	SearchDown(currentNode);
	SearchDownLeft(currentNode);
	SearchLeft(currentNode);
	SearchUpLeft(currentNode);
	SearchUp(currentNode);
	SearchUpRight(currentNode);
	SearchRight(currentNode);
	SearchDownRight(currentNode);
}

void DijkstraFloodfill::SearchDown(DijkstraPathfindingNode * currentNode)
{
	int newRow = currentNode->m_row + 1;
	int newCol = currentNode->m_col;
	unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_ONE;
	DijkstraPathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
	PushNewNode(newSuccessor, currentNode, (ArrayDirections)currentNode->m_directionFromStart, Down, givenCost);
}

void DijkstraFloodfill::SearchDownRight(DijkstraPathfindingNode * currentNode)
{
	int newRow = currentNode->m_row + 1;
	int newCol = currentNode->m_col + 1;
	unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SQRT_2;
	DijkstraPathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
	PushNewNode(newSuccessor, currentNode, (ArrayDirections)currentNode->m_directionFromStart, DownRight, givenCost);
}

void DijkstraFloodfill::SearchRight(DijkstraPathfindingNode * currentNode)
{
	int newRow = currentNode->m_row;
	int newCol = currentNode->m_col + 1;
	unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_ONE;
	DijkstraPathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
	PushNewNode(newSuccessor, currentNode, (ArrayDirections)currentNode->m_directionFromStart, Right, givenCost);
}

void DijkstraFloodfill::SearchUpRight(DijkstraPathfindingNode * currentNode)
{
	int newRow = currentNode->m_row - 1;
	int newCol = currentNode->m_col + 1;
	unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SQRT_2;
	DijkstraPathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
	PushNewNode(newSuccessor, currentNode, (ArrayDirections)currentNode->m_directionFromStart, UpRight, givenCost);
}

void DijkstraFloodfill::SearchUp(DijkstraPathfindingNode * currentNode)
{
	int newRow = currentNode->m_row - 1;
	int newCol = currentNode->m_col;
	unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_ONE;
	DijkstraPathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
	PushNewNode(newSuccessor, currentNode, (ArrayDirections)currentNode->m_directionFromStart, Up, givenCost);
}

void DijkstraFloodfill::SearchUpLeft(DijkstraPathfindingNode * currentNode)
{
	int newRow = currentNode->m_row - 1;
	int newCol = currentNode->m_col - 1;
	unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SQRT_2;
	DijkstraPathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
	PushNewNode(newSuccessor, currentNode, (ArrayDirections)currentNode->m_directionFromStart, UpLeft, givenCost);
}

void DijkstraFloodfill::SearchLeft(DijkstraPathfindingNode * currentNode)
{
	int newRow = currentNode->m_row;
	int newCol = currentNode->m_col - 1;
	unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_ONE;
	DijkstraPathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
	PushNewNode(newSuccessor, currentNode, (ArrayDirections)currentNode->m_directionFromStart, Left, givenCost);
}

void DijkstraFloodfill::SearchDownLeft(DijkstraPathfindingNode * currentNode)
{
	int newRow = currentNode->m_row + 1;
	int newCol = currentNode->m_col - 1;
	unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SQRT_2;
	DijkstraPathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
	PushNewNode(newSuccessor, currentNode, (ArrayDirections)currentNode->m_directionFromStart, DownLeft, givenCost);
}

void DijkstraFloodfill::PushNewNode(
	DijkstraPathfindingNode * newSuccessor, 
	DijkstraPathfindingNode * currentNode, 
	ArrayDirections startDirection, 
	ArrayDirections parentDirection, 
	unsigned int givenCost)
{
	if (newSuccessor->m_iteration != m_currentIteration)
	{
		// Place node on the Open list (we've never seen it before)
		newSuccessor->m_parent = currentNode;
		newSuccessor->m_directionFromStart = startDirection;
		newSuccessor->m_directionFromParent = parentDirection;
		newSuccessor->m_givenCost = givenCost;
		newSuccessor->m_listStatus = PathfindingNode::OnOpen;
		newSuccessor->m_iteration = m_currentIteration;

#ifdef USE_FAST_OPEN_LIST
		m_fastOpenList->Push(newSuccessor);
#else
		m_openList.add(newSuccessor);
#endif
	}
	else if (givenCost < newSuccessor->m_givenCost &&
		newSuccessor->m_listStatus == PathfindingNode::OnOpen)
	{
		// We found a cheaper way to this node - update it
		unsigned int lastCost = newSuccessor->m_givenCost;
		newSuccessor->m_parent = currentNode;
		newSuccessor->m_directionFromStart = startDirection;
		newSuccessor->m_directionFromParent = parentDirection;
		newSuccessor->m_givenCost = givenCost;

#ifdef USE_FAST_OPEN_LIST
		m_fastOpenList->DecreaseKey(newSuccessor, lastCost);
#else
		m_openList.decreaseKey(newSuccessor);
#endif
	}
}

inline bool DijkstraFloodfill::IsEmpty(int r, int c)
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

template <typename T>
void DijkstraFloodfill::InitArray(T**& t, int width, int height)
{
	t = new T*[height];
	for (int i = 0; i < height; ++i)
	{
		t[i] = new T[width];
		memset(t[i], 0, sizeof(T)*width);
	}
}

template <typename T>
void DijkstraFloodfill::DestroyArray(T**& t)
{
	for (int i = 0; i < m_height; ++i)
		delete[] t[i];
	delete[] t;

	t = 0;
}

