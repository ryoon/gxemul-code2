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
 *  $Id: CommandInterpreter.cc,v 1.9 2008/03/12 11:45:41 debug Exp $
 */

#include "assert.h"
#include <iostream>

#include "GXemul.h"
#include "CommandInterpreter.h"

// Built-in commands (autogenerated list by the configure script):
#include "../../commands_h.h"


CommandInterpreter::CommandInterpreter(GXemul* owner)
	: m_GXemul(owner)
	, m_currentCommandCursorPosition(0)
	, m_inEscapeSequence(false)
	, m_historyEntryToCopyFrom(0)
	, m_commandHistoryInsertPosition(0)
	, m_commandHistoryMaxSize(100)
{
	m_commandHistory.resize(m_commandHistoryMaxSize, "");

	// It would be bad to run without a working GXemul instance.
	assert(m_GXemul != NULL);

	// Add the default built-in commands:
	// (This list is autogenerated by the configure script.)
#include "../../commands.h"
}


void CommandInterpreter::AddCommand(refcount_ptr<Command> command)
{
	m_commands[command->GetCommandName()] = command;
}


const Commands& CommandInterpreter::GetCommands() const
{
	return m_commands;
}


int CommandInterpreter::AddLineToCommandHistory(const string& command)
{
	if (command == "")
		return m_commandHistoryInsertPosition;

	size_t lastInsertedPosition =
	    (m_commandHistoryInsertPosition - 1 + m_commandHistoryMaxSize)
	    % m_commandHistoryMaxSize;

	if (m_commandHistory[lastInsertedPosition] == command)
		return m_commandHistoryInsertPosition;

	m_commandHistory[m_commandHistoryInsertPosition ++] = command;
	m_commandHistoryInsertPosition %= m_commandHistoryMaxSize;

	return m_commandHistoryInsertPosition;
}


string CommandInterpreter::GetHistoryLine(int nStepsBack) const
{
	if (nStepsBack == 0)
		return "";

	int index = (m_commandHistoryInsertPosition - nStepsBack +
	    m_commandHistoryMaxSize) % m_commandHistoryMaxSize;

	return m_commandHistory[index];
}


void CommandInterpreter::TabComplete()
{
	string wordToComplete;
	bool firstWordOnLine = true;

	size_t pos = m_currentCommandCursorPosition;
	while (pos > 0) {
		pos --;
		if (m_currentCommandString[pos] == ' ')
			break;
		wordToComplete = m_currentCommandString[pos] + wordToComplete;
	}

	while (pos > 0) {
		pos --;
		if (m_currentCommandString[pos] != ' ') {
			firstWordOnLine = false;
			break;
		}
	}

	bool completeCommands = firstWordOnLine;
	bool completeComponents = !firstWordOnLine;
	// TODO: StateVariables, etc. Generalize this.

	if (wordToComplete == "") {
		// Show all available words.

		if (completeCommands) {
			// All available commands:
			vector<string> allCommands;
			for (Commands::const_iterator it = m_commands.begin();
			    it != m_commands.end(); ++it)
				allCommands.push_back(it->first);
		
			ShowAvailableWords(allCommands);
		}

		if (completeComponents) {
			ShowAvailableWords(m_GXemul->GetRootComponent()->
			    FindPathByPartialMatch(""));
		}

		return;
	}

	vector<string> matches;

	if (completeCommands) {
		Commands::const_iterator it = m_commands.begin();
		for (; it != m_commands.end(); ++it) {
			const string& commandName = it->first;
			if (commandName.substr(0, wordToComplete.length())
			    == wordToComplete) {
				matches.push_back(commandName);
			}
		}
	}

	if (completeComponents) {
		matches = m_GXemul->GetRootComponent()->
		    FindPathByPartialMatch(wordToComplete);
	}

	// TODO: Check for matches among StateVariable names...

	if (matches.size() == 0)
		return;

	string completedWord;

	// Single match, or multiple matches?
	if (matches.size() == 1) {
		// Insert the rest of the command name into the input line:
		completedWord = matches[0];
	} else {
		// Figure out the longest possible match, and add that:
		size_t i, n = matches.size();
		for (size_t pos = 0; ; pos ++) {
			stringchar ch = matches[0][pos];
			for (i=1; i<n; i++) {
				if (matches[i][pos] != ch)
					break;
			}
			if (i == n)
				completedWord += ch;
			else
				break;
		}
		
		// Show available words, so the user knows what there
		// is to choose from.
		ShowAvailableWords(matches);
	}

	// Erase the old (incomplete) word, and insert the completed word:
	m_currentCommandCursorPosition -= wordToComplete.length();
	m_currentCommandString.erase(m_currentCommandCursorPosition,
	    wordToComplete.length());	
	m_currentCommandString.insert(m_currentCommandCursorPosition,
	    completedWord);
	m_currentCommandCursorPosition += completedWord.length();

	// Special case: If there was a single match, and we are at the end
	// of the line, add a space (" "). This behaviour feels better, and
	// this is how other tab completors seems to work.
	if (matches.size() == 1 && m_currentCommandCursorPosition
	    == m_currentCommandString.length()) {
		m_currentCommandString += " ";
		m_currentCommandCursorPosition ++;
	}
}


