## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums
import string

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvPediaCityClass:
	"Civilopedia Screen for City Classes"

	def __init__(self, main):
		self.iCityClass = -1
		self.top = main

		self.FILTERS =	[
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNFILTERED", ())',		# Name of the filter as it appears in dropdown menu.  Needs to be unique for this screen
				"Purpose" : "Clears all currently active filters",		# Description for the sanity of those who modify this file, appears and is used nowhere else
				"Hardcoded" : False,		# If this is True, then only those items in the HardcodeList will pass the check
				"HardcodeList" : [],
				"Value to Check" : 'None',	# Note that eClass is the CityClassInfo object being tested
				"Desired Result" : 'None',
			},
			]

		# List the filters which you want to be available initially, or self.FILTERS to have all of them available from the start
		self.ALLOWED_FILTERS = self.FILTERS
		self.CURRENT_FILTER = self.FILTERS[0]

		self.SORTS =	[
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_ALPHABETICAL", ())',
				"Purpose" : "Default Sorting Method",
				"Value to Sort" : 'eClass.getShortDescription()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_XML_ORDER", ())',
				"Purpose" : "Default, unsorted method",
				"Value to Sort" : None,
			},
			]

		# List the sorts which you want to be available initially, or self.SORTS to have all of them available from the start
		self.ALLOWED_SORTS = self.SORTS
		self.CURRENT_SORT = self.SORTS[0]
		self.SUB_SORT = self.SORTS[0]

	# Screen construction function
	def interfaceScreen(self, iCityClass):
		self.iCityClass = iCityClass

		self.top.deleteAllWidgets()

		screen = self.top.getScreen()

		bNotActive = (not screen.isActive())
		if bNotActive:
			self.top.setPediaCommonWidgets()

		#Filter/Sort dropdowns
		self.top.FILTER_DROPDOWN_ID = self.top.getNextWidgetName()
		screen.addDropDownBoxGFC(self.top.FILTER_DROPDOWN_ID, self.top.X_FILTER_DROPDOWN, self.top.Y_DROPDOWN, self.top.W_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		for i, filter in enumerate(self.ALLOWED_FILTERS):
			screen.addPullDownString(self.top.FILTER_DROPDOWN_ID, eval(filter["name"]), i, i, filter == self.CURRENT_FILTER )

		self.top.SORT_DROPDOWN_ID = self.top.getNextWidgetName()
		screen.addDropDownBoxGFC(self.top.SORT_DROPDOWN_ID, self.top.X_SORT_DROPDOWN, self.top.Y_DROPDOWN, self.top.W_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		for i, sort in enumerate(self.ALLOWED_SORTS):
			screen.addPullDownString(self.top.SORT_DROPDOWN_ID, eval(sort["name"]), 1, 1, sort == self.CURRENT_SORT )

		# Header...
		szHeader = u"<font=4b>" + gc.getCityClassInfo(self.iCityClass).getShortDescription().upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Top
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.top.X_MENU, self.top.Y_MENU, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CITYCLASS, -1)

		if self.top.iLastScreen	!= CvScreenEnums.PEDIA_CITYCLASS or bNotActive:
			self.placeLinks(true)
			self.top.iLastScreen = CvScreenEnums.PEDIA_CITYCLASS
		else:
			self.placeLinks(false)

		self.placeBuilding()
		self.placeUnit()
		self.placeBlockedBuilding()
		self.placeBlockedUnit()
		self.placeEffects()
		self.placeHistory()
		self.placeStrategy()
		return

	def placeBuilding(self):
		screen = self.top.getScreen()
		self.X_SPLIT = ( self.top.X_LINKS - 512 ) / 2
		self.X_BUILDING = self.top.EXT_SPACING
		self.Y_BUILDING = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.W_BUILDING = self.X_SPLIT - self.X_BUILDING - self.top.INT_SPACING
		self.H_BUILDING = self.top.H_BLUE50_PANEL
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_CIV_MENU_UNIQUE_BUILDINGS", ()), "", false, true,
				self.X_BUILDING, self.Y_BUILDING, self.W_BUILDING, self.H_BUILDING, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.setPanelColor(panelName, 0, 100, 0)

		for iBuildingClass in range(gc.getNumBuildingClassInfos()):
			iUniqueBuilding = gc.getCityClassInfo(self.iCityClass).getCityClassBuildings(iBuildingClass);
			iDefaultBuilding = gc.getBuildingClassInfo(iBuildingClass).getDefaultBuildingIndex();
			if (iDefaultBuilding > -1 and iUniqueBuilding > -1 and iDefaultBuilding != iUniqueBuilding) or (iUniqueBuilding > -1 and gc.getBuildingClassInfo(iBuildingClass).isUnique()):
				if not gc.getBuildingInfo(iUniqueBuilding).isGraphicalOnly(): # hide graphical only buildings
					screen.attachImageButton( panelName, "", gc.getBuildingInfo(iUniqueBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iUniqueBuilding, 1, False )

	def placeUnit(self):
		screen = self.top.getScreen()
		self.X_UNIT = self.top.EXT_SPACING
		self.Y_UNIT = self.Y_BUILDING + self.H_BUILDING
		self.W_UNIT = self.W_BUILDING
		self.H_UNIT = self.top.H_BLUE50_PANEL
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_FREE_UNITS", ()), "", false, true,
				 self.X_UNIT, self.Y_UNIT, self.W_UNIT, self.H_UNIT, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.setPanelColor(panelName, 0, 100, 0)

		# List all UnitCombats
		git = gc.getInfoTypeForString
		iRecon     = git('UNITCOMBAT_RECON')
		iArcher    = git('UNITCOMBAT_ARCHER')
		iMounted   = git('UNITCOMBAT_MOUNTED')
		iMelee     = git('UNITCOMBAT_MELEE')
		iSiege     = git('UNITCOMBAT_SIEGE')
		iAdept     = git('UNITCOMBAT_ADEPT')
		iDisciple  = git('UNITCOMBAT_DISCIPLE')
		iAnimal    = git('UNITCOMBAT_ANIMAL')
		iNaval     = git('UNITCOMBAT_NAVAL')
		iBeast     = git('UNITCOMBAT_BEAST')
		iWorker    = git('UNITCOMBAT_WORKER')
		iCommander = git('UNITCOMBAT_COMMANDER')
		iRogue     = git('UNITCOMBAT_ROGUE')
		iDefense   = git('UNITCOMBAT_DEFENSIVE_MELEE')
		
		# Create a dictionary to store Unique Units
		dictUnique = {
			"WorldUnit"   : {1:[],2:[],3:[],4:[]},
			iRecon        : {1:[],2:[],3:[],4:[]},
			iArcher       : {1:[],2:[],3:[],4:[]},
			iMounted      : {1:[],2:[],3:[],4:[]},
			iMelee        : {1:[],2:[],3:[],4:[]},
			iSiege        : {1:[],2:[],3:[],4:[]},
			iAdept        : {1:[],2:[],3:[],4:[]},
			iDisciple     : {1:[],2:[],3:[],4:[]},
			iAnimal       : {1:[],2:[],3:[],4:[]},
			iNaval        : {1:[],2:[],3:[],4:[]},
			iBeast        : {1:[],2:[],3:[],4:[]},
			iWorker       : {1:[],2:[],3:[],4:[]},
			iCommander    : {1:[],2:[],3:[],4:[]},
			iRogue        : {1:[],2:[],3:[],4:[]},
			iDefense      : {1:[],2:[],3:[],4:[]},
			"noUnitCombat": {1:[],2:[],3:[],4:[]},
			"noTier"      : {1:[]}
		}
		
		# World units 1st, then normal UnitCombats, then units missing either UnitCombat or Tier
		listOrdered = ["WorldUnit", iMelee, iDefense, iRecon, iRogue, iMounted, iArcher, iSiege, iAdept, iDisciple, iCommander, iWorker, iNaval, iAnimal, iBeast, "noUnitCombat","noTier"]
		bPipe = False
		listUnique = []
		
		# Find and sort Unique Units
		for iUnit in range(gc.getNumUnitClassInfos()):
			iUniqueUnit = gc.getCityClassInfo(self.iCityClass).getCityClassUnits(iUnit);
			iDefaultUnit = gc.getUnitClassInfo(iUnit).getDefaultUnitIndex();
			if (iDefaultUnit > -1 and iUniqueUnit > -1 and iDefaultUnit != iUniqueUnit) or (iUniqueUnit > -1 and gc.getUnitClassInfo(iUnit).isUnique()):
				if not iUniqueUnit in listUnique:# If there is a duplicate unit, we ignore it
					listUnique.append(iUniqueUnit)
					info = gc.getUnitInfo(iUniqueUnit)
					iTier = info.getTier()
					if iTier in [1,2,3,4]:
						if isWorldUnitClass(info.getUnitClassType()):
							dictUnique["WorldUnit"][iTier].append(iUniqueUnit)
						else:#if not a World Unit
							iUnitCombat = info.getUnitCombatType()
							if iUnitCombat in listOrdered:
								dictUnique[iUnitCombat][iTier].append(iUniqueUnit)
							else:#if noUnitCombat (golems...)
								dictUnique["noUnitCombat"][iTier].append(iUniqueUnit)
					else:#if noTier (fort commanders and forgotten units: Mother, baby spider...)
							dictUnique["noTier"][1].append(iUniqueUnit)
		
		# Show Unique Units in the specified order
		for keyUnitCombat in listOrdered:
			dictUnitCombat = dictUnique[keyUnitCombat]
			for keyTier in dictUnitCombat:
				listTier = dictUnitCombat[keyTier]
				if listTier:
					if bPipe:
						screen.attachLabel(panelName, "", "|")
						bPipe = False
					for iUniqueUnit in listTier:
						szButton = gc.getUnitInfo(iUniqueUnit).getButton()
						if self.top.iActivePlayer != -1:
							szButton = gc.getPlayer(self.top.iActivePlayer).getUnitButton(iUniqueUnit)
						screen.attachImageButton( panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iUniqueUnit, 1, False )
			bPipe = True

	def placeBlockedBuilding(self):
		screen = self.top.getScreen()
		self.X_XBUILDING = self.top.EXT_SPACING
		self.Y_XBUILDING = self.Y_UNIT + self.H_UNIT
		self.W_XBUILDING = self.W_BUILDING
		self.H_XBUILDING = self.top.H_BLUE50_PANEL
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_CIV_MENU_BLOCKED_BUILDINGS", ()), "", False, True, self.X_XBUILDING, self.Y_XBUILDING, self.W_XBUILDING, self.H_XBUILDING, PanelStyles.PANEL_STYLE_BLUE50)
		screen.setPanelColor(panelName, 206, 65, 69)
		#screen.attachLabel(panelName, "", "  ")

		for iBuildingClass in range(gc.getNumBuildingClassInfos()):
			iUniqueBuilding = gc.getCityClassInfo(self.iCityClass).getCityClassBuildings(iBuildingClass);
			iDefaultBuilding = gc.getBuildingClassInfo(iBuildingClass).getDefaultBuildingIndex();
			if iDefaultBuilding != BuildingTypes.NO_BUILDING and not gc.getBuildingClassInfo(iBuildingClass).isUnique() and not gc.getBuildingInfo(iDefaultBuilding).isGraphicalOnly():
				if iUniqueBuilding == BuildingTypes.NO_BUILDING:
					szButton = gc.getBuildingInfo(iDefaultBuilding).getButton()
					screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iDefaultBuilding, 1, False)

	def placeBlockedUnit(self):
		screen = self.top.getScreen()
		self.X_XUNIT = self.top.EXT_SPACING
		self.Y_XUNIT = self.Y_XBUILDING + self.H_XBUILDING
		self.W_XUNIT = self.W_BUILDING
		self.H_XUNIT = self.top.H_BLUE50_PANEL
		panelName = self.top.getNextWidgetName()
		screen.addPanel(panelName, localText.getText("TXT_KEY_PEDIA_CIV_MENU_BLOCKED_UNITS", ()), "", False, True, self.X_XUNIT, self.Y_XUNIT, self.W_XUNIT, self.H_XUNIT, PanelStyles.PANEL_STYLE_BLUE50)
		screen.setPanelColor(panelName, 206, 65, 69)
		#screen.attachLabel(panelName, "", "  ")

		for iUnitClass in range(gc.getNumUnitClassInfos()):
			iUniqueUnit = gc.getCityClassInfo(self.iCityClass).getCityClassUnits(iUnitClass);
			iDefaultUnit = gc.getUnitClassInfo(iUnitClass).getDefaultUnitIndex();
			if iDefaultUnit != UnitTypes.NO_UNIT and not gc.getUnitClassInfo(iUnitClass).isUnique():
				if iUniqueUnit == UnitTypes.NO_UNIT:
					szButton = gc.getUnitInfo(iDefaultUnit).getButton()
					screen.attachImageButton(panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, iDefaultUnit, 1, False)

	def placeEffects(self): # Do we need that?
		screen = self.top.getScreen()
		self.X_EFFECTS = self.top.EXT_SPACING
		self.Y_EFFECTS = self.Y_XUNIT + self.H_XUNIT
		self.H_EFFECTS = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - self.Y_EFFECTS
		self.W_EFFECTS = self.W_BUILDING
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", true, false,
				 self.X_EFFECTS, self.Y_EFFECTS, self.W_EFFECTS, self.H_EFFECTS, PanelStyles.PANEL_STYLE_BLUE50 )
		#screen.attachLabel(panelName, "", " ")

		szName = self.top.getNextWidgetName()
		szText = ""
		screen.attachMultilineText( panelName, szName, szText, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def placeHistory(self):
		screen = self.top.getScreen()

		self.X_HISTORY = self.X_SPLIT
		self.Y_HISTORY = self.Y_BUILDING
		self.W_HISTORY = self.top.X_LINKS - self.X_HISTORY - self.top.EXT_SPACING
		# the remaining vertical space under the logo is split in 2. upper-half --> History ; bottom-half --> Strategy
		self.H_HISTORY = int(0.5 * (self.top.H_SCREEN-self.top.H_BOT_BAR-self.top.EXT_SPACING-self.Y_HISTORY))
		HistoryPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryPanel, localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()), "", True, True, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, PanelStyles.PANEL_STYLE_BLUE50 )
		# InterfaceUpgrade: Better Pedia - Added by Grey Fox 04/18/2008
		HistoryTextPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryTextPanel, "", "", true, true,self.X_HISTORY+self.top.HM_TEXT, self.Y_HISTORY+self.top.VM_TEXT, self.W_HISTORY - 2 * self.top.HM_TEXT, self.H_HISTORY - self.top.VM_TEXT, PanelStyles.PANEL_STYLE_EMPTY)
		# InterfaceUpgrade: Better Pedia - End Add
		HistoryText = gc.getCityClassInfo(self.iCityClass).getCivilopedia()
		screen.attachMultilineText( HistoryTextPanel, "", HistoryText, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeStrategy(self):
		screen = self.top.getScreen()
		self.X_STRATEGY = self.X_HISTORY
		self.Y_STRATEGY = self.Y_HISTORY + self.H_HISTORY
		self.W_STRATEGY = self.W_HISTORY
		self.H_STRATEGY = self.H_HISTORY
		StrategyPanel = self.top.getNextWidgetName()
		screen.addPanel( StrategyPanel, localText.getText("TXT_KEY_STRATEGY", ()), "", true, true,self.X_STRATEGY, self.Y_STRATEGY, self.W_STRATEGY, self.H_STRATEGY, PanelStyles.PANEL_STYLE_BLUE50 )
		StrategyTextPanel = self.top.getNextWidgetName()
		screen.addPanel( StrategyTextPanel, "", "", true, true,self.X_STRATEGY+self.top.HM_TEXT, self.Y_STRATEGY+self.top.VM_TEXT, self.W_STRATEGY - 2 * self.top.HM_TEXT, self.H_STRATEGY - self.top.VM_TEXT, PanelStyles.PANEL_STYLE_EMPTY)
		StrategyText = gc.getCityClassInfo(self.iCityClass).getStrategy()
		screen.attachMultilineText( StrategyTextPanel, "", StrategyText,WidgetTypes.WIDGET_GENERAL,-1,-1, CvUtil.FONT_LEFT_JUSTIFY )

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()

		if bRedraw:
			screen.clearListBoxGFC(self.top.LIST_ID)

		listSorted = self.getSortedList()

		i = 0
		iSelected = 0
		for iI in range(len(listSorted)):
			iCityClass = listSorted[iI][2]
			szName = listSorted[iI][3]
			if bRedraw:
				screen.appendListBoxString(self.top.LIST_ID, szName, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CITYCLASS, iCityClass, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if iCityClass == self.iCityClass:
				iSelected = i
			i += 1

		screen.setSelectedListBoxStringGFC(self.top.LIST_ID, iSelected)

	def getSortedList(self):
		listCityClasses = []
		iCount = 0
		for iClass in range(gc.getNumCityClassInfos()):
			eClass = gc.getCityClassInfo(iClass)
			if not eClass.isGraphicalOnly():
				if self.CURRENT_FILTER["Hardcoded"]:
					for cityclass in self.CURRENT_FILTER["HardcodeList"]:
						if iClass == gc.getInfoTypeForString(cityclass):
							listCityClasses.append(iClass)
							iCount += 1
				elif eval(self.CURRENT_FILTER["Value to Check"]) == eval(self.CURRENT_FILTER["Desired Result"]):
					listCityClasses.append(iClass)
					iCount += 1

		listSorted = [(0,0,0,0,0,0)] * iCount
		iI = 0
		for iClass in listCityClasses:
			eClass = gc.getCityClassInfo(iClass)
			sort1 = 0
			sort2 = 0
			if not self.CURRENT_SORT["Value to Sort"] == None:
				sort1 = eval(self.CURRENT_SORT["Value to Sort"])
				if not self.SUB_SORT["Value to Sort"] == None:
					sort2 = eval(self.SUB_SORT["Value to Sort"])
			listSorted[iI] = (sort1, sort2, iClass, eClass.getShortDescription(), eClass.getButton(), 1)
			iI += 1
		listSorted.sort()
		return listSorted

	def applyFilterSort(self, filter, sort):
		if not self.CURRENT_SORT == sort:
			self.SUB_SORT = self.CURRENT_SORT
			self.CURRENT_SORT = sort
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CITYCLASS, True)

		if not self.CURRENT_FILTER == filter:
			self.CURRENT_FILTER = filter
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_CITYCLASS, True)

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			screen = self.top.getScreen()
			iFilterIndex = screen.getSelectedPullDownID(self.top.FILTER_DROPDOWN_ID)
			iSortIndex = screen.getSelectedPullDownID(self.top.SORT_DROPDOWN_ID)
			self.applyFilterSort(self.ALLOWED_FILTERS[iFilterIndex], self.ALLOWED_SORTS[iSortIndex])
			return 1

		return 0