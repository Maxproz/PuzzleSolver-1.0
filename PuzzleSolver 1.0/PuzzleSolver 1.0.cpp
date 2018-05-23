// PuzzleSolver 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <vector> // vector
#include <utility> // pair
#include <sstream>   // for TimerCode ~ ostringstream variable
#include "Windows.h" // for TimerCode ~ LARGE_INTEGER and QueryPerformanceCounter(&li);
#include <tuple> // for std::get
#include <string>
#include <fstream>
#include <map>
#include <algorithm> // std::noneof

// Headers needed for Grid class
#include <memory>
#include <iostream> // For the PrintGrid() function


// Headers needed for Grid class and Region class 
#include <set>

// Need a grid class that will define what a region is, and also store a container of an undecided format similar to - vector<vector<shared_ptr<Cells>>
const int Width = 10;
const int Height = 9;

using namespace std;


long long counter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

long long frequency()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return li.QuadPart;
}

string format_time(const long long start, const long long finish)
{
	ostringstream oss;

	if ((finish - start) * 1000 < frequency())
	{
		oss << (finish - start) * 1000000.0 / frequency() << " microseconds";
	}
	else if (finish - start < frequency())
	{
		oss << (finish - start) * 1000.0 / frequency() << " milliseconds";
	}
	else
	{
		oss << (finish - start) * 1.0 / frequency() << " seconds";
	}

	return oss.str();
}






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
private:
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
		int size() const { return m_Coords.size(); }

		bool contains(const int x, const int y) const 
		{
			return m_Coords.find(make_pair(x, y)) != m_Coords.end();
		}

		template <typename InIt>
		void insert(InIt first, InIt last)
		{
			m_Coords.insert(first, last);
		}


		bool black() const { return m_RegionState == State::BLACK; }
		bool white() const { return m_RegionState == State::WHITE; }

		bool numbered() const { return static_cast<int>(m_RegionState) >= 1; }

		const int number() const 
		{
			if (!numbered())
				throw std::logic_error("invalid call for number()");
			else
			{
				return static_cast<int>(m_RegionState);
			}
		}
		
		set<pair<int, int>>::const_iterator unk_begin() const { return m_Unknowns.begin(); }
		set<pair<int, int>>::const_iterator unk_end() const { return m_Unknowns.end(); }
		int unk_size() const { return m_Unknowns.size(); }

		template <typename InIt> void unk_insert(InIt first, InIt last)
		{
			m_Unknowns.insert(first, last);
		}

		void unk_erase(const int x, const int y)
		{
			m_Unknowns.erase(make_pair(x, y));
		}

	private:
		set<pair<int, int>> m_Coords;
		set<pair<int, int>>  m_Unknowns;
		State m_RegionState;
	};

public:
	Grid() = delete;
	Grid(const int Width, const int Height, const Islands& Islands) :
		m_Width{ Width }, m_Height{ Height }, m_Cells(), m_Regions(), m_Total_Black_Cells(m_Width * m_Height), m_SolveStatus(SolveStatus::KEEP_GOING), m_output()
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

		print("I'm okay to go!");
	}

	Grid(const Grid& Copy) : 
		m_Width{ Copy.m_Width }, m_Height{ Copy.m_Height }, m_Cells(Copy.m_Cells), m_Regions(), m_Total_Black_Cells(Copy.m_Total_Black_Cells), m_SolveStatus(Copy.m_SolveStatus),
		m_output(Copy.m_output)
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

	enum SolveStatus
	{
		CONTRADICTION,
		KEEP_GOING,
		SOLUTION_FOUND,
		UNABLE_TO_PROCEED
	};

	SolveStatus SolvePuzzle(const bool IsGuessing = true, const bool IsVerbose = true);

	

	void write(ostream& os, const long long start, const long long finish) const;


	void print(const string& s, const set<pair<int, int>>& updated = set<pair<int, int>>());


	//// TODO: This is a temp print function to test output
	//void PrintGameBoard() const
	//{
	//	// For printing the Grid 
	//	// W == White Cell
	//	// B == Black Cell
	//	// ? == Unknown Cell
	//	// (1 - 9) is the number of a numbered cell (Island)
	//	for (int y = 0; y < m_Height; ++y)
	//	{
	//		for (int x = 0; x < m_Width; ++x)
	//		{
	//			auto CurrentCellState = cell(x, y);

	//			if (CurrentCellState == Grid::State::WHITE)
	//			{
	//				cout << "W" << " ";
	//			}
	//			else if (CurrentCellState == Grid::State::BLACK)
	//			{
	//				cout << "B" << " ";
	//			}
	//			else if (CurrentCellState == Grid::State::UNKNOWN)
	//			{
	//				cout << "?" << " ";
	//			}
	//			else
	//			{
	//				// Prints the number of a numbered cell
	//				cout << static_cast<int>(CurrentCellState) << " ";
	//			}
	//		}
	//		cout << endl; // End of the row or column idk..

	//	} cout << endl;
	//}

	int Grid::known() const
	{
		int ret = 0;

		for (int x = 0; x < m_Width; ++x)
		{
			for (int y = 0; y < m_Height; ++y)
			{
				if (cell(x, y) != State::UNKNOWN)
				{
					++ret;
				}
			}
		}

		return ret;
	}

