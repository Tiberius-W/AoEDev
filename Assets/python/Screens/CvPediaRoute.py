## Sid Meier's Civilization 4
## Copyright Firaxis Games 2005
from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums

# globals
gc = CyGlobalContext()
ArtFileMgr = CyArtFileMgr()
localText = CyTranslator()

class CvPediaRoute:
	"Civilopedia Screen for Route"

	def __init__(self, main):
		self.iRoute = -1
		self.top = main

		self.X_UPPER_PANE = 20
		self.Y_UPPER_PANE = 65
		self.W_UPPER_PANE = 750
		self.H_UPPER_PANE = 210

		self.X_ICON = 165
		self.Y_ICON = 100
		self.W_ICON = 150
		self.H_ICON = 150
		self.ICON_SIZE = 64

		self.X_ROUTES_PANE = self.X_UPPER_PANE
		self.Y_ROUTES_PANE = self.Y_UPPER_PANE + self.H_UPPER_PANE + 20
		self.W_ROUTES_PANE = 500
		self.H_ROUTES_PANE = 250

		self.X_BONUS_YIELDS_PANE = self.X_ROUTES_PANE + self.W_ROUTES_PANE + 25
		self.Y_BONUS_YIELDS_PANE = self.Y_UPPER_PANE + self.H_UPPER_PANE + 20
		self.W_BONUS_YIELDS_PANE = 230
		self.H_BONUS_YIELDS_PANE = 402

		self.X_REQUIRES = self.X_UPPER_PANE
		self.Y_REQUIRES = self.Y_ROUTES_PANE + self.H_ROUTES_PANE + 20
		self.W_REQUIRES = 500
		self.H_REQUIRES = 110

		self.X_HISTORY = self.X_UPPER_PANE
		self.Y_HISTORY = self.Y_REQUIRES + self.H_REQUIRES + 20
		self.W_HISTORY = 500
		self.H_HISTORY = 120

		self.FILTERS =	[
			{
				"name" : 'localText.getText("TXT_KEY_PEDIA_FILTER_UNFILTERED", ())',		# Name of the filter as it appears in dropdown menu.  Needs to be unique for this screen
				"Purpose" : "Clears all currently active filters",		# Description for the sanity of those who modify this file, appears and is used nowhere else
				"Hardcoded" : False,		# If this is True, then only those items in the HardcodeList will pass the check
				"HardcodeList" : [],
				"Value to Check" : 'None',	# Note that eRoute is the RouteInfo object being tested
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
				"Value to Sort" : 'eRoute.getDescription()',
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
	def interfaceScreen(self, iRoute):

		self.iRoute = iRoute

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
		sImpDesc = gc.getRouteInfo(self.iRoute).getDescription()
		sImpDescCaps = CvUtil.repairedUpper(sImpDesc)
		szHeader = u"<font=4b>" + sImpDescCaps + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.X_SCREEN, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)

		# Title (upper-right corner, above the list)
		szHeader = u"<font=3b>" + localText.getText("TXT_KEY_PEDIA_CATEGORY_ROUTE", ()).upper() + u"</font>"
		szHeaderId = self.top.getNextWidgetName()
		screen.setLabel(szHeaderId, "", szHeader, CvUtil.FONT_CENTER_JUSTIFY, self.top.W_SCREEN -self.top.W_LINKS/2, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, CivilopediaPageTypes.CIVILOPEDIA_PAGE_ROUTE, -1)

		# Top
		link = CivilopediaPageTypes.CIVILOPEDIA_PAGE_ROUTE
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_LEFT_JUSTIFY, self.top.X_MENU, self.top.Y_MENU, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, link, -1)

		if self.top.iLastScreen	!= CvScreenEnums.PEDIA_ROUTE or bNotActive:
			self.placeLinks(true)
			self.top.iLastScreen = CvScreenEnums.PEDIA_ROUTE
		else:
			self.placeLinks(false)

		# Icon
		screen.addPanel( self.top.getNextWidgetName(), "", "", False, False,
			self.X_UPPER_PANE, self.Y_UPPER_PANE, self.W_UPPER_PANE, self.H_UPPER_PANE, PanelStyles.PANEL_STYLE_BLUE50)
		screen.addPanel(self.top.getNextWidgetName(), "", "", false, false,
			self.X_ICON, self.Y_ICON, self.W_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getRouteInfo(self.iRoute).getButton(),
			self.X_ICON + self.W_ICON/2 - self.ICON_SIZE/2, self.Y_ICON + self.H_ICON/2 - self.ICON_SIZE/2, self.ICON_SIZE, self.ICON_SIZE, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		self.placeRequires()
		self.placeRoutes()
		self.placeBonusMovement()
#		self.placeHistory()

	def placeRequires(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.Y_REQUIRES = self.Y_UPPER_PANE + self.H_UPPER_PANE
		self.H_REQUIRES = self.top.H_BLUE50_PANEL
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_REQUIRES", ()), "", false, true,
				 self.X_REQUIRES, self.Y_REQUIRES, self.W_REQUIRES, self.H_REQUIRES, PanelStyles.PANEL_STYLE_BLUE50 )

		screen.attachLabel(panelName, "", "  ")
		
		# add resource buttons
		bFirst = True
		iPrereq = gc.getRouteInfo(self.iRoute).getPrereqBonus()
		if (iPrereq >= 0):
			bFirst = False
			screen.attachImageButton( panelName, "", gc.getBonusInfo(iPrereq).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iPrereq, -1, False )

		# count the number of OR resources
		nOr = 0
		for j in range(gc.getDefineINT("NUM_ROUTE_PREREQ_OR_BONUSES")):
			if (gc.getRouteInfo(self.iRoute).getPrereqOrBonus(j) > -1):
				nOr += 1

		szLeftDelimeter = ""
		szRightDelimeter = ""
		#  Display a bracket if we have more than one OR resource and an AND resource
		if (not bFirst):
			if (nOr > 1):
				szLeftDelimeter = localText.getText("TXT_KEY_AND", ()) + "( "
				szRightDelimeter = " ) "
			elif (nOr > 0):
				szLeftDelimeter = localText.getText("TXT_KEY_AND", ())

		if len(szLeftDelimeter) > 0:
			screen.attachLabel(panelName, "", szLeftDelimeter)

		bFirst = True
		for j in range(gc.getDefineINT("NUM_ROUTE_PREREQ_OR_BONUSES")):
			eBonus = gc.getRouteInfo(self.iRoute).getPrereqOrBonus(j)
			if (eBonus > -1):
				if (not bFirst):
					screen.attachLabel(panelName, "", localText.getText("TXT_KEY_OR", ()))
				else:
					bFirst = False
				screen.attachImageButton( panelName, "", gc.getBonusInfo(eBonus).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, eBonus, -1, False )

		if len(szRightDelimeter) > 0:
			screen.attachLabel(panelName, "", szRightDelimeter)

		
	#	lBonus = []
	#	iBonusPrereq = gc.getRouteInfo(self.iRoute).getPrereqBonus()
	#	if iBonusPrereq != -1:
	#		lBonus.append(iBonusPrereq)
	#	for i in xrange(gc.getDefineINT("NUM_ROUTE_PREREQ_OR_BONUSES")):
	#		iBonusPrereqOr = gc.getRouteInfo(self.iRoute).getPrereqOrBonus(i)
	#		if iBonusPrereqOr != -1 and iBonusPrereqOr != iBonusPrereq:
	#			lBonus.append(iBonusPrereqOr)
	#	if lBonus:
	#		for iBonus in lBonus:
	#			screen.attachImageButton( panelName, "", gc.getBonusInfo(iBonus).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, 1, False )

	def placeRoutes(self):
		screen = self.top.getScreen()

		panelName = self.top.getNextWidgetName()
		self.Y_ROUTES_PANE = self.Y_REQUIRES + self.H_REQUIRES
		self.H_ROUTES_PANE = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - self.Y_ROUTES_PANE
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_CATEGORY_ROUTE", ()), "", true, true,
						 self.X_ROUTES_PANE, self.Y_ROUTES_PANE, self.W_ROUTES_PANE, self.H_ROUTES_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		sText = u""
		sText += localText.getText("TXT_KEY_UNIT_FLAT_MOVEMENT", ())
		sText += ": 1 / " + str( gc.getMOVE_DENOMINATOR() / gc.getRouteInfo(self.iRoute).getFlatMovementCost () ) + u" %c" %(CyGame().getSymbolID(FontSymbols.MOVES_CHAR)) + u"\n"
		sText += localText.getText("TXT_KEY_ROUTE_MOVEMENT", ())
		sText += ": 1 / " + str( gc.getMOVE_DENOMINATOR() / gc.getRouteInfo(self.iRoute).getMovementCost () ) + u" %c" %(CyGame().getSymbolID(FontSymbols.MOVES_CHAR)) + u"\n"
		childPanelName = self.top.getNextWidgetName()
		screen.addMultilineText(childPanelName, sText, self.X_ROUTES_PANE+5, self.Y_ROUTES_PANE+40, self.W_ROUTES_PANE-10, self.H_ROUTES_PANE-15, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeBonusMovement(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.Y_BONUS_YIELDS_PANE = self.Y_REQUIRES
		self.H_BONUS_YIELDS_PANE = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - self.Y_BONUS_YIELDS_PANE
		screen.addPanel( panelName, localText.getText("TXT_KEY_PEDIA_ROUTE_TECH_CHANGE", ()), "", True, True,
				 self.X_BONUS_YIELDS_PANE, self.Y_BONUS_YIELDS_PANE, self.W_BONUS_YIELDS_PANE, self.H_BONUS_YIELDS_PANE, PanelStyles.PANEL_STYLE_BLUE50 )

		info = gc.getRouteInfo(self.iRoute)

		for iTech in xrange(gc.getNumTechInfos()):
			if info.getTechMovementChange(iTech) != 0:
				szChange = "  1 / " + str( gc.getMOVE_DENOMINATOR() / ( info.getTechMovementChange(iTech) + gc.getRouteInfo(self.iRoute).getMovementCost () ) ) + u" %c" %(CyGame().getSymbolID(FontSymbols.MOVES_CHAR))
				childPanelName = self.top.getNextWidgetName()
				screen.attachPanel(panelName, childPanelName, "", "", False, False, PanelStyles.PANEL_STYLE_EMPTY)
				screen.attachLabel(childPanelName, "", "  ")
				screen.attachImageButton( childPanelName, "", gc.getTechInfo(iTech).getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TECH, iTech, 1, False )
				screen.attachLabel(childPanelName, "", u"<font=4>" + szChange + u"</font>")



	def placeHistory(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.X_HISTORY = self.X_BONUS_YIELDS_PANE + self.W_BONUS_YIELDS_PANE + self.top.INT_SPACING
		self.Y_HISTORY = self.top.H_TOP_BAR + self.top.INT_SPACING
		self.W_HISTORY = self.top.X_LINKS - self.X_HISTORY - self.top.EXT_SPACING
		self.H_HISTORY = self.top.H_SCREEN - self.top.H_BOT_BAR - self.top.EXT_SPACING - self.Y_HISTORY

		screen.addPanel( panelName, localText.getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()), "", true, false,
				 self.X_HISTORY, self.Y_HISTORY, self.W_HISTORY, self.H_HISTORY, PanelStyles.PANEL_STYLE_BLUE50 )

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()

		if bRedraw:
			screen.clearListBoxGFC(self.top.LIST_ID)

		listSorted = self.getSortedList()

		i = 0
		iSelected = 0
		for iI in range(len(listSorted)):
			iRoute = listSorted[iI][2]
			szName = listSorted[iI][3]
			if bRedraw:
				screen.appendListBoxString(self.top.LIST_ID, szName, WidgetTypes.WIDGET_PEDIA_JUMP_TO_ROUTE, iRoute, -1, CvUtil.FONT_LEFT_JUSTIFY)
			if iRoute == self.iRoute:
				iSelected = i
			i += 1

		screen.setSelectedListBoxStringGFC(self.top.LIST_ID, iSelected)

	def getRouteSortedList(self):
		listRoutes = []
		iCount = 0
		for iRoute in range(gc.getNumRouteInfos()):
			if (not gc.getRouteInfo(iRoute).isGraphicalOnly()):
				listRoutes.append(iRoute)
				iCount += 1

		listSorted = [(0,0)] * iCount
		iI = 0
		for iRoute in listRoutes:
			listSorted[iI] = (gc.getRouteInfo(iRoute).getDescription(), iRoute)
			iI += 1
		listSorted.sort()
		return listSorted

	def getSortedList(self):
		listRoutes = []
		iCount = 0
		for iRoute in range(gc.getNumRouteInfos()):
			eRoute = gc.getRouteInfo(iRoute)
			if not eRoute.isGraphicalOnly():
				if self.CURRENT_FILTER["Hardcoded"]:
					for route in self.CURRENT_FILTER["HardcodeList"]:
						if iRoute == gc.getInfoTypeForString(route):
							listRoutes.append(iRoute)
							iCount += 1
				elif eval(self.CURRENT_FILTER["Value to Check"]) == eval(self.CURRENT_FILTER["Desired Result"]):
					listRoutes.append(iRoute)
					iCount += 1

		listSorted = [(0,0,0,0,0,0)] * iCount
		iI = 0
		for iRoute in listRoutes:
			eRoute = gc.getRouteInfo(iRoute)
			sort1 = 0
			sort2 = 0
			if not self.CURRENT_SORT["Value to Sort"] == None:
				sort1 = eval(self.CURRENT_SORT["Value to Sort"])
				sort1 = CvUtil.removeAccent(self,sort1) # alphabetical sorting in the Pedia for languages using accents/diacritics
				if not self.SUB_SORT["Value to Sort"] == None:
					sort2 = eval(self.SUB_SORT["Value to Sort"])
			listSorted[iI] = (sort1, sort2, iRoute, eRoute.getDescription(), eRoute.getButton(), 1)
			iI += 1
		listSorted.sort()

		return listSorted

	def applyFilterSort(self, filter, sort):
		if not self.CURRENT_SORT == sort:
			self.SUB_SORT = self.CURRENT_SORT
			self.CURRENT_SORT = sort
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_ROUTE, True)

		if not self.CURRENT_FILTER == filter:
			self.CURRENT_FILTER = filter
			self.top.pediaJump(CvScreenEnums.PEDIA_MAIN, CivilopediaPageTypes.CIVILOPEDIA_PAGE_ROUTE, True)

	# Will handle the input for this screen...
	def handleInput (self, inputClass):
		if (inputClass.getNotifyCode() == NotifyCode.NOTIFY_LISTBOX_ITEM_SELECTED):
			screen = self.top.getScreen()
			iFilterIndex = screen.getSelectedPullDownID(self.top.FILTER_DROPDOWN_ID)
			iSortIndex = screen.getSelectedPullDownID(self.top.SORT_DROPDOWN_ID)
			self.applyFilterSort(self.ALLOWED_FILTERS[iFilterIndex], self.ALLOWED_SORTS[iSortIndex])
			return 1

		return 0