/*
 * BucketPriorityQueue.cpp
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

#include "stdafx.h"
#include "BucketPriorityQueue.h"
#include "UnsortedPriorityQueue.h"

BucketPriorityQueue::BucketPriorityQueue(int buckets, int arraySize, unsigned int division)
{
	m_numBuckets = buckets;
	m_division = division;

	Reset();

	// Allocate a bunch of free buckets
	m_maxFreeBuckets = 200;
	m_nextFreeBucket = 0;
	m_freeBuckets = new UnsortedPriorityQueue*[m_maxFreeBuckets];
	for (int m = 0; m < m_maxFreeBuckets; m++)
	{
		m_freeBuckets[m] = new UnsortedPriorityQueue(arraySize);
	}

	// Allocate bucket slots
	m_bin = new UnsortedPriorityQueue*[m_numBuckets];
	for (int m = 0; m < m_numBuckets; m++)
	{
		m_bin[m] = 0;
	}
}

BucketPriorityQueue::~BucketPriorityQueue()
{
	for (int m = 0; m < m_maxFreeBuckets; m++)
	{
		if(m_freeBuckets[m] != 0)
		{
			delete m_freeBuckets[m];
		}
	}

	delete[] m_freeBuckets;

	for (int m = 0; m < m_numBuckets; m++)
	{
		if(m_bin[m] != 0)
		{
			delete m_bin[m];
		}
	}

	delete[] m_bin;
}

void BucketPriorityQueue::Push(DijkstraPathfindingNode* node)
{
	m_numNodesTracked++;
	int index = GetBinIndex(node->m_givenCost);

	if(m_bin[index] == 0)
	{
		m_bin[index] = m_freeBuckets[m_nextFreeBucket++];
	}

	m_bin[index]->Push(node);

	if (index < m_lowestNonEmptyBin)
	{
		m_lowestNonEmptyBin = index;
	}
}

DijkstraPathfindingNode* BucketPriorityQueue::Pop(void)
{
	DijkstraPathfindingNode* node = m_bin[m_lowestNonEmptyBin]->Pop();
	m_numNodesTracked--;

	if(m_bin[m_lowestNonEmptyBin]->Empty(node->m_iteration))
	{
		m_freeBuckets[--m_nextFreeBucket] = m_bin[m_lowestNonEmptyBin];
		m_bin[m_lowestNonEmptyBin] = 0;
	}

	if (m_numNodesTracked > 0)
	{
		// Find the next non-empty bin
		for (m_lowestNonEmptyBin; m_lowestNonEmptyBin < m_numBuckets; m_lowestNonEmptyBin++)
		{
			if (m_bin[m_lowestNonEmptyBin] != 0 && 
				!m_bin[m_lowestNonEmptyBin]->Empty(node->m_iteration))
			{
				break;
			}
		}
	}
	else
	{
		m_lowestNonEmptyBin = m_numBuckets;
	}

	return node;
}

void BucketPriorityQueue::DecreaseKey(DijkstraPathfindingNode* node, unsigned int lastCost)
{
	// Remove node
	int index = GetBinIndex(lastCost);
	m_bin[index]->Remove(node);

	if(m_bin[index]->Empty(node->m_iteration))
	{
		m_freeBuckets[--m_nextFreeBucket] = m_bin[index];
		m_bin[index] = 0;
	}

	// Push node
	index = GetBinIndex(node->m_givenCost);

	if(m_bin[index] == 0)
	{
		m_bin[index] = m_freeBuckets[m_nextFreeBucket++];
	}

	m_bin[index]->Push(node);

	if (index < m_lowestNonEmptyBin)
	{
		m_lowestNonEmptyBin = index;
	}
}
