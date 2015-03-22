/*
 * JPSPlus.cpp
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
#include "JPSPlus.h"

// Ideal choice of fixed-point equivalent to 1.0 that can almost perfectly represent sqrt(2) and (sqrt(2) - 1) in whole numbers
// 1.000000000 = 2378
// 0.414213624 = 985 / 2378
// 1.414213625 = 3363 / 2378
// 1.414213562 = Actual sqrt(2)
// 0.00000006252 = Difference between actual sqrt(2) and fixed-point sqrt(2)
#define FIXED_POINT_MULTIPLIER 2378
#define FIXED_POINT_SHIFT(x) ((x) * FIXED_POINT_MULTIPLIER)
#define SQRT_2 3363
#define SQRT_2_MINUS_ONE 985

typedef const void (JPSPlus::*FunctionPointer)(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);

JPSPlus::JPSPlus(JumpDistancesAndGoalBounds** jumpDistancesAndGoalBoundsMap, std::vector<bool> &rawMap, int w, int h)
{
	// Map properties
	m_width = w;
	m_height = h;

	// Adjust preallocation for worst-case
	m_simpleUnsortedPriorityQueue = new SimpleUnsortedPriorityQueue(10000);
	m_fastStack = new FastStack(1000);

	m_jumpDistancesAndGoalBounds = jumpDistancesAndGoalBoundsMap;
	m_currentIteration = 1;	// This gets incremented on each search

	// Initialize nodes
	InitArray(m_mapNodes, m_width, m_height);
	for (int r = 0; r<m_height; r++)
	{
		for (int c = 0; c<m_width; c++)
		{
			PathfindingNode& node = m_mapNodes[r][c];
			node.m_row = r;
			node.m_col = c;
			node.m_listStatus = PathfindingNode::OnNone;
			node.m_iteration = 0;
		}
	}
}

JPSPlus::~JPSPlus()
{
	delete m_fastStack;
	delete m_simpleUnsortedPriorityQueue;
	DestroyArray(m_jumpDistancesAndGoalBounds);
	DestroyArray(m_mapNodes);
}

template <typename T>
void JPSPlus::InitArray(T**& t, int width, int height)
{
	t = new T*[height];
	for (int i = 0; i < height; ++i)
	{
		t[i] = new T[width];
		memset(t[i], 0, sizeof(T)*width);
	}
}

template <typename T>
void JPSPlus::DestroyArray(T**& t)
{
	for (int i = 0; i < m_height; ++i)
		delete[] t[i];
	delete[] t;

	t = 0;
}

bool JPSPlus::GetPath(xyLocJPS& s, xyLocJPS& g, std::vector<xyLocJPS> &path)
{
	if (path.size() > 0)
	{
		path.push_back(g);
		return true;
	}

	int startRow = s.y;
	int startCol = s.x;
	m_goalRow = g.y;
	m_goalCol = g.x;

	{
		// Initialize map
		path.clear();

		m_goalNode = &m_mapNodes[m_goalRow][m_goalCol];
		m_currentIteration++;

		m_fastStack->Reset();
		m_simpleUnsortedPriorityQueue->Reset();
	}

	// Create starting node
	PathfindingNode* startNode = &m_mapNodes[startRow][startCol];
	startNode->m_parent = NULL;
	startNode->m_givenCost = 0;
	startNode->m_finalCost = 0;
	startNode->m_listStatus = PathfindingNode::OnOpen;
	startNode->m_iteration = m_currentIteration;

	// Actual search
	PathStatus status = SearchLoop(startNode);

	if (status == PathFound)
	{
		FinalizePath(path);
		if (path.size() > 0)
		{
			path.pop_back();
			return false;
		}
		return true;
	}
	else
	{
		// No path
		return true;
	}
}

PathStatus JPSPlus::SearchLoop(PathfindingNode* startNode)
{
	// Create 2048 entry function pointer lookup table
	#define CASE(x) &JPSPlus::Explore_ ## x ## ,
	static const FunctionPointer exploreDirections[2048] = 
	{ 
		#include "cases.h"
	};
	#undef CASE(x)

	{
		// Special case for the starting node

		if (startNode == m_goalNode)
		{
			return PathFound;
		}

		JumpDistancesAndGoalBounds* jumpDistancesAndGoalBounds = &m_jumpDistancesAndGoalBounds[startNode->m_row][startNode->m_col];
		Explore_AllDirections(startNode, jumpDistancesAndGoalBounds);
		startNode->m_listStatus = PathfindingNode::OnClosed;
	}

	while (!m_simpleUnsortedPriorityQueue->Empty() || !m_fastStack->Empty())
	{
		PathfindingNode* currentNode;

		if(!m_fastStack->Empty())
		{
			currentNode = m_fastStack->Pop();
		}
		else
		{
			currentNode = m_simpleUnsortedPriorityQueue->Pop();
		}

		if (currentNode == m_goalNode)
		{
			return PathFound;
		}
		
		// Explore nodes based on parent
		JumpDistancesAndGoalBounds* jumpDistancesAndGoalBounds = 
			&m_jumpDistancesAndGoalBounds[currentNode->m_row][currentNode->m_col];

		(this->*exploreDirections[(jumpDistancesAndGoalBounds->blockedDirectionBitfield * 8) + 
			currentNode->m_directionFromParent])(currentNode, jumpDistancesAndGoalBounds);

		currentNode->m_listStatus = PathfindingNode::OnClosed;
	}
	return NoPathExists;
}

void JPSPlus::FinalizePath(std::vector<xyLocJPS> &finalPath)
{
	PathfindingNode* prevNode = NULL;
	PathfindingNode* curNode = m_goalNode;

	while (curNode != NULL)
	{
		xyLocJPS loc;
		loc.x = curNode->m_col;
		loc.y = curNode->m_row;

		if (prevNode != NULL)
		{
			// Insert extra nodes if needed (may not be neccessary depending on final path use)
			int xDiff = curNode->m_col - prevNode->m_col;
			int yDiff = curNode->m_row - prevNode->m_row;

			int xInc = 0;
			int yInc = 0;

			if (xDiff > 0) { xInc = 1; }
			else if (xDiff < 0) { xInc = -1; xDiff = -xDiff; }

			if (yDiff > 0) { yInc = 1; }
			else if (yDiff < 0) { yInc = -1; yDiff = -yDiff; }

			int x = prevNode->m_col;
			int y = prevNode->m_row;
			int steps = xDiff - 1;
			if (yDiff > xDiff) { steps = yDiff - 1; }

			for (int i = 0; i < steps; i++)
			{
				x += xInc;
				y += yInc;

				xyLocJPS locNew;
				locNew.x = x;
				locNew.y = y;

				finalPath.push_back(locNew);
			}
		}

		finalPath.push_back(loc);
		prevNode = curNode;
		curNode = curNode->m_parent;
	}
	std::reverse(finalPath.begin(), finalPath.end());
}

// Macro definitions for exploring in a particular direction
#define MacroExploreDown \
	if (m_goalRow >= map->bounds[Down][MinRow] && \
		m_goalRow <= map->bounds[Down][MaxRow] && \
		m_goalCol >= map->bounds[Down][MinCol] && \
		m_goalCol <= map->bounds[Down][MaxCol]) SearchDown(currentNode, map->jumpDistance[Down]);

#define MacroExploreDownRight \
	if (m_goalRow >= map->bounds[DownRight][MinRow] && \
		m_goalRow <= map->bounds[DownRight][MaxRow] && \
		m_goalCol >= map->bounds[DownRight][MinCol] && \
		m_goalCol <= map->bounds[DownRight][MaxCol]) SearchDownRight(currentNode, map->jumpDistance[DownRight]);

#define MacroExploreRight \
	if (m_goalRow >= map->bounds[Right][MinRow] && \
		m_goalRow <= map->bounds[Right][MaxRow] && \
		m_goalCol >= map->bounds[Right][MinCol] && \
		m_goalCol <= map->bounds[Right][MaxCol]) SearchRight(currentNode, map->jumpDistance[Right]);

#define MacroExploreUpRight \
	if (m_goalRow >= map->bounds[UpRight][MinRow] && \
		m_goalRow <= map->bounds[UpRight][MaxRow] && \
		m_goalCol >= map->bounds[UpRight][MinCol] && \
		m_goalCol <= map->bounds[UpRight][MaxCol]) SearchUpRight(currentNode, map->jumpDistance[UpRight]);

#define MacroExploreUp \
	if (m_goalRow >= map->bounds[Up][MinRow] && \
		m_goalRow <= map->bounds[Up][MaxRow] && \
		m_goalCol >= map->bounds[Up][MinCol] && \
		m_goalCol <= map->bounds[Up][MaxCol]) SearchUp(currentNode, map->jumpDistance[Up]);

#define MacroExploreUpLeft \
	if (m_goalRow >= map->bounds[UpLeft][MinRow] && \
		m_goalRow <= map->bounds[UpLeft][MaxRow] && \
		m_goalCol >= map->bounds[UpLeft][MinCol] && \
		m_goalCol <= map->bounds[UpLeft][MaxCol]) SearchUpLeft(currentNode, map->jumpDistance[UpLeft]);

#define MacroExploreLeft \
	if (m_goalRow >= map->bounds[Left][MinRow] && \
		m_goalRow <= map->bounds[Left][MaxRow] && \
		m_goalCol >= map->bounds[Left][MinCol] && \
		m_goalCol <= map->bounds[Left][MaxCol]) SearchLeft(currentNode, map->jumpDistance[Left]);

#define MacroExploreDownLeft \
	if (m_goalRow >= map->bounds[DownLeft][MinRow] && \
		m_goalRow <= map->bounds[DownLeft][MaxRow] && \
		m_goalCol >= map->bounds[DownLeft][MinCol] && \
		m_goalCol <= map->bounds[DownLeft][MaxCol]) SearchDownLeft(currentNode, map->jumpDistance[DownLeft]);

inline const void JPSPlus::Explore_Null(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	// Purposely does nothing
}

inline const void JPSPlus::Explore_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
}

inline const void JPSPlus::Explore_DR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDownRight;
}

inline const void JPSPlus::Explore_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
}

inline const void JPSPlus::Explore_UR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUpRight;
}

inline const void JPSPlus::Explore_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUp;
}

inline const void JPSPlus::Explore_UL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUpLeft;
}

inline const void JPSPlus::Explore_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreLeft;
}

inline const void JPSPlus::Explore_DL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDownLeft;
}

// Adjacent Doubles

inline const void JPSPlus::Explore_D_DR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreDownRight;
}

inline const void JPSPlus::Explore_DR_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDownRight;
	MacroExploreRight;
}

inline const void JPSPlus::Explore_R_UR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
	MacroExploreUpRight;
}

inline const void JPSPlus::Explore_UR_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUpRight;
	MacroExploreUp;
}

inline const void JPSPlus::Explore_U_UL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUp;
	MacroExploreUpLeft;
}

inline const void JPSPlus::Explore_UL_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUpLeft;
	MacroExploreLeft;
}

inline const void JPSPlus::Explore_L_DL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreLeft;
	MacroExploreDownLeft;
}

inline const void JPSPlus::Explore_DL_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDownLeft;
	MacroExploreDown;
}

// Non-Adjacent Cardinal Doubles

inline const void JPSPlus::Explore_D_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreRight;
}

inline const void JPSPlus::Explore_R_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
	MacroExploreUp;
}

inline const void JPSPlus::Explore_U_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUp;
	MacroExploreLeft;
}

inline const void JPSPlus::Explore_L_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreLeft;
	MacroExploreDown;
}

inline const void JPSPlus::Explore_D_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreUp;
}

inline const void JPSPlus::Explore_R_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
	MacroExploreLeft;
}

// Adjacent Triples

inline const void JPSPlus::Explore_D_DR_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreDownRight;
	MacroExploreRight;
}

inline const void JPSPlus::Explore_DR_R_UR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDownRight;
	MacroExploreRight;
	MacroExploreUpRight;
}

inline const void JPSPlus::Explore_R_UR_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
	MacroExploreUpRight;
	MacroExploreUp;
}

inline const void JPSPlus::Explore_UR_U_UL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUpRight;
	MacroExploreUp;
	MacroExploreUpLeft;
}

inline const void JPSPlus::Explore_U_UL_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUp;
	MacroExploreUpLeft;
	MacroExploreLeft;
}

inline const void JPSPlus::Explore_UL_L_DL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUpLeft;
	MacroExploreLeft;
	MacroExploreDownLeft;
}

inline const void JPSPlus::Explore_L_DL_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreLeft;
	MacroExploreDownLeft;
	MacroExploreDown;
}

inline const void JPSPlus::Explore_DL_D_DR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDownLeft;
	MacroExploreDown;
	MacroExploreDownRight;
}

// Non-Adjacent Cardinal Triples

inline const void JPSPlus::Explore_D_R_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreRight;
	MacroExploreUp;
}

inline const void JPSPlus::Explore_R_U_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
	MacroExploreUp;
	MacroExploreLeft;
}

inline const void JPSPlus::Explore_U_L_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUp;
	MacroExploreLeft;
	MacroExploreDown;
}

inline const void JPSPlus::Explore_L_D_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreLeft;
	MacroExploreDown;
	MacroExploreRight;
}

// Quads

inline const void JPSPlus::Explore_R_DR_D_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
	MacroExploreDownRight;
	MacroExploreDown;
	MacroExploreLeft;
}

inline const void JPSPlus::Explore_R_D_DL_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
	MacroExploreDown;
	MacroExploreDownLeft;
	MacroExploreLeft;
}

inline const void JPSPlus::Explore_U_UR_R_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUp;
	MacroExploreUpRight;
	MacroExploreRight;
	MacroExploreDown;
}

inline const void JPSPlus::Explore_U_R_DR_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUp;
	MacroExploreRight;
	MacroExploreDownRight;
	MacroExploreDown;
}

inline const void JPSPlus::Explore_L_UL_U_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreLeft;
	MacroExploreUpLeft;
	MacroExploreUp;
	MacroExploreRight;
}

inline const void JPSPlus::Explore_L_U_UR_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreLeft;
	MacroExploreUp;
	MacroExploreUpRight;
	MacroExploreRight;
}

inline const void JPSPlus::Explore_D_DL_L_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreDownLeft;
	MacroExploreLeft;
	MacroExploreUp;
}

inline const void JPSPlus::Explore_D_L_UL_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreLeft;
	MacroExploreUpLeft;
	MacroExploreUp;
}

// Quints

inline const void JPSPlus::Explore_R_DR_D_DL_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreRight;
	MacroExploreDownRight;
	MacroExploreDown;
	MacroExploreDownLeft;
	MacroExploreLeft;
}

inline const void JPSPlus::Explore_U_UR_R_DR_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreUp;
	MacroExploreUpRight;
	MacroExploreRight;
	MacroExploreDownRight;
	MacroExploreDown;
}

inline const void JPSPlus::Explore_L_UL_U_UR_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreLeft;
	MacroExploreUpLeft;
	MacroExploreUp;
	MacroExploreUpRight;
	MacroExploreRight;
}

inline const void JPSPlus::Explore_D_DL_L_UL_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreDownLeft;
	MacroExploreLeft;
	MacroExploreUpLeft;
	MacroExploreUp;
}

inline const void JPSPlus::Explore_AllDirections(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map)
{
	MacroExploreDown;
	MacroExploreDownLeft;
	MacroExploreLeft;
	MacroExploreUpLeft;
	MacroExploreUp;
	MacroExploreUpRight;
	MacroExploreRight;
	MacroExploreDownRight;
}

void JPSPlus::SearchDown(PathfindingNode * currentNode, int jumpDistance)
{
	int row = currentNode->m_row;
	int col = currentNode->m_col;

	// Consider straight line to Goal
	if (col == m_goalCol && row < m_goalRow)
	{
		int absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		if ((row + absJumpDistance) >= m_goalRow)
		{
			unsigned int diff = m_goalRow - row;
			unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(diff);
			PathfindingNode * newSuccessor = m_goalNode;
			PushNewNode(newSuccessor, currentNode, Down, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int newRow = row + jumpDistance;
		unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(jumpDistance);
		PathfindingNode * newSuccessor = &m_mapNodes[newRow][col];
		PushNewNode(newSuccessor, currentNode, Down, givenCost);
	}
}

void JPSPlus::SearchDownRight(PathfindingNode * currentNode, int jumpDistance)
{
	int row = currentNode->m_row;
	int col = currentNode->m_col;

	// Check for goal in general direction (straight line to Goal or Target Jump Point)
	if (row < m_goalRow && col < m_goalCol)
	{
		int absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		int diffRow = m_goalRow - row;
		int diffCol = m_goalCol - col;
		int smallerDiff = diffRow;
		if (diffCol < smallerDiff) { smallerDiff = diffCol; }

		if (smallerDiff <= absJumpDistance)
		{
			int newRow = row + smallerDiff;
			int newCol = col + smallerDiff;
			unsigned int givenCost = currentNode->m_givenCost + (SQRT_2 * smallerDiff);
			PathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
			PushNewNode(newSuccessor, currentNode, DownRight, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int newRow = row + jumpDistance;
		int newCol = col + jumpDistance;
		unsigned int givenCost = currentNode->m_givenCost + (SQRT_2 * jumpDistance);
		PathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
		PushNewNode(newSuccessor, currentNode, DownRight, givenCost);
	}
}

void JPSPlus::SearchRight(PathfindingNode * currentNode, int jumpDistance)
{
	int row = currentNode->m_row;
	int col = currentNode->m_col;

	// Consider straight line to Goal
	if (row == m_goalRow && col < m_goalCol)
	{
		int absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		if ((col + absJumpDistance) >= m_goalCol)
		{
			unsigned int diff = m_goalCol - col;
			unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(diff);
			PathfindingNode * newSuccessor = m_goalNode;
			PushNewNode(newSuccessor, currentNode, Right, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int newCol = col + jumpDistance;
		unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(jumpDistance);
		PathfindingNode * newSuccessor = &m_mapNodes[row][newCol];
		PushNewNode(newSuccessor, currentNode, Right, givenCost);
	}
}

void JPSPlus::SearchUpRight(PathfindingNode * currentNode, int jumpDistance)
{
	int row = currentNode->m_row;
	int col = currentNode->m_col;

	// Check for goal in general direction (straight line to Goal or Target Jump Point)
	if (row > m_goalRow && col < m_goalCol)
	{
		int absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		int diffRow = row - m_goalRow;
		int diffCol = m_goalCol - col;
		int smallerDiff = diffRow;
		if (diffCol < smallerDiff) { smallerDiff = diffCol; }

		if (smallerDiff <= absJumpDistance)
		{
			int newRow = row - smallerDiff;
			int newCol = col + smallerDiff;
			unsigned int givenCost = currentNode->m_givenCost + (SQRT_2 * smallerDiff);
			PathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
			PushNewNode(newSuccessor, currentNode, UpRight, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int newRow = row - jumpDistance;
		int newCol = col + jumpDistance;
		unsigned int givenCost = currentNode->m_givenCost + (SQRT_2 * jumpDistance);
		PathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
		PushNewNode(newSuccessor, currentNode, UpRight, givenCost);
	}
}

void JPSPlus::SearchUp(PathfindingNode * currentNode, int jumpDistance)
{
	int row = currentNode->m_row;
	int col = currentNode->m_col;

	// Consider straight line to Goal
	if (col == m_goalCol && row > m_goalRow)
	{
		int absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		if ((row - absJumpDistance) <= m_goalRow)
		{
			unsigned int diff = row - m_goalRow;
			unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(diff);
			PathfindingNode * newSuccessor = m_goalNode;
			PushNewNode(newSuccessor, currentNode, Up, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int newRow = row - jumpDistance;
		unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(jumpDistance);
		PathfindingNode * newSuccessor = &m_mapNodes[newRow][col];
		PushNewNode(newSuccessor, currentNode, Up, givenCost);
	}
}

void JPSPlus::SearchUpLeft(PathfindingNode * currentNode, int jumpDistance)
{
	int row = currentNode->m_row;
	int col = currentNode->m_col;

	// Check for goal in general direction (straight line to Goal or Target Jump Point)
	if (row > m_goalRow && col > m_goalCol)
	{
		int absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		int diffRow = row - m_goalRow;
		int diffCol = col - m_goalCol;
		int smallerDiff = diffRow;
		if (diffCol < smallerDiff) { smallerDiff = diffCol; }

		if (smallerDiff <= absJumpDistance)
		{
			int newRow = row - smallerDiff;
			int newCol = col - smallerDiff;
			unsigned int givenCost = currentNode->m_givenCost + (SQRT_2 * smallerDiff);
			PathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
			PushNewNode(newSuccessor, currentNode, UpLeft, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int newRow = row - jumpDistance;
		int newCol = col - jumpDistance;
		unsigned int givenCost = currentNode->m_givenCost + (SQRT_2 * jumpDistance);
		PathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
		PushNewNode(newSuccessor, currentNode, UpLeft, givenCost);
	}
}

void JPSPlus::SearchLeft(PathfindingNode * currentNode, int jumpDistance)
{
	int row = currentNode->m_row;
	int col = currentNode->m_col;

	// Consider straight line to Goal
	if (row == m_goalRow && col > m_goalCol)
	{
		int absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		if ((col - absJumpDistance) <= m_goalCol)
		{
			unsigned int diff = col - m_goalCol;
			unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(diff);
			PathfindingNode * newSuccessor = m_goalNode;
			PushNewNode(newSuccessor, currentNode, Left, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int newCol = col - jumpDistance;
		unsigned int givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(jumpDistance);
		PathfindingNode * newSuccessor = &m_mapNodes[row][newCol];
		PushNewNode(newSuccessor, currentNode, Left, givenCost);
	}
}

void JPSPlus::SearchDownLeft(PathfindingNode * currentNode, int jumpDistance)
{
	int row = currentNode->m_row;
	int col = currentNode->m_col;

	// Check for goal in general direction (straight line to Goal or Target Jump Point)
	if (row < m_goalRow && col > m_goalCol)
	{
		int absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		int diffRow = m_goalRow - row;
		int diffCol = col - m_goalCol;
		int smallerDiff = diffRow;
		if (diffCol < smallerDiff) { smallerDiff = diffCol; }

		if (smallerDiff <= absJumpDistance)
		{
			int newRow = row + smallerDiff;
			int newCol = col - smallerDiff;
			unsigned int givenCost = currentNode->m_givenCost + (SQRT_2 * smallerDiff);
			PathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
			PushNewNode(newSuccessor, currentNode, DownLeft, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int newRow = row + jumpDistance;
		int newCol = col - jumpDistance;
		unsigned int givenCost = currentNode->m_givenCost + (SQRT_2 * jumpDistance);
		PathfindingNode * newSuccessor = &m_mapNodes[newRow][newCol];
		PushNewNode(newSuccessor, currentNode, DownLeft, givenCost);
	}
}

void JPSPlus::PushNewNode(
	PathfindingNode * newSuccessor, 
	PathfindingNode * currentNode, 
	ArrayDirections parentDirection, 
	unsigned int givenCost)
{
	if (newSuccessor->m_iteration != m_currentIteration)
	{
		// Place node on the Open list (we've never seen it before)

		// Compute heuristic using octile calculation (optimized: minDiff * SQRT_2_MINUS_ONE + maxDiff)
		unsigned int diffrow = abs(m_goalRow - newSuccessor->m_row);
		unsigned int diffcolumn = abs(m_goalCol - newSuccessor->m_col);
		unsigned int heuristicCost;
		if (diffrow <= diffcolumn)
		{
			heuristicCost = (diffrow * SQRT_2_MINUS_ONE) + FIXED_POINT_SHIFT(diffcolumn);
		}
		else
		{
			heuristicCost = (diffcolumn * SQRT_2_MINUS_ONE) + FIXED_POINT_SHIFT(diffrow);
		}

		newSuccessor->m_parent = currentNode;
		newSuccessor->m_directionFromParent = parentDirection;
		newSuccessor->m_givenCost = givenCost;
		newSuccessor->m_finalCost = givenCost + heuristicCost;
		newSuccessor->m_listStatus = PathfindingNode::OnOpen;
		newSuccessor->m_iteration = m_currentIteration;

		if(newSuccessor->m_finalCost <= currentNode->m_finalCost)
		{
			m_fastStack->Push(newSuccessor);
		}
		else
		{
			m_simpleUnsortedPriorityQueue->Add(newSuccessor);
		}
	}
	else if (givenCost < newSuccessor->m_givenCost &&
		newSuccessor->m_listStatus == PathfindingNode::OnOpen)	// Might be valid to remove this 2nd condition for extra speed (a node on the closed list wouldn't be cheaper)
	{
		// We found a cheaper way to this node - update node

		// Extract heuristic cost (was previously calculated)
		unsigned int heuristicCost = newSuccessor->m_finalCost - newSuccessor->m_givenCost;

		newSuccessor->m_parent = currentNode;
		newSuccessor->m_directionFromParent = parentDirection;
		newSuccessor->m_givenCost = givenCost;
		newSuccessor->m_finalCost = givenCost + heuristicCost;

		// No decrease key operation necessary (already in unsorted open list)
	}
}