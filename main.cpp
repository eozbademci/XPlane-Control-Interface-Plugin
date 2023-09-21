#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include <string.h>
#include "XPLMMenus.h"
#include "XPWidgets.h"
#include "XPStandardWidgets.h"
#include "XPLMDataAccess.h"

#include <stdio.h>

#if IBM
#include <windows.h>
#endif
#if LIN
#include <GL/gl.h>
#elif __GNUC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifndef XPLM300
#error This is made to be compiled against the XPLM300 SDK
#endif

// 2. Pencere tanýmlarý
XPLMWindowID gWindow = NULL;



// --------menu
int g_menu_container_idx; // The index of our menu item in the Plugins menu
XPLMMenuID g_menu_id; // The menu container we'll append all our menu items to
void menu_handler(void*, void*);

static int x = 300, y = 500, w = 390, h = 290;
int window_Width, window_Height;
float stateArmDisarm = 0.0, roll_degree = 0.0, pitch_degree = 0.0, yaw_degree = 0.0;
double lon_data = 0, lat_data = 0, alt_data = 0;
static void InterfaceEngineStarterWidget(int x1, int y1, int w, int h);
static void MyDrawWindowCallback(
	XPLMWindowID         inWindowID,
	void* inRefcon);


int MenuItem1;
int MenuItem2;

bool arm_disarmFlag = false;
XPLMDataRef gEngineSmokeDataRef = NULL;
XPLMDataRef gCounterDataRef = NULL;
XPLMDataRef gCounterDataRef2 = NULL;
XPLMDataRef gArmDataRef = NULL;
XPLMDataRef gPitchDataRef = NULL;
XPLMDataRef gRollDataRef = NULL;
XPLMDataRef gYawDataRef = NULL;
XPLMDataRef gLatDataRef = NULL;
XPLMDataRef gLongDataRef = NULL;
XPLMDataRef gAltDataRef = NULL;
XPLMDataRef gWidthDataRef = NULL;
XPLMDataRef gHeightDataRef = NULL;

XPWidgetID	IgnitersState = NULL;
XPWidgetID	ArmState = NULL;
XPWidgetID	DisarmState = NULL;
XPWidgetID	InterfaceStarterWidget = NULL, InterfaceStarterWindow = NULL;
XPWidgetID	visibleWidget = NULL;
XPWidgetID	Description = NULL;

XPWidgetID	StartButton = NULL;
XPWidgetID	ThrottleBar = NULL;
XPWidgetID	XPlaneStarterDurationEdit = NULL;
XPWidgetID	StarterDurationText = NULL;
XPWidgetID	StarterDurationEdit = NULL;
XPWidgetID	ReleaseDelayText = NULL;
XPWidgetID	ReleaseDelayEdit = NULL;


static int EngineStarterHandler(
	XPWidgetMessage			inMessage,
	XPWidgetID				inWidget,
	intptr_t				inParam1,
	intptr_t				inParam2);

static int DataInterfaceHandler(
	XPWidgetMessage			inMessage,
	XPWidgetID				inWidget,
	intptr_t				inParam1,
	intptr_t				inParam2);

PLUGIN_API int XPluginStart(
	char* outName,
	char* outSig,
	char* outDesc)
{


	strcpy(outName, "BasicControlPlugin");
	strcpy(outSig, "xpsdk.examples.BasicControlPlugin");
	strcpy(outDesc, "A Basic Control plug-in for the XPLM300 SDK.");

	gWidthDataRef = XPLMFindDataRef("sim/graphics/view/window_width");
	gHeightDataRef = XPLMFindDataRef("sim/graphics/view/window_height");
	gEngineSmokeDataRef = XPLMFindDataRef("sim/flightmodel/failures/smoking");
	gCounterDataRef = XPLMFindDataRef("sim/cockpit2/engine/actuators/throttle_ratio_all");
	gCounterDataRef2 = XPLMFindDataRef("BSUB/CounterDataRef");

	gRollDataRef = XPLMFindDataRef("sim/flightmodel/position/phi");
	gPitchDataRef = XPLMFindDataRef("sim/flightmodel/position/theta");
	gYawDataRef = XPLMFindDataRef("sim/flightmodel/position/psi");
	gLatDataRef = XPLMFindDataRef("sim/flightmodel/position/latitude");
	gLongDataRef = XPLMFindDataRef("sim/flightmodel/position/longitude");
	gAltDataRef = XPLMFindDataRef("sim/flightmodel/position/elevation");

	gArmDataRef = XPLMFindDataRef("sim/cockpit2/engine/actuators/mixture_ratio_all");
	g_menu_container_idx = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "GUI", 0, 0);
	g_menu_id = XPLMCreateMenu("GUI", XPLMFindPluginsMenu(), g_menu_container_idx, menu_handler, NULL);
	XPLMAppendMenuItem(g_menu_id, "Control Interface", (void*)"Menu Item 1", 1);
	XPLMAppendMenuSeparator(g_menu_id);
	XPLMAppendMenuItem(g_menu_id, "Data Interface", (void*)"Menu Item 2", 1);
	

	MenuItem1 = 0;
	MenuItem2 = 0;


	return 1;



}