private:


	bool process(bool verbose, const set<pair<int, int>>& mark_as_black,
		const set<pair<int, int>>& mark_as_white, const string& s);


	const std::string detect_contradictions(map<shared_ptr<Region>, set<pair<int, int>>>& InCache) const
	{
		for (int x = 0; x < m_Width - 1; ++x) {
			for (int y = 0; y < m_Height - 1; ++y) {
				if (cell(x, y) == State::BLACK
					&& cell(x + 1, y) == State::BLACK
					&& cell(x, y + 1) == State::BLACK
					&& cell(x + 1, y + 1) == State::BLACK)
				{

					return "Contradiction found! Pool detected.";
				}
			}
		}

		int black_cells = 0;
		int white_cells = 0;

		for (auto i = m_Regions.begin(); i != m_Regions.end(); ++i)
		{
			const Region& r = **i;

			// We don't need to look for gigantic black regions because
			// counting black cells is strictly more powerful.

			if (r.white() && impossibly_big_white_region(r.size())
				|| r.numbered() && r.size() > r.number())
			{

				return "Contradiction found! Gigantic region detected.";
			}

			(r.black() ? black_cells : white_cells) += r.size();


			if (confined(*i, InCache)) {
				return "Contradiction found! Confined region detected.";
			}
		}

		if (black_cells > m_Total_Black_Cells)
		{
			return "Contradiction found! Too many black cells detected.";
		}

		if (white_cells > (m_Width * m_Height) - m_Total_Black_Cells)
		{
			return "Contradiction found! Too many white/numbered cells detected.";
		}

		return "";
	}




	bool impossibly_big_white_region(const int N) const
	{
		// A white region of N cells is impossibly big
		// if it could never be connected to any numbered region.

		return none_of(m_Regions.begin(), m_Regions.end(),
			[N](const shared_ptr<Region>& p)
		{
			// Add one because a bridge would be needed.
			return p->numbered() && p->size() + N + 1 <= p->number();
		});
	}


	// TODO: Finish
	// Is r confined, assuming that we can't consume forbidden cells?
	bool confined(const shared_ptr<Region>& r,
		map<shared_ptr<Region>, set<pair<int, int>>>& cache,
		const set<pair<int, int>>& forbidden = set<pair<int, int>>()) const
	{

		// When we look for contradictions, we run confinement analysis (A) without forbidden cells.
		// This gives us an opportunity to accelerate later confinement analysis (B)
		// when we have forbidden cells.
		// During A, we'll record the unknown cells that we consumed.
		// During B, if none of the forbidden cells are ones that we consumed,
		// then the forbidden cells can't confine us.

		return false;
	}


	void Swap(Grid& other)
	{
		std::swap(m_Width, other.m_Width);
		std::swap(m_Height, other.m_Width);
		std::swap(m_Cells, other.m_Cells);
		std::swap(m_Regions, other.m_Regions);
		std::swap(m_Total_Black_Cells, other.m_Total_Black_Cells);
		std::swap(m_SolveStatus, other.m_SolveStatus);
		std::swap(m_output, other.m_output);

	}



	



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

	void mark(const State& InState, const int x, const int y)
	{
		if (InState != State::BLACK && InState != State::WHITE)
		{
			throw std::logic_error("InState in mark() must be black or white");
		}

		if (cell(x, y) != State::UNKNOWN)
		{
			m_SolveStatus = CONTRADICTION;
			return;
		}

		// Set the cell's new state. Because it's now known,
		cell(x, y) = InState;

		// update each region's set of surrounding unknown cells.
		for (auto i = m_Regions.begin(); i != m_Regions.end(); ++i) 
		{
			(*i)->unk_erase(x, y);
		}

		// Marking a cell as white or black could create an independent region,
		// could be added to an existing region, or could connect 2, 3, or 4 separate regions.
		// The easiest thing to do is to create a region for this cell,
		// and then fuse it to any adjacent compatible regions.

		AddRegion(x, y);

		// Don't attempt to cache these regions.
		// Each fusion could change this cell's region or its neighbors' regions.

		For_All_Valid_Neighbors(x, y, [this, x, y](const int a, const int b) 
		{
			fuse_regions(region(x, y), region(a, b));
		});

	}
	
	// Note that r1 and r2 are passed by modifiable value. It's convenient to be able to swap them.
	void fuse_regions(shared_ptr<Region> r1, shared_ptr<Region> r2)
	{

		// If we don't have two different regions, we're done.

		if (!r1 || !r2 || r1 == r2) {
			return;
		}

		// If we're asked to fuse two numbered regions, we've encountered a contradiction.
		// Remember this, so that solve() can report the contradiction.

		if (r1->numbered() && r2->numbered())
		{
			m_SolveStatus = CONTRADICTION;
			return;
		}

		// Black regions can't be fused with non-black regions.

		if (r1->black() != r2->black()) 
		{
			return;
		}

		// We'll use r1 as the "primary" region, to which r2's cells are added.
		// It would be efficient to process as few cells as possible.
		// Therefore, we'd like to use the bigger region as the primary region.

		if (r2->size() > r1->size()) 
		{
			std::swap(r1, r2);
		}

		// However, if the secondary region is numbered, then the primary region
		// must be white, so we need to swap them, even if the numbered region is smaller.

		if (r2->numbered()) 
		{
			std::swap(r1, r2);
		}

		// Fuse the secondary region into the primary region.

		r1->insert(r2->begin(), r2->end());
		r1->unk_insert(r2->unk_begin(), r2->unk_end());


		// (just need to update the pointer since we already fused the m_coords and m_unknowns in the last step)
		// as of right now, the shared_ptr count for r2 is >= 3 (maybe more depending on amount of cell pointers)

		// Update the secondary region's cells to point to the primary region.
		for (auto i = r2->begin(); i != r2->end(); ++i)
		{
			region(i->first, i->second) = r1; // shared_ptr keeps this alive
		} // as of right now, the shared_ptr count for r2 is 2 (the one in m_regions, and r2) maybe

		  // Erase the secondary region from the set of all regions.
		  // When this function returns, the secondary region will be destroyed.

		m_Regions.erase(r2);
		// as of right now, the shared_ptr count for r2 is 1 (scope of r2 in this function)
	} // as of right now, the shared_ptr count for r2 is 0 (region pointed to by r2 deleted on the heap)