bool CommandInterpreter::AddKey(stringchar key)
{
	if (m_inEscapeSequence) {
		m_escapeSequence += key;

		// Handle some common escape sequences, and convert
		// them into simpler 1-byte keys/characters:

		if (m_escapeSequence == "[C") {			// right
			m_inEscapeSequence = false;
			AddKey('\6');	// CTRL-F
		} else if (m_escapeSequence == "[D") {		// left
			m_inEscapeSequence = false;
			AddKey('\2');	// CTRL-B
		} else if (m_escapeSequence == "OH") {		// home
			m_inEscapeSequence = false;
			AddKey('\1');	// CTRL-A
		} else if (m_escapeSequence == "[H") {		// home
			m_inEscapeSequence = false;
			AddKey('\1');	// CTRL-A
		} else if (m_escapeSequence == "OF") {		// end
			m_inEscapeSequence = false;
			AddKey('\5');	// CTRL-E
		} else if (m_escapeSequence == "[F") {		// end
			m_inEscapeSequence = false;
			AddKey('\5');	// CTRL-E
		} else if (m_escapeSequence == "[A") {		// up
			m_inEscapeSequence = false;
			AddKey('\20');	// CTRL-P
		} else if (m_escapeSequence == "[B") {		// down
			m_inEscapeSequence = false;
			AddKey('\16');	// CTRL-N
		} else if (m_escapeSequence.length() > 2) {
			// Let's bail out of escape sequence handling...
			//
			// Note: If you trace execution here for some key that
			// you feel _should_ be handled, please send me a mail
			// about it.
			//
			m_inEscapeSequence = false;
			AddKey('?');
		}
		
		return false;
	}

	switch (key) {

	case '\0':
		// Add nothing, just reshow/update the command buffer.
		break;

	case '\1':	// CTRL-A: move to start of line
		m_currentCommandCursorPosition = 0;
		break;

	case '\2':	// CTRL-B: move back (left)
		if (m_currentCommandCursorPosition > 0)
			m_currentCommandCursorPosition --;
		break;

	case '\4':	// CTRL-D: remove the character to the right
		if (m_currentCommandCursorPosition <
		    m_currentCommandString.length())
			m_currentCommandString.erase(
			    m_currentCommandCursorPosition, 1);
		break;

	case '\5':	// CTRL-E: move to end of line
		m_currentCommandCursorPosition =
		    m_currentCommandString.length();
		break;

	case '\6':	// CTRL-F: move forward (right)
		if (m_currentCommandCursorPosition <
		    m_currentCommandString.length())
			m_currentCommandCursorPosition ++;
		break;

	case '\13':	// CTRL-K: kill to end of line
		ClearCurrentInputLineVisually();
		m_currentCommandString.resize(m_currentCommandCursorPosition);
		break;

	case '\16':	// CTRL-N: next in history (down)
		ClearCurrentInputLineVisually();

		m_historyEntryToCopyFrom --;
		if (m_historyEntryToCopyFrom < 0)
			m_historyEntryToCopyFrom = 0;

		m_currentCommandString =
		    GetHistoryLine(m_historyEntryToCopyFrom);
		m_currentCommandCursorPosition =
		    m_currentCommandString.length();
		break;

	case '\20':	// CTRL-P: previous in history (up)
		ClearCurrentInputLineVisually();

		m_historyEntryToCopyFrom ++;
		m_currentCommandString =
		    GetHistoryLine(m_historyEntryToCopyFrom);

		// We went too far? Then back down.
		if (m_currentCommandString == "") {
			m_historyEntryToCopyFrom --;
			m_currentCommandString =
			    GetHistoryLine(m_historyEntryToCopyFrom);
		}
		m_currentCommandCursorPosition =
		    m_currentCommandString.length();
		break;

	case '\24':	// CTRL-T: show status
		m_GXemul->GetUI()->ShowDebugMessage("\n");
		RunCommand("status");
		break;

	case '\27':	// CTRL-W: remove current word (backspacing)
		ClearCurrentInputLineVisually();

		// 1. Remove any spaces left to the cursor.
		while (m_currentCommandCursorPosition > 0) {
			if (m_currentCommandString[
			    m_currentCommandCursorPosition-1] == ' ') {
				m_currentCommandCursorPosition --;
				m_currentCommandString.erase(
				    m_currentCommandCursorPosition, 1);
			} else {
				break;
			}
		}

		// 2. Remove non-spaces left to the cusror, either until
		//	the cursor is at position 0, or until there is a
		//	space again.
		while (m_currentCommandCursorPosition > 0) {
			if (m_currentCommandString[
			    m_currentCommandCursorPosition-1] != ' ') {
				m_currentCommandCursorPosition --;
				m_currentCommandString.erase(
				    m_currentCommandCursorPosition, 1);
			} else {
				break;
			}
		}

		break;

	case '\177':	// ASCII 127 (octal 177) = del
	case '\b':	// backspace
		if (m_currentCommandCursorPosition > 0) {
			m_currentCommandCursorPosition --;
			m_currentCommandString.erase(
			    m_currentCommandCursorPosition, 1);
		}
		break;

	case '\e':
		// Escape key handling:
		m_inEscapeSequence = true;
		m_escapeSequence = "";
		break;

	case '\t':
		// Tab completion:
		TabComplete();
		break;

	case '\n':
	case '\r':
		// Newline executes the command, if it is non-empty:
		m_GXemul->GetUI()->InputLineDone();

		if (!m_currentCommandString.empty()) {
			AddLineToCommandHistory(m_currentCommandString);
			RunCommand(m_currentCommandString);
			ClearCurrentCommandBuffer();
		}
		break;

	default:
		// Most other keys just add/insert a character into the command
		// string:
		if (key >= ' ') {
			m_currentCommandString.insert(
			    m_currentCommandCursorPosition, 1, key);
			m_currentCommandCursorPosition ++;
		}
	}

	if (key != '\n' && key != '\r')
		ReshowCurrentCommandBuffer();

	// Return value is true for newline/cr, false otherwise:
	return key == '\n' || key == '\r';
}


