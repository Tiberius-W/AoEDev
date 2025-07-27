#pragma once

#ifndef CyPlot_h
#define CyPlot_h

//#include "CvEnums.h"

//
// Python wrapper class for CvPlot
//
class CyArea;
class CvPlot;
class CyUnit;
class CyCity;
class CyPlot
{
public:
	DllExport CyPlot();
	DllExport CyPlot(CvPlot*);			// Call from C++
	CvPlot* getPlot() { return m_pPlot; }	// Call from C++
	void setPlot(CvPlot* p) { m_pPlot=p; }	// Call from C++
	bool isNone() { return (m_pPlot==NULL); }
	void erase();
	DllExport NiPoint3 getPoint();
	int getTeam();

	void nukeExplosion(int iRange, CyUnit* pNukeUnit);

	bool isConnectedTo(CyCity* pCity);
	bool isConnectedToCapital(int /*PlayerTypes*/ ePlayer);
	int getPlotGroupConnectedBonus(int /*PlayerTypes*/ ePlayer, int /*BonusTypes*/ eBonus);
	bool isPlotGroupConnectedBonus(int /*PlayerTypes*/ ePlayer, int /*BonusTypes*/ eBonus);
	bool isAdjacentPlotGroupConnectedBonus(int /*PlayerTypes*/ ePlayer, int /*BonusTypes*/ eBonus);

	void updateVisibility();
	bool isAdjacentToArea(CyArea* pArea);
	bool shareAdjacentArea(CyPlot* pPlot);
	bool isAdjacentToLand();
	bool isCoastalLand();

	bool isWithinTeamCityRadius(int /*TeamTypes*/ eTeam, int /*PlayerTypes*/ eIgnorePlayer);

	bool isLake();
	bool isFreshWater();
	bool isPotentialIrrigation();
	bool canHavePotentialIrrigation();
	bool isIrrigationAvailable(bool bIgnoreSelf);

	bool isRiverSide();
	bool isRiver();
	bool isRiverConnection(int /*DirectionTypes*/ eDirection);

	int getNearestLandArea();
	int seeFromLevel(int /*TeamTypes*/ eTeam);
	CyPlot* getNearestLandPlot();
	int seeThroughLevel();
	bool canHaveBonus(int /*BonusTypes*/ eBonus, bool bIgnoreLatitude);
/*************************************************************************************************/
/**	CivPlotMods								04/02/09								Jean Elcard	**/
/**																								**/
/**						Replaced with two argument type checking versions.						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
	bool canHaveImprovement(int eImprovement, int eTeam, bool bPotential);
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Improvements Mods by Jeckel		imported by Ahwaric	20.09.09 | Valkrionn	09.24.09		**/
/*************************************************************************************************/
	bool isImprovementInRange(int /* ImprovementTypes */ eImprovement, int iRange, bool bCheckBuildProgress);
/*************************************************************************************************/
/**	Improvements End	END								**/
/*************************************************************************************************/

	bool canBuild(int /*BuildTypes*/ eBuild, int /*PlayerTypes*/ ePlayer, bool bTestVisible);
	int getBuildTime(int /*BuildTypes*/ eBuild);
	int getBuildTurnsLeft(int /*BuildTypes*/ eBuild, int iNowExtra, int iThenExtra);
	int getFeatureProduction(int /*BuildTypes*/ eBuild, int /*TeamTypes*/ eTeam, CyCity* ppCity);

	CyUnit* getBestDefender(int /*PlayerTypes*/ eOwner, int /*PlayerTypes*/ eAttackingPlayer, CyUnit* pAttacker, bool bTestAtWar, bool bTestPotentialEnemy, bool bTestCanMove);
	CyUnit* getSelectedUnit();
	int getUnitPower(int /* PlayerTypes */ eOwner);

	int defenseModifier(int /*TeamTypes*/ iDefendTeam, bool bIgnoreBuilding, bool bHelp);
	int movementCost(CyUnit* pUnit, CyPlot* pFromPlot);

	int getExtraMovePathCost();
	void changeExtraMovePathCost(int iChange);

