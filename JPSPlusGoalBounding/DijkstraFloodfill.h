/*
 * DijkstraFloodfill.h
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
#include "PathfindingNode.h"
#include "GenericHeap.h"
#include "BucketPriorityQueue.h"
#include "PrecomputeMap.h"

#define MAX_WIDTH 2048
#define MAX_HEIGHT 2048

#define USE_FAST_OPEN_LIST	// Significantly faster, but preallocated memory might not be large enough. Verify in cpp file.

// A Dijkstra floodfill has no goal. It floods the map from the starting node until all connected nodes are exhausted.
// This Dijkstra floodfill propagate to all explored nodes the original direction it left the staring node. This is
// needed to determine the Goal Bounds for each neighboring edge of the starting node.

class DijkstraFloodfill
{
public:
	DijkstraFloodfill(int width, int height, std::vector<bool> map, DistantJumpPoints** distantJumpPointMap);
	~DijkstraFloodfill();

	void Flood(int r, int c);
	inline int GetCurrentInteration() { return m_currentIteration; }

	DijkstraPathfindingNode ** m_mapNodes;

private:

	// 48 function variations of exploring (used in 2048 entry look-up table)
	// D = Down, U = Up, R = Right, L = Left, DR = Down Right, DL = Down Left, UR = Up Right, UL = Up Left
	const void Explore_Null(DijkstraPathfindingNode * currentNode);
	const void Explore_D(DijkstraPathfindingNode * currentNode);
	const void Explore_DR(DijkstraPathfindingNode * currentNode);
	const void Explore_R(DijkstraPathfindingNode * currentNode);
	const void Explore_UR(DijkstraPathfindingNode * currentNode);
	const void Explore_U(DijkstraPathfindingNode * currentNode);
	const void Explore_UL(DijkstraPathfindingNode * currentNode);
	const void Explore_L(DijkstraPathfindingNode * currentNode);
	const void Explore_DL(DijkstraPathfindingNode * currentNode);
	const void Explore_D_DR(DijkstraPathfindingNode * currentNode);
	const void Explore_DR_R(DijkstraPathfindingNode * currentNode);
	const void Explore_R_UR(DijkstraPathfindingNode * currentNode);
	const void Explore_UR_U(DijkstraPathfindingNode * currentNode);
	const void Explore_U_UL(DijkstraPathfindingNode * currentNode);
	const void Explore_UL_L(DijkstraPathfindingNode * currentNode);
	const void Explore_L_DL(DijkstraPathfindingNode * currentNode);
	const void Explore_DL_D(DijkstraPathfindingNode * currentNod);
	const void Explore_D_R(DijkstraPathfindingNode * currentNode);
	const void Explore_R_U(DijkstraPathfindingNode * currentNode);
	const void Explore_U_L(DijkstraPathfindingNode * currentNode);
	const void Explore_L_D(DijkstraPathfindingNode * currentNode);
	const void Explore_D_U(DijkstraPathfindingNode * currentNode);
	const void Explore_R_L(DijkstraPathfindingNode * currentNode);
	const void Explore_D_DR_R(DijkstraPathfindingNode * currentNode);
	const void Explore_DR_R_UR(DijkstraPathfindingNode * currentNode);
	const void Explore_R_UR_U(DijkstraPathfindingNode * currentNode);
	const void Explore_UR_U_UL(DijkstraPathfindingNode * currentNode);
	const void Explore_U_UL_L(DijkstraPathfindingNode * currentNode);
	const void Explore_UL_L_DL(DijkstraPathfindingNode * currentNode);
	const void Explore_L_DL_D(DijkstraPathfindingNode * currentNode);
	const void Explore_DL_D_DR(DijkstraPathfindingNode * currentNode);
	const void Explore_D_R_U(DijkstraPathfindingNode * currentNode);
	const void Explore_R_U_L(DijkstraPathfindingNode * currentNode);
	const void Explore_U_L_D(DijkstraPathfindingNode * currentNode);
	const void Explore_L_D_R(DijkstraPathfindingNode * currentNode);
	const void Explore_R_DR_D_L(DijkstraPathfindingNode * currentNode);
	const void Explore_R_D_DL_L(DijkstraPathfindingNode * currentNode);
	const void Explore_U_UR_R_D(DijkstraPathfindingNode * currentNode);
	const void Explore_U_R_DR_D(DijkstraPathfindingNode * currentNode);
	const void Explore_L_UL_U_R(DijkstraPathfindingNode * currentNode);
	const void Explore_L_U_UR_R(DijkstraPathfindingNode * currentNode);
	const void Explore_D_DL_L_U(DijkstraPathfindingNode * currentNode);
	const void Explore_D_L_UL_U(DijkstraPathfindingNode * currentNode);
	const void Explore_R_DR_D_DL_L(DijkstraPathfindingNode * currentNode);
	const void Explore_U_UR_R_DR_D(DijkstraPathfindingNode * currentNode);
	const void Explore_L_UL_U_UR_R(DijkstraPathfindingNode * currentNode);
	const void Explore_D_DL_L_UL_U(DijkstraPathfindingNode * currentNode);
	const void Explore_AllDirections(DijkstraPathfindingNode * currentNode);

	const void Explore_AllDirectionsWithChecks(DijkstraPathfindingNode * currentNode);

	void SearchDown(DijkstraPathfindingNode * currentNode);
	void SearchDownRight(DijkstraPathfindingNode * currentNode);
	void SearchRight(DijkstraPathfindingNode * currentNode);
	void SearchUpRight(DijkstraPathfindingNode * currentNode);
	void SearchUp(DijkstraPathfindingNode * currentNode);
	void SearchUpLeft(DijkstraPathfindingNode * currentNode);
	void SearchLeft(DijkstraPathfindingNode * currentNode);
	void SearchDownLeft(DijkstraPathfindingNode * currentNode);

	void PushNewNode(
		DijkstraPathfindingNode * newSuccessor, 
		DijkstraPathfindingNode * currentNode, 
		ArrayDirections startDirection, 
		ArrayDirections parentDirection, 
		unsigned int givenCost);

	// Helper functions for heap open list
	struct PathfindingNodeEqual {
		bool operator()(const DijkstraPathfindingNode* i1, const DijkstraPathfindingNode* i2)
		{
			return (i1 == i2);
		}
	};

	struct PathfindingNodeCmp : std::binary_function<bool, DijkstraPathfindingNode* const, DijkstraPathfindingNode* const> {
		bool operator()(DijkstraPathfindingNode* const lhs, DijkstraPathfindingNode* const rhs) const {
			return lhs->m_givenCost > rhs->m_givenCost;
		}
	};

	struct SearchNodeHash {
		size_t operator()(const DijkstraPathfindingNode* x) const
		{
			return (size_t)(x->m_col + (x->m_row * MAX_WIDTH));
		}
	};

	typedef GenericHeap<DijkstraPathfindingNode*, SearchNodeHash,
		PathfindingNodeEqual, PathfindingNodeCmp> PQueue;

	// Open list
	PQueue m_openList;
#ifdef USE_FAST_OPEN_LIST
	BucketPriorityQueue* m_fastOpenList;
#endif

	// Map properties
	int m_width, m_height;
	std::vector<bool> m_map;

	// Search specific info
	int m_currentIteration;	// This allows us to know if a node has been touched this iteration (faster than clearing all the nodes before each search)

	// Wall queries
	bool IsEmpty(int r, int c);

	// 2D array initialization and destruction
	template <typename T> void InitArray(T**& t, int width, int height);
	template <typename T> void DestroyArray(T**& t);
};