void CommandInterpreter::ShowAvailableWords(const vector<string>& words)
{
	m_GXemul->GetUI()->ShowDebugMessage("\n");

	const size_t n = words.size();
	size_t i;

	// Find the longest word first:
	size_t maxLen = 0;
	for (i=0; i<n; ++i) {
		size_t len = words[i].length();
		if (len > maxLen)
			maxLen = len;
	}

	maxLen += 4;

	// Generate msg:
	std::stringstream msg;
	size_t lineLen = 0;
	for (i=0; i<n; ++i) {
		if (lineLen == 0)
			msg << "  ";

		size_t len = words[i].length();
		msg << words[i];
		lineLen += len;

		for (size_t j=len; j<maxLen; j++) {
			msg << " ";
			lineLen ++;
		}

		if (lineLen >= 77 - maxLen || i == n-1) {
			msg << "\n";
			lineLen = 0;
		}
	}
	
	m_GXemul->GetUI()->ShowDebugMessage(msg.str());
}


void CommandInterpreter::ReshowCurrentCommandBuffer()
{
	m_GXemul->GetUI()->RedisplayInputLine(
	    m_currentCommandString, m_currentCommandCursorPosition);
}


void CommandInterpreter::ClearCurrentInputLineVisually()
{
	string clearString = "";
	clearString.insert((size_t)0, m_currentCommandString.length(), ' ');

	m_GXemul->GetUI()->RedisplayInputLine(
	    clearString, m_currentCommandCursorPosition);
}