	bool isAdjacentOwned();
	bool isAdjacentPlayer(int /*PlayerTypes*/ ePlayer, bool bLandOnly);
	bool isAdjacentTeam(int /*TeamTypes*/ eTeam, bool bLandOnly);
	bool isWithinCultureRange(int /*PlayerTypes*/ ePlayer);
	int getNumCultureRangeCities(int /*PlayerTypes*/ ePlayer);
	int /*PlayerTypes*/ calculateCulturalOwner();

	bool isOwned();
	bool isBarbarian();
	bool isRevealedBarbarian();
	bool isVisible(int /*TeamTypes*/ eTeam, bool bDebug);
	bool isActiveVisible(bool bDebug);
	bool isVisibleToWatchingHuman();
	bool isAdjacentVisible(int /*TeamTypes*/ eTeam, bool bDebug);
	bool isAdjacentNonvisible(int /*TeamTypes*/ eTeam);
	bool isAdjacentRevealed(int /*TeamTypes*/ eTeam);
	bool isAdjacentNonrevealed(int /*TeamTypes*/ eTeam);

	void removeGoody();
	bool isGoody();
	bool isRevealedGoody(int /*TeamTypes*/ eTeam);

	bool isCity();
	bool isFriendlyCity(CyUnit* pUnit, bool bCheckImprovement);
	bool isEnemyCity(CyUnit* pUnit);
	bool isOccupation();
	bool isBeingWorked();

	bool isUnit();
	bool isInvestigate(int /*TeamTypes*/ eTeam);
	bool isVisibleEnemyDefender(CyUnit* pUnit);
	int getNumDefenders(int /*PlayerTypes*/ ePlayer);
	int getNumVisibleEnemyDefenders(CyUnit* pUnit);
	int getNumVisiblePotentialEnemyDefenders(CyUnit* pUnit);
	bool isVisibleEnemyUnit(int /*PlayerTypes*/ ePlayer);
	bool isVisibleOtherUnit(int /*PlayerTypes*/ ePlayer);
	bool isFighting();

	bool canHaveFeature(int /*FeatureTypes*/ eFeature);
	bool canHavePlotEffect(int /*PlotEffectTypes*/ eFeature);
	bool isRoute();
	bool isNetworkTerrain(int /*TeamTypes*/ eTeam);
	bool isBonusNetwork(int /*TeamTypes*/ eTeam);

	bool isTradeNetworkImpassable(int /*TeamTypes*/ eTeam);
	bool isTradeNetwork(int /*TeamTypes*/ eTeam);
	bool isTradeNetworkConnected(CyPlot* pPlot, int /*TeamTypes*/ eTeam);
	bool isValidDomainForLocation(CyUnit* pUnit) const;
	bool isValidDomainForAction(CyUnit* pUnit) const;
	bool isImpassable();

	int getX();
	int getY();
	bool at(int iX, int iY);
	int getLatitude();
	CyArea* area();
	CyArea* waterArea();
	int getArea();
	int getFeatureVariety();

	int getOwnershipDuration();
	bool isOwnershipScore();
	void setOwnershipDuration(int iNewValue);
	void changeOwnershipDuration(int iChange);

	int getImprovementDuration();
	void setImprovementDuration(int iNewValue);
	void changeImprovementDuration(int iChange);

	int getUpgradeProgress();
	int getUpgradeTimeLeft(int /*ImprovementTypes*/ eImprovement, int /*PlayerTypes*/ ePlayer);

	void setUpgradeProgress(int iNewValue);
	void changeUpgradeProgress(int iChange);

	int getForceUnownedTimer();
	bool isForceUnowned();
	void setForceUnownedTimer(int iNewValue);
	void changeForceUnownedTimer(int iChange);

	int getCityRadiusCount();
	int isCityRadius();

	bool isStartingPlot();
	void setStartingPlot(bool bNewValue);
	bool isNOfRiver();
	void setNOfRiver(bool bNewValue, CardinalDirectionTypes eRiverDir);
	bool isWOfRiver();
	void setWOfRiver(bool bNewValue, CardinalDirectionTypes eRiverDir);
	CardinalDirectionTypes getRiverWEDirection();
	CardinalDirectionTypes getRiverNSDirection();
	bool isIrrigated();

