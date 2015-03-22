# JPSPlusWithGoalBounding
JPS+ with Goal Bounding

This project implements 2D grid pathfinding using JPS+ with Goal Bounding. JPS+ is an optimized preprocessed version of the Jump Point Search pathfinding algorithm. Goal Bounding is a node pruning pathfinding technique.

The Goal Bounding concept was first conceived by Steve Rabin in Jan 2015 and was first unveiled to the public in the GDC AI Summit lecture "JPS+: Over 100x Faster than A*" in San Francisco on March 2015. This presentation can be viewed at www.gdcvault.com and the PowerPoint slides are part of this project.

Goal Bounding is a node pruning technique that greatly speeds up pathfinding. Goal Bounding is the concept of computing and storing an axis-aligned bounding box for each node edge that contains all nodes that are optimally reachable via that edge. During runtime, any edge (neighboring node) will only be explored if the goal node is contained in the axis-aligned bounding box for that edge. Goal Bounding can be applied to any search space (grid, graph, NavMesh, etc.) using any search algorithm (Dijkstra, A*, JPS+, etc.) and the search space can be unidirectional and have non-uniform costs between nodes. The compromise with using Goal Bounding is that the Goal Bounding data must be preprocessed offline taking O(n^2) time, thus making it not feasible to support dynamic runtime modifications of the search space (adding or removing edges/walls). Goal Bounding requires linear storage of data, O(n), with respect to the search space (typically 4 values per node edge, with a grid representation having 8 edges per node, for a total of 32 values per node).

JPS+ was independently invented by Steve Rabin one month before it was unveiled by Harabor and Grastien at ICAPS in June 2014. A description of JPS+ can be found in the book Game AI Pro 2, published by CRC Press (April 2015). JPS+ is an optimized preprocessed version of Jump Point Search. JPS+ is a node pruning technique, like Goal Bounding, but both techniques are orthogonal to each other as they prune nodes in unrelated, but complementary, ways. JPS+ only works on grid search spaces with uniform cost. Because of the preprocessing of JPS+, the search space cannot be easily updated at runtime (adding or removing edges/walls). JPS+ requires O(n) precomputation and storage linear in the number of nodes, O(n), consisting of 1 value per node edge (8 values per grid node).

If you have any questions, comments, or suggestions, please contact Steve Rabin at steve_rabin@hotmail.com.



