// Goes in MyBot.cpp
#include <fstream>
#include <stdlib.h>
#include <time.h>
#include <map>
#include <unordered_map>
#include <queue>
#include <cassert>

#include "hlt.hpp"
#include "networking.hpp"

std::vector<std::string> DIRECTIONSTR={"STILL", "NORTH", "EAST", "SOUTH", "WEST"};
std::string BOTNAME = "C++Bot18o";
unsigned char DIST = 9;
unsigned char WAIT = 5;

bool isNeutualEnemyBorder(hlt::GameMap &m_map, const hlt::Location &loc, unsigned char myID)
{
    hlt::Site c = m_map.getSite(loc);
    if (c.owner != 0 || c.strength == 0) return false;
    bool enemy = false, mySite = false;
    for (auto d : CARDINALS)
    {
        hlt::Site nb = m_map.getSite(loc, d);
        if (nb.owner == myID)
        {
            mySite = true;
        }else if (nb.owner != 0)
        {
            enemy = true;
        }
    }
    return mySite && enemy;
}

bool isEnemyBorder(hlt::GameMap &m_map, const hlt::Location &loc, unsigned char myID)
{
    hlt::Site c = m_map.getSite(loc);
    if (c.owner != 0 || c.strength != 0) return false;
    bool enemy = false, mySite = false;
    for (auto d : CARDINALS)
    {
        hlt::Site nb = m_map.getSite(loc, d);
        if (nb.owner == myID)
        {
            mySite = true;
        }else if (nb.owner != 0)
        {
            enemy = true;
        }
    }
    return mySite;
}