void CommandInterpreter::ClearCurrentCommandBuffer()
{
	m_currentCommandString = "";
	m_currentCommandCursorPosition = 0;
	m_historyEntryToCopyFrom = 0;
}


static void SplitIntoWords(const string& command,
	string& commandName, vector<string>& arguments)
{
	// Split command into words, ignoring all whitespace:
	arguments.clear();
	commandName = "";
	size_t pos = 0;

	while (pos < command.length()) {
		// Skip initial whitespace:
		while (pos < command.length() && command[pos] == ' ')
			pos ++;
		
		if (pos >= command.length())
			break;
		
		// This is a new word. Add all characters, until
		// whitespace or end of string:
		string newWord = "";
		while (pos < command.length() && command[pos] != ' ') {
			newWord += command[pos];
			pos ++;
		}

		if (commandName.empty())
			commandName = newWord;
		else
			arguments.push_back(newWord);
	}
}


bool CommandInterpreter::RunCommand(const string& command)
{
	string commandName;
	vector<string> arguments;
	SplitIntoWords(command, commandName, arguments);

	// Find the command...
	Commands::iterator it = m_commands.find(commandName);
	if (it == m_commands.end()) {
		m_GXemul->GetUI()->ShowDebugMessage(commandName +
		    ": unknown command. Type  help  for help.\n");
		return false;
	}

	if (arguments.size() != 0 && (it->second)->GetArgumentFormat() == "") {
		m_GXemul->GetUI()->ShowDebugMessage(commandName +
		    " takes no arguments. Type  help " + commandName +
		    "  for help on the syntax.\n");
		return false;
	}

	// ... and execute it:
	(it->second)->Execute(*m_GXemul, arguments);
	
	return true;
}


const string& CommandInterpreter::GetCurrentCommandBuffer() const
{
	return m_currentCommandString;
}


/*****************************************************************************/


#ifndef WITHOUTUNITTESTS

static void Test_CommandInterpreter_AddKey_ReturnValue()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	UnitTest::Assert("addkey of regular char should return false",
	    ci.AddKey('a') == false);

	UnitTest::Assert("addkey of nul char should return false",
	    ci.AddKey('\0') == false);

	UnitTest::Assert("addkey of newline should return true",
	    ci.AddKey('\n') == true);

	UnitTest::Assert("addkey of carriage return should return true too",
	    ci.AddKey('\r') == true);
}

static void Test_CommandInterpreter_KeyBuffer()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	UnitTest::Assert("buffer should initially be empty",
	    ci.GetCurrentCommandBuffer() == "");

	ci.AddKey('a');		// normal char

	UnitTest::Assert("buffer should contain 'a'",
	    ci.GetCurrentCommandBuffer() == "a");

	ci.AddKey('\0');	// nul char should have no effect

	UnitTest::Assert("buffer should still contain only 'a'",
	    ci.GetCurrentCommandBuffer() == "a");

	ci.AddKey('A');		// multiple chars
	ci.AddKey('B');
	UnitTest::Assert("buffer should contain 'aAB'",
	    ci.GetCurrentCommandBuffer() == "aAB");

	ci.AddKey('\177');	// del

	UnitTest::Assert("buffer should contain 'aA' (del didn't work?)",
	    ci.GetCurrentCommandBuffer() == "aA");

	ci.AddKey('\b');	// backspace

	UnitTest::Assert("buffer should contain 'a' again (BS didn't work)",
	    ci.GetCurrentCommandBuffer() == "a");

	ci.AddKey('\b');

	UnitTest::Assert("buffer should now be empty '' again",
	    ci.GetCurrentCommandBuffer() == "");

	ci.AddKey('\b');	// cannot be emptier than... well... empty :)

	UnitTest::Assert("buffer should still be empty",
	    ci.GetCurrentCommandBuffer() == "");

	ci.AddKey('a');

	UnitTest::Assert("buffer should contain 'a' again",
	    ci.GetCurrentCommandBuffer() == "a");

	ci.AddKey('Q');

	UnitTest::Assert("buffer should contain 'aQ'",
	    ci.GetCurrentCommandBuffer() == "aQ");

	ci.AddKey('\n');	// newline should execute the command

	UnitTest::Assert("buffer should be empty after executing '\\n'",
	    ci.GetCurrentCommandBuffer() == "");

	ci.AddKey('Z');
	ci.AddKey('Q');

	UnitTest::Assert("new command should have been possible",
	    ci.GetCurrentCommandBuffer() == "ZQ");

	ci.AddKey('\r');	// carriage return should work like newline

	UnitTest::Assert("buffer should be empty after executing '\\r'",
	    ci.GetCurrentCommandBuffer() == "");
}