	bool isPotentialCityWork();
	bool isPotentialCityWorkForArea(CyArea* pArea);

	bool isFlagDirty();
	void setFlagDirty(bool bNewValue);

	int /*PlayerTypes*/ getOwner();
	void setOwner(int /*PlayerTypes*/ eNewValue);
	void setOwnerNoUnitCheck(int /*PlayerTypes*/ eNewValue);
/*************************************************************************************************/
/**	Improvements Mods by Jeckel		imported by Ahwaric	20.09.09 | Valkrionn	09.24.09		**/
/*************************************************************************************************/
	int /*PlayerTypes*/ getImprovementOwner();
	void setImprovementOwner(int /*PlayerTypes*/ eNewValue);

	int getCultureControl(int /*PlayerTypes*/ eIndex);
	int countTotalCultureControl();
	int /*PlayerTypes*/ findHighestCultureControlPlayer();

	int calculateCultureControlPercent(int /*PlayerTypes*/ eIndex);
	int calculateTeamCultureControlPercent(int /*TeamTypes*/ eIndex);
	void setCultureControl(int /*PlayerTypes*/ eIndex, int iNewValue, bool bUpdate);
	void changeCultureControl(int /*PlayerTypes*/ eIndex, int iChange, bool bUpdate);

	void addCultureControl(int /*PlayerTypes*/ ePlayer, int /*ImprovementTypes*/ eImprovement, bool bUpdateInterface);
	void clearCultureControl(int /*PlayerTypes*/ ePlayer, int /*ImprovementTypes*/ eImprovement, bool bUpdateInterface);
/*************************************************************************************************/
/**	Improvements Mods	END								**/
/*************************************************************************************************/
	PlotTypes getPlotType();
	bool isWater();
	bool isFlatlands();
	bool isHills();
	bool isPeak();

	void setPlotType(PlotTypes eNewValue, bool bRecalculate, bool bRebuildGraphics);
	int /*TerrainTypes*/ getTerrainType();
	void setTerrainType(int /*TerrainTypes*/ eNewValue, bool bRecalculate, bool bRebuildGraphics);
	int /*FeatureTypes*/ getFeatureType();
	void setFeatureType(int /*FeatureTypes*/ eNewValue, int iVariety);
	int /*PlotEffectTypes*/ getPlotEffectType();
	void setPlotEffectType(int /*PlotEffectTypes*/ eNewValue);
	void setFeatureDummyVisibility(std::string dummyTag, bool show);
	void addFeatureDummyModel(std::string dummyTag, std::string modelTag);
	void setFeatureDummyTexture(std::string dummyTag, std::string textureTag);
	std::string pickFeatureDummyTag(int mouseX, int mouseY);
	void resetFeatureModel();
	int /* BonusTypes */ getBonusType(int /*TeamTypes*/ eTeam);
	int /* BonusTypes */ getNonObsoleteBonusType(int /*TeamTypes*/ eTeam);
	void setBonusType(int /* BonusTypes */ eNewValue);
	int /* ImprovementTypes */ getImprovementType();
	void setImprovementType(int /* ImprovementTypes */ eNewValue);
	int /* RouteTypes */ getRouteType();
	void setRouteType(int /*RouteTypes*/ eNewValue);

	CyCity* getPlotCity();
	CyCity* getWorkingCity();
	CyCity* getWorkingCityOverride();
	int getRiverID() const;
	void setRiverID(int iNewValue);
	int getMinOriginalStartDist();
	int getReconCount();
	int getRiverCrossingCount();
	int getYield(YieldTypes eIndex);
	int calculateNatureYield(YieldTypes eIndex, TeamTypes eTeam, bool bIgnoreFeature = false);
	int calculateBestNatureYield(YieldTypes eIndex, TeamTypes eTeam);
	int calculateTotalBestNatureYield(TeamTypes eTeam);
	int calculateImprovementYieldChange(int /*ImprovementTypes*/ eImprovement, YieldTypes eYield, int /*PlayerTypes*/ ePlayer, bool bOptimal);
	int calculateYield(YieldTypes eIndex, bool bDisplay);
	bool hasYield();

	int getCulture(int /*PlayerTypes*/ eIndex);
	int countTotalCulture();
	int /*TeamTypes*/ findHighestCultureTeam();

