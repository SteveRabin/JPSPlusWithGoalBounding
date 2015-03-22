/*
 * BucketPriorityQueue.h
 *
 * Copyright (c) 2015, Steve Rabin
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
#include "UnsortedPriorityQueue.h"

// This data structure is dangerous since there are no safeguards if the max size is exceeded.
// However, any method to detect a problem and deal with it gracefully will sacrifice speed.
// The number of buckets can be reduced if the maximum final cost is known to be lower (significant memory savings)
// In this code base, this is only used by the Dijkstra floodfill for Goal Bounding preprocessing.

class BucketPriorityQueue
{
public:
	BucketPriorityQueue(int buckets, int arraySize, unsigned int division);
	~BucketPriorityQueue();

	inline void SetBaseCost(unsigned int baseCost) { m_baseCost = baseCost; }
	inline void Reset() { m_lowestNonEmptyBin = m_numBuckets; m_numNodesTracked = 0; m_baseCost = 0; }
	inline bool Empty() { return m_numNodesTracked == 0; }
	void Push(DijkstraPathfindingNode* node);
	DijkstraPathfindingNode* Pop(void);
	void DecreaseKey(DijkstraPathfindingNode* node, unsigned int lastCost);

private:
	int m_numBuckets;
	int m_lowestNonEmptyBin;
	int m_numNodesTracked;
	unsigned int m_division;
	unsigned int m_baseCost;
	UnsortedPriorityQueue** m_bin;

	int m_maxFreeBuckets;
	int m_nextFreeBucket;
	UnsortedPriorityQueue** m_freeBuckets;

	inline int GetBinIndex(unsigned int cost) { return ((cost - m_baseCost) / m_division); }
};

