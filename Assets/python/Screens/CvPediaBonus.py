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

class CvPediaBonus:
	"Civilopedia Screen for Bonus Resources"

	def __init__(self, main):
		self.iBonus = -1
		self.top = main

		self.FILTERS =	[
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNFILTERED", ())',		# Name of the filter as it appears in dropdown menu.  Needs to be unique for this screen
				"Purpose" : "Clears all currently active filters",		# Description for the sanity of those who modify this file, appears and is used nowhere else
				"Hardcoded" : False,		# If this is True, then only those items in the HardcodeList will pass the check
				"HardcodeList" : [],
				"Value to Check" : 'None',	# Note that eBonus is the BonusInfo object being tested
				"Desired Result" : 'None',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BONUS_LUXURIES", ())',
				"Purpose" : "Happiness-granting Resources",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'eBonus.getHappiness() > 0',
				"Desired Result" : 'True',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BONUS_HEALTH", ())',
				"Purpose" : "Health-granting Resources",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'eBonus.getHealth() > 0',
				"Desired Result" : 'True',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BONUS_MANA", ())',
				"Purpose" : "Mana Bonuses",
				"Hardcoded" : False,
				"HardcodeList" : [],
				"Value to Check" : 'eBonus.getBonusClassType() == gc.getInfoTypeForString("BONUSCLASS_MANA")',
				"Desired Result" : 'True',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BONUS_HELL", ())',
				"Purpose" : "Hell Terrain Only",
				"Hardcoded" : True,
				"HardcodeList" : [
					'BONUS_GULAGARM',
					'BONUS_NIGHTMARE',
					'BONUS_RAZORWEED',
					'BONUS_SHEUT_STONE',
					'BONUS_TOAD',],
				"Value to Check" : 'None',
				"Desired Result" : 'None',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BONUS_LIMITED", ())',
				"Purpose" : "Building, Civilization or Unique Feature only",
				"Hardcoded" : True,
				"HardcodeList" : [
					'BONUS_ALE',
					'BONUS_FINE_CLOTHES',
					'BONUS_FRUIT_OF_YGGDRASIL',
					'BONUS_PATRIAN_ARTIFACTS',
					'BONUS_PEARL',
					'BONUS_ASH',
					'BONUS_OBSIDIAN',],
				"Value to Check" : 'None',
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
				"Value to Sort" : 'eBonus.getDescription()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_FOOD", ())',
				"Purpose" : "Sort By Tile boost",
				"Value to Sort" : '-eBonus.getYieldChange(YieldTypes.YIELD_FOOD)',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_PRODUCTION_BONUS", ())',
				"Purpose" : "Sort By Tile boost",
				"Value to Sort" : '-eBonus.getYieldChange(YieldTypes.YIELD_PRODUCTION)',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_COMMERCE", ())',
				"Purpose" : "Sort By Tile boost",
				"Value to Sort" : '-eBonus.getYieldChange(YieldTypes.YIELD_COMMERCE)',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BONUS_HEALTH_SORT", ())',
				"Purpose" : "Sort By City Boost",
				"Value to Sort" : '-eBonus.getHealth()',
			},
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_BONUS_HAPPINESS", ())',
				"Purpose" : "Sort By City Boost",
				"Value to Sort" : '-eBonus.getHappiness()',
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
	def interfaceScreen(self, iBonus):
		self.iBonus = iBonus

		self.top.deleteAllWidgets()

		screen = self.top.getScreen()

		bNotActive = (not screen.isActive())
		if bNotActive:
			self.top.setPediaCommonWidgets()

		#Filter/Sort dropdowns
		self.top.FILTER_DROPDOWN_ID = self.top.getNextWidgetName()
		screen.addDropDownBoxGFC(self.top.FILTER_DROPDOWN_ID,self.top.X_FILTER_DROPDOWN, self.top.Y_DROPDOWN, self.top.W_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		for i, filter in enumerate(self.ALLOWED_FILTERS):
			screen.addPullDownString(self.top.FILTER_DROPDOWN_ID, eval(filter["name"]), i, i, filter == self.CURRENT_FILTER )

		self.top.SORT_DROPDOWN_ID = self.top.getNextWidgetName()
		screen.addDropDownBoxGFC(self.top.SORT_DROPDOWN_ID, self.top.X_SORT_DROPDOWN, self.top.Y_DROPDOWN, self.top.W_DROPDOWN, WidgetTypes.WIDGET_GENERAL, -1, -1, FontTypes.GAME_FONT)
		for i, sort in enumerate(self.ALLOWED_SORTS):
			screen.addPullDownString(self.top.SORT_DROPDOWN_ID, eval(sort["name"]), 1, 1, sort == self.CURRENT_SORT )

		# Header...
		szHeader = u"<font=4b>" + gc.getBonusInfo(self.iBonus).getDescription().upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS, iBonus)

		# Title (upper-right corner, above the list)
		szHeader = u"<font=3b>" + localText.getText("TXT_KEY_PEDIA_CATEGORY_BONUS", ()).upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.W_SCREEN -self.top.W_LINKS/2, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS, -1)

		# Top
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.top.X_MENU, self.top.Y_MENU, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS, -1)

		if self.top.iLastScreen	!= CvScreenEnums.PEDIA_BONUS or bNotActive:
			self.placeLinks(true)
			self.top.iLastScreen = CvScreenEnums.PEDIA_BONUS
		else:
			self.placeLinks(false)

		# Icon
		self.X_BONUS_PANE = self.top.EXT_SPACING
		self.Y_BONUS_PANE = 55
		self.W_BONUS_PANE = 433
		self.H_BONUS_PANE = 210
		self.X_ICON = 48
		self.Y_ICON = 90
		self.W_ICON = 150
		self.H_ICON = 150
		self.ICON_SIZE = 64
		screen.addPanel( self.top.getNextWidgetName(), "", "", False, False,
			self.X_BONUS_PANE, self.Y_BONUS_PANE, self.W_BONUS_PANE, self.H_BONUS_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", false, false,
			self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getBonusInfo(self.iBonus).getButton(),
			self.X_ICON + self.W_ICON/2 - self.ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - self.ICON_SIZE/2, self.ICON_SIZE, self.ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		# Bonus animation
		self.X_BONUS_ANIMATION = self.X_BONUS_PANE + self.W_BONUS_PANE + self.top.INT_SPACING
		self.Y_BONUS_ANIMATION = 63
		self.W_BONUS_ANIMATION = 303
		self.H_BONUS_ANIMATION = 200
		self.X_ROTATION_BONUS_ANIMATION = -20
		self.Z_ROTATION_BONUS_ANIMATION = 30
		self.SCALE_ANIMATION = 0.6
		screen.addBonusGraphicGFC(self.top.getNextWidgetName(), self.iBonus,
			self.X_BONUS_ANIMATION, self.Y_BONUS_ANIMATION, self.W_BONUS_ANIMATION, self.H_BONUS_ANIMATION, WidgetTypes.WIDGET_GENERAL, -1, -1, self.X_ROTATION_BONUS_ANIMATION, self.Z_ROTATION_BONUS_ANIMATION, self.SCALE_ANIMATION, True)

		self.placeStats()
		self.placeImprovements()
		self.placeEffects()
		self.placeRequires()
		self.placeBuildings()
		self.placeAllows()
		self.placeHistory()
		self.placeStrategy()

		return

	def placeStats(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_STATS_PANE = 200
		self.Y_STATS_PANE = 150
		self.W_STATS_PANE = 240
		self.H_STATS_PANE = 200
		screen.addListBoxGFC(panelName, "", self.X_STATS_PANE, self.Y_STATS_PANE, self.W_STATS_PANE, self.H_STATS_PANE, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panelName, False)

		for k in range(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = gc.getBonusInfo(self.iBonus).getYieldChange(k)
			if (iYieldChange != 0):
				if (iYieldChange > 0):
					sign = "+"
				else:
					sign = ""

				szYield = (u"%s: %s%i " % (gc.getYieldInfo(k).getDescription(), sign, iYieldChange))
				screen.appendListBoxString(panelName, u"<font=4>" + szYield.upper() + (u"%c" % gc.getYieldInfo(k).getChar()) + u"</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)

	def placeImprovements(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_IMPROVEMENTS_PANE = self.top.EXT_SPACING
		self.Y_IMPROVEMENTS_PANE = self.Y_BONUS_PANE + self.H_BONUS_PANE
		self.W_IMPROVEMENTS_PANE = 300
		self.H_IMPROVEMENTS_PANE = self.top.H_SCREEN-self.top.H_BOT_BAR-self.top.EXT_SPACING-self.Y_IMPROVEMENTS_PANE - self.top.H_BLUE50_PANEL # 1 panel under this one
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_CATEGORY_IMPROVEMENT", ()), "", true, true,
				 self.X_IMPROVEMENTS_PANE, self.Y_IMPROVEMENTS_PANE, self.W_IMPROVEMENTS_PANE, self.H_IMPROVEMENTS_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		bonusInfo = gc.getBonusInfo(self.iBonus)

		for j in range(gc.getNumImprovementInfos()):
			bFirst = True
			szYield = u""
			bEffect = False
			if (gc.getImprovementInfo(j).isGraphicalOnly()):
				continue
			for k in range(YieldTypes.NUM_YIELD_TYPES):
				iYieldChange = gc.getImprovementInfo(j).getImprovementBonusYield(self.iBonus, k)
				if (iYieldChange != 0):
					bEffect = True
			if (bEffect):
				for k in range(YieldTypes.NUM_YIELD_TYPES):
					iYieldChange = gc.getImprovementInfo(j).getImprovementBonusYield(self.iBonus, k)
					iYieldChange += gc.getImprovementInfo(j).getYieldChange(k)
					if (iYieldChange > 0):
						if (bFirst):
							bFirst = False
						else:
							szYield += ", "
						szYield += (u"%i%c" % (iYieldChange, gc.getYieldInfo(k).getChar()))
				childPanelName = self.top.getNextWidgetName()
				screen.attachPanel(panelName, childPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)

				screen.attachLabel(childPanelName, "", "  ")
				screen.attachImageButton(childPanelName, "", gc.getImprovementInfo(j).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, j, 1, False )
				screen.attachLabel(childPanelName, "", szYield)

	def placeEffects(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_EFFECTS_PANE = self.X_IMPROVEMENTS_PANE + self.W_IMPROVEMENTS_PANE + self.top.INT_SPACING
		self.Y_EFFECTS_PANE = self.Y_BONUS_PANE + self.H_BONUS_PANE
		self.W_EFFECTS_PANE = self.X_BONUS_ANIMATION + self.W_BONUS_ANIMATION - self.X_EFFECTS_PANE
		self.H_EFFECTS_PANE = self.top.H_SCREEN-self.top.H_BOT_BAR-self.top.EXT_SPACING-self.Y_EFFECTS_PANE - 3 * self.top.H_BLUE50_PANEL # 3 panels under this one
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", true, false,
				 self.X_EFFECTS_PANE, self.Y_EFFECTS_PANE, self.W_EFFECTS_PANE, self.H_EFFECTS_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		listName = self.top.getNextWidgetName()
		screen.attachListBoxGFC( panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY )
		screen.enableSelect(listName, False)

		szSpecialText = CyGameTextMgr().getBonusHelp(self.iBonus, True)
		splitText = string.split( szSpecialText, "\n" )
		for special in splitText:
			if len( special ) != 0:
				screen.appendListBoxString( listName, special, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_REQUIRES = self.X_EFFECTS_PANE
		self.Y_REQUIRES = self.Y_EFFECTS_PANE + self.H_EFFECTS_PANE
		self.W_REQUIRES = self.W_EFFECTS_PANE
		self.H_REQUIRES = self.top.H_BLUE50_PANEL
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", false, true,
				 self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		iTech = gc.getBonusInfo(self.iBonus).getTechReveal()
		if (iTech > -1):
			screen.attachImageButton( panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
			screen.attachLabel(panelName, "", u"(" + localText.getText("TXT_KEY_PEDIA_BONUS_APPEARANCE", ()) + u")")
		iTech = gc.getBonusInfo(self.iBonus).getTechCityTrade()
		if (iTech > -1):
			screen.attachImageButton( panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
			screen.attachLabel(panelName, "", u"(" + localText.getText("TXT_KEY_PEDIA_BONUS_TRADE", ()) + u")")

	def placeBuildings(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_BUILDINGS = self.X_REQUIRES
		self.Y_BUILDINGS = self.Y_REQUIRES + self.H_REQUIRES
		self.W_BUILDINGS = self.top.X_LINKS - self.X_BUILDINGS - self.top.EXT_SPACING
		self.H_BUILDINGS = self.top.H_BLUE50_PANEL
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_BONUS_MENU_GIVEN_BY", ()), "", false, true, self.X_BUILDINGS, self.Y_BUILDINGS, self.W_BUILDINGS, self.H_BUILDINGS, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		for iBuilding in range(gc.getNumBuildingInfos()): # buildings can have up to 3 bonuses
			bonus1 = gc.getBuildingInfo(iBuilding).getFreeBonus()
			bonus2 = gc.getBuildingInfo(iBuilding).getFreeBonus2()
			bonus3 = gc.getBuildingInfo(iBuilding).getFreeBonus3()
			if self.iBonus in [bonus1,bonus2,bonus3]:
				screen.attachImageButton( panelName, "", gc.getBuildingInfo(iBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, iBuilding, 1, False )

	def placeAllows(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_ALLOWS_PANE = self.top.EXT_SPACING
		self.Y_ALLOWS_PANE = self.Y_IMPROVEMENTS_PANE + self.H_IMPROVEMENTS_PANE
		self.W_ALLOWS_PANE = self.top.X_LINKS - self.X_ALLOWS_PANE - self.top.EXT_SPACING
		self.H_ALLOWS_PANE = self.top.H_BLUE50_PANEL
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_ALLOWS", ()), "", false, true,
				 self.X_ALLOWS_PANE, self.Y_ALLOWS_PANE, self.W_ALLOWS_PANE, self.H_ALLOWS_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		# add unit buttons
		for eLoopUnit in range(gc.getNumUnitInfos()):
			bFound = False
			if (eLoopUnit >= 0):
				if (gc.getUnitInfo(eLoopUnit).getPrereqAndBonus() == self.iBonus):
					bFound = True
				else:
					j = 0
					while (not bFound and j < gc.getNUM_UNIT_PREREQ_OR_BONUSES()):
						if (gc.getUnitInfo(eLoopUnit).getPrereqOrBonuses(j) == self.iBonus):
							bFound = True
						j += 1
			if bFound:
				szButton = gc.getUnitInfo(eLoopUnit).getButton()
				if self.top.iActivePlayer != -1:
					szButton = gc.getPlayer(self.top.iActivePlayer).getUnitButton(eLoopUnit)
				screen.attachImageButton( panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, eLoopUnit, 1, False )

		for eLoopBuilding in range(gc.getNumBuildingInfos()):
			bFound = False
			if (gc.getBuildingInfo(eLoopBuilding).getPrereqAndBonus() == self.iBonus):
				bFound = True
			else:
				j = 0
				while (not bFound and j < gc.getNUM_BUILDING_PREREQ_OR_BONUSES()):
					if (gc.getBuildingInfo(eLoopBuilding).getPrereqOrBonuses(j) == self.iBonus):
						bFound = True
					j += 1
			if bFound:
				screen.attachImageButton( panelName, "", gc.getBuildingInfo(eLoopBuilding).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, eLoopBuilding, 1, False )

	def placeHistory(self):
		screen = self.top.getScreen()
		self.X_HISTORY = self.X_BONUS_ANIMATION + self.W_BONUS_ANIMATION + self.top.INT_SPACING
		self.Y_HISTORY = self.top.H_TOP_BAR + self.top.INT_SPACING

		self.W_HISTORY = self.top.X_LINKS - self.X_HISTORY - self.top.EXT_SPACING
		# the remaining vertical space under the logo is split in 2. upper-half --> History ; bottom-half --> Strategy
		self.H_HISTORY = int(0.5*(self.Y_BUILDINGS - self.Y_HISTORY))
		HistoryPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryPanel, localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()), "", True, True, self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, PanelStyles.PANEL_STYLE_BLUE50 )
		# InterfaceUpgrade: Better Pedia - Added by Grey Fox 04/18/2008
		HistoryTextPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryTextPanel, "", "", true, true,self.X_HISTORY+self.top.HM_TEXT, self.Y_HISTORY+self.top.VM_TEXT, self.W_HISTORY - 2 * self.top.HM_TEXT, self.H_HISTORY - self.top.VM_TEXT, PanelStyles.PANEL_STYLE_EMPTY)
		# InterfaceUpgrade: Better Pedia - End Add
		HistoryText = gc.getBonusInfo(self.iBonus).getCivilopedia()
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
		StrategyText = gc.getBonusInfo(self.iBonus).getStrategy()
		screen.attachMultilineText( StrategyTextPanel, "", StrategyText,WidgetTypes.WIDGET_GENERAL,-1,-1, CvUtil.FONT_LEFT_JUSTIFY )

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()

		if bRedraw:
			screen.clearListBoxGFC(self.top.LIST_ID)

		listSorted = self.getSortedList()

		i = 0
		iSelected = 0
		for iI in range(len(listSorted)):
			iBonus = listSorted[iI][2]
			szName = listSorted[iI][3]
			if bRedraw:
				screen.appendListBoxString(self.top.LIST_ID, szName, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if iBonus == self.iBonus:
				iSelected = i
			i += 1

		screen.setSelectedListBoxStringGFC(self.top.LIST_ID, iSelected)

	def getSortedList(self):
		listBonuses = []
		iCount = 0
		for iBonus in range(gc.getNumBonusInfos()):
			eBonus = gc.getBonusInfo(iBonus)
			if not eBonus.isGraphicalOnly():
				if self.CURRENT_FILTER["Hardcoded"]:
					for bonus in self.CURRENT_FILTER["HardcodeList"]:
						if iBonus == gc.getInfoTypeForString(bonus):
							listBonuses.append(iBonus)
							iCount += 1
				elif eval(self.CURRENT_FILTER["Value to Check"]) == eval(self.CURRENT_FILTER["Desired Result"]):
					listBonuses.append(iBonus)
					iCount += 1

		listSorted = [(0,0,0,0,0,0)] * iCount
		iI = 0
		for iBonus in listBonuses:
			eBonus = gc.getBonusInfo(iBonus)
			sort1 = 0
			sort2 = 0
			if not self.CURRENT_SORT["Value to Sort"] == None:
				sort1 = eval(self.CURRENT_SORT["Value to Sort"])
				if self.CURRENT_SORT["Value to Sort"] == 'eBonus.getDescription()':
					sort1 = CvUtil.removeAccent(self,sort1) # alphabetical sorting in the Pedia for languages using accents/diacritics
				if not self.SUB_SORT["Value to Sort"] == None:
					sort2 = eval(self.SUB_SORT["Value to Sort"])
			listSorted[iI] = (sort1, sort2, iBonus, eBonus.getDescription(), eBonus.getButton(), 1)
			iI += 1
		listSorted.sort()

		return listSorted

	def applyFilterSort(self, filter, sort):
		if not self.CURRENT_SORT == sort:
			self.SUB_SORT = self.CURRENT_SORT
			self.CURRENT_SORT = sort
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS, True)

		if not self.CURRENT_FILTER == filter:
			self.CURRENT_FILTER = filter
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_BONUS, True)

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			screen = self.top.getScreen()
			iFilterIndex = screen.getSelectedPullDownID(self.top.FILTER_DROPDOWN_ID)
			iSortIndex = screen.getSelectedPullDownID(self.top.SORT_DROPDOWN_ID)
			self.applyFilterSort(self.ALLOWED_FILTERS[iFilterIndex], self.ALLOWED_SORTS[iSortIndex])
			return 1

		return 0