	int calculateCulturePercent(int /*PlayerTypes*/ eIndex);
	int calculateTeamCulturePercent(int /*TeamTypes*/ eIndex);
	void setCulture(int /*PlayerTypes*/ eIndex, int iNewValue, bool bUpdate);
	void changeCulture(int /*PlayerTypes*/ eIndex, int iChange, bool bUpdate);

	int countNumAirUnits(int /*TeamTypes*/ ePlayer);

	int getFoundValue(int /*PlayerTypes*/ eIndex);
	bool isBestAdjacentFound(int /*PlayerTypes*/ eIndex);

	int getPlayerCityRadiusCount(int /*PlayerTypes*/ eIndex);
	bool isPlayerCityRadius(int /*PlayerTypes*/ eIndex);

	int getVisibilityCount(int /*TeamTypes*/ eTeam);
	void changeVisibilityCount(int /*TeamTypes*/ eTeam, int iChange, int /*InvisibleTypes*/ eSeeInvisible);

	int getStolenVisibilityCount(int /*TeamTypes*/ eTeam);

	int /*PlayerTypes*/ getRevealedOwner(int /*TeamTypes*/ eTeam, bool bDebug);
	int /*TeamTypes*/ getRevealedTeam(int /*TeamTypes*/ eTeam, bool bDebug);

	bool isRiverCrossing(DirectionTypes eIndex);

	bool isRevealed(int /*TeamTypes*/ eTeam, bool bDebug);
	void setRevealed(int /*TeamTypes*/ eTeam, bool bNewValue, bool bTerrainOnly, int /*TeamTypes*/ eFromTeam);
	int /* ImprovementTypes */ getRevealedImprovementType(int /*TeamTypes*/ eTeam, bool bDebug);
	int /* RouteTypes */ getRevealedRouteType(int /*TeamTypes*/ eTeam, bool bDebug);
	int getBuildProgress(int /*BuildTypes*/ eBuild);
	bool changeBuildProgress(int /*BuildTypes*/ eBuild, int iChange, int /*TeamTypes*/ eTeam, bool bLinked);

	int getCultureRangeCities(int /*PlayerTypes*/ eOwnerIndex, int iRangeIndex);
	bool isCultureRangeCity(int /*PlayerTypes*/ eOwnerIndex, int iRangeIndex);

	int getInvisibleVisibilityCount(int /*TeamTypes*/ eTeam, int /*InvisibleTypes*/ eInvisible);
	bool isInvisibleVisible(int /*TeamTypes*/ eTeam, int /*InvisibleTypes*/ eInvisible);
	void changeInvisibleVisibilityCount(int /*TeamTypes*/ eTeam, int /*InvisibleTypes*/ eInvisible, int iChange);

	int getNumUnits();
	CyUnit* getUnit(int iIndex);

	std::string getScriptData() const;
	void setScriptData(std::string szNewValue);

//FfH: Added by Kael 08/15/2007
	void changePlotCounter(int iChange);
	void setPlotCounter(int iNewValue);
	int getPlotCounter() const;
	int getPortalExitX() const;
	void setPortalExitX(int iNewValue);
	int getPortalExitY() const;
	void setPortalExitY(int iNewValue);
	bool isMoveDisabledAI() const;
	void setMoveDisabledAI(bool bNewValue);
	bool isMoveDisabledHuman() const;
	void setMoveDisabledHuman(bool bNewValue);
	bool isBuildDisabled() const;
	void setBuildDisabled(bool bNewValue);
	bool isFoundDisabled() const;
	void setFoundDisabled(bool bNewValue);
	bool isPythonActive() const;
	void setPythonActive(bool bNewValue);
	bool isAdjacentToWater() const;
	void setMinLevel(int iNewValue);
	int getNumAnimalUnits() const;
	void changeTempTerrainTimer(int iChange);
	void setTempTerrainType(int /*TerrainTypes*/ eNewValue, int iTimer);
	bool isVisibleToCivTeam() const;
//FfH: End Add

/*************************************************************************************************/
/**	New Tag Defs	(TerrainInfos)			09/19/08								Jean Elcard	**/
/**	New Tag Defs	(PlotInfos)				12/31/08								Xienwolf	**/
/**								Defines Function for Use in .cpp								**/
/*************************************************************************************************/
	int getTempTerrainTimer();
	int getMinLevel();
	void setTempTerrainTypeFM(int eTerrain, int iTimer, bool bRecalculateAreas = true, bool bRebuildPlot = true);
	void setTempFeatureType(int  eFeature, int iVariety, int iTimer);
	void setTempBonusType(int  eBonus, int iTimer);
	int getNumSpawnsAlive();
	void changeNumSpawnsAlive(int iChange);