static void Test_CommandInterpreter_KeyBuffer_CursorMovement()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	ci.AddKey('A');
	ci.AddKey('B');
	ci.AddKey('C');
	ci.AddKey('D');
	ci.AddKey('E');
	UnitTest::Assert("buffer should contain 'ABCDE'",
	    ci.GetCurrentCommandBuffer(), "ABCDE");

	ci.AddKey('\2');	// CTRL-B should move back (left)
	ci.AddKey('\2');
	ci.AddKey('\2');
	UnitTest::Assert("buffer should still contain 'ABCDE'",
	    ci.GetCurrentCommandBuffer(), "ABCDE");

	ci.AddKey('\b');
	UnitTest::Assert("buffer should now contain 'ACDE'",
	    ci.GetCurrentCommandBuffer(), "ACDE");

	ci.AddKey('\6');	// CTRL-F should move forward (right)
	ci.AddKey('\6');
	UnitTest::Assert("buffer should still contain 'ACDE'",
	    ci.GetCurrentCommandBuffer(), "ACDE");

	ci.AddKey('\b');
	UnitTest::Assert("buffer should now contain 'ACE'",
	    ci.GetCurrentCommandBuffer(), "ACE");

	ci.AddKey('\1');	// CTRL-A should move to start of line
	UnitTest::Assert("buffer should still contain 'ACE'",
	    ci.GetCurrentCommandBuffer(), "ACE");

	ci.AddKey('1');
	ci.AddKey('2');
	UnitTest::Assert("buffer should now contain '12ACE'",
	    ci.GetCurrentCommandBuffer(), "12ACE");

	ci.AddKey('\5');	// CTRL-E should move to end of line
	UnitTest::Assert("buffer should still contain '12ACE'",
	    ci.GetCurrentCommandBuffer(), "12ACE");

	ci.AddKey('x');
	ci.AddKey('y');
	UnitTest::Assert("buffer should now contain '12ACExy'",
	    ci.GetCurrentCommandBuffer(), "12ACExy");

	ci.AddKey('\1');	// CTRL-A move to start of line again
	ci.AddKey('\6');	// CTRL-F move to the right
	ci.AddKey('\4');	// CTRL-D should remove character to the right
	UnitTest::Assert("buffer should now contain '1ACExy'",
	    ci.GetCurrentCommandBuffer(), "1ACExy");
}

static void Test_CommandInterpreter_KeyBuffer_CtrlK()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	ci.AddKey('A');
	ci.AddKey('B');
	ci.AddKey('C');
	ci.AddKey('D');
	ci.AddKey('E');
	UnitTest::Assert("buffer should contain 'ABCDE'",
	    ci.GetCurrentCommandBuffer(), "ABCDE");

	ci.AddKey('\2');	// CTRL-B should move back (left)
	ci.AddKey('\2');
	UnitTest::Assert("buffer should still contain 'ABCDE'",
	    ci.GetCurrentCommandBuffer(), "ABCDE");

	ci.AddKey('\13');	// CTRL-K
	UnitTest::Assert("buffer should now contain 'ABC'",
	    ci.GetCurrentCommandBuffer(), "ABC");

	ci.AddKey('X');
	ci.AddKey('\13');	// CTRL-K again, at end of line
	UnitTest::Assert("buffer should now contain 'ABCX'",
	    ci.GetCurrentCommandBuffer(), "ABCX");

	ci.AddKey('\1');	// CTRL-A to move to start of line
	ci.AddKey('\13');	// CTRL-K again, should erase everything
	UnitTest::Assert("buffer should now be empty",
	    ci.GetCurrentCommandBuffer(), "");
}