bool hasAccessToEnemy(hlt::GameMap &m_map, const hlt::Location &loc, unsigned char myID, unsigned char maxdist = 4)
{
    unsigned char dist = 1;
    std::queue<hlt::Location> border;
    border.push(loc);
    hlt::Location MARKER{m_map.width, m_map.width};
    border.push(MARKER);
    std::unordered_map<unsigned short, unsigned char> visit;
    while(!border.empty() && dist < maxdist)
    {
        hlt::Location l = border.front();
        unsigned short lkey = l.x + l.y * m_map.width;
        border.pop();
        if (l.x == m_map.width)
        {
            ++dist;
            if (!border.empty()) border.push(l);
        }else
        {
            for (auto d : CARDINALS)
            {
                hlt::Location nl = m_map.getLocation(l, d);
                hlt::Site ns = m_map.getSite(nl);
                unsigned short nlkey = nl.x + nl.y * m_map.width; 
                if (ns.owner == myID && (visit.find(nlkey) == visit.end()))
                {
                    border.push(nl);
                    visit[nlkey] = 1;
                }else if (ns.owner == 0 && ns.strength == 0)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

unsigned char reverseDirection(unsigned char direction)
{
    if (direction == 0) return 0;
    else if (direction < 5)
    {
        return (direction + 1) % 4 + 1;
    }else
    {
        return direction + 1;
    }
}

std::string directionString(unsigned char d)
{    
    static std::vector<std::string> direction{"STILL", "NORTH", "EAST", "SOUTH", "WEST"};
    return direction[d];	
}

bool isBoard(hlt::GameMap & m_map, const hlt::Location& loc, unsigned char m_playerID)
{
    for (auto d: CARDINALS)
    {
    	if (m_map.getSite(loc, d).owner != m_playerID) return true;
    }
    return false;
}

unsigned short distancePenalty(unsigned short distance)
{
    return 5 * distance;
}
unsigned short valueSite(const hlt::Site & site)
{
    unsigned char strength = (site.owner == 0 ? site.strength : 0);
    if (site.production == 0 && strength != 0) return 1;
    return 257 + site.production - strength / (std::max)(site.production, (unsigned char)(1));
}

unsigned short valueSite(const hlt::Location & loc, hlt::GameMap & presentMap, const unsigned char myID)
{
    if(isNeutualEnemyBorder(presentMap, loc, myID) && !hasAccessToEnemy(presentMap, loc, myID)) return 1; 
    
    unsigned short totalVal = 0;
    hlt::Site site = presentMap.getSite(loc);
    if (site.owner != myID) totalVal += valueSite(site);
    bool hasEnemy = false;
    unsigned short totalProd = site.production;//give high priority to area with high production
    for (auto d: CARDINALS)
    {
    	hlt::Site sd = presentMap.getSite(loc, d);
    	if (sd.owner != myID)
    	{
    		unsigned short sdVal = valueSite(sd);
    		totalVal += (sdVal > 250 ? sdVal - 250 : 0);
                if (sd.owner != 0) 
                {
                    hasEnemy = true;
                    totalVal += 10;
                }
    	}
        totalProd += sd.production;
    }
    if (hasEnemy)
    {
        //totalVal += 5 * totalProd;
    }
    return totalVal;
}

unsigned short valueBordMove(hlt::GameMap & presentMap, const hlt::Location & loc, unsigned char direction)
{
    hlt::Location newLoc = presentMap.getLocation(loc, direction);
    hlt::Site s = presentMap.getSite(newLoc);
    hlt::Site c = presentMap.getSite(loc);
    if (s.owner != c.owner)
    {
    	// production formula
    	unsigned short damage = valueSite(newLoc, presentMap, c.owner);
    	if (damage == 1) return 1;
        //if (s.production == 0) damage -= 10;
    	// wait penalty
        if (s.strength >= c.strength)
    	{
    		unsigned short penalty = (s.strength - c.strength) / (std::max)(s.production, (unsigned char)(1));
    		if (damage > penalty) damage -= penalty;
    		else return 0;
    	}
    	// over kill damage
        unsigned short overkill = 0;
    	bool hasEnemy = false;
        for (int d : CARDINALS)
    	{
    		hlt::Site sd = presentMap.getSite(newLoc, d);
    		if (sd.owner != 0 && sd.owner != c.owner)
    		{
    			overkill += std::min(c.strength, sd.strength);
                        hasEnemy = true;
    		}
    	}
        if (hasEnemy && overkill <= c.strength && s.strength > 0)
        {
            damage  = 0;
        }else
        {
            damage += overkill;
        }
    	return damage;
    }else
    return 0;
}

unsigned short maxBordMove(hlt::GameMap & presentMap, const hlt::Location & loc)
{
    unsigned short maxVal = 0;
    for (int d : CARDINALS)
    {
    	unsigned short val = valueBordMove(presentMap, loc, d);
    	if (val > maxVal)
    	{
    		maxVal = val;
    	}
    }
    return maxVal;
}

unsigned short valueInnerMove (hlt::GameMap & presentMap, const hlt::Location & loc, unsigned char direction)
{
    if (presentMap.getSite(loc).strength <= presentMap.getSite(loc).production * 4) return 0;
    bool hasEnemy = false;
    for (auto d : CARDINALS)
    {
        hlt::Site s = presentMap.getSite(loc, d);
        if (s.owner == 0 && s.strength == 0) hasEnemy = true;
    }
    if (hasEnemy) return 0;
    
    hlt::Location newLoc = presentMap.getLocation(loc, direction);
    unsigned short val = maxBordMove(presentMap, newLoc);
    unsigned short sum = presentMap.getSite(loc).strength + presentMap.getSite(newLoc).strength;
    /*if (sum > 255)
    {
    	if (val + 255 > sum)
    	{
    		val -= (sum - 255);
    	}else
    	{
    		return 0;
    	}
    }*/
   //
    unsigned char myID = presentMap.getSite(loc).owner;
    unsigned char distance = 0;
    unsigned short maxDistance = (std::min)(presentMap.width, presentMap.height) / 2;
    while(presentMap.getSite(newLoc).owner == myID && distance < maxDistance)
    {
    	newLoc = presentMap.getLocation(newLoc, direction);
        ++distance;
    }
    unsigned short valueBoard = valueSite(newLoc, presentMap, myID);
    unsigned short distPenalty = distancePenalty(distance);
    if (valueBoard > distPenalty)
    {
        valueBoard -= distPenalty;
    }else
    {
        valueBoard = 0;
    }
    if (val > distancePenalty(1))//try to mimic bot16
    {
        val -= distancePenalty(1);
    }else
    {
        val = 0;
    }
    if (hasEnemy) return valueBoard; 
    else return std::max(val, valueBoard);
}

unsigned short valueMove(hlt::GameMap & presentMap, const hlt::Location & loc, unsigned char direction)
{
    hlt::Location newLoc = presentMap.getLocation(loc, direction);
    if (presentMap.getSite(loc).owner != presentMap.getSite(newLoc).owner)
    {
    	return valueBordMove(presentMap, loc, direction);
    }else
    {
    	return valueInnerMove(presentMap, loc, direction);
    }
}

unsigned char boarderOptimize(hlt::GameMap & presentMap, const hlt::Location & loc)
{
    hlt::Site s = presentMap.getSite(loc);
    unsigned short valueMax = ((s.strength + s.production > 200 )? 0 : (1 + s.production * 10));
    unsigned char direction = STILL;
    for (int d : CARDINALS)
    {
    	unsigned short value = valueMove(presentMap, loc, d);
    	if (value > valueMax)
    	{
    		direction = d;
    		valueMax = value;
    	}
    }
    return direction;
}

unsigned char findNearestEnemyDirection(hlt::GameMap & presentMap, const hlt::Location & loc, const unsigned char myID, unsigned short stage){
    unsigned short maxDistance = (std::min)(presentMap.width, presentMap.height) / 2;
    hlt::Site currentSite = presentMap.getSite(loc);
    unsigned short maxVal = (255 - currentSite.strength) / std::max(currentSite.production, (unsigned char)(1)) / 2 + (6 + 4 * stage) * maxDistance;
    unsigned char direction = STILL;
    for (int d : CARDINALS){
        unsigned short distance = 0;
        unsigned short val = 0;
        hlt::Location current = loc;
        hlt::Site nextSite = presentMap.getSite(loc, d);
        while(nextSite.owner == myID && distance < maxDistance){
            ++distance;
            current = presentMap.getLocation(current, d);
            nextSite = presentMap.getSite(current);
        }
        if (nextSite.owner != myID)
    	{
    		//unsigned short valBoard = avgBordMove(presentMap, current);
    		unsigned short valBoard = valueSite(current, presentMap, presentMap.getSite(loc).owner);
    		val = valBoard + (6 + 4 * stage) * (maxDistance - distance);
    		hlt::Site s = presentMap.getSite(loc);
    	}
        if (val > maxVal){
            direction = d;
            maxVal = val;
        }
    }
    return direction;
}

int main() {
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);
    sendInit(BOTNAME);

    std::set<hlt::Move> moves;
    unsigned int i = 0;
    unsigned int halfMap = presentMap.width * presentMap.height / 2;
    unsigned short stage = 0;
    
    std::vector<std::vector<unsigned char>> direction_map(presentMap.height, std::vector<unsigned char>(presentMap.width, 5));
    std::vector<std::vector<unsigned short>> strength_map(presentMap.height, std::vector<unsigned short>(presentMap.width, 0));
    while(true) {
        moves.clear();
        getFrame(presentMap);
        
        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                direction_map[a][b] = ((direction_map[a][b] << 4) | 5);
                strength_map[a][b] = 0;
            }
        }
        
    	unsigned int unclaimed = 0;
        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                if (presentMap.getSite({ b, a }).owner == myID) {
                    bool border = false;
                    bool movedPiece = false;
                    unsigned char dir = STILL;
                    for(int d : CARDINALS) {
                        if(presentMap.getSite({ b, a }, d).owner != myID){
                            border = true;
                        }
                    }
                    if (border) 
                    {
                        unsigned char direction= boarderOptimize(presentMap, {b, a});
                        if (presentMap.getSite({b, a}, direction).strength >= presentMap.getSite({b, a}).strength && presentMap.getSite({b, a}, direction).owner != myID)
                        {
                       	    dir = STILL;
    			}else
    			{
        		    dir = direction;
    			}
                        //dir = direction;
                        movedPiece = true;
                    }

                    if(!movedPiece && (presentMap.getSite({ b, a }).strength <= presentMap.getSite({ b, a }).production * WAIT)) {
                        dir = STILL;
                        movedPiece = true;
                    }

                    if(!movedPiece && !border) {
                        unsigned char direction = findNearestEnemyDirection(presentMap, {b, a}, myID, stage);
                        dir = direction;
                        movedPiece = true;
                    }
                    direction_map[a][b] = ((direction_map[a][b] | 0x0f) & (0xf0 + dir));//&= (0xf0 + dir);
                    hlt::Location newLoc = presentMap.getLocation({b, a}, dir);
                    strength_map[newLoc.y][newLoc.x] += presentMap.getSite({b, a}).strength; 
                }
                else if (presentMap.getSite({ b, a }).owner == 0)
                {
                    ++unclaimed;
    	    	}	
            }
        }
        
    hlt::GameMap & m_map = presentMap;
    unsigned char m_playerID = myID;
    
    std::unordered_map<unsigned short, std::pair<unsigned char, unsigned char>> visit;
    hlt::Location MARKER{m_map.width, m_map.height};
    for (unsigned short y = 0; y < m_map.height; ++y)
    {
    	for (unsigned short x = 0; x < m_map.width; ++x)
    	{
    	    hlt::Location loc{x, y};
    	    if (isEnemyBorder(m_map, loc, myID))
    	    {
                unsigned char dist = 1;
                std::queue<hlt::Location> border;
                border.push(loc);
                border.push(MARKER);
                while(!border.empty() && dist < DIST)
                {
                    hlt::Location l = border.front();
                    unsigned short lkey = l.x + l.y * m_map.width;
                    border.pop();
                    if (l.x == m_map.width)
                    {
                        ++dist;
                        if (!border.empty()) border.push(l);
                    }else
                    {
                        for (auto d : CARDINALS)
                        {
                            hlt::Location nl = m_map.getLocation(l, d);
                            hlt::Site ns = m_map.getSite(nl);
                            unsigned short nlkey = nl.x + nl.y * m_map.width; 
                            if ( ns.owner == myID && (visit.find(nlkey) == visit.end() || visit[nlkey].first > dist))
                            {
                                border.push(nl);
                                visit[nlkey] = std::pair<unsigned char, unsigned char>(dist, reverseDirection(d));
                            }
                        }
                    }
                }
    	    }
    	}   
    }
    
    for (unsigned short y = 0; y < m_map.height; ++y)
    {
    	for (unsigned short x = 0; x < m_map.width; ++x)
    	{
    	    hlt::Location loc{x, y};
            unsigned short lockey = loc.x + loc.y * m_map.width;
            hlt::Site c = m_map.getSite(loc);
    	    if (c.owner == myID && visit.find(lockey) != visit.end())
    	    {
                unsigned char oldDir = direction_map[loc.y][loc.x] & 0x0f;
                unsigned short maxDistance = (std::min)(presentMap.width, presentMap.height) / 2;
                bool stayStill = (oldDir == STILL && c.strength > 200 && !isBoard(m_map, loc, m_playerID));
                bool goTooFar = false;
                if (oldDir != STILL && oldDir < 5)
                {
                    unsigned short maxDistance = (std::min)(presentMap.width, presentMap.height) / 2;
                    hlt::Location nl = m_map.getLocation(loc, oldDir);
                    hlt::Site s = m_map.getSite(nl);
                    unsigned short dist = 1;
                    while (s.owner == myID && dist < maxDistance)
                    {
                        nl = m_map.getLocation(nl, oldDir);
                        s = m_map.getSite(nl);
                        ++dist;
                    }
                    if (s.owner == m_playerID || dist > visit[lockey].first) goTooFar = true;
                }
                if (visit[lockey].first > 2 && (stayStill || goTooFar) && oldDir != visit[lockey].second)
                {
                    direction_map[loc.y][loc.x] =  ((direction_map[loc.y][loc.x] | 0x0f) & (0xf0 + visit[lockey].second));
                    hlt::Location oldLoc = m_map.getLocation(loc, oldDir);
                    hlt::Location newLoc = m_map.getLocation(loc, visit[lockey].second);
                    strength_map[newLoc.y][newLoc.x] += m_map.getSite(loc).strength;
                    strength_map[oldLoc.y][oldLoc.x] -= m_map.getSite(loc).strength;
                }
    	    }
    	}   
    }

    for (unsigned short y = 0; y < m_map.height; ++y)
    {
    	for (unsigned short x = 0; x < m_map.width; ++x)
    	{
    		hlt::Location loc{x, y};
    		if (m_map.getSite(loc).owner == m_playerID || strength_map[y][x] > 256)
    		{
    			unsigned char direction = (direction_map[y][x] & 0x0f);
                        std::vector<std::vector<unsigned char>> dirs;
                        for (auto d : DIRECTIONS)
    			{
 			    hlt::Location newLoc = m_map.getLocation(loc, d);
    			    if (m_map.getSite(newLoc).owner == m_playerID && (direction_map[newLoc.y][newLoc.x] & 0x0f) == reverseDirection(d))
                            {
                                dirs.push_back(std::vector<unsigned char>{static_cast<unsigned char>(d), m_map.getSite(newLoc).strength});
                            }
    			}
                        std::sort(dirs.begin(), dirs.end(), [](const std::vector<unsigned char> & v1, const std::vector<unsigned char> & v2){return v1[1] < v2[1];});

    			hlt::Location newLoc = m_map.getLocation(loc, direction);
    			hlt::Site newSite = m_map.getSite(newLoc);
    			hlt::Location revLoc = m_map.getLocation(loc, reverseDirection(direction));
    			hlt::Site revSite = m_map.getSite(revLoc);
    			
    			//if (newSite.owner == m_playerID && direction != STILL  )
    			if ((newSite.owner != 0 || newSite.strength == 0) && direction != STILL && direction < 5)
    			{
    			    if ((strength_map[y][x] + m_map.getSite(loc).strength < 256) && ((dirs.size() > 0 && (strength_map[y][x] + 150 > m_map.getSite(loc).strength)) || ((direction_map[newLoc.y][newLoc.x] >> 4) == reverseDirection(direction) && newSite.owner == myID)))//stay still if other bots is comming to this site or it is returning to previous location
    			    {
                                direction_map[y][x] = ((direction_map[y][x] | 0x0f) & (0xf0 + STILL));//&= (0xf0 + STILL);
    			        strength_map[y][x] += m_map.getSite(loc).strength;
                                strength_map[newLoc.y][newLoc.x] -= m_map.getSite(loc).strength;
                            }else if (((direction_map[revLoc.y][revLoc.x] & 0x0f)  == direction) && (revSite.strength > m_map.getSite(loc).strength) 
    							&& (strength_map[revLoc.y][revLoc.x] + m_map.getSite(loc).strength < 256) && (revSite.strength - m_map.getSite(loc).strength > 40))
    			    {
    		    		direction_map[y][x] = ((direction_map[y][x] | 0x0f) & (0xf0 + reverseDirection(direction)));
    			    	strength_map[revLoc.y][revLoc.x] += m_map.getSite(loc).strength;
    		    		strength_map[newLoc.y][newLoc.x] -= m_map.getSite(loc).strength;
                            }
    			}
    			
    			
                        for (auto dir: dirs)//soft reverse
    			{
                            hlt::Location newLoc = m_map.getLocation(loc, dir[0]);
                            if (strength_map[y][x] > 256)
                            {
                                if(dir[0] != STILL && strength_map[newLoc.y][newLoc.x] + dir[1] <= 256)
  				{
                                    direction_map[newLoc.y][newLoc.x] = ((direction_map[newLoc.y][newLoc.x] | 0x0f) & (0xf0 + STILL));
    				    strength_map[newLoc.y][newLoc.x] += dir[1];
	    			    strength_map[y][x] -= dir[1];
  			    	}else if (dir[0] == STILL)
  				{
  				    unsigned short strength = 255;
  		    	            unsigned char d = STILL;
  		        	    for (auto dd : CARDINALS)
  				    {
  		    			hlt::Location l = m_map.getLocation(loc, dd);
    					if (m_map.getSite(l).owner == m_playerID && (strength_map[l.y][l.x] < strength || ((strength_map[l.y][l.x] == strength) && (direction_map[l.y][l.x] & 0x0f) == reverseDirection(dd))))
					{
  					    d = dd;
                                            strength = strength_map[l.y][l.x];
  			    		}
  				    }
  			    	    hlt::Location l = m_map.getLocation(loc, d);
  				    if (strength_map[l.y][l.x] + dir[1] < 256 && dir[1] > 0)
  				    {
  				        direction_map[y][x] = ((direction_map[y][x] | 0x0f) & (0xf0 + d));//&= (0xf0 + d);
  	    				strength_map[l.y][l.x] += dir[1];
                                        strength_map[y][x] -= dir[1];
  				    }
  				}
    			    }
    			}

                        direction = (direction_map[y][x] & 0x0f);
                        newLoc = m_map.getLocation(loc, direction);
                        newSite = m_map.getSite(newLoc);
                        
    			hlt::Location nextNextLoc = m_map.getLocation(newLoc, direction);
    			hlt::Site nextNextSite = m_map.getSite(nextNextLoc);
                        hlt::Site nNNSite = m_map.getSite(nextNextLoc, direction);
                        bool enemyAfterWall = (direction != STILL && direction < 5 && newSite.owner == 0 && newSite.strength > 0 
                                && nextNextSite.owner != 0 && nextNextSite.owner != m_playerID 
                                && nextNextSite.strength > m_map.getSite(loc).strength); // don't go to neutual terrority if next tile belongs to enemy and has larger strength than my tile

                        bool hasEnemy = false;
                        if (direction != 0 && direction < 5 && newSite.owner == myID && newSite.strength > 0)
                        {
                            unsigned char d = direction_map[newLoc.y][newLoc.x] & 0x0f;
                            hlt::Site nnSite = m_map.getSite(newLoc, d);
                            if (nnSite.owner == 0 && nnSite.strength == 0)
                            {
                                hlt::Location ll = m_map.getLocation(newLoc, d);
                                for (auto dd : CARDINALS)
                                {
                                    hlt::Site nnnSite = m_map.getSite(ll, dd);
                                    if (nnnSite.owner != 0 && nnnSite.owner != m_playerID && nnnSite.strength > 0)
                                        hasEnemy = true;
                                }
                            }
                        }
                        bool secondToEnemy = (hasEnemy && m_map.getSite(loc).strength + m_map.getSite(newLoc).strength > 256 
                                && m_map.getSite(loc).strength < m_map.getSite(newLoc).strength + 40); // leave a gap if this is the second first bot to enemy
                        
                        if (/*enemyAfterWall ||*/ secondToEnemy) // set direction to STILL if second position to enemy
                        {
                                direction_map[y][x] = ((direction_map[y][x] | 0x0f) & (0xf0 + STILL));
    			        strength_map[y][x] += m_map.getSite(loc).strength;
                                strength_map[newLoc.y][newLoc.x] -= m_map.getSite(loc).strength;
                        }

                        std::queue<hlt::Location> overCap; //hard reverse, if there is an overcapping, set all still
                        if (strength_map[loc.y][loc.x] > 256) overCap.push(loc);
                        while(!overCap.empty())
                        {
                            hlt::Location l = overCap.front();
                            overCap.pop();
                            std::vector<std::vector<unsigned char>> dirs;
                            
                            if (strength_map[l.y][l.x] > 256)
                            {
                                for (auto d : CARDINALS)
    		                {
 		                    hlt::Location newLoc = m_map.getLocation(l, d);
    		                    if (m_map.getSite(newLoc).owner == m_playerID && (direction_map[newLoc.y][newLoc.x] & 0x0f) == reverseDirection(d))
                                    {
                                        dirs.push_back(std::vector<unsigned char>{static_cast<unsigned char>(d), m_map.getSite(newLoc).strength});
                                    }
    		                }
                                std::sort(dirs.begin(), dirs.end(), [](const std::vector<unsigned char> & v1, const std::vector<unsigned char> & v2){return v1[1] < v2[1];});
                            }

                            for (auto dir : dirs)
                            {
                                unsigned char d = dir[0];
                                hlt::Location nl = m_map.getLocation(l, d);
                                if ((direction_map[nl.y][nl.x] & 0x0f) == reverseDirection(d) && strength_map[l.y][l.x] > 256)
                                {
                                    direction_map[nl.y][nl.x] = ((direction_map[nl.y][nl.x] | 0x0f) & (0xf0 + STILL));
                                    strength_map[l.y][l.x] -= m_map.getSite(nl).strength;
                                    strength_map[nl.y][nl.x] += m_map.getSite(nl).strength;
                                    if (strength_map[nl.y][nl.x] > 256) overCap.push(nl);
                                }
                            }
                        }

    		}
    	}
    }
        
        for (unsigned short y = 0; y < m_map.height; ++y)
    {
    	for (unsigned short x = 0; x < m_map.width; ++x)
    	{
    		hlt::Location loc{x, y};
    		if (m_map.getSite(loc).owner == m_playerID)
    		{
    			unsigned char direction = (direction_map[y][x] & 0x0f);
    			moves.insert({loc, direction});
    		}
    	}
    }
        
        stage = unclaimed / halfMap;
        if (stage == 0) WAIT = 9;
        sendFrame(moves);
    }
    return 0;
}
