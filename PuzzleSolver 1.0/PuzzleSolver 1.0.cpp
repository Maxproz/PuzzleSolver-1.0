// PuzzleSolver 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <vector> // vector
#include <utility> // pair



// Headers needed for Grid class
#include <memory>
#include <iostream> // For the PrintGrid() function


// Headers needed for Grid class and Region class 
#include <set>

// Need a grid class that will define what a region is, and also store a container of an undecided format similar to - vector<vector<shared_ptr<Cells>>
const int Width = 10;
const int Height = 9;

using namespace std;

// TODO: Add the performance tracking code from the first version of the puzzle solver.


// TODO: Filter into function
using Islands = vector<pair<pair<int, int>, int>>;

// Instead of reading user string input or something, we will have this code initalize our Grid for simplicity
Islands NumberedIslandCells
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

// NOTE: We are using an upper left origin in our grid/gameboard
class Grid final
{
public:
	Grid() = delete;
	Grid(const int Width, const int Height, const Islands& Islands) :
		m_Width{ Width }, m_Height{ Height }, m_Cells(), m_Regions(), m_Total_Black_Cells(m_Width * m_Height)
	{
		if (m_Width <= 0 || m_Height <= 0)
		{
			throw logic_error("The height or width of the gameboard have to be greater than zero");
		}

		// Initialize m_cells. We must set everything to UNKNOWN before calling add_region() below.

		m_Cells.resize(m_Width, vector<pair<State, shared_ptr<Region>>>(m_Height, make_pair(Grid::State::UNKNOWN, shared_ptr<Region>())));

		for (int x = 0; x < m_Width; ++x)
		{
			for (int y = 0; y < m_Height; ++y)
			{
				for (const auto& Island : Islands)
				{
					if (x == Island.first.first && y == Island.first.second)
					{
						cell(x, y) = static_cast<State>(Island.second);

						AddRegion(x, y);

						m_Total_Black_Cells -= Island.second;
					}
				}
			}
		}
	}

	Grid(const Grid& Copy) : 
		m_Width{ Copy.m_Width }, m_Height{ Copy.m_Height }, m_Cells(Copy.m_Cells), m_Regions(), m_Total_Black_Cells(Copy.m_Total_Black_Cells)
	{
		// m_Cells doesn't need a deep copy since we change the shared_ptr it points to inside of the function body anyway.

		for (auto i = Copy.m_Regions.begin(); i != Copy.m_Regions.end(); ++i) 
		{
			m_Regions.insert(make_shared<Region>(**i));
		}

		for (auto i = m_Regions.begin(); i != m_Regions.end(); ++i)
		{
			for (auto k = (*i)->begin(); k != (*i)->end(); ++k)
			{
				region(k->first, k->second) = *i;
			}
		}
	}
	//Grid(Grid&& Move);
	//Grid& operator=(const Grid& CopyAssign);
	//Grid& operator=(Grid&& MoveAssign);



	// TODO: This is a temp print function to test output
	void PrintGameBoard() const
	{
		// For printing the Grid 
		// W == White Cell
		// B == Black Cell
		// ? == Unknown Cell
		// (1 - 9) is the number of a numbered cell (Island)
		for (int y = 0; y < m_Height; ++y)
		{
			for (int x = 0; x < m_Width; ++x)
			{
				auto CurrentCellState = cell(x, y);

				if (CurrentCellState == Grid::State::WHITE)
				{
					cout << "W" << " ";
				}
				else if (CurrentCellState == Grid::State::BLACK)
				{
					cout << "B" << " ";
				}
				else if (CurrentCellState == Grid::State::UNKNOWN)
				{
					cout << "?" << " ";
				}
				else
				{
					// Prints the number of a numbered cell
					cout << static_cast<int>(CurrentCellState) << " ";
				}
			}
			cout << endl; // End of the row or column idk..

		} cout << endl;
	}


private:

	void Swap(Grid& other)
	{
		std::swap(m_Width, other.m_Width);
		std::swap(m_Height, other.m_Width);
		std::swap(m_Cells, other.m_Cells);
		std::swap(m_Regions, other.m_Regions);
		std::swap(m_Total_Black_Cells, other.m_Total_Black_Cells);
	}