static void Test_CommandInterpreter_KeyBuffer_CtrlW()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	UnitTest::Assert("buffer should contain ''",
	    ci.GetCurrentCommandBuffer(), "");
	ci.AddKey('\27');	// CTRL-W
	UnitTest::Assert("buffer should still contain ''",
	    ci.GetCurrentCommandBuffer(), "");

	ci.AddKey('a');
	ci.AddKey('b');
	ci.AddKey('c');
	UnitTest::Assert("buffer should contain abc",
	    ci.GetCurrentCommandBuffer(), "abc");
	ci.AddKey('\27');	// CTRL-W
	UnitTest::Assert("buffer should be empty again",
	    ci.GetCurrentCommandBuffer(), "");

	ci.AddKey(' ');
	ci.AddKey(' ');
	ci.AddKey('a');
	ci.AddKey('b');
	ci.AddKey('c');
	UnitTest::Assert("buffer should contain '  abc'",
	    ci.GetCurrentCommandBuffer(), "  abc");
	ci.AddKey('\27');	// CTRL-W
	UnitTest::Assert("buffer should contain only two spaces",
	    ci.GetCurrentCommandBuffer(), "  ");

	ci.AddKey('a');
	ci.AddKey('b');
	ci.AddKey('c');
	ci.AddKey(' ');
	UnitTest::Assert("buffer should contain '  abc '",
	    ci.GetCurrentCommandBuffer(), "  abc ");
	ci.AddKey('\27');	// CTRL-W
	UnitTest::Assert("buffer should again contain only two spaces",
	    ci.GetCurrentCommandBuffer(), "  ");

	ci.AddKey('a');
	ci.AddKey('b');
	ci.AddKey('c');
	ci.AddKey('d');
	ci.AddKey(' ');
	ci.AddKey('e');
	ci.AddKey('f');
	ci.AddKey('g');
	ci.AddKey('h');
	ci.AddKey('i');
	ci.AddKey(' ');
	ci.AddKey('\2');	// CTRL-B = move left
	ci.AddKey('\2');
	ci.AddKey('\2');
	UnitTest::Assert("buffer should contain '  abcd efghi '",
	    ci.GetCurrentCommandBuffer(), "  abcd efghi ");
	ci.AddKey('\27');	// CTRL-W
	UnitTest::Assert("buffer should now contain '  abcd hi '",
	    ci.GetCurrentCommandBuffer(), "  abcd hi ");
}

static void Test_CommandInterpreter_CommandHistory()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	UnitTest::Assert("history should still be empty (1)",
	    ci.AddLineToCommandHistory(""), 0);

	UnitTest::Assert("history should still be empty (2)",
	    ci.AddLineToCommandHistory(""), 0);

	UnitTest::Assert("A: history line 0",
	    ci.GetHistoryLine(0), "");
	UnitTest::Assert("A: history line 1 not set yet",
	    ci.GetHistoryLine(1), "");
	UnitTest::Assert("A: history line 2 not set yet",
	    ci.GetHistoryLine(2), "");

	UnitTest::Assert("history should contain one entry",
	    ci.AddLineToCommandHistory("hello"), 1);

	UnitTest::Assert("B: history line 0",
	    ci.GetHistoryLine(0), "");
	UnitTest::Assert("B: history line 1",
	    ci.GetHistoryLine(1), "hello");
	UnitTest::Assert("B: history line 2 not set yet",
	    ci.GetHistoryLine(2), "");

	UnitTest::Assert("history should contain two entries",
	    ci.AddLineToCommandHistory("world"), 2);

	UnitTest::Assert("history should still contain two entries",
	    ci.AddLineToCommandHistory("world"), 2);

	UnitTest::Assert("C: history line 0",
	    ci.GetHistoryLine(0), "");
	UnitTest::Assert("C: history line 1",
	    ci.GetHistoryLine(1), "world");
	UnitTest::Assert("C: history line 2",
	    ci.GetHistoryLine(2), "hello");

	UnitTest::Assert("history should contain three entries",
	    ci.AddLineToCommandHistory("hello"), 3);

	UnitTest::Assert("D: history line 0",
	    ci.GetHistoryLine(0), "");
	UnitTest::Assert("D: history line 1",
	    ci.GetHistoryLine(1), "hello");
	UnitTest::Assert("D: history line 2",
	    ci.GetHistoryLine(2), "world");

	UnitTest::Assert("history should still contain three entries",
	    ci.AddLineToCommandHistory(""), 3);
}

