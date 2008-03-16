#ifndef ADDCOMPONENTCOMMAND_H
#define	ADDCOMPONENTCOMMAND_H

/*
 *  Copyright (C) 2008  Anders Gavare.  All rights reserved.
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
 *  $Id: AddComponentCommand.h,v 1.1 2008/01/12 08:29:56 debug Exp $
 */

#include "misc.h"

#include "Command.h"
#include "UnitTest.h"


/**
 * \brief A Command which adds a Component to some place in a GXemul
 *	instance' component tree.
 */
class AddComponentCommand
	: public Command
{
public:
	/**
	 * \brief Constructs an %AddComponentCommand.
	 */
	AddComponentCommand();

	virtual ~AddComponentCommand();

	/**
	 * \brief Executes the add command.
	 *
	 * @param gxemul A reference to the GXemul instance.
	 * @param arguments A vector of zero or more string arguments.
	 */
	virtual void Execute(GXemul& gxemul,
		const vector<string>& arguments);

	virtual string GetShortDescription() const;

	virtual string GetLongDescription() const;
};


#endif	// ADDCOMPONENTCOMMAND_H