PLUGIN_API void	XPluginStop(void)
{
	// Since we created the window, we'll be good citizens and clean it up
	if (MenuItem1 == 1)
	{
		XPDestroyWidget(InterfaceStarterWidget, 1);
		MenuItem1 = 0;
	}
	if (MenuItem2 == 1) {
		XPLMDestroyWindow(gWindow);
		XPDestroyWidget(visibleWidget, 1);
		MenuItem2 = 0;
	}
	
	
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int  XPluginEnable(void) { 
	
	
	return 1; }

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) { }



void menu_handler(void* in_menu_ref, void* in_item_ref)
{
	if (!strcmp((const char*)in_item_ref, "Menu Item 1"))
	{
		if (MenuItem1 == 0)
		{
			InterfaceEngineStarterWidget(x, y, w, h);
			MenuItem1 = 1;
		}
		else
			if (!XPIsWidgetVisible(InterfaceStarterWidget))
				XPShowWidget(InterfaceStarterWidget);
		
	}
	else if (!strcmp((const char*)in_item_ref, "Menu Item 2"))
	{
		if (MenuItem2 == 0) {
			window_Width = XPLMGetDatai(gWidthDataRef);
			window_Height = XPLMGetDatai(gHeightDataRef);
			gWindow = XPLMCreateWindow(
				0, window_Height, window_Width * 0.5, window_Height - (window_Height * 0.03),			/* Area of the window. */
				1,							/* Start visible. */
				MyDrawWindowCallback,		/* Callbacks */
				NULL,
				NULL,
				NULL);
			MenuItem2 = 1;
		}
		else {
			XPLMDestroyWindow(gWindow);
			MenuItem2 = 0;
		}
	}
	
}

