// =====================================================================================================================

//
// File        : LicenseBreaker.cpp
// Project     : LicenseBreaker
// Author      : Stefan Arentz, stefan.arentz@soze.com
// Version     : $Id$
// Environment : BeOS 4.5/Intel
//

// =====================================================================================================================

#include <ScreenSaver.h>
#include "ElfUtils.h"

// =====================================================================================================================

class SetupView;

// =====================================================================================================================

class LicenseBreaker : public BScreenSaver
{
	public:
	
		LicenseBreaker(BMessage *message, image_id id);

		status_t SaveState(BMessage *into) const;
		void StartConfig(BView *view);
		status_t StartSaver(BView *v, bool preview);
		void Draw(BView *v, int32 frame);
		
	private:
	
		void Print(char* inString);
		void RotateHighColor(void);
		
		BView* mSetupView;
		BView* mView;
		
		int mHighColor;
		
		int mLineHeight;
		int mMaxLines;
		int mLine;

		symbol_table_t* mSymbolTable;
		function_t* mCurrentFunction;
		
		unsigned char* mCode;
		unsigned char* mProgramCounter;
		
		bool mRunning;
		
		int32 mScrollSpeed;
};

// =====================================================================================================================