private:

	int m_Width{ 0 };
	int m_Height{ 0 };

	// This is (m_Width * m_Height) - sum of all numbered islands
	int m_Total_Black_Cells{ 0 };

	SolveStatus m_SolveStatus = SolveStatus::KEEP_GOING;

	// This stores the output that is generated during solving, to be converted into HTML later.
	vector<tuple<string, vector<vector<State>>, set<pair<int, int>>, long long>> m_output;

	vector<vector<pair<Grid::State, shared_ptr<Region>>>>  m_Cells;

	set<shared_ptr<Region>> m_Regions;
};

void Grid::print(const string& s, const set<pair<int, int>>& updated)
{
	//auto w = static_cast<State>(m_width);
	//auto h = static_cast<State>(m_height);
	vector<vector<State>> v(m_Width, std::vector<State>(m_Height));

	for (int x = 0; x < m_Width; ++x)
	{
		for (int y = 0; y < m_Height; ++y)
		{
			v[x][y] = cell(x, y);
		}
	}

	m_output.push_back(make_tuple(s, v, updated, counter()));
}


void Grid::write(ostream& os, const long long start, const long long finish) const
{
	os <<
		"<!DOCTYPE html>\n"
		"<html>\n"
		"  <head>\n"
		"    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />\n"
		"    <style type=\"text/css\">\n"
		"      body {\n"
		"        font-family: Verdana, sans-serif;\n"
		"        line-height: 1.4;\n"
		"      }\n"
		"      table {\n"
		"        border: solid 3px #000000;\n"
		"        border-collapse: collapse;\n"
		"      }\n"
		"      td {\n"
		"        border: solid 1px #000000;\n"
		"        text-align: center;\n"
		"        width: 20px;\n"
		"        height: 20px;\n"
		"      }\n"
		"      td.unknown   { background-color: #C0C0C0; }\n"
		"      td.white.new { background-color: #FFFF00; }\n"
		"      td.white.old { }\n"
		"      td.black.new { background-color: #008080; }\n"
		"      td.black.old { background-color: #808080; }\n"
		"      td.number    { }\n"
		"    </style>\n"
		"    <title>Nurikabe</title>\n"
		"  </head>\n"
		"  <body>\n";

	long long old_ctr = start;

	for (auto i = m_output.begin(); i != m_output.end(); ++i) {
		const string& s = get<0>(*i);
		const auto& v = get<1>(*i);
		const auto& updated = get<2>(*i);
		const long long ctr = get<3>(*i);

		os << s << " (" << format_time(old_ctr, ctr) << ")\n";

		old_ctr = ctr;

		os << "<table>\n";

		for (int y = 0; y < m_Height; ++y) {
			os << "<tr>";

			for (int x = 0; x < m_Width; ++x) {
				os << "<td class=\"";
				os << (updated.find(make_pair(x, y)) != updated.end() ? "new " : "old ");

				switch (v[x][y]) {
					case State::UNKNOWN: os << "unknown\"> ";           break;
					case State::WHITE:   os << "white\">.";           break;
					case State::BLACK:   os << "black\">#";           break;
					default:      os << "number\">" << static_cast<int>(v[x][y]); break;
				}

				os << "</td>";
			}

			os << "</tr>\n";
		}

		os << "</table><br/>\n";
	}

	os << "Total: " << format_time(start, finish) << "\n";

	os <<
		"  </body>\n"
		"</html>\n";
}

