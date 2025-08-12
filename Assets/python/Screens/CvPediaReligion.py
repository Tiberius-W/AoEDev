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

class CvPediaReligion:
	"Civilopedia Screen for Religions"

	def __init__(self, main):
		self.iReligion = -1
		self.top = main
		
		self.X_MAIN_PANE = 50
		self.Y_MAIN_PANE = 90
		self.W_MAIN_PANE = 250
		self.H_MAIN_PANE = 210

		self.X_ICON = 98
		self.Y_ICON = 125
		self.W_ICON = 150
		self.H_ICON = 150
		self.ICON_SIZE = 64

	# Screen construction function
	def interfaceScreen(self, iReligion):
		self.iReligion = iReligion
		self.top.deleteAllWidgets()
		screen = self.top.getScreen()
		bNotActive = (not screen.isActive())
		if bNotActive:
			self.top.setPediaCommonWidgets()

		# Header...
		szHeader = u"<font=4b>" + gc.getReligionInfo(self.iReligion).getDescription().upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		
		# Top
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.top.X_MENU, self.top.Y_MENU, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_RELIGION, -1)

		if self.top.iLastScreen	!= CvScreenEnums.PEDIA_RELIGION or bNotActive:
			self.placeLinks(true)
			self.top.iLastScreen = CvScreenEnums.PEDIA_RELIGION
		else:
			self.placeLinks(false)

		# Icon
		screen.addPanel( self.top.getNextWidgetName(), "", "", False, False,
		    self.X_MAIN_PANE, self.Y_MAIN_PANE, self.W_MAIN_PANE, self.H_MAIN_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", false, false,
		    self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getReligionInfo(self.iReligion).getButton(),
		    self.X_ICON + self.W_ICON/2 - self.ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - self.ICON_SIZE/2, self.ICON_SIZE, self.ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		self.placeRequires()
		self.placeSpecial()
		self.placeHistory()
		self.placeUnits()
		self.placeStrategy()

	def placeRequires(self):
		screen = self.top.getScreen()
		self.X_REQUIRES = 330
		self.Y_REQUIRES = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.W_REQUIRES = 150
		self.H_REQUIRES = self.top.H_BLUE50_PANEL
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", false, true,
				 self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50 )
		screen.attachLabel(panelName, "", "  ")
		
		iTech = gc.getReligionInfo(self.iReligion).getTechPrereq()
		if (iTech > -1):
			screen.attachImageButton( panelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
			
	def placeSpecial(self):
		screen = self.top.getScreen()
		self.X_SPECIAL = self.X_REQUIRES + self.W_REQUIRES + self.top.INT_SPACING
		self.Y_SPECIAL = self.Y_REQUIRES
		self.W_SPECIAL = self.top.X_LINKS - self.X_SPECIAL - self.top.EXT_SPACING
		self.H_SPECIAL = self.top.H_BLUE50_PANEL * 2
		panelName = self.top.getNextWidgetName()
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_EFFECTS", ()), "", true, false,
				 self.X_SPECIAL, self.Y_SPECIAL, self.W_SPECIAL, self.H_SPECIAL, PanelStyles.PANEL_STYLE_BLUE50 )
				
		listName = self.top.getNextWidgetName()
		screen.attachListBoxGFC( panelName, listName, "", TableStyles.TABLE_STYLE_EMPTY )
		screen.enableSelect(listName, False)
		
		szSpecialText = CyGameTextMgr().parseReligionInfo(self.iReligion, True)
		splitText = string.split( szSpecialText, "\n" )
		for special in splitText:
			if len( special ) != 0:
				screen.appendListBoxString( listName, special, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY )

	def placeHistory(self):
		screen = self.top.getScreen()
		self.X_HISTORY = self.X_MAIN_PANE
		self.Y_HISTORY = self.Y_MAIN_PANE + self.H_MAIN_PANE + 20
		self.W_HISTORY = (self.top.X_LINKS - self.X_HISTORY - self.top.INT_SPACING - self.top.EXT_SPACING)/2
		self.H_HISTORY = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - self.Y_HISTORY
		HistoryPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryPanel, localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()), "", True, True,
						self.X_HISTORY, self.Y_HISTORY,self.W_HISTORY, self.H_HISTORY,
						PanelStyles.PANEL_STYLE_BLUE50 )
		HistoryTextPanel = self.top.getNextWidgetName()
		screen.addPanel( HistoryTextPanel, "", "", true, true,self.X_HISTORY+self.top.HM_TEXT, self.Y_HISTORY+self.top.VM_TEXT, self.W_HISTORY - 2 * self.top.HM_TEXT, self.H_HISTORY - self.top.VM_TEXT, PanelStyles.PANEL_STYLE_EMPTY)
		HistoryText = gc.getReligionInfo(self.iReligion).getCivilopedia()
		screen.attachMultilineText( HistoryTextPanel, "", HistoryText, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	# Place units...
	def placeUnits(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_UNIT_PANE = self.X_HISTORY + self.W_HISTORY + self.top.INT_SPACING
		self.Y_UNIT_PANE = self.Y_HISTORY
		self.W_UNIT_PANE = self.top.X_LINKS - self.X_UNIT_PANE - self.top.EXT_SPACING
		self.H_UNIT_PANE = self.top.H_BLUE50_PANEL
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_UNITS_ENABLED", ()), "", false, true, self.X_UNIT_PANE, self.Y_UNIT_PANE, self.W_UNIT_PANE, self.H_UNIT_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")

		for eLoopUnit in range(gc.getNumUnitInfos()):
			if (eLoopUnit != -1):
				iPrereq = gc.getUnitInfo(eLoopUnit).getPrereqReligion()
				if (iPrereq == self.iReligion):
					szButton = gc.getUnitInfo(eLoopUnit).getButton()
					if self.top.iActivePlayer != -1:
						szButton = gc.getPlayer(self.top.iActivePlayer).getUnitButton(eLoopUnit)
					screen.attachImageButton( panelName, "", szButton, GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, eLoopUnit, 1, False )

	def placeStrategy(self):
		screen = self.top.getScreen()
		self.X_STRATEGY = self.X_UNIT_PANE
		self.Y_STRATEGY = self.Y_UNIT_PANE + self.H_UNIT_PANE
		self.W_STRATEGY = self.W_UNIT_PANE
		self.H_STRATEGY = self.H_HISTORY - self.top.H_BLUE50_PANEL
		StrategyPanel = self.top.getNextWidgetName()
		screen.addPanel( StrategyPanel, localText.getText("TXT_KEY_STRATEGY", ()), "", true, true,self.X_STRATEGY, self.Y_STRATEGY, self.W_STRATEGY, self.H_STRATEGY, PanelStyles.PANEL_STYLE_BLUE50 )
		StrategyTextPanel = self.top.getNextWidgetName()
		screen.addPanel( StrategyTextPanel, "", "", true, true,self.X_STRATEGY+self.top.HM_TEXT, self.Y_STRATEGY+self.top.VM_TEXT, self.W_STRATEGY - 2 * self.top.HM_TEXT, self.H_STRATEGY - self.top.VM_TEXT, PanelStyles.PANEL_STYLE_EMPTY)
		StrategyText = gc.getReligionInfo(self.iReligion).getStrategy()
		screen.attachMultilineText( StrategyTextPanel, "", StrategyText,WidgetTypes.WIDGET_GENERAL,-1,-1, CvUtil.FONT_LEFT_JUSTIFY )

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()
		if bRedraw:
			screen.clearListBoxGFC(self.top.LIST_ID)
		
		# sort Improvements alphabetically
		listSorted=[(0,0)]*gc.getNumReligionInfos()
		for j in range(gc.getNumReligionInfos()):
			listSorted[j] = (gc.getReligionInfo(j).getDescription(), j)
		listSorted.sort()
		iSelected = 0
		i = 0
		for iI in range(gc.getNumReligionInfos()):
			if (not gc.getReligionInfo(iI).isGraphicalOnly()):
				if bRedraw:
					screen.appendListBoxString( self.top.LIST_ID, listSorted[iI][0], WidgetTypes.WIDGET_PEDIA_JUMP_TO_RELIGION, listSorted[iI][1], 0, CvUtil.FONT_LEFT_JUSTIFY )
				if listSorted[iI][1] == self.iReligion:
					iSelected = i
				i += 1
		screen.setSelectedListBoxStringGFC(self.top.LIST_ID, iSelected)

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		return 0