	bool isNeedsRebuilding();
	void setNeedsRebuilding(bool bNewValue);
	void rebuildGraphics();

//CivPlotMods:
	int calculatePlayerNatureYield(YieldTypes eIndex, PlayerTypes ePlayer, bool bIgnoreFeature = false);
	int calculatePlayerBestNatureYield(YieldTypes eIndex, PlayerTypes ePlayer);
	int calculatePlayerTotalBestNatureYield(PlayerTypes ePlayer);
	bool canPlayerHaveImprovement(int eImprovement, PlayerTypes ePlayer, bool bPotential);
	bool canTeamHaveImprovement(int eImprovement, TeamTypes eTeam, bool bPotential);
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Sidar Mist 								25/06/10								Grey Fox	**/
/*************************************************************************************************/
	int getPerceptionCost() const;
	void setPerceptionCost(int iNewValue);
	void changePerceptionCost(int iChange);
	int getMistChangeTimer() const;
	void setMistChangeTimer(int iNewValue);
	void changeMistChangeTimer(int iChange);
	int getMistChangeTemp() const;
	void setMistChangeTemp(int iNewValue);
	void changeMistChangeTemp(int iChange);
/*************************************************************************************************/
/**	END                                                                   						**/
/*************************************************************************************************/

/*************************************************************************************************/
/** Exposing climate stuff					Opera												**/
/*************************************************************************************************/
	int /*ClimateZoneTypes*/ getClimate() const;
	void setClimate(int /*ClimateZoneTypes*/ eClimate);
	void updateClimate();
	int /*ClimateZoneTypes*/ getNaturalClimate() const;
	void setNaturalClimate(int /*ClimateZoneTypes*/ eClimate);
	void updateNaturalClimate();
	int getTemperature() const;
	void changeTemperature(int iChange);
	void setTemperature(int iTemperature);
	int getHumidity() const;
	void changeHumidity(int iChange);
	void setHumidity(int iHumidity);
	int getNaturalTemperature() const;
	void changeNaturalTemperature(int iChange);
	void setNaturalTemperature(int iNewValue);
	int getNaturalHumidity() const;
	void changeNaturalHumidity(int iChange);
	void setNaturalHumidity(int iNewValue);
	int getTemperatureStrain() const;
	void changeTemperatureStrain(int iChange);
	void setTemperatureSrain(int iValue);
	int getHumidityStrain() const;
	void changeHumidityStrain(int iChange);
	void setHumidityStrain(int iValue);
	int getTemperatureTo(int /*ClimateZoneTypes*/ eClimate) const;
	int getHumidityTo(int /*ClimateZoneTypes*/ eClimate) const;
	int /*ClimateZoneTypes*/ getClimateMatch(int iTemperature, int iHumidity);
	int /*ClimateZoneTypes*/ getWantedClimate();
	int /*ClimateZoneTypes*/ getNextClimate();
	int getNextClimateTurnsLeft();
	int getScaledTemperatureThreshold() const;
	int getScaledHumidityThreshold() const;
	int /*TerrainClassTypes*/ getTerrainClassType() const;
	void setTerrainClassType(int /*TerrainClassTypes*/ eNewValue, bool bRecalculate, bool bRebuildGraphics);
	int getRealTerrainType() const;
	int /* BonusTypes */ getRealBonusType();
	int getExploreNextTurn();
	void setExploreNextTurn(int iNewValue);


/*************************************************************************************************/
/** Exposing climate stuff					END													**/
/*************************************************************************************************/

private:
	CvPlot* m_pPlot;
};
#endif	// CyPlot_h