void InterfaceEngineStarterWidget(int x, int y, int w, int h)
{
	int Item = 1;

	int x2 = x + w;
	int y2 = y - h;

	// Create the Main Widget window
	InterfaceStarterWidget = XPCreateWidget(x, y, x2, y2,
		1,	// Visible
		"Control Panel",	// desc
		1,		// root
		NULL,	// no container
		xpWidgetClass_MainWindow);

	// Add Close Box decorations to the Main Widget
	XPSetWidgetProperty(InterfaceStarterWidget, xpProperty_MainWindowHasCloseBoxes, 1);

	// Create the Sub Widget window
	InterfaceStarterWindow = XPCreateWidget(x + 10, y - 30, x2 - 10, y2 + 10,
		1,	// Visible
		"",	// desc
		0,		// root
		InterfaceStarterWidget,
		xpWidgetClass_SubWindow);

	// Set the style to sub window
	XPSetWidgetProperty(InterfaceStarterWindow, xpProperty_SubWindowType, xpSubWindowStyle_SubWindow);



	//-----------------------------------------------

	XPCreateWidget(x + 20, y - 40, x + 82, y - 62,
		1,	// Visible
		"Engine Smoke",// desc
		0,		// root
		InterfaceStarterWidget,
		xpWidgetClass_Caption);

	// Create a check box for the Igniters
	IgnitersState = XPCreateWidget(x + 105, y - 42, x + 127, y - 64,
		1, "", 0, InterfaceStarterWidget,
		xpWidgetClass_Button);

	// Set it to be check box
	XPSetWidgetProperty(IgnitersState, xpProperty_ButtonType, xpRadioButton);
	XPSetWidgetProperty(IgnitersState, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);
	XPSetWidgetProperty(IgnitersState, xpProperty_ButtonState, 0);

	XPCreateWidget(x + 20, y - 80, x + 82, y - 102,
		1,	// Visible
		"Throttle",// desc
		0,		// root
		InterfaceStarterWidget,
		xpWidgetClass_Caption);


	//Create scrollbar
	ThrottleBar = XPCreateWidget(x + 105, y - 77, x + 225, y - 107,
		1, "", 0, InterfaceStarterWidget,
		xpWidgetClass_ScrollBar);

	// Set it to be scrollbar
	XPSetWidgetProperty(ThrottleBar, xpProperty_ScrollBarType, xpScrollBarTypeScrollBar );
	XPSetWidgetProperty(ThrottleBar, xpProperty_ScrollBarSliderPosition, 500000);
	XPSetWidgetProperty(ThrottleBar, xpProperty_ScrollBarMin, 0);
	XPSetWidgetProperty(ThrottleBar, xpProperty_ScrollBarMax, 1000000);

	//arm disarm

	
	XPCreateWidget(x + 20, y - 130, x + 62, y - 152,
		1,	// Visible
		"Arm",// desc
		0,		// root
		InterfaceStarterWidget,
		xpWidgetClass_Caption);

	ArmState = XPCreateWidget(x + 55, y - 132, x + 77, y - 154,
		1, "", 0, InterfaceStarterWidget,
		xpWidgetClass_Button);

	// Set it to be check box
	XPSetWidgetProperty(ArmState, xpProperty_ButtonType, xpRadioButton);
	XPSetWidgetProperty(ArmState, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);	
	XPSetWidgetProperty(ArmState, xpProperty_ButtonState, 0);


	XPCreateWidget(x + 80, y - 130, x + 130, y - 152,
		1,	// Visible
		"Disarm",// desc
		0,		// root
		InterfaceStarterWidget,
		xpWidgetClass_Caption);

	DisarmState = XPCreateWidget(x + 133, y - 132, x + 155, y - 154,
		1, "", 0, InterfaceStarterWidget,
		xpWidgetClass_Button);

	// Set it to be check box
	XPSetWidgetProperty(DisarmState, xpProperty_ButtonType, xpRadioButton);
	XPSetWidgetProperty(DisarmState, xpProperty_ButtonBehavior, xpButtonBehaviorCheckBox);
	XPSetWidgetProperty(DisarmState, xpProperty_ButtonState, 1);


	StartButton = XPCreateWidget(x + 163, y - 130, x + 213, y - 152,
		1, " Start", 0, InterfaceStarterWidget,
		xpWidgetClass_Button);

	// Set it to be normal push button
	XPSetWidgetProperty(StartButton, xpProperty_ButtonType, xpPushButton);




	XPAddWidgetCallback(InterfaceStarterWidget, EngineStarterHandler);

}


int	EngineStarterHandler(
	XPWidgetMessage			inMessage,
	XPWidgetID				inWidget,
	intptr_t				inParam1,
	intptr_t				inParam2)
{
	char Buffer[255];
	
	int  State;
	float State_ThrottleBar;
	
	float StarterDuration, StarterDurationArray;
	
	State = (int)XPGetWidgetProperty(IgnitersState, xpProperty_ButtonState, 0);
	State_ThrottleBar = (float)XPGetWidgetProperty(ThrottleBar, xpProperty_ScrollBarSliderPosition, NULL);
	State_ThrottleBar = State_ThrottleBar * 0.000001;
	if (inMessage == xpMessage_CloseButtonPushed)
	{
		if (MenuItem1 == 1)
		{
			XPHideWidget(InterfaceStarterWidget);
			
		}
		return 1;
	}

	if (State == 1) {
		XPLMSetDatai(gEngineSmokeDataRef, 1);
		
	}
	else {
		XPLMSetDatai(gEngineSmokeDataRef, 0);

	}
	//// Handle any button pushes
	if (inMessage == xpMsg_ScrollBarSliderPositionChanged )
	{
		//XPLMSetDatavi(gEngineSmokeDataRef, 1);
		XPLMSetDataf(gCounterDataRef, State_ThrottleBar);
		
		
		
	}
	if (inMessage == xpMsg_ButtonStateChanged) {
		
		
		if ((int)XPGetWidgetProperty(DisarmState, xpProperty_ButtonState, 0) == 1) {
			if (arm_disarmFlag == true) {
				XPSetWidgetProperty(ArmState, xpProperty_ButtonState, 0);
				arm_disarmFlag = false;
				XPLMSetDatai(gEngineSmokeDataRef, 1);
			}
		}

		if ((int)XPGetWidgetProperty(ArmState, xpProperty_ButtonState, 0) == 1) {
			if (arm_disarmFlag == false) {
				XPSetWidgetProperty(DisarmState, xpProperty_ButtonState, 0);
				arm_disarmFlag = true;
			}
		}

		if ((int)XPGetWidgetProperty(ArmState, xpProperty_ButtonState, 0) == 0 && (int)XPGetWidgetProperty(DisarmState, xpProperty_ButtonState, 0) == 0) {
			if (arm_disarmFlag == false) {
				XPSetWidgetProperty(DisarmState, xpProperty_ButtonState, 1);

			}
			else {
				XPSetWidgetProperty(ArmState, xpProperty_ButtonState, 1);
			}
		}
	}

	
	
	if (inMessage == xpMsg_PushButtonPressed) {
		
		if (!arm_disarmFlag) {
			XPLMSetDataf(gArmDataRef, 0.0);
			
		}
		else {
			XPLMSetDataf(gArmDataRef, 1.0);
			
		}
	}

	
	return 0;
}

