#include "CvGameCoreDLL.h"
#include "CyPlot.h"
#include "CyCity.h"
#include "CyArea.h"
#include "CyUnit.h"
#include "CvPlot.h"
//# include <boost/python/manage_new_object.hpp>
//# include <boost/python/return_value_policy.hpp>

//
// published python interface for CyPlot
//

void CyPlotPythonInterface1(python::class_<CyPlot>& x)
{
	OutputDebugString("Python Extension Module - CyPlotPythonInterface1\n");

	x
		.def("isNone", &CyPlot::isNone, "bool ()")
		.def("erase", &CyPlot::erase, "void ()")
		.def("getPoint", &CyPlot::getPoint, "NiPoint3 ()")
		.def("getTeam", &CyPlot::getTeam, "int ()")

		.def("nukeExplosion", &CyPlot::nukeExplosion, "void (int iRange, CyUnit* pNukeUnit)")

		.def("isConnectedTo", &CyPlot::isConnectedTo, "bool (CvCity* pCity) - returns whether this plot is connected to the provided city")
		.def("isConnectedToCapital", &CyPlot::isConnectedToCapital, "bool (int (PlayerTypes) ePlayer) - returns whether this plot is connected to the capital of the provided player")
		.def("getPlotGroupConnectedBonus", &CyPlot::getPlotGroupConnectedBonus, "int (int (PlayerTypes) ePlayer, int (BonusTypes) eBonus)")
		.def("isPlotGroupConnectedBonus", &CyPlot::isPlotGroupConnectedBonus, "bool (int (PlayerTypes) ePlayer, int (BonusTypes) eBonus)")
		.def("isAdjacentPlotGroupConnectedBonus", &CyPlot::isAdjacentPlotGroupConnectedBonus, "bool (int (PlayerTypes) ePlayer, int (BonusTypes) eBonus)")

		.def("updateVisibility", &CyPlot::updateVisibility, "void () Refreshes all of the plots")
		.def("isAdjacentToArea", &CyPlot::isAdjacentToArea, "bool (CyArea)")
		.def("shareAdjacentArea", &CyPlot::shareAdjacentArea, "bool (CyPlot)")
		.def("isAdjacentToLand", &CyPlot::isAdjacentToLand, "bool ()")
		.def("isCoastalLand", &CyPlot::isCoastalLand, "bool ()")

		.def("isWithinTeamCityRadius", &CyPlot::isWithinTeamCityRadius, "bool (int /*TeamTypes*/ eTeam, int /*PlayerTypes*/ eIgnorePlayer)")

		.def("isLake", &CyPlot::isLake, "bool ()")
		.def("isFreshWater", &CyPlot::isFreshWater, "bool ()")
		.def("isPotentialIrrigation", &CyPlot::isPotentialIrrigation, "bool ()")
		.def("canHavePotentialIrrigation", &CyPlot::canHavePotentialIrrigation, "bool ()")
		.def("isIrrigationAvailable", &CyPlot::isIrrigationAvailable, "bool (bool bIgnoreSelf)")

		.def("isRiverSide", &CyPlot::isRiverSide, "bool ()")
		.def("isRiver", &CyPlot::isRiver, "bool ()")
		.def("isRiverConnection", &CyPlot::isRiverConnection, "bool (int /*DirectionTypes*/ eDirection)")

		.def("getNearestLandArea", &CyPlot::getNearestLandArea, "int ()")
		.def("getNearestLandPlot", &CyPlot::getNearestLandPlot, python::return_value_policy<python::manage_new_object>(), "CyPlot* ()")
		.def("seeFromLevel", &CyPlot::seeFromLevel, "int (int eTeam), bool bAerial")
		.def("seeThroughLevel", &CyPlot::seeThroughLevel, "int ()")
		.def("canHaveBonus", &CyPlot::canHaveBonus, "bool (int /*BonusTypes*/ eBonus, bool bIgnoreLatitude)")
/*************************************************************************************************/
/**	CivPlotMods								04/02/09								Jean Elcard	**/
/**																								**/
/**						Replaced with two argument type checking versions.						**/
/*************************************************************************************************/
/**								---- Start Original Code ----									**
		.def("canHaveImprovement", &CyPlot::canHaveImprovement, "bool (int (ImprovementTypes) eImprovement, int (TeamTypes) eTeam, bool bPotential)")
/**								----  End Original Code  ----									**/
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Improvements Mods by Jeckel		imported by Ahwaric	20.09.09 | Valkrionn	09.24.09		**/
/*************************************************************************************************/
		.def("isImprovementInRange", &CyPlot::isImprovementInRange, "bool (int /*ImprovementTypes*/ eImprovement, int iRange, bool bCheckBuildProgress)")
/*************************************************************************************************/
/**	Improvements Mods	END								**/
/*************************************************************************************************/

		.def("canBuild", &CyPlot::canBuild, "bool (int (BuildTypes) eBuild, int (PlayerTypes) ePlayer, bool bTestVisible)")
		.def("getBuildTime", &CyPlot::getBuildTime, "int (int /*BuildTypes*/ eBuild)")
		.def("getBuildTurnsLeft", &CyPlot::getBuildTurnsLeft, "int (int (BuildTypes) eBuild, int iNowExtra, int iThenExtra)")
		.def("getFeatureProduction", &CyPlot::getFeatureProduction, "int (int (BuildTypes) eBuild, int (TeamTypes) eTeam, CvCity** ppCity)")

		.def("getBestDefender", &CyPlot::getBestDefender, python::return_value_policy<python::manage_new_object>(), "CyUnit* (int (PlayerTypes) eOwner, int (PlayerTypes) eAttackingPlayer, CvUnit* pAttacker, bool bTestAtWar, bool bTestPotentialEnemy, bool bTestCanMove)")
		.def("getSelectedUnit", &CyPlot::getSelectedUnit, python::return_value_policy<python::manage_new_object>(), "CyUnit* ()")
		.def("getUnitPower", &CyPlot::getUnitPower, "int (int /*PlayerTypes*/ eOwner)")

		.def("defenseModifier", &CyPlot::defenseModifier, "int (int /*TeamTypes*/, bool bIgnoreBuilding, bool bHelp)")
		.def("movementCost", &CyPlot::movementCost, "int (CyUnit* pUnit, CyPlot* pFromPlot)")

		.def("getExtraMovePathCost", &CyPlot::getExtraMovePathCost, "int ()")
		.def("changeExtraMovePathCost", &CyPlot::changeExtraMovePathCost, "int (int iChange)")

		.def("isAdjacentOwned", &CyPlot::isAdjacentOwned, "bool ()")
		.def("isAdjacentPlayer", &CyPlot::isAdjacentPlayer, "bool (int /*PlayerTypes*/ ePlayer, bool bLandOnly)")
		.def("isAdjacentTeam", &CyPlot::isAdjacentTeam, "bool (int /*TeamTypes*/ eTeam, bool bLandOnly)")
		.def("isWithinCultureRange", &CyPlot::isWithinCultureRange, "bool (int /*PlayerTypes*/ ePlayer)")
		.def("getNumCultureRangeCities", &CyPlot::getNumCultureRangeCities, "bool (int /*PlayerTypes*/ ePlayer)")
		.def("calculateCulturalOwner", &CyPlot::calculateCulturalOwner, "int ()")
		.def("isOwned", &CyPlot::isOwned, "bool ()")
		.def("isBarbarian", &CyPlot::isBarbarian, "bool ()")
		.def("isRevealedBarbarian", &CyPlot::isRevealedBarbarian, "bool ()")
		.def("isVisible", &CyPlot::isVisible, "bool (int /*TeamTypes*/ eTeam, bool bDebug)")
		.def("isActiveVisible", &CyPlot::isActiveVisible, "bool (bool bDebug)")
		.def("isVisibleToWatchingHuman", &CyPlot::isVisibleToWatchingHuman, "bool ()")
		.def("isAdjacentVisible", &CyPlot::isAdjacentVisible)
		.def("isAdjacentNonvisible", &CyPlot::isAdjacentNonvisible)
		.def("isAdjacentNonrevealed", &CyPlot::isAdjacentNonrevealed)
		.def("isAdjacentRevealed", &CyPlot::isAdjacentRevealed)

		.def("removeGoody", &CyPlot::removeGoody, "void ()")
		.def("isGoody", &CyPlot::isGoody, "bool ()")
		.def("isRevealedGoody", &CyPlot::isRevealedGoody, "bool (int (TeamTypes) eTeam)")

		.def("isCity", &CyPlot::isCity, "bool ()")
		.def("isFriendlyCity", &CyPlot::isFriendlyCity, "bool (CyUnit* pUnit, bool bCheckImprovement)")
		.def("isEnemyCity", &CyPlot::isEnemyCity, "bool (CyUnit* pUnit)")
		.def("isOccupation", &CyPlot::isOccupation, "bool ()")
		.def("isBeingWorked", &CyPlot::isBeingWorked, "bool ()")

		.def("isUnit", &CyPlot::isUnit, "bool ()")
		.def("isInvestigate", &CyPlot::isInvestigate, "bool (int /*TeamTypes*/ eTeam)")
		.def("isVisibleEnemyDefender", &CyPlot::isVisibleEnemyDefender, "bool (CyUnit* pUnit)")
		.def("getNumDefenders", &CyPlot::getNumDefenders, "int (int /*PlayerTypes*/ ePlayer)")
		.def("getNumVisibleEnemyDefenders", &CyPlot::getNumVisibleEnemyDefenders, "int (CyUnit* pUnit)")
		.def("getNumVisiblePotentialEnemyDefenders", &CyPlot::getNumVisiblePotentialEnemyDefenders, "int (CyUnit* pUnit)")
		.def("isVisibleEnemyUnit", &CyPlot::isVisibleEnemyUnit, "bool (int /*PlayerTypes*/ ePlayer)")
		.def("isVisibleOtherUnit", &CyPlot::isVisibleOtherUnit, "bool (int /*PlayerTypes*/ ePlayer)")
		.def("isFighting", &CyPlot::isFighting, "bool ()")

		.def("canHaveFeature", &CyPlot::canHaveFeature, "bool (int /*FeatureTypes*/ eFeature)")
		.def("canHavePlotEffect", &CyPlot::canHavePlotEffect, "bool (int /*PlotEffectTypes*/ eFeature)")
		.def("isRoute", &CyPlot::isRoute, "bool ()")
		.def("isNetworkTerrain", &CyPlot::isNetworkTerrain, "bool (int (TeamTypes) eTeam)")
		.def("isBonusNetwork", &CyPlot::isBonusNetwork, "bool (int (TeamTypes) eTeam)")

		.def("isTradeNetworkImpassable", &CyPlot::isTradeNetworkImpassable, "bool (int (TeamTypes) eTeam)")
		.def("isTradeNetwork", &CyPlot::isTradeNetwork, "bool (int eTeam)")
		.def("isTradeNetworkConnected", &CyPlot::isTradeNetworkConnected, "bool (CyPlot, int eTeam)")
		.def("isValidDomainForLocation", &CyPlot::isValidDomainForLocation, "bool (CyUnit* pUnit)")
		.def("isValidDomainForAction", &CyPlot::isValidDomainForAction, "bool (CyUnit* pUnit)")
		.def("isImpassable", &CyPlot::isImpassable, "bool ()")

		.def("getX", &CyPlot::getX, "int ()")
		.def("getY", &CyPlot::getY, "int ()")
		.def("at", &CyPlot::at, "bool (int iX, int iY)")
		.def("getLatitude", &CyPlot::getLatitude, "int ()")
		.def("area", &CyPlot::area, python::return_value_policy<python::manage_new_object>(), "CyArea* ()")
		.def("waterArea", &CyPlot::waterArea, python::return_value_policy<python::manage_new_object>(), "CyArea* ()")
		.def("getArea", &CyPlot::getArea, "int ()")
		.def("getFeatureVariety", &CyPlot::getFeatureVariety, "int ()")

		.def("getOwnershipDuration", &CyPlot::getOwnershipDuration, "int ()")
		.def("isOwnershipScore", &CyPlot::isOwnershipScore, "int ()")
		.def("setOwnershipDuration", &CyPlot::setOwnershipDuration, "int (int iNewValue)")
		.def("changeOwnershipDuration", &CyPlot::changeOwnershipDuration, "int (int iChange)")

		.def("getImprovementDuration", &CyPlot::getImprovementDuration, "int ()")
		.def("setImprovementDuration", &CyPlot::setImprovementDuration, "int (int iNewValue)")
		.def("changeImprovementDuration", &CyPlot::changeImprovementDuration, "int (int iChange)")

		.def("getUpgradeProgress", &CyPlot::getUpgradeProgress, "int ()")
		.def("getUpgradeTimeLeft", &CyPlot::getUpgradeTimeLeft, "int (int /*ImprovementTypes*/ eImprovement, int /*PlayerTypes*/ ePlayer)")

		.def("setUpgradeProgress", &CyPlot::setUpgradeProgress, "void (int iNewValue)")
		.def("changeUpgradeProgress", &CyPlot::changeUpgradeProgress, "void (int iChange)")

		.def("getForceUnownedTimer", &CyPlot::getForceUnownedTimer, "int ()")
		.def("isForceUnowned", &CyPlot::isForceUnowned, "int ()")
		.def("setForceUnownedTimer", &CyPlot::setForceUnownedTimer, "void (int iNewValue)")
		.def("changeForceUnownedTimer", &CyPlot::changeForceUnownedTimer, "void (int iChange)")

		.def("getCityRadiusCount", &CyPlot::getCityRadiusCount, "int ()")
		.def("isCityRadius", &CyPlot::isCityRadius, "int ()")

		.def("isStartingPlot", &CyPlot::isStartingPlot, "bool ()")
		.def("setStartingPlot", &CyPlot::setStartingPlot, "void (bool bNewValue)")
		.def("isNOfRiver", &CyPlot::isNOfRiver, "bool ()")
		.def("setNOfRiver", &CyPlot::setNOfRiver, "void (bool bNewValue, CardinalDirectionTypes eRiverDir)")
		.def("isWOfRiver", &CyPlot::isWOfRiver, "bool ()")
		.def("setWOfRiver", &CyPlot::setWOfRiver, "void (bool bNewValue, CardinalDirectionTypes eRiverDir)")
		.def("getRiverWEDirection", &CyPlot::getRiverWEDirection, "CardinalDirectionTypes ()")
		.def("getRiverNSDirection", &CyPlot::getRiverNSDirection, "CardinalDirectionTypes ()")
		.def("isIrrigated", &CyPlot::isIrrigated, "bool ()")

		.def("isPotentialCityWork", &CyPlot::isPotentialCityWork, "bool ()")
		.def("isPotentialCityWorkForArea", &CyPlot::isPotentialCityWorkForArea, "bool (CyArea* pArea)")

		.def("isFlagDirty", &CyPlot::isFlagDirty, "bool ()")
		.def("setFlagDirty", &CyPlot::setFlagDirty, "void (bool bNewValue)")

		.def("getOwner", &CyPlot::getOwner, "int ()")
		.def("setOwner", &CyPlot::setOwner, "void (int /*PlayerTypes*/ eNewValue)")
		.def("setOwnerNoUnitCheck", &CyPlot::setOwnerNoUnitCheck, "void (int /*PlayerTypes*/ eNewValue)")
/*************************************************************************************************/
/**	Improvements Mods by Jeckel		imported by Ahwaric	20.09.09 | Valkrionn	09.24.09		**/
/*************************************************************************************************/
		.def("getImprovementOwner", &CyPlot::getImprovementOwner, "int ()")
		.def("setImprovementOwner", &CyPlot::setImprovementOwner, "void (int /*PlayerTypes*/ eNewValue)")

		.def("getCultureControl", &CyPlot::getCultureControl, "int (int /*PlayerTypes*/ eIndex)")
		.def("countTotalCultureControl", &CyPlot::countTotalCultureControl, "int ()")
		.def("findHighestCultureControlPlayer", &CyPlot::findHighestCultureControlPlayer, "int /*PlayerTypes*/ ()")

		.def("calculateCultureControlPercent", &CyPlot::calculateCultureControlPercent, "int (int /*PlayerTypes*/ eIndex)")
		.def("calculateTeamCultureControlPercent", &CyPlot::calculateTeamCultureControlPercent, "int (int /*TeamTypes*/ eIndex)")
		.def("setCultureControl", &CyPlot::setCultureControl, "void (int /*PlayerTypes*/ eIndex, int iNewValue, bool bUpdate)")
		.def("changeCultureControl", &CyPlot::changeCultureControl, "void (int /*PlayerTypes*/ eIndex, int iChange, bool bUpdate)")

		.def("addCultureControl", &CyPlot::addCultureControl, "void (int /*PlayerTypes*/ ePlayer, int /*ImprovementTypes*/ eImprovement, bool bUpdateInterface)")
		.def("clearCultureControl", &CyPlot::clearCultureControl, "void (int /*PlayerTypes*/ ePlayer, int /*ImprovementTypes*/ eImprovement, bool bUpdateInterface)")
/*************************************************************************************************/
/**	Improvements Mods	END								**/
/*************************************************************************************************/
		.def("getPlotType", &CyPlot::getPlotType, "int ()")
		.def("isWater", &CyPlot::isWater, "bool ()")
		.def("isFlatlands", &CyPlot::isFlatlands, "bool ()")
		.def("isHills", &CyPlot::isHills, "bool ()")
		.def("isPeak", &CyPlot::isPeak, "bool ()")
		.def("setPlotType", &CyPlot::setPlotType, "void (PlotTypes eNewValue, bool bRecalculate, bool bRebuildGraphics)")
		.def("getTerrainType", &CyPlot::getTerrainType, "int ()")
		.def("setTerrainType", &CyPlot::setTerrainType, "void (TerrainTypes eNewValue, bool bRecalculate, bool bRebuildGraphics)")
		.def("getFeatureType", &CyPlot::getFeatureType, "int ()")
		.def("setFeatureType", &CyPlot::setFeatureType, "void (int /*FeatureTypes*/ eNewValue, int iVariety)")
			.def("getPlotEffectType", &CyPlot::getPlotEffectType, "int ()")
			.def("setPlotEffectType", &CyPlot::setPlotEffectType, "void (int /*FeatureTypes*/ eNewValue, int iVariety)")
			.def("setFeatureDummyVisibility", &CyPlot::setFeatureDummyVisibility, "void (string dummyTag, bool show)")
		.def("addFeatureDummyModel", &CyPlot::addFeatureDummyModel, "void (string dummyTag, string modelTag)")
		.def("setFeatureDummyTexture", &CyPlot::setFeatureDummyTexture, "void (string dummyTag, string textureTag)")
		.def("pickFeatureDummyTag", &CyPlot::pickFeatureDummyTag, "string (int mouseX, int mouseY)")
		.def("resetFeatureModel", &CyPlot::resetFeatureModel, "void ()")
		.def("getBonusType", &CyPlot::getBonusType, "int (int /*TeamTypes*/ eTeam)")
		.def("getNonObsoleteBonusType", &CyPlot::getNonObsoleteBonusType, "int (int /*TeamTypes*/ eTeam)")
		.def("setBonusType", &CyPlot::setBonusType, "void (int eNewValue)")
		.def("getImprovementType", &CyPlot::getImprovementType, "int ()")
		.def("setImprovementType", &CyPlot::setImprovementType, "void (int eNewValue)")
		.def("getRouteType", &CyPlot::getRouteType, "int ()")
		.def("setRouteType", &CyPlot::setRouteType, "void (int (RouteTypes) eNewValue)")

		.def("getPlotCity", &CyPlot::getPlotCity, python::return_value_policy<python::manage_new_object>(), "CyCity* ()")
		.def("getWorkingCity", &CyPlot::getWorkingCity, python::return_value_policy<python::manage_new_object>(), "CyCity* ()")
		.def("getWorkingCityOverride", &CyPlot::getWorkingCityOverride, python::return_value_policy<python::manage_new_object>(), "CyCity* ()")
		.def("getRiverID", &CyPlot::getRiverID, "int ()")
		.def("setRiverID", &CyPlot::setRiverID, "void (int)")
		.def("getMinOriginalStartDist", &CyPlot::getMinOriginalStartDist, "int ()")
		.def("getRiverCrossingCount", &CyPlot::getRiverCrossingCount, "int ()")
		.def("getYield", &CyPlot::getYield, "int (YieldTypes eIndex)")
		.def("calculateNatureYield", &CyPlot::calculateNatureYield, "int (int (YieldTypes) eYield, int (TeamTypes) eTeam, bool bIgnoreFeature)")
		.def("calculateBestNatureYield", &CyPlot::calculateBestNatureYield, "int (int (YieldTypes) eYield, int (TeamTypes) eTeam)")
		.def("calculateTotalBestNatureYield", &CyPlot::calculateTotalBestNatureYield, "int (int (TeamTypes) eTeam)")
		.def("calculateImprovementYieldChange", &CyPlot::calculateImprovementYieldChange, "int (int (ImprovementTypes) eImprovement, int (YieldTypes) eYield, int (PlayerTypes) ePlayer, bool bOptimal)")
		.def("calculateYield", &CyPlot::calculateYield, "int (YieldTypes eYield, bool bDisplay)")
		.def("hasYield", &CyPlot::hasYield, "bool ()")

		.def("getCulture", &CyPlot::getCulture, "int (int /*PlayerTypes*/ eIndex)")
		.def("countTotalCulture", &CyPlot::countTotalCulture, "int ()")
		.def("findHighestCultureTeam", &CyPlot::findHighestCultureTeam, "int /*TeamTypes*/ ()")

		.def("calculateCulturePercent", &CyPlot::calculateCulturePercent, "int (int /*PlayerTypes*/ eIndex)")
		.def("calculateTeamCulturePercent", &CyPlot::calculateTeamCulturePercent, "int (int /*TeamTypes*/ eIndex)")
		.def("setCulture", &CyPlot::setCulture, "void (int /*PlayerTypes*/ eIndex, int iNewValue, bool bUpdate)")
		.def("changeCulture", &CyPlot::changeCulture, "void (int /*PlayerTypes*/ eIndex, int iChange, bool bUpdate)")

		.def("countNumAirUnits", &CyPlot::countNumAirUnits, "int (int /*TeamTypes*/ ePlayer)")

		.def("getFoundValue", &CyPlot::getFoundValue, "int (int /*PlayerTypes*/ eIndex)")
		.def("isBestAdjacentFound", &CyPlot::isBestAdjacentFound, "bool (int /*PlayerTypes*/ eIndex)")

		.def("getPlayerCityRadiusCount", &CyPlot::getPlayerCityRadiusCount, "int (int /*PlayerTypes*/ eIndex)")
		.def("isPlayerCityRadius", &CyPlot::isPlayerCityRadius, "bool (int /*PlayerTypes*/ eIndex)")

		.def("getVisibilityCount", &CyPlot::getVisibilityCount, "int (int /*TeamTypes*/ eTeam)")
		.def("changeVisibilityCount", &CyPlot::changeVisibilityCount, "void (int (TeamTypes) eTeam, int iChange, int (InvisibleTypes) eSeeInvisible)")

		.def("getStolenVisibilityCount", &CyPlot::getStolenVisibilityCount, "int (int /*TeamTypes*/ eTeam)")

		.def("getRevealedOwner", &CyPlot::getRevealedOwner, "int (int (TeamTypes) eTeam, bool bDebug)")
		.def("getRevealedTeam", &CyPlot::getRevealedTeam, "int (int /*TeamTypes*/ eTeam, bool bDebug)")

		.def("isRiverCrossing", &CyPlot::isRiverCrossing, "bool (DirectionTypes eIndex)")

		.def("isRevealed", &CyPlot::isRevealed, "bool (int /*TeamTypes*/ eTeam, bool bDebug)")
		.def("setRevealed", &CyPlot::setRevealed, "void (int /*TeamTypes*/ eTeam, bool bNewValue, bool bTerrainOnly, int /*TeamTypes*/ eFromTeam)")
		.def("getRevealedImprovementType", &CyPlot::getRevealedImprovementType, "int (int /*TeamTypes*/ eTeam, bool bDebug)")
		.def("getRevealedRouteType", &CyPlot::getRevealedRouteType, "int (int /*TeamTypes*/ eTeam, bool bDebug)")
		.def("getBuildProgress", &CyPlot::getBuildProgress, "int (int /*BuildTypes*/ eBuild)")
		.def("changeBuildProgress", &CyPlot::changeBuildProgress, "bool (int /*BuildTypes*/ eBuild, int iChange, int /*TeamTypes*/ eTeam, bool bLinked)")

		.def("getCultureRangeCities", &CyPlot::getCultureRangeCities, "int (int /*PlayerTypes*/ eOwnerIndex, int iRangeIndex)")
		.def("isCultureRangeCity", &CyPlot::isCultureRangeCity, "bool (int /*PlayerTypes*/ eOwnerIndex, int iRangeIndex)")

		.def("getInvisibleVisibilityCount", &CyPlot::getInvisibleVisibilityCount, "int (int (TeamTypes eTeam), int (InvisibleTypes) eInvisible)")
		.def("isInvisibleVisible", &CyPlot::isInvisibleVisible, "int (int (TeamTypes eTeam), int (InvisibleTypes) eInvisible)")
		.def("changeInvisibleVisibilityCount", &CyPlot::changeInvisibleVisibilityCount, "int (int (TeamTypes eTeam), int (InvisibleTypes) eInvisible, int iChange)")

		.def("getNumUnits", &CyPlot::getNumUnits, "int ()")
		.def("getUnit", &CyPlot::getUnit, python::return_value_policy<python::manage_new_object>(), "CyUnit* (int iIndex)")

		.def("getScriptData", &CyPlot::getScriptData, "str () - Get stored custom data")
		.def("setScriptData", &CyPlot::setScriptData, "void (str) - Set stored custom data")

//FfH: Added by Kael 08/15/2007
		.def("changePlotCounter", &CyPlot::changePlotCounter, "int (int iChange)")
		.def("setPlotCounter", &CyPlot::setPlotCounter, "int (int iNewValue)")
		.def("getPlotCounter", &CyPlot::getPlotCounter, "int ()")
		.def("getPortalExitX", &CyPlot::getPortalExitX, "int ()")
		.def("setPortalExitX", &CyPlot::setPortalExitX, "int (int iNewValue)")
		.def("getPortalExitY", &CyPlot::getPortalExitY, "int ()")
		.def("setPortalExitY", &CyPlot::setPortalExitY, "int (int iNewValue)")
		.def("isMoveDisabledAI", &CyPlot::isMoveDisabledAI, "bool ()")
		.def("setMoveDisabledAI", &CyPlot::setMoveDisabledAI, "void ()")
		.def("isMoveDisabledHuman", &CyPlot::isMoveDisabledHuman, "bool ()")
		.def("setMoveDisabledHuman", &CyPlot::setMoveDisabledHuman, "void ()")
		.def("isBuildDisabled", &CyPlot::isBuildDisabled, "bool ()")
		.def("setBuildDisabled", &CyPlot::setBuildDisabled, "void ()")
		.def("isFoundDisabled", &CyPlot::isFoundDisabled, "bool ()")
		.def("setFoundDisabled", &CyPlot::setFoundDisabled, "void ()")
		.def("isPythonActive", &CyPlot::isPythonActive, "bool ()")
		.def("setPythonActive", &CyPlot::setPythonActive, "void ()")
		.def("isAdjacentToWater", &CyPlot::isAdjacentToWater, "bool ()")
		.def("setMinLevel", &CyPlot::setMinLevel, "int ()")
		.def("getNumAnimalUnits", &CyPlot::getNumAnimalUnits, "int ()")
		.def("changeTempTerrainTimer", &CyPlot::changeTempTerrainTimer, "void (int iChange)")
		.def("setTempTerrainType", &CyPlot::setTempTerrainType, "void (TerrainTypes eNewValue, int iTimer)")
		.def("getRealTerrainType", &CyPlot::getRealTerrainType, "int ()")
		.def("getRealBonusType", &CyPlot::getRealBonusType, "int ()")
		.def("getExploreNextTurn", &CyPlot::getExploreNextTurn, "int ()")
		.def("setExploreNextTurn", &CyPlot::setExploreNextTurn, "void (int iNewValue)")

		.def("isVisibleToCivTeam", &CyPlot::isVisibleToCivTeam, "bool ()")
//FfH: End Add
/*************************************************************************************************/
/**	New Tag Defs	(TerrainInfos)			09/19/08								Jean Elcard	**/
/**	New Tag Defs	(PlotInfos)				12/31/08								Xienwolf	**/
/**								Exposes the Functions to Python									**/
/*************************************************************************************************/
		.def("getTempTerrainTimer", &CyPlot::getTempTerrainTimer, "int ()")
		.def("getMinLevel", &CyPlot::getMinLevel, "int ()")
		.def("setTempTerrainTypeFM", &CyPlot::setTempTerrainTypeFM, "void (TerrainTypes eNewValue, int iTimer, bool bRecalculateAreas, bool bRebuildPlot)")
		.def("setTempFeatureType", &CyPlot::setTempFeatureType, "void (FeatureTypes eFeature, int iVariety, int iTimer)")
		.def("setTempBonusType", &CyPlot::setTempBonusType, "void (BonusTypes eBonus, int iTimer)")
		.def("getNumLairSpawnsAlive", &CyPlot::getNumLairSpawnsAlive, "int ()")
		.def("changeNumLairSpawnsAlive", &CyPlot::changeNumLairSpawnsAlive, "void (int iChange)")
		.def("isNeedsRebuilding", &CyPlot::isNeedsRebuilding, "bool ()")
		.def("setNeedsRebuilding", &CyPlot::setNeedsRebuilding, "void (bool bNewValue)")
		.def("rebuildGraphics", &CyPlot::rebuildGraphics, "void ()")
//CivPlotMods:
		.def("calculateNatureYield", &CyPlot::calculatePlayerNatureYield, "int (int (YieldTypes) eYield, int (PlayerTypes) ePlayer, bool bIgnoreFeature)")
		.def("calculateBestNatureYield", &CyPlot::calculatePlayerBestNatureYield, "int (int (YieldTypes) eYield, int (PlayerTypes) ePlayer)")
		.def("calculateTotalBestNatureYield", &CyPlot::calculatePlayerTotalBestNatureYield, "int (int (PlayerTypes) ePlayer)")
		.def("canHaveImprovement", &CyPlot::canPlayerHaveImprovement, "bool (int (ImprovementTypes) eImprovement, int (PlayerTypes) ePlayer, bool bPotential)")
		.def("canHaveImprovement", &CyPlot::canTeamHaveImprovement, "bool (int (ImprovementTypes) eImprovement, int (TeamTypes) eTeam, bool bPotential)")
/*************************************************************************************************/
/**	New Tag Defs							END													**/
/*************************************************************************************************/
/*************************************************************************************************/
/**	Sidar Mist 								25/06/10								Grey Fox	**/
/*************************************************************************************************/
	//	.def("getPerceptionCost", &CyPlot::getPerceptionCost, "int ()")
	//	.def("setPerceptionCost", &CyPlot::setPerceptionCost, "void (int iNewValue)")
	//	.def("changePerceptionCost", &CyPlot::changePerceptionCost, "void (int iChange)")

	//	.def("getMistChangeTimer", &CyPlot::getMistChangeTimer, "int ()")
	//	.def("setMistChangeTimer", &CyPlot::setMistChangeTimer, "void (int iNewValue)")
	//	.def("changeMistChangeTimer", &CyPlot::changeMistChangeTimer, "void (int iChange)")

	//	.def("getMistChangeTemp", &CyPlot::getMistChangeTemp, "int ()")
	//	.def("setMistChangeTemp", &CyPlot::setMistChangeTemp, "void (int iNewValue)")
	//	.def("changeMistChangeTemp", &CyPlot::changeMistChangeTemp, "void (int iChange)")
/*************************************************************************************************/
/**	END                                                                   						**/
/*************************************************************************************************/

/*************************************************************************************************/
/** Exposing climate stuff					Opera												**/
/*************************************************************************************************/
		.def("getClimate", &CyPlot::getClimate, "int ()")
		.def("setClimate", &CyPlot::setClimate, "void (int (ClimateZoneTypes) eClimate)")
		.def("updateClimate", &CyPlot::updateClimate, "void ()")
		.def("getNaturalClimate", &CyPlot::getNaturalClimate, "int ()")
		.def("setNaturalClimate", &CyPlot::setNaturalClimate, "void (int (ClimateZoneTypes) eClimate)")
		.def("updateNaturalClimate", &CyPlot::updateNaturalClimate, "void ()")
		.def("getTemperature", &CyPlot::getTemperature, "int ()")
		.def("changeTemperature", &CyPlot::changeTemperature, "void (int iChange)")
		.def("setTemperature", &CyPlot::setTemperature, "void (int iTemperature)")
		.def("getHumidity", &CyPlot::getHumidity, "int ()")
		.def("changeHumidity", &CyPlot::changeHumidity, "void (int iChange)")
		.def("setHumidity", &CyPlot::setHumidity, "void (int iHumidity)")
		.def("getNaturalTemperature", &CyPlot::getNaturalTemperature, "int ()")
		.def("changeNaturalTemperature", &CyPlot::changeNaturalTemperature, "void (int iChange)")
		.def("setNaturalTemperature", &CyPlot::setNaturalTemperature, "void (int iNewValue)")
		.def("getNaturalHumidity", &CyPlot::getNaturalHumidity, "int ()")
		.def("changeNaturalHumidity", &CyPlot::changeNaturalHumidity, "void (int iChange)")
		.def("setNaturalHumidity", &CyPlot::setNaturalHumidity, "void (int iNewValue)")
		.def("getTemperatureStrain", &CyPlot::getTemperatureStrain, "int ()")
		.def("changeTemperatureStrain", &CyPlot::changeTemperatureStrain, "void (int iChange)")
		.def("setTemperatureStrain", &CyPlot::setTemperatureSrain, "void (int iValue)")
		.def("getHumidityStrain", &CyPlot::getHumidityStrain, "int ()")
		.def("changeHumidityStrain", &CyPlot::changeHumidityStrain, "void (int iChange)")
		.def("setHumidityStrain", &CyPlot::setHumidityStrain, "void (int iValue)")
		.def("getTemperatureTo", &CyPlot::getTemperatureTo, "int (int (ClimateZoneTypes) eClimate)")
		.def("getHumidityTo", &CyPlot::getHumidityTo, "int (int (ClimateZoneTypes) eClimate)")
		.def("getClimateMatch", &CyPlot::getClimateMatch, "int (int iTemperature, int iHumidity)")
		.def("getWantedClimate", &CyPlot::getWantedClimate, "int ()")
		.def("getNextClimate", &CyPlot::getNextClimate, "int ()")
		.def("getNextClimateTurnsLeft", &CyPlot::getNextClimateTurnsLeft, "int ()")
		.def("getScaledTemperatureThreshold", &CyPlot::getScaledTemperatureThreshold, "int ()")
		.def("getScaledHumidityThreshold", &CyPlot::getScaledHumidityThreshold, "int ()")
		.def("getTerrainClassType", &CyPlot::getTerrainClassType, "int ()")
		.def("setTerrainClassType", &CyPlot::setTerrainClassType, "void (int (TerrainClassTypes) eNewValue, bool bRecalculate, bool bRebuildGraphics)")
/*************************************************************************************************/
/** Exposing climate stuff					Opera												**/
/*************************************************************************************************/

	;
}
