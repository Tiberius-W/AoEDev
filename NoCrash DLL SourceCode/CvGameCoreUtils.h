#pragma once

// utils.h

#ifndef CIV4_GAMECORE_UTILS_H
#define CIV4_GAMECORE_UTILS_H


//#include "CvStructs.h"
#include "CvGlobals.h"
#include "CvMap.h"

#ifndef _USRDLL
// use non inline functions when not in the dll
#define getMapINLINE	getMap
#define getGridHeightINLINE	getGridHeight
#define getGridWidthINLINE	getGridWidth
#define isWrapYINLINE	isWrapY
#define isWrapXINLINE	isWrapX
#define plotINLINE	plot
#define getX_INLINE	getX
#define getY_INLINE	getY

#endif

class CvPlot;
class CvCity;
class CvUnit;
class CvString;
class CvRandom;
class FAStarNode;
class FAStar;
class CvInfoBase;


#ifndef SQR
#define SQR(x) ( (x)*(x))
#endif

#undef max
#undef min

//sign function taken from FirePlace - JW
template<class T> __forceinline T getSign( T x ) { return (( x < 0 ) ? T(-1) : x > 0 ? T(1) : T(0)); };

inline int range(int iNum, int iLow, int iHigh)
{
	FAssertMsg(iHigh >= iLow, "High should be higher than low");

	if (iNum < iLow)
	{
		return iLow;
	}
	else if (iNum > iHigh)
	{
		return iHigh;
	}
	else
	{
		return iNum;
	}
}

inline float range(float fNum, float fLow, float fHigh)
{
	FAssertMsg(fHigh >= fLow, "High should be higher than low");

	if (fNum < fLow)
	{
		return fLow;
	}
	else if (fNum > fHigh)
	{
		return fHigh;
	}
	else
	{
		return fNum;
	}
}

inline int coordDistance(int iFrom, int iTo, int iRange, bool bWrap)
{
	if (bWrap && (abs(iFrom - iTo) > (iRange / 2)))
	{
		return (iRange - abs(iFrom - iTo));
	}

	return abs(iFrom - iTo);
}

inline int wrapCoordDifference(int iDiff, int iRange, bool bWrap)
{
	if (bWrap)
	{
		if (iDiff > (iRange / 2))
		{
			return (iDiff - iRange);
		}
		else if (iDiff < -(iRange / 2))
		{
			return (iDiff + iRange);
		}
	}

	return iDiff;
}

inline int xDistance(int iFromX, int iToX)
{
	return coordDistance(iFromX, iToX, GC.getMapINLINE().getGridWidthINLINE(), GC.getMapINLINE().isWrapXINLINE());
}

inline int yDistance(int iFromY, int iToY)
{
	return coordDistance(iFromY, iToY, GC.getMapINLINE().getGridHeightINLINE(), GC.getMapINLINE().isWrapYINLINE());
}

inline int dxWrap(int iDX)																													// Exposed to Python
{
	return wrapCoordDifference(iDX, GC.getMapINLINE().getGridWidthINLINE(), GC.getMapINLINE().isWrapXINLINE());
}

inline int dyWrap(int iDY)																													// Exposed to Python
{
	return wrapCoordDifference(iDY, GC.getMapINLINE().getGridHeightINLINE(), GC.getMapINLINE().isWrapYINLINE());
}

// 4 | 4 | 3 | 3 | 3 | 4 | 4
// -------------------------
// 4 | 3 | 2 | 2 | 2 | 3 | 4
// -------------------------
// 3 | 2 | 1 | 1 | 1 | 2 | 3
// -------------------------
// 3 | 2 | 1 | 0 | 1 | 2 | 3
// -------------------------
// 3 | 2 | 1 | 1 | 1 | 2 | 3
// -------------------------
// 4 | 3 | 2 | 2 | 2 | 3 | 4
// -------------------------
// 4 | 4 | 3 | 3 | 3 | 4 | 4
//
// Returns the distance between plots according to the pattern above...
inline int plotDistance(int iX1, int iY1, int iX2, int iY2)													// Exposed to Python
{
	int iDX;
	int iDY;

	iDX = xDistance(iX1, iX2);
	iDY = yDistance(iY1, iY2);

	return (std::max(iDX, iDY) + (std::min(iDX, iDY) / 2));
}