// 2. pencere



void MyDrawWindowCallback(
	XPLMWindowID         inWindowID,
	void* inRefcon)
{
	
	char	str[300];
	int		left, top, right, bottom;
	float	color[] = { 1.0, 1.0, 1.0 };
	
	/* First get our window's location. */
	XPLMGetWindowGeometry(inWindowID, &left, &top, &right, &bottom);

	/* Draw a translucent dark box as our window outline. */
	XPLMDrawTranslucentDarkBox(left, top, right, bottom);


	if (stateArmDisarm == 0.0) {
		float colors[] = { 1.0, 0.0, 0.0 };
		XPLMDrawString(colors, left + 5, top - 15, "DISARM", NULL, xplmFont_Basic);
	}
	else {
		float colors[] = { 0.0, 1.0, 0.0 };
		XPLMDrawString(colors, left + 5, top - 15, "ARM", NULL, xplmFont_Basic);
	}
	

	sprintf(str, " Roll : %f  |  Pitch : %f  |  Yaw : %f    ||    Latitude : %lf  |  Longitude : %lf  |  Altitude : %lf",
		XPLMGetDataf(gRollDataRef),
		XPLMGetDataf(gPitchDataRef),
		XPLMGetDataf(gYawDataRef),
		XPLMGetDatad(gLatDataRef),
		XPLMGetDatad(gLongDataRef),
		XPLMGetDatad(gAltDataRef)
		);
	
	///* Draw the string into the window. */


	XPLMDrawString(color, left + 45, top - 15, str, NULL, xplmFont_Basic);
	
	visibleWidget = XPCreateWidget(left, top, right, bottom,
		0,	// Visible
		"",	// desc
		1,		// root
		NULL,	// no container
		xpWidgetClass_MainWindow);
	XPCreateWidget(left, top, left + 30, bottom - 100,
		1,	// Visible
		"",// desc
		0,		// root
		visibleWidget,
		xpWidgetClass_Caption);

	XPAddWidgetCallback(visibleWidget, DataInterfaceHandler);
}

int	DataInterfaceHandler(
	XPWidgetMessage			inMessage,
	XPWidgetID				inWidget,
	intptr_t				inParam1,
	intptr_t				inParam2)
{
	if (MenuItem2 == 1) {
		window_Width = XPLMGetDatai(gWidthDataRef);
		window_Height = XPLMGetDatai(gHeightDataRef);
		stateArmDisarm = XPLMGetDataf(gArmDataRef);
		roll_degree = XPLMGetDataf(gRollDataRef);
		pitch_degree = XPLMGetDataf(gPitchDataRef);
		yaw_degree = XPLMGetDataf(gYawDataRef);
		lat_data = XPLMGetDatad(gLatDataRef);
		lon_data = XPLMGetDatad(gLongDataRef);
		alt_data = XPLMGetDatad(gAltDataRef);
		XPLMSetWindowGeometry(gWindow, 0, window_Height, window_Width * 0.5, window_Height - (window_Height * 0.03));
	}
	return 0;
}