bool Grid::process(const bool verbose, const set<pair<int, int>>& mark_as_black,
	const set<pair<int, int>>& mark_as_white, const string& s) 
{

	if (mark_as_black.empty() && mark_as_white.empty()) 
	{
		return false;
	}

	for (auto i = mark_as_black.begin(); i != mark_as_black.end(); ++i)
	{
		mark(State::BLACK, i->first, i->second);
	}

	for (auto i = mark_as_white.begin(); i != mark_as_white.end(); ++i) 
	{
		mark(State::WHITE, i->first, i->second);
	}

	if (verbose)
	{
		set<pair<int, int>> updated(mark_as_black);
		updated.insert(mark_as_white.begin(), mark_as_white.end());

		string t = s;

		if (m_SolveStatus == SolveStatus::CONTRADICTION)
		{
			t += " (Contradiction found! Attempted to fuse two numbered regions"
				" or mark an already known cell.)";
		}

		print(t, updated);
	}

	return true;
}


Grid::SolveStatus Grid::SolvePuzzle(const bool IsGuessing, const bool IsVerbose)
{
	map<shared_ptr<Region>, set<pair<int, int>>> cache;


	// Look for contradictions. Do this first.

	{
		const string s = detect_contradictions(cache);

		if (!s.empty())
		{
			if (IsVerbose)
			{
				print(s);
			}

			return SolveStatus::CONTRADICTION;
		}
	}


	// See if we're done. Do this second.

	if (known() == m_Width * m_Height)
	{
		if (IsVerbose)
		{
			print("I'm done!");
		}

		return SolveStatus::SOLUTION_FOUND;
	}


	set<pair<int, int>> mark_as_black;
	set<pair<int, int>> mark_as_white;


	// Look for complete islands.
	for (auto i = m_Regions.begin(); i != m_Regions.end(); ++i)
	{
		const Region& r = **i;

		if (r.numbered())
		{
			if (r.size() == r.number())
			{
				mark_as_black.insert(r.unk_begin(), r.unk_end());
			}
		}
	}

	if (process(IsVerbose, mark_as_black, mark_as_white, "Updated complete islands"))
	{
		return m_SolveStatus;
	}


	// Look for partial regions that can expand into only one cell. They must expand.
	for (auto i = m_Regions.begin(); i != m_Regions.end(); ++i)
	{
		const Region& r = **i;

		if (r.numbered())
		{
			if ((r.size() < r.number()) && r.unk_size() == 1)
			{
				mark_as_white.insert(*r.unk_begin());
			}
		}
		else if (r.white() && r.unk_size() == 1)
		{
			mark_as_white.insert(*r.unk_begin());
		}
		else // r.black()
		{
			if (r.size() < m_Total_Black_Cells && r.unk_size() == 1)
			{
				mark_as_black.insert(*r.unk_begin());
			}
		}
	}

	if (process(IsVerbose, mark_as_black, mark_as_white, "Updated partial regions that expand into one cell"))
	{
		return m_SolveStatus;
	}





	// Look for N - 1 islands with exactly two diagonal liberties.





	if (process(IsVerbose, mark_as_black, mark_as_white, "N - 1 islands with exactly two diagonal liberties found.")) 
	{
		return m_SolveStatus;
	}

	// Look for unreachable cells. They must be black.
	// This supersedes complete island analysis and forbidden bridge analysis.
	// (We run complete island analysis above because it's fast
	// and it makes the output easier to understand.)




	// Look for squares of one unknown and three black cells, or two unknown and two black cells.


	// Look for isolated unknown regions.
	


	// Look for Confinement
	// A region would be "confined" if it could not be completed.
	// Black regions need to consume m_total_black cells.
	// White regions need to escape to a number.
	// Numbered regions need to consume N cells.

	// Confinement analysis consists of imagining what would happen if a particular unknown cell
	// were black or white. If that would cause any region to be confined, the unknown cell
	// must be the opposite color.

	// Black cells can't confine black regions, obviously.
	// Black cells can confine white regions, by isolating them.
	// Black cells can confine numbered regions, by confining them to an insufficiently large space.

	// White cells can confine black regions, by confining them to an insufficiently large space.
	//   (Humans look for isolation here, i.e. permanently separated black regions.
	//   That's harder for us to detect, but counting cells is similarly powerful.)
	// White cells can't confine white regions.
	//   (This is true for freestanding white cells, white cells added to other white regions,
	//   and white cells added to numbered regions.)
	// White cells can confine numbered regions, when added to other numbered regions.
	//   This is the most complicated case to analyze. For example:
	//   ####3
	//   #6 xXx
	//   #.  x
	//   ######
	//   Imagining cell 'X' to be white additionally prevents region 6 from consuming
	//   three 'x' cells. (This is true regardless of what other cells region 3 would
	//   eventually occupy.)


	// If we get here, try guessing cells repeatedly to see if we can figure out what to mark a cell
	// If (IsGuessing)
	// {
			// Try to guess the rest if stuck

	// }


	if (IsVerbose)
	{
		print("I have no idea");
	}

	return SolveStatus::UNABLE_TO_PROCEED;

}