// 4 | 4 | 3 | 3 | 3 | 4 | 4
// -------------------------
// 4 | 3 | 2 | 2 | 2 | 3 | 4
// -------------------------
// 3 | 2 | 1 | 1 | 1 | 2 | 3
// -------------------------
// 3 | 2 | 1 | 0 | 1 | 2 | 3
// -------------------------
// 3 | 2 | 1 | 1 | 1 | 2 | 3
// -------------------------
// 4 | 3 | 2 | 2 | 2 | 3 | 4
// -------------------------
// 4 | 4 | 3 | 3 | 3 | 4 | 4
//
// Overload for using two plots, returning distance according to the pattern above...
inline int plotDistance(const CvPlot* pPlot1, const CvPlot* pPlot2)
{
	return (plotDistance(pPlot1->getX_INLINE(), pPlot1->getY_INLINE(), pPlot2->getX_INLINE(), pPlot2->getY_INLINE()));
}

// 3 | 3 | 3 | 3 | 3 | 3 | 3
// -------------------------
// 3 | 2 | 2 | 2 | 2 | 2 | 3
// -------------------------
// 3 | 2 | 1 | 1 | 1 | 2 | 3
// -------------------------
// 3 | 2 | 1 | 0 | 1 | 2 | 3
// -------------------------
// 3 | 2 | 1 | 1 | 1 | 2 | 3
// -------------------------
// 3 | 2 | 2 | 2 | 2 | 2 | 3
// -------------------------
// 3 | 3 | 3 | 3 | 3 | 3 | 3
//
// Returns the distance between plots according to the pattern above...
inline int stepDistance(int iX1, int iY1, int iX2, int iY2)													// Exposed to Python
{
	return std::max(xDistance(iX1, iX2), yDistance(iY1, iY2));
}

inline CvPlot* plotDirection(int iX, int iY, DirectionTypes eDirection)							// Exposed to Python
{
	if(eDirection == NO_DIRECTION)
	{
		return GC.getMapINLINE().plotINLINE(iX, iY);
	}
	else
	{
		return GC.getMapINLINE().plotINLINE((iX + GC.getPlotDirectionX()[eDirection]), (iY + GC.getPlotDirectionY()[eDirection]));
	}
}

inline CvPlot* plotCardinalDirection(int iX, int iY, CardinalDirectionTypes eCardinalDirection)	// Exposed to Python
{
	return GC.getMapINLINE().plotINLINE((iX + GC.getPlotCardinalDirectionX()[eCardinalDirection]), (iY + GC.getPlotCardinalDirectionY()[eCardinalDirection]));
}

inline CvPlot* plotXY(int iX, int iY, int iDX, int iDY)																// Exposed to Python
{
	return GC.getMapINLINE().plotINLINE((iX + iDX), (iY + iDY));
}

inline DirectionTypes directionXY(int iDX, int iDY)																		// Exposed to Python
{
	if ((abs(iDX) > DIRECTION_RADIUS) || (abs(iDY) > DIRECTION_RADIUS))
	{
		return NO_DIRECTION;
	}
	else
	{
		return GC.getXYDirection((iDX + DIRECTION_RADIUS), (iDY + DIRECTION_RADIUS));
	}
}

inline DirectionTypes directionXY(const CvPlot* pFromPlot, const CvPlot* pToPlot)			// Exposed to Python
{
	return directionXY(dxWrap(pToPlot->getX_INLINE() - pFromPlot->getX_INLINE()), dyWrap(pToPlot->getY_INLINE() - pFromPlot->getY_INLINE()));
}

