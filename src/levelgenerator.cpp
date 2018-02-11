#include "levelgenerator.h"

#include "util.h"

#include <iostream>

LevelGenerator::LevelGenerator(size_t w, size_t h, size_t n, bool box_changes)
    : width(w)
    , height(h)
    , numBoxes(n)
    , box_changes(box_changes)
    , available(0)
    , m_max(0)
{
}

size_t LevelGenerator::generate(std::vector<char> v)
{
    available = 0;

    m_map.init(width, height, v);

    for(const auto& c : m_map)
    {
        if(c == ' ')
        {
            available++;
        }
    }

    closedSet.clear();
    parents.clear();
    openSet.clear();
    checked.clear();

    placeGoals();

    std::set<Position> ps;

    ps.insert(goals.begin(), goals.end());

    const auto playerPositions = initPlayer(ps);

    for (const auto& p : playerPositions)
    {
        openSet.push_back(State(p, ps, 0));
        checked.insert(State(p, ps, 0));
    }

    while(!openSet.empty())
    {
        State current = openSet.front();
        openSet.pop_front();

        auto it = parents.find(current);
        if (it == parents.end())
        {
            closedSet.insert(std::pair<State, size_t>(current, 0));
        }
        else
        {
            State child = it->first;
            State parent = it->second;
            size_t pValue = closedSet.find(parent)->second;
            pValue += box_changes ? child.getBoxChange() : 1;
            closedSet.insert(std::pair<State, size_t>(current, pValue));
        }

        auto states = expand(current);
        for(const auto& s : states)
        {
            if (checked.count(s) == 0)
            {
                openSet.push_back(s);
                checked.insert(s);
                parents.insert(std::pair<State, State>(s, current));
            }
        }
    }

    size_t max = 0;
    State best;

    for(const auto& [state,value] : closedSet)
    {
        if(value >= max)
        {
            max = value;
            best = state;
        }
    }

    if(m_max < max)
    {
        m_max = max;
        m_best = best;
        m_bestMap = m_map;

        m_bestSolution.clear();
        m_bestGoals.clear();
        m_bestGoals = goals;
        State t = best;
        m_bestSolution.push_back(best);
        while(parents.find(t) != parents.end())
        {
            m_bestSolution.push_back(parents.find(t)->second);
            t = parents.find(t)->second;
        }
    }

    return max;
}

std::vector<Position> LevelGenerator::initPlayer(std::set<Position> boxes)
{
    size_t count = 0;
    std::vector<Position> pos;
    Position p, min;

    Map tempMap = m_map;

    for(const auto& b : boxes)
    {
        m_map(b) = '$';
    }

    while(count != available - numBoxes)
    {
        p.x = Random(0, width);
        p.y = Random(0, height);

        min = p;

        if(m_map(p) != ' ')
        {
            continue;
        }
        count += floodfill(p, min);
        pos.push_back(min);
    }
    m_map = tempMap;

    return pos;
}

Position LevelGenerator::placePlayer(std::set<Position> boxes, Position prev)
{
    size_t count = 0;
    Position min;

    Map tempMap = m_map;

    for(const auto& b : boxes)
    {
        m_map(b) = '$';
    }

    min = prev;
    count += floodfill(prev, min);
    m_map = tempMap;

    return min;
}

size_t LevelGenerator::floodfill(Position p, Position& min)
{
    if (p.x < 0 || p.x >= width
        || p.y < 0 || p.y >= height
        || m_map(p) != ' ')
    {
        return 0;
    }
    
    size_t c = 1;
    if (p < min)
    {
        min = p;
    }

    m_map(p) = '_';

    c += floodfill(Position(p.x + 1, p.y), min);
    c += floodfill(Position(p.x - 1, p.y), min);
    c += floodfill(Position(p.x, p.y + 1), min);
    c += floodfill(Position(p.x, p.y - 1), min);

    return c;
}

std::vector<char>& LevelGenerator::getMap()
{
    return m_map.getMap();
}

std::vector<State> LevelGenerator::expand(State s)
{
    std::vector<State> eStates;
    Position min(0,0);
    Position max(width, height);

    Map tempMap = m_map;

    Position minPos = s.getPlayer();
    std::set<Position> boxes = s.getBoxes();
    for(const auto& b : boxes)
    {
        m_map(b) = '$';
    }
    floodfill(s.getPlayer(), minPos);

    Map floodedMap = m_map;
    m_map = tempMap;

    for(const auto& box : boxes)
    {
        Position actual;

        for(size_t i = 0; i < 4; ++i)
        {
            if (box.isInInterval(min,max,direction[i])
                && floodedMap(box + direction[i]) == '_')
            {
                actual = box;
                actual += direction[i];

                while (actual.isInInterval(min,max,direction[i])
                    && floodedMap(actual + direction[i]) == '_')
                {
                    std::set<Position> ps(boxes.begin(), boxes.end());
                    auto pushedBox = *(ps.find(box));
                    ps.erase(pushedBox);
                    ps.insert(actual);
                    Position newPlayer = placePlayer(ps, actual + direction[i]);
                    eStates.push_back(State(newPlayer, ps, pushedBox.diff(actual).abs()));
                    actual += direction[i];
                }
            }
        }
    }
    return eStates;
}

