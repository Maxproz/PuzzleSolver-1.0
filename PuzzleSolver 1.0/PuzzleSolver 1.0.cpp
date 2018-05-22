// PuzzleSolver 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <vector> // vector
#include <utility> // pair




// Need a grid class that will define what a region is, and also store a container of an undecided format similar to - vector<vector<shared_ptr<Cells>>
const int Width = 10;
const int Height = 9;

using namespace std;

// TODO: Filter into function
using Islands = vector<pair<pair<int, int>, int>>;

// Instead of reading user string input or something, we will have this code initalize our Grid for simplicity
vector<Islands> NumberedIslandCells
{
	{
	make_pair(make_pair(0, 0), 2),
	make_pair(make_pair(0, 6), 2),
	make_pair(make_pair(1, 2), 2),
	make_pair(make_pair(1, 8), 1),
	make_pair(make_pair(2, 5), 2),
	make_pair(make_pair(3, 6), 4),
	make_pair(make_pair(4, 2), 7),
	make_pair(make_pair(6, 1), 2),
	make_pair(make_pair(6, 4), 3),
	make_pair(make_pair(6, 8), 2),
	make_pair(make_pair(7, 5), 3),
	make_pair(make_pair(8, 4), 3),
	make_pair(make_pair(8, 8), 4),
	make_pair(make_pair(9, 0), 2)
	}
};



int main()
{



    return 0;
}