/**
 * \brief A dummy Command, for unit testing purposes
 */
class DummyCommand2
	: public Command
{
public:
	DummyCommand2(int& valueRef)
		: Command("dummycommand", "[args]")
		, m_value(valueRef)
	{
	}

	~DummyCommand2()
	{
	}

	void Execute(GXemul& gxemul, const vector<string>& arguments)
	{
		m_value ++;
	}

	string GetShortDescription() const
	{
		return "A dummy command used for unit testing.";
	}

	string GetLongDescription() const
	{
		return "This is just a dummy command used for unit testing.";
	}

private:
	int&	m_value;
};

/**
 * \brief A dummy Command, for unit testing purposes
 */
class DummyCommand3
	: public Command
{
public:
	DummyCommand3(int& valueRef)
		: Command("dummycmd", "[args]")
		, m_value(valueRef)
	{
	}

	~DummyCommand3()
	{
	}

	void Execute(GXemul& gxemul, const vector<string>& arguments)
	{
		m_value ++;
	}

	string GetShortDescription() const
	{
		return "A dummy command used for unit testing.";
	}

	string GetLongDescription() const
	{
		return "This is just a dummy command used for unit testing.";
	}

private:
	int&	m_value;
};

static void Test_CommandInterpreter_AddCommand()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	size_t nCommands = ci.GetCommands().size();
	UnitTest::Assert("there should be some commands already",
	    nCommands > 0);

	ci.AddCommand(new VersionCommand);

	UnitTest::Assert("it should not be possible to have multiple commands"
		" with the same name",
	    ci.GetCommands().size() == nCommands);

	int dummyInt = 42;
	ci.AddCommand(new DummyCommand2(dummyInt));

	UnitTest::Assert("it should be possible to add new commands",
	    ci.GetCommands().size() == nCommands + 1);
}

static void Test_CommandInterpreter_TabCompletion_FullWord()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	ci.AddKey('d');
	ci.AddKey('u');
	ci.AddKey('m');
	ci.AddKey('m');
	ci.AddKey('Z');
	ci.AddKey('\2');	// CTRL-B = move left
	UnitTest::Assert("initial buffer contents mismatch",
	    ci.GetCurrentCommandBuffer(), "dummZ");

	ci.AddKey('\t');
	UnitTest::Assert("tab completion should have failed",
	    ci.GetCurrentCommandBuffer(), "dummZ");

	int dummyInt = 42;
	ci.AddCommand(new DummyCommand2(dummyInt));

	ci.AddKey('\t');
	UnitTest::Assert("tab completion should have succeeded",
	    ci.GetCurrentCommandBuffer(), "dummycommandZ");

	ci.AddKey('X');
	UnitTest::Assert("tab completion should have placed cursor at end of"
		" the tab-completed word",
	    ci.GetCurrentCommandBuffer(), "dummycommandXZ");
}

static void Test_CommandInterpreter_TabCompletion_SpacesFirstOnLine()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	ci.AddKey(' ');
	ci.AddKey(' ');
	ci.AddKey('v');
	ci.AddKey('e');
	ci.AddKey('r');
	ci.AddKey('s');
	UnitTest::Assert("initial buffer contents mismatch",
	    ci.GetCurrentCommandBuffer(), "  vers");

	ci.AddKey('\t');
	UnitTest::Assert("tab completion should have succeeded",
	    ci.GetCurrentCommandBuffer(), "  version ");
}

static void Test_CommandInterpreter_TabCompletion_Partial()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	ci.AddKey('d');
	ci.AddKey('u');
	ci.AddKey('m');
	ci.AddKey('m');
	ci.AddKey('Z');
	ci.AddKey('\2');	// CTRL-B = move left
	UnitTest::Assert("initial buffer contents mismatch",
	    ci.GetCurrentCommandBuffer(), "dummZ");

	int dummyInt = 42;
	ci.AddCommand(new DummyCommand2(dummyInt));
	ci.AddCommand(new DummyCommand3(dummyInt));

	ci.AddKey('\t');
	UnitTest::Assert("tab completion should have partially succeeded",
	    ci.GetCurrentCommandBuffer(), "dummycZ");
}