CvPlot* plotCity(int iX, int iY, int iIndex);																			// Exposed to Python
int plotCityXY(int iDX, int iDY);																									// Exposed to Python
int plotCityXY(const CvCity* pCity, const CvPlot* pPlot);													// Exposed to Python

//FfH: Added by Kael 12/03/2007
int plotCityXY(int iDX, int iDY, int iRadius);
//FfH: End Add

CardinalDirectionTypes getOppositeCardinalDirection(CardinalDirectionTypes eDir);	// Exposed to Python
DirectionTypes cardinalDirectionToDirection(CardinalDirectionTypes eCard);				// Exposed to Python
DllExport bool isCardinalDirection(DirectionTypes eDirection);															// Exposed to Python
DllExport DirectionTypes estimateDirection(int iDX, int iDY);																// Exposed to Python
DllExport DirectionTypes estimateDirection(const CvPlot* pFromPlot, const CvPlot* pToPlot);
DllExport float directionAngle(DirectionTypes eDirection);

DllExport bool atWar(TeamTypes eTeamA, TeamTypes eTeamB);												// Exposed to Python
bool isPotentialEnemy(TeamTypes eOurTeam, TeamTypes eTheirTeam);			// Exposed to Python

DllExport CvCity* getCity(IDInfo city);	// Exposed to Python
DllExport CvUnit* getUnit(IDInfo unit);	// Exposed to Python

bool isBeforeUnitCycle(const CvUnit* pFirstUnit, const CvUnit* pSecondUnit);
/*************************************************************************************************/
/**	Xienwolf Tweak							02/01/09											**/
/**																								**/
/**						Tracker added to prevent lockups from infinite loop cases				**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
bool isPromotionValid(PromotionTypes ePromotion, UnitTypes eUnit, bool bLeader);	// Exposed to Python
/**								----  End Original Code  ----									**/
bool isPromotionValid(PromotionTypes ePromotion, UnitTypes eUnit, bool bLeader, int iCycle = 0);	// Exposed to Python
/*************************************************************************************************/
/**	Tweak									END													**/
/*************************************************************************************************/

int getPopulationAsset(int iPopulation);								// Exposed to Python
int getLandPlotsAsset(int iLandPlots);									// Exposed to Python
int getPopulationPower(int iPopulation);								// Exposed to Python
int getPopulationScore(int iPopulation);								// Exposed to Python
int getLandPlotsScore(int iLandPlots);									// Exposed to Python
int getTechScore(TechTypes eTech);											// Exposed to Python
int getWonderScore(BuildingClassTypes eWonderClass);		// Exposed to Python

/*************************************************************************************************/
/**	MyLand									04/04/09								Xienwolf	**/
/**																								**/
/**				Not every Civ can fully upgrade every improvement that they can build			**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
ImprovementTypes finalImprovementUpgrade(ImprovementTypes eImprovement, int iCount = 0);		// Exposed to Python
/**								----  End Original Code  ----									**/
ImprovementTypes finalImprovementUpgrade(ImprovementTypes eImprovement, CivilizationTypes eCiv = NO_CIVILIZATION, int iCount = 0, ImprovementTypes eOriginalImprovement = NO_IMPROVEMENT);
/*************************************************************************************************/
/**	MyLand									END													**/
/*************************************************************************************************/

int getWorldSizeMaxConscript(CivicTypes eCivic);								// Exposed to Python

bool isReligionTech(TechTypes eTech);														// Exposed to Python

bool isTechRequiredForUnit(TechTypes eTech, UnitTypes eUnit);							// Exposed to Python
bool isTechRequiredForBuilding(TechTypes eTech, BuildingTypes eBuilding);	// Exposed to Python
bool isTechRequiredForProject(TechTypes eTech, ProjectTypes eProject);		// Exposed to Python

bool isWorldUnitClass(UnitClassTypes eUnitClass);											// Exposed to Python
bool isTeamUnitClass(UnitClassTypes eUnitClass);											// Exposed to Python
bool isNationalUnitClass(UnitClassTypes eUnitClass);									// Exposed to Python
/*************************************************************************************************/
/**	WeightedCost							 1/30/2010								Valkrionn	**/
/**																								**/
/**						Modifies Cost based on Population and number of Cities					**/
/*************************************************************************************************/
bool isCountCitiesAsUnitClass(UnitClassTypes eUnitClass);
/*************************************************************************************************/
/**	WeightedCost								END												**/
/*************************************************************************************************/
bool isLimitedUnitClass(UnitClassTypes eUnitClass);										// Exposed to Python

bool isWorldWonderClass(BuildingClassTypes eBuildingClass);						// Exposed to Python
bool isTeamWonderClass(BuildingClassTypes eBuildingClass);						// Exposed to Python
bool isNationalWonderClass(BuildingClassTypes eBuildingClass);				// Exposed to Python
bool isLimitedWonderClass(BuildingClassTypes eBuildingClass);					// Exposed to Python
int limitedWonderClassLimit(BuildingClassTypes eBuildingClass);

bool isWorldProject(ProjectTypes eProject);														// Exposed to Python
bool isTeamProject(ProjectTypes eProject);														// Exposed to Python
bool isLimitedProject(ProjectTypes eProject);													// Exposed to Python

/*************************************************************************************************/
/**	Higher hitpoints				30/01/11				Imported from wiser orcs by Snarko	**/
/**						Makes higher values than 100 HP possible.								**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
__int64 getBinomialCoefficient(int iN, int iK);
/**								----  End Original Code  ----									**/
double getBinomialCoefficient(int iN, int iK); // Wiser Orcs - Combat accuracy
/*************************************************************************************************/
/**	Higher hitpoints						END													**/
/*************************************************************************************************/
int getCombatOdds(CvUnit* pAttacker, CvUnit* pDefender);							// Exposed to Python

int getEspionageModifier(TeamTypes eOurTeam, TeamTypes eTargetTeam);							// Exposed to Python

DllExport void setTradeItem(TradeData* pItem, TradeableItems eItemType = TRADE_ITEM_NONE, int iData = 0);

bool isPlotEventTrigger(EventTriggerTypes eTrigger);

TechTypes getDiscoveryTech(UnitTypes eUnit, PlayerTypes ePlayer);

void setListHelp(wchar* szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst);
void setListHelp(CvWString& szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst);
void setListHelp(CvWStringBuffer& szBuffer, const wchar* szStart, const wchar* szItem, const wchar* szSeparator, bool bFirst);