int main()
{
	struct Data
	{
		int m_Width;
		int m_Height;
		std::string m_Name;
		Islands m_Islands;

		Data(const int Width, const int Height, const std::string Name, const Islands& InIslands) :
			m_Width{ Width }, m_Height{ Height }, m_Name{ Name }, m_Islands{ InIslands }
		{

		}
	};


	Data NurikabeHard(Width, Height, "Nurikabe Hard", NumberedIslandCells);

	Grid Gameboard(NurikabeHard.m_Width, NurikabeHard.m_Height, NurikabeHard.m_Islands);

	//Gameboard.PrintGameBoard();

	try
	{
		const long long start = counter();

		while (Gameboard.SolvePuzzle() == Grid::SolveStatus::KEEP_GOING) {}

		const long long finish = counter();


		ofstream f(NurikabeHard.m_Name + string(".html"));

		Gameboard.write(f, start, finish);

		cout << NurikabeHard.m_Name << ": " << format_time(start, finish) << ", ";


		const int k = Gameboard.known();
		const int cells = NurikabeHard.m_Width * NurikabeHard.m_Height;

		cout << k << "/" << cells << " (" << k * 100.0 / cells << "%) solved" << endl;


	}
	catch (const exception& e)
	{
		cerr << "EXCEPTION CAUGHT! \"" << e.what() << "\"" << endl;
		return EXIT_FAILURE;
	}
	catch (...)
	{
		cerr << "UNKNOWN EXCEPTION CAUGHT!" << endl;
		return EXIT_FAILURE;
	}

    return 0;
}