void LevelGenerator::calculateSolution()
{
    m_solution.clear();
    Position player = m_bestSolution[0].getPlayer();
    std::set<Position> boxes;
    std::set<Position> next;
    std::set<Position>::iterator b;
    std::set<Position>::iterator n;
    char s[] = "RLDU";
    char s1[] = "rldu";

    for (size_t i = 0; i < m_bestSolution.size()-1; ++i)
    {
        Map temp = m_bestMap;

        boxes = m_bestSolution[i].getBoxes();
        next = m_bestSolution[i+1].getBoxes();

        for (const auto& b : boxes)
        {
            m_bestMap(b) = '$';
        }

        b = boxes.begin();
        n = next.begin();

        while (boxes.find(*n) != boxes.end())
        {
            n++;
        }
        while (next.find(*b) != next.end())
        {
            b++;
        }

        Position diff = (*b).diff(*n);
        Position norm = diff.normal();
        Position pNext = *b;
        pNext += norm.inv();

        if (!(pNext == player))
        {
            std::string str;
            std::deque<std::pair<Position, std::string>> d;

            d.push_back(make_pair(player, std::string()));

            while (!d.empty())
            {
                std::pair<Position, std::string> p = d.front();
                d.pop_front();

                if (p.first == pNext) 
                {
                    str = p.second;
                    break;
                }

                for (size_t i = 0; i < 4; ++i)
                {
                    if (p.first.isInInterval(Position(0,0),Position(width,height),direction[i])
                        && m_bestMap(p.first + direction[i]) == ' ')
                    {
                        m_bestMap(p.first + direction[i]) = '_';
                        d.push_back(make_pair(p.first + direction[i],p.second + s1[i]));
                    }
                }
            }
            m_solution += str;
            player = pNext;
        }

        for(size_t i = 0; i < 4; ++i)
        {
            if(norm == direction[i])
            {
                for(size_t j = 0; j < diff.abs(); ++j)
                {
                    m_solution += s[i];
                }
                player += diff;
                break;
            }
        }

        m_bestMap = temp;
    }
}

void LevelGenerator::placeGoals()
{
    size_t count;
    bool success = false;
    Position p;
    std::set<Position> pos;

    goals.clear();

    while (!success)
    {
        p = Position(Random(0,width),Random(0,height));

        count = 0;
        pos.clear();

        while (m_map(p) == ' '
            && pos.find(p) == pos.end())
        {
            count++;
            pos.insert(p);
            p += direction[Random(0,4)];
            if (!p.isInInterval(Position(0,0),Position(width,height),Position(0,0)))
            {
                break;
            }
            if (count == numBoxes)
            {
                success = true;
                break;
            }
        }
    }

    goals = pos;
}

void LevelGenerator::placeBest()
{
    Position player = m_best.getPlayer();

    std::set<Position> boxes = m_best.getBoxes();

    for(const auto& goal : m_bestGoals)
    {
        m_bestMap(goal) = '.';
    }

    for(const auto& box : boxes)
    {
        m_bestMap(box) = (m_bestGoals.find(box) == m_bestGoals.end()) ? '$' : '*';
    }

    m_bestMap(player) = (m_bestGoals.find(player) == m_bestGoals.end()) ? '@' : '+';

    m_map = m_bestMap;
}

void LevelGenerator::printBest()
{
    for(int i = m_bestSolution.size()-1; i >= 0; --i)
    {
        Map temp = m_bestMap;

        Position player = m_bestSolution[i].getPlayer();
        std::set<Position> boxes = m_bestSolution[i].getBoxes();

        for(const auto& goal : m_bestGoals)
        {
            m_bestMap(goal) = '.';
        }

        for(const auto& box : boxes)
        {
            m_bestMap(box) = (m_bestGoals.find(box) == m_bestGoals.end()) ? '$' : '*';
        }

        m_bestMap(player) = (m_bestGoals.find(player) == m_bestGoals.end()) ? '@' : '+';

        std::cout << m_bestMap;
        m_bestMap = temp;
    }
}

void LevelGenerator::printMap() const
{
    std::cout << m_map;
}

size_t LevelGenerator::getMax() const
{
    return m_max;
}

std::string LevelGenerator::getSolution() const
{
    return m_solution;
}

const Position LevelGenerator::direction[] = { Position(1,0), Position(-1,0), Position(0,1), Position(0,-1) };
