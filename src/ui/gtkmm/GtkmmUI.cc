/*
 *  Copyright (C) 2007-2008  Anders Gavare.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright  
 *     notice, this list of conditions and the following disclaimer in the 
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE   
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
 *
 *  $Id: GtkmmUI.cc,v 1.4 2008/01/05 13:13:50 debug Exp $
 */

#ifdef WITH_GTKMM

#include <gtkmm.h>
#include "ui/gtkmm/GtkmmUI.h"
#include "ui/gtkmm/GXemulWindow.h"


GtkmmUI::GtkmmUI(GXemul *gxemul)
	: UI(gxemul)
{
}


GtkmmUI::~GtkmmUI()
{
}


void GtkmmUI::Initialize()
{
	// No specific initialization necessary.
}


void GtkmmUI::ShowStartupBanner()
{
	// No startup banner, for now.
}


void GtkmmUI::ShowDebugMessage(const string& msg)
{
	// TODO
}


void GtkmmUI::InputLineDone()
{
	// TODO
}


void GtkmmUI::RedisplayInputLine(const string& inputline,
    size_t cursorPosition)
{
	// TODO
}


int GtkmmUI::MainLoop()
{
	GXemulWindow window(m_gxemul);
	Gtk::Main::run(window);
	return 0;
}


#endif	// WITH_GTKMM