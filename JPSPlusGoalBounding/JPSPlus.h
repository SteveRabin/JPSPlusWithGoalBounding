/*
 * JPSPlus.h
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
#include "PathfindingNode.h"
#include "PrecomputeMap.h"
#include "FastStack.h"
#include "SimpleUnsortedPriorityQueue.h"
#include <stdint.h>

struct xyLocJPS {
	int16_t x;
	int16_t y;
};

enum PathStatus
{
	Working,
	PathFound,
	NoPathExists
};

// This version of JPS+ is intimately tied with Goal Bounding.
// Every effort has been made to maximize speed.
// If you develop a way to improve this code or make it faster, contact steve.rabin@gmail.com

class JPSPlus
{
public:
	JPSPlus(JumpDistancesAndGoalBounds** jumpDistancesAndGoalBoundsMap, std::vector<bool> &rawMap, int w, int h);
	~JPSPlus();

	bool GetPath(xyLocJPS& s, xyLocJPS& g, std::vector<xyLocJPS> &path);

protected:

	PathStatus SearchLoop(PathfindingNode* startNode);
	void FinalizePath(std::vector<xyLocJPS> &finalPath);

	// 48 function variations of exploring (used in 2048 entry look-up table)
	// D = Down, U = Up, R = Right, L = Left, DR = Down Right, DL = Down Left, UR = Up Right, UL = Up Left
	const void Explore_Null(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_DR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_UR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_UL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_DL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D_DR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_DR_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R_UR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_UR_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_U_UL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_UL_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_L_DL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_DL_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_U_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_L_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D_DR_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_DR_R_UR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R_UR_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_UR_U_UL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_U_UL_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_UL_L_DL(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_L_DL_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_DL_D_DR(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D_R_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R_U_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_U_L_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_L_D_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R_DR_D_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R_D_DL_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_U_UR_R_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_U_R_DR_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_L_UL_U_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_L_U_UR_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D_DL_L_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D_L_UL_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_R_DR_D_DL_L(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_U_UR_R_DR_D(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_L_UL_U_UR_R(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_D_DL_L_UL_U(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);
	const void Explore_AllDirections(PathfindingNode * currentNode, JumpDistancesAndGoalBounds * map);

	void SearchDown(PathfindingNode * currentNode, int jumpDistance);
	void SearchDownRight(PathfindingNode * currentNode, int jumpDistance);
	void SearchRight(PathfindingNode * currentNode, int jumpDistance);
	void SearchUpRight(PathfindingNode * currentNode, int jumpDistance);
	void SearchUp(PathfindingNode * currentNode, int jumpDistance);
	void SearchUpLeft(PathfindingNode * currentNode, int jumpDistance);
	void SearchLeft(PathfindingNode * currentNode, int jumpDistance);
	void SearchDownLeft(PathfindingNode * currentNode, int jumpDistance);

	void PushNewNode(PathfindingNode * newSuccessor, PathfindingNode * currentNode, ArrayDirections parentDirection, unsigned int givenCost);

	// 2D array initialization and destruction
	template <typename T> void InitArray(T**& t, int width, int height);
	template <typename T> void DestroyArray(T**& t);

	// Map properties
	int m_width, m_height;

	// Open list structures
	FastStack* m_fastStack;
	SimpleUnsortedPriorityQueue* m_simpleUnsortedPriorityQueue;

	// Precomputed data
	JumpDistancesAndGoalBounds** m_jumpDistancesAndGoalBounds;

	// Preallocated nodes
	PathfindingNode** m_mapNodes;

	// Search specific info
	unsigned short m_currentIteration;	// This allows us to know if a node has been touched this iteration (faster than clearing all the nodes before each search)
	PathfindingNode* m_goalNode;
	int m_goalRow, m_goalCol;
};

