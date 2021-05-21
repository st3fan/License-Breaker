// =====================================================================================================================

//
// File        : SetupView.cpp
// Project     : LicenseBreaker
// Author      : Stefan Arentz, stefan.arentz@soze.com
// Version     : $Id$
// Environment : BeOS 4.5/Intel
//

// =====================================================================================================================

#include <stdio.h>

#include <be/support/SupportDefs.h>

#include <be/interface/StringView.h>
#include <be/interface/Slider.h>
#include <be/interface/View.h>
#include <be/interface/Font.h>

#include "SetupView.h"

// =====================================================================================================================

SetupView::SetupView(BRect frame, const char *name, int32* ioScrollSpeed)
	:	BView(frame, name, 0, B_FOLLOW_ALL),
		mScrollSpeed(ioScrollSpeed)
{
	mSpeedSlider = NULL;
}

// =====================================================================================================================

void SetupView::AttachedToWindow()
{
	// Use the same background color as the screensaver does

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Speed slider

	mSpeedSlider = new BSlider
		(
			BRect( 10, 10, 240, 40 ),
			"const:slider1",
			"Scroll Speed",
			new BMessage('sped'),
			0, 1000
		);
	
	mSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	mSpeedSlider->SetHashMarkCount(10);
	mSpeedSlider->SetLimitLabels("Slow", "Fast");
	
	mSpeedSlider->SetValue(*mScrollSpeed);
	
	mSpeedSlider->SetTarget(this);
	this->AddChild(mSpeedSlider);

	// Credits

	this->AddChild
		(
			new BStringView
				(
					BRect(10, 220, 240, 240),
					B_EMPTY_STRING,
					"(C) 1999 Stefan Arentz / stefan@soze.com"
				)
		);
}

// =====================================================================================================================

void SetupView::MessageReceived(BMessage *msg)
{	
	switch(msg->what)
	{
		case 'sped':
			*mScrollSpeed = mSpeedSlider->Value() + 1;
			break;

		default :
			BView::MessageReceived(msg);
			break;
	}
}

// =====================================================================================================================
