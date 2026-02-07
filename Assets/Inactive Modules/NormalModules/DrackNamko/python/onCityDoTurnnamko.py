
from CvPythonExtensions import *
import CvUtil
import CvScreensInterface
import CvDebugTools
import CvWBPopups
import PyHelpers
import Popup as PyPopup
import CvCameraControls
import CvTopCivs
import sys
import CvWorldBuilderScreen
import CvAdvisorUtils
import CvTechChooser
import pickle # required for loads

import CvIntroMovieScreen
import CustomFunctions

def getNamkoMaxPopulation(pCity):
	gc                  = CyGlobalContext() 
	git					= gc.getInfoTypeForString
	getN				= pCity.getNumBuilding
	iMax = 3
	bPalace				= git('BUILDING_PALACE_NAMKO')
	bHousings			= git('BUILDING_NAMKO_ARISEN_HOUSINGS')
	bRecreation			= git('BUILDING_NAMKO_RECREATION_FURNACE')
	bFortress			= git('BUILDING_NAMKO_ARISEN_FORTRESS')
	bMageQuarters		= git('BUILDING_NAMKO_MAGE_QUARTERS')
	bTemple				= git('BUILDING_NAMKO_TEMPLE_OF_ETERNITY')
	bSoulExchange		= git('BUILDING_NAMKO_MONEYCHANGER')
	bLibrary			= git('BUILDING_NAMKO_LIBRARY')
	isCapital = getN(bPalace)

	if isCapital:
		iMax += 2

	# Certain Buildings increases Max Size
	# Max Size: 5 (9 if Capital)
	if getN(bHousings) == 1:
		iMax += 2
		if isCapital:
			iMax += 2
	# Max Size: 7 (11 if Capital)
	if getN(bSoulExchange) == 1:
		iMax += 2
		if isCapital:
			iMax += 2
   
	# Max Size: 8 (13 if Capital)
	if getN(bLibrary) == 1:
		iMax += 1
		if isCapital:
			iMax += 1

	# If Capital, Size is increased further if Furnace was build
	# MaxSize: 22 (38 if Capital)
	if getN(bRecreation):
		if isCapital:
			if getN(bFortress) == 1:
				iMax += 6
			if getN(bMageQuarters) == 1:
				iMax += 5
			if getN(bTemple) == 1:
				iMax += 12
		else:
			if getN(bFortress) == 1:
				iMax += 4
			if getN(bMageQuarters) == 1:
				iMax += 3
			if getN(bTemple) == 1:
				iMax += 7

	# Base Cities Max Size = 8
	# Furnace City Max Size = 22
	# Capital with Furnace Max Size = 38

	return iMax

def isUnhappy(pCity):
	iHappy = pCity.happyLevel()
	iUnhappy = pCity.unhappyLevel(0)

	if iHappy - iUnhappy <= 0:
		return True
	else:
		return False

def onCityDoTurn(self, argsList):
	'City Production'
	pCity = argsList[0]
	iPlayer = argsList[1]
	gc                  = CyGlobalContext() 
	pPlayer             = gc.getPlayer(iPlayer)
	iCiv                = pPlayer.getCivilizationType()
	Civ                 = gc.getInfoTypeForString('CIVILIZATION_NAMKO')
	pPopulation = pCity.getPopulation()
	currentCityCounter = pCity.getCityCounter()

	if(iCiv != Civ):
		return
	
	# Calculate if population already has reached limit
	iMax = getNamkoMaxPopulation(pCity)

	# If Too Big
	if pPopulation > iMax:
		pCity.changePopulation(-1)
		pCity.setCityCounter(0)

	# If Unhappy, no increase
	if(isUnhappy(pCity)):
		pCity.setCityCounter(0)
		return

	# If Max Size already reached, no Increase
	if pPopulation == iMax:
		pCity.setCityCounter(0)
		return
	
	# For Small Cities, increase quickly
	if pPopulation < 3:
		pCity.changePopulation(1)
		pCity.setCityCounter(0)
		return

	# Increase City Size if Counter reached
	if pPopulation < 15:
		# Increase only on every 3 turn
		if currentCityCounter >= 2:
			pCity.changePopulation(1)
			pCity.setCityCounter(0)
			return
	else:
		# Increase only on every 5 turn
		if currentCityCounter >= 4:
			pCity.changePopulation(1)
			pCity.setCityCounter(0)
			return
	
	pCity.changeCityCounter(1)