	enum class State : int
	{
		BLACK = -3,
		WHITE = -2, 
		UNKNOWN = -1
		// Numbered > 0
	};

	class Region final
	{
	public:
		Region() = delete;
		Region(const int x, const int y, const State& InState, const set<pair<int, int>>& Unknowns) :
			m_Unknowns(), m_Coords()
		{
			if (InState == Grid::State::UNKNOWN)
			{
				throw logic_error("LOGIC ERROR: Grid::Region::Region() - state must be known!");
			}

			m_RegionState = InState;
			m_Unknowns = Unknowns;

			m_Coords.insert(make_pair(x, y));
		}

		// rest of the copy/move/destrutor can be implicit

		// TODO: Rest of public interface for region

		set<pair<int, int>>::const_iterator begin() const { return m_Coords.begin(); }
		set<pair<int, int>>::const_iterator end() const { return m_Coords.end(); }

	private:
		set<pair<int, int>> m_Coords;
		set<pair<int, int>>  m_Unknowns;
		State m_RegionState;
	};
	



	void AddRegion(const int x, const int y)
	{
		auto Unknowns = set<pair<int, int>>();
		InsertValidUnknownNeighbors(x, y, Unknowns);

		auto NewReg = make_shared<Region>(x, y, cell(x, y), Unknowns);

		region(x, y) = NewReg;

		m_Regions.insert(NewReg);
	}

	Grid::State& cell(const int x, const int y) { return m_Cells[x][y].first; }
	shared_ptr<Region>& region(const int x, const int y) { return m_Cells[x][y].second; }

	const Grid::State& cell(const int x, const int y) const { return m_Cells[x][y].first; }
	const shared_ptr<Region>& region(const int x, const int y) const { return m_Cells[x][y].second; }

	bool IsValid(const int x, const int y)
	{
		if (x >= 0 && x < m_Width && y >= 0 && y < m_Height)
		{
			return true;
		}
		return false;
	}

	template <typename Func>
	void For_All_Valid_Neighbors(const int x, const int y, Func F)
	{
		if (IsValid(x - 1, y))
		{
			F(x - 1, y);
		}

		if (IsValid(x + 1, y))
		{
			F(x + 1, y);
		}

		if (IsValid(x, y + 1))
		{
			F(x, y + 1);
		}

		if (IsValid(x, y - 1))
		{
			F(x, y - 1);
		}
	}

	template <typename Func>
	void For_All_Valid_Unknown_Neighbors(const int x, const int y, Func F)
	{
		if (IsValid(x - 1, y))
		{
			if (cell(x - 1, y) == Grid::State::UNKNOWN)
			{
				F(x - 1, y);
			}
		}

		if (IsValid(x + 1, y))
		{
			if (cell(x + 1, y) == Grid::State::UNKNOWN)
			{
				F(x + 1, y);
			}
		}

		if (IsValid(x, y + 1))
		{
			if (cell(x, y + 1) == Grid::State::UNKNOWN)
			{
				F(x, y + 1);
			}
		}

		if (IsValid(x, y - 1))
		{
			if (cell(x, y - 1) == Grid::State::UNKNOWN)
			{
				F(x, y - 1);
			}
		}
	}

	void InsertValidNeighbors(const int x, const int y, set<pair<int, int>>& OutSet)
	{
		For_All_Valid_Neighbors(x, y, [&](auto neighbor_x, auto neighbor_y) -> auto
		{
			OutSet.insert(make_pair(neighbor_x, neighbor_y));
		});
	}

	void InsertValidUnknownNeighbors(const int x, const int y, set<pair<int, int>>& OutSet)
	{
		For_All_Valid_Unknown_Neighbors(x, y, [&](auto neighbor_x, auto neighbor_y) -> auto
		{
			OutSet.insert(make_pair(neighbor_x, neighbor_y));
		});
	}


	
private:

	int m_Width{ 0 };
	int m_Height{ 0 };

	// This is (m_Width * m_Height) - sum of all numbered islands
	int m_Total_Black_Cells{ 0 };


	vector<vector<pair<Grid::State, shared_ptr<Region>>>>  m_Cells;

	set<shared_ptr<Region>> m_Regions;
};




int main()
{
	Grid Gameboard(Width, Height, NumberedIslandCells);

	Gameboard.PrintGameBoard();


    return 0;
}