// PlotUnitFunc's...
bool PUF_isGroupHead( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isPlayer( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isTeam( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isCombatTeam(const CvUnit* pUnit, int iData1, int iData2);
bool PUF_isOtherPlayer( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isOtherTeam( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isEnemy( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isVisible( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isVisibleDebug( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_canSiege( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isPotentialEnemy( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_canDeclareWar( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_canDefend( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_cannotDefend( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_canDefendGroupHead( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_canDefendEnemy( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_canDefendPotentialEnemy( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_canAirAttack( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_canAirDefend( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isFighting( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isAnimal( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isMilitaryHappiness( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isInvestigate( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isCounterSpy( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isSpy( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isUnitType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isDomainType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isUnitAIType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isCityAIType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isNotCityAIType( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_isSelected( const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_makeInfoBarDirty(CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isNoMission(const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
bool PUF_isFiniteRange(const CvUnit* pUnit, int iData1 = -1, int iData2 = -1);
/*************************************************************************************************/
/**	TileTrack								01/12/09								Xienwolf	**/
/**																								**/
/**					Tracks Units/Promotions present on a tile for speed of access				**/
/*************************************************************************************************/
bool PUF_isUnitClass( const CvUnit* pUnit, int iData1, int iData2 = -1);
bool PUF_hasPromotion( const CvUnit* pUnit, int iData1, int iData2);
/*************************************************************************************************/
/**	TileTrack								END													**/
/*************************************************************************************************/
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/15/09                                jdog5000      */
/*                                                                                              */
/* General AI                                                                                   */
/************************************************************************************************/
bool PUF_isAvailableUnitAITypeGroupie(const CvUnit* pUnit, int iData1, int iData2);
bool PUF_isUnitAITypeGroupie(const CvUnit* pUnit, int iData1, int iData2);
bool PUF_isFiniteRangeAndNotJustProduced(const CvUnit* pUnit, int iData1, int iData2);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

// FAStarFunc...
int potentialIrrigation(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int checkFreshWater(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int changeIrrigated(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int pathDestValid(int iToX, int iToY, const void* pointer, FAStar* finder);
int pathHeuristic(int iFromX, int iFromY, int iToX, int iToY);
int pathCost(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
/*************************************************************************************************/
/**	K-mod merger								16/02/12								Snarko	**/
/**																								**/
/**					Merging in features of K-mod, most notably the pathfinder					**/
/*************************************************************************************************/
int pathValid_join(FAStarNode* parent, FAStarNode* node, CvSelectionGroup* pSelectionGroup, int iFlags); // K-Mod
int pathValid_source(FAStarNode* parent, CvSelectionGroup* pSelectionGroup, int iFlags); // K-Mod
/*************************************************************************************************/
/**	K-mod merger								END												**/
/*************************************************************************************************/
int pathValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int pathAdd(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int stepDestValid(int iToX, int iToY, const void* pointer, FAStar* finder);
int stepHeuristic(int iFromX, int iFromY, int iToX, int iToY);
int stepValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int stepCost(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int stepAdd(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD					11/30/08				jdog5000	*/
/* 																			*/
/* 																			*/
/********************************************************************************/
int teamStepValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
/********************************************************************************/
/* 	BETTER_BTS_AI_MOD						END								*/
/********************************************************************************/
int routeValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int borderValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int areaValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int joinArea(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int plotGroupValid(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);
int countPlotGroup(FAStarNode* parent, FAStarNode* node, int data, const void* pointer, FAStar* finder);

int baseYieldToSymbol(int iNumYieldTypes, int iYieldStack);

bool isPickableName(const TCHAR* szName);

DllExport int* shuffle(int iNum, CvRandom& rand);
void shuffleArray(int* piShuffle, int iNum, CvRandom& rand);

int getTurnMonthForGame(int iGameTurn, int iStartYear, CalendarTypes eCalendar, GameSpeedTypes eSpeed);
int getTurnYearForGame(int iGameTurn, int iStartYear, CalendarTypes eCalendar, GameSpeedTypes eSpeed);

void boolsToString(const bool* pBools, int iNumBools, CvString* szOut);
void stringToBools(const char* szString, int* iNumBools, bool** ppBools);

void getDirectionTypeString(CvWString& szString, DirectionTypes eDirectionType);
void getCardinalDirectionTypeString(CvWString& szString, CardinalDirectionTypes eDirectionType);
void getActivityTypeString(CvWString& szString, ActivityTypes eActivityType);
void getMissionTypeString(CvWString& szString, MissionTypes eMissionType);
void getMissionAIString(CvWString& szString, MissionAITypes eMissionAI);
void getUnitAIString(CvWString& szString, UnitAITypes eUnitAI);

/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      02/21/10                                jdog5000      */
/*                                                                                              */
/* Lead From Behind                                                                             */
/************************************************************************************************/
// From Lead From Behind by UncutDragon
int LFBgetCombatOdds(int iAttackerLowFS,	int iAttackerHighFS, int iDefenderLowFS, int iDefenderHighFS, int iNeededRoundsAttacker, int iNeededRoundsDefender, int iAttackerOdds);
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/

#endif
