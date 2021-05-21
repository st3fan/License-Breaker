// =====================================================================================================================

//
// File        : LicenseBreaker.cpp
// Project     : LicenseBreaker
// Author      : Stefan Arentz, stefan.arentz@soze.com
// Version     : $Id$
// Environment : BeOS 4.5/Intel
//
//

// =====================================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <be/interface/StringView.h>
#include <be/interface/Slider.h>
#include <be/interface/Font.h>
#include <be/support/SupportDefs.h>

#include <udis86.h>

#include "SetupView.h"
#include "ElfUtils.h"
#include "LicenseBreaker.h"

// =====================================================================================================================

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new LicenseBreaker(message, image);
}

// =====================================================================================================================

LicenseBreaker::LicenseBreaker(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
	if (message->FindInt32("scrollspeed", &mScrollSpeed) != B_OK) {
		mScrollSpeed = 15;
	}
	
	mSymbolTable = NULL;
	mCurrentFunction = NULL;
}

// =====================================================================================================================

status_t LicenseBreaker::SaveState(BMessage *into) const
{
	into->AddInt32("scrollspeed", mScrollSpeed);
	return B_OK;
}

// =====================================================================================================================

void LicenseBreaker::StartConfig(BView *view)
{
	mSetupView = new SetupView(view->Bounds(), "setup", &mScrollSpeed);
	view->AddChild(mSetupView);
}

// =====================================================================================================================

status_t LicenseBreaker::StartSaver(BView *view, bool preview)
{
	//SetTickSize(100000);

	mView = view;
	
	BFont font(be_fixed_font);
	font.SetSize(preview ? 5 : 16);
	view->SetFont(&font);
	
	mHighColor = 0;
	
	font_height fh;
	font.GetHeight(&fh);
	mLineHeight = (int)(fh.ascent + fh.descent + fh.leading);
	mMaxLines = (view->Bounds().IntegerHeight() / mLineHeight) - 1;
	mLine = 1;

	SetTickSize((1000 - mScrollSpeed) * 100);
	
	mRunning = true;
	
	return B_OK;
}

// =====================================================================================================================

void LicenseBreaker::Print(char* inString)
{
	mView->MovePenTo(10, (mLine * mLineHeight));
	mView->DrawString(inString);
	
	mLine++;
	if (mLine > mMaxLines)
	{
		static int i;
		mView->ScrollBy(0, mLineHeight);
	}
}

// =====================================================================================================================

void LicenseBreaker::RotateHighColor(void)
{
	mView->SetHighColor(127 + (rand() % 127), 127 + (rand() % 127), 127 + (rand() % 127));
}

// =====================================================================================================================

void LicenseBreaker::Draw(BView *view, int32 frame)
{
	static ud_t u;
	if (mRunning == false) {
		return;
	}

	//
	// Initialize on frame 0.
	//

	if (frame == 0)
	{
		view->SetLowColor(0, 0, 0);
		view->FillRect(view->Bounds(), B_SOLID_LOW);
		ud_init(&u);
		ud_set_mode(&u, 32);
		ud_set_syntax(&u, UD_SYN_INTEL);
	}
	
	if (mSymbolTable == NULL)
	{
		char* error;
		GetSymbolTable("/system/kernel_x86", &mSymbolTable, &error);
	}
	
	if (mCurrentFunction == NULL)
	{
		if (mSymbolTable != NULL)
		{
			mCurrentFunction = &(mSymbolTable->functions)[(rand() + clock()) % mSymbolTable->count];
			
			//
			// Read the code into memory
			//
		
			mCode = (unsigned char*) malloc(mCurrentFunction->size);
			if (mCode != NULL)
			{
				FILE* file = fopen("/system/kernel_x86", "rb");
				if (file != NULL)
				{
					fseek(file, mCurrentFunction->offset, SEEK_SET);
					fread(mCode, mCurrentFunction->size, 1, file);
					fclose(file);
				}
			}
			
			mProgramCounter = mCode;
			
			this->RotateHighColor();
			
			char string[1024];
			sprintf(string, "Disassembling from %s:", mCurrentFunction->name);
			this->Print(string);

			ud_set_input_buffer(&u, mCode, mCurrentFunction->size);
			return;
		}
	}

	//
	// Disassemble the code
	//

	if (mProgramCounter != NULL)
	{
		// Check if we're past the end of the current function
		


		const char* out;
		size_t err = ud_disassemble(&u);
		if (err)
		{
      		out = ud_insn_asm(&u);
			// Convert the opcodes to hex
			
			char hex[20];
			hex[0] = 0x00;
			for (int j = 0; j < err; j++)
			{
				char tmp[8];
				sprintf(tmp, " %02x", mProgramCounter[j]);
				strcat(hex, tmp);
			}
	
			// Draw the string
	
			char string[1024];
			sprintf(string, "  + 0x%08x %-40s |%s", mProgramCounter - mCode, out, hex);
			this->Print(string);
	
		}


		// Move to the next instruction

		mProgramCounter += err;

		if (mProgramCounter >= (mCode + mCurrentFunction->size))
		{
			free(mCode);
			mCode = mProgramCounter = NULL;
			mCurrentFunction = NULL;
		}
	}
}

// =====================================================================================================================