static void Test_CommandInterpreter_TabCompletion_OnlyCommandAsFirstWord()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	ci.AddKey('v');
	ci.AddKey('e');
	ci.AddKey('r');
	ci.AddKey('s');
	UnitTest::Assert("initial buffer contents mismatch",
	    ci.GetCurrentCommandBuffer(), "vers");

	ci.AddKey('\t');
	UnitTest::Assert("first tab completion should have succeeded",
	    ci.GetCurrentCommandBuffer(), "version ");

	ci.AddKey('v');
	ci.AddKey('e');
	ci.AddKey('r');
	ci.AddKey('s');
	UnitTest::Assert("buffer contents mismatch",
	    ci.GetCurrentCommandBuffer(), "version vers");

	ci.AddKey('\t');
	UnitTest::Assert("second tab completion should have failed",
	    ci.GetCurrentCommandBuffer(), "version vers");
}

static void Test_CommandInterpreter_TabCompletion_ComponentName()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	ci.RunCommand("add dummy root");
	UnitTest::Assert("initial buffer should be empty",
	    ci.GetCurrentCommandBuffer(), "");

	ci.AddKey('a');
	ci.AddKey('d');
	ci.AddKey('d');
	ci.AddKey(' ');
	ci.AddKey('d');
	ci.AddKey('u');
	ci.AddKey('m');
	ci.AddKey('m');
	ci.AddKey('y');
	ci.AddKey(' ');
	ci.AddKey('d');
	ci.AddKey('u');
	UnitTest::Assert("buffer contents mismatch",
	    ci.GetCurrentCommandBuffer(), "add dummy du");

	ci.AddKey('\t');
	UnitTest::Assert("tab completion should have completed the "
		"component name",
	    ci.GetCurrentCommandBuffer(), "add dummy root.dummy0 ");
}

static void Test_CommandInterpreter_NonExistingCommand()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	UnitTest::Assert("nonexisting (nonsense) command should fail",
	    ci.RunCommand("nonexistingcommand") == false);
}

static void Test_CommandInterpreter_SimpleCommand()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	UnitTest::Assert("simple command should succeed",
	    ci.RunCommand("version") == true);

	UnitTest::Assert("simple command with whitespace should succeed",
	    ci.RunCommand("   version   ") == true);
}

static void Test_CommandInterpreter_SimpleCommand_NoArgsAllowed()
{
	GXemul gxemul(false);
	CommandInterpreter& ci = gxemul.GetCommandInterpreter();

	UnitTest::Assert("simple command should succeed",
	    ci.RunCommand("version") == true);

	UnitTest::Assert("simple command which does not take arguments should"
		" fail when attempt is made to execute it with arguments",
	    ci.RunCommand("version hello") == false);
}

UNITTESTS(CommandInterpreter)
{
	// Key and current buffer:
	UNITTEST(Test_CommandInterpreter_AddKey_ReturnValue);
	UNITTEST(Test_CommandInterpreter_KeyBuffer);
	UNITTEST(Test_CommandInterpreter_KeyBuffer_CursorMovement);
	UNITTEST(Test_CommandInterpreter_KeyBuffer_CtrlK);
	UNITTEST(Test_CommandInterpreter_KeyBuffer_CtrlW);

	// Command History:
	UNITTEST(Test_CommandInterpreter_CommandHistory);

	// AddCommand / GetCommands:
	UNITTEST(Test_CommandInterpreter_AddCommand);

	// Tab completion:
	UNITTEST(Test_CommandInterpreter_TabCompletion_FullWord);
	UNITTEST(Test_CommandInterpreter_TabCompletion_SpacesFirstOnLine);
	UNITTEST(Test_CommandInterpreter_TabCompletion_Partial);
	UNITTEST(Test_CommandInterpreter_TabCompletion_OnlyCommandAsFirstWord);
	UNITTEST(Test_CommandInterpreter_TabCompletion_ComponentName);

	// RunCommand:
	UNITTEST(Test_CommandInterpreter_NonExistingCommand);
	UNITTEST(Test_CommandInterpreter_SimpleCommand);
	UNITTEST(Test_CommandInterpreter_SimpleCommand_NoArgsAllowed);
}

#endif