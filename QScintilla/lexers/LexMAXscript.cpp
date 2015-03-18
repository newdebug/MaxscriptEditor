#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "QScintilla\include\ILexer.h"
#include "QScintilla\include\Scintilla.h"
#include "QScintilla\include\SciLexer.h"

#include "QScintilla\lexlib\WordList.h"
#include "QScintilla\lexlib\LexAccessor.h"
#include "QScintilla\lexlib\Accessor.h"
#include "QScintilla\lexlib\StyleContext.h"
#include "QScintilla\lexlib\CharacterSet.h"
#include "QScintilla\lexlib\LexerModule.h"
#include "QScintilla\lexlib\OptionSet.h"
#include "QScintilla\lexlib\SparseState.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static void ColouriseMAXScriptDoc(
  unsigned int startPos,
  int length,
  int initStyle,
  WordList *keywordlists[],
  Accessor &styler);
  
/***************************************/
static inline bool IsAWordChar(const int ch)
{
	return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_' );
}

/***************************************/
static inline bool IsAWordStart(const int ch)
{
	return (ch < 0x80) && (isalnum(ch) || ch == '_');
}
  
/***************************************/
inline bool IsABlank(unsigned int ch)
{
	return (ch == ' ') || (ch == 0x09) || (ch == 0x0b) ;
}

/***************************************/
static void ColouriseMAXScriptDoc(
  unsigned int startPos,
  int length,
  int initStyle,
  WordList *keywordlists[],
  Accessor &styler)
{
	WordList &Keywords   = *keywordlists[0];
	WordList &Operators  = *keywordlists[1];
	WordList &Attributes = *keywordlists[2];
	WordList &Functions  = *keywordlists[3];
	WordList &Packages   = *keywordlists[4];
	WordList &Types      = *keywordlists[5];
	WordList &User       = *keywordlists[6];
	
	StyleContext sc(startPos, length, initStyle, styler);
	for (; sc.More(); sc.Forward())
	{

		// Determine if the current state should terminate.
		if (sc.state == SCE_MAXSCRIPT_OPERATOR)
		{
			sc.SetState(SCE_MAXSCRIPT_DEFAULT);
		}
		else if (sc.state == SCE_MAXSCRIPT_NUMBER)
		{
			if (!IsAWordChar(sc.ch) && (sc.ch != '#') && ( sc.ch != '$') && ( sc.ch != '&'))
			{
				sc.SetState(SCE_MAXSCRIPT_DEFAULT);
			}
		}
		else if (sc.state == SCE_MAXSCRIPT_IDENTIFIER)
		{
		  if (!IsAWordChar(sc.ch) || (sc.ch == '.'))
		  {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				if (Keywords.InList(s))
				{
					sc.ChangeState(SCE_MAXSCRIPT_KEYWORD);
				}
				else if (Operators.InList(s))
				{
					sc.ChangeState(SCE_MAXSCRIPT_STDOPERATOR);
				}
				else if (Attributes.InList(s))
				{
					sc.ChangeState(SCE_MAXSCRIPT_ATTRIBUTE);
				}
				else if (Functions.InList(s))
				{
					sc.ChangeState(SCE_MAXSCRIPT_STDFUNCTION);
				}
				else if (Packages.InList(s))
				{
					sc.ChangeState(SCE_MAXSCRIPT_STDPACKAGE);
				}
				else if (Types.InList(s))
				{
					sc.ChangeState(SCE_MAXSCRIPT_STDTYPE);
				}
				else if (User.InList(s))
				{
					sc.ChangeState(SCE_MAXSCRIPT_USERWORD);
				}
				sc.SetState(SCE_MAXSCRIPT_DEFAULT);
			}
		}
		else if (sc.state == SCE_MAXSCRIPT_COMMENT || sc.state == SCE_MAXSCRIPT_COMMENTLINEBANG)
		{
			if (sc.atLineEnd)
			{
				sc.SetState(SCE_MAXSCRIPT_DEFAULT);
			}
		}
		else if (sc.state == SCE_MAXSCRIPT_STRING)
		{
			if (sc.ch == '\\')
			{
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\')
				{
					sc.Forward();
				}
			}
			else if (sc.ch == '\"')
			{
				sc.ForwardSetState(SCE_MAXSCRIPT_DEFAULT);
			}
			else if (sc.atLineEnd)
			{
				sc.ChangeState(SCE_MAXSCRIPT_STRINGEOL);
				sc.ForwardSetState(SCE_MAXSCRIPT_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_MAXSCRIPT_DEFAULT)
		{
			// ���֣�С����
			if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext)))
			{
				sc.SetState(SCE_MAXSCRIPT_NUMBER);
			}
			else if (IsAWordStart(sc.ch))
			{
				//���ʵĿ�ʼ����ʶ��
				sc.SetState(SCE_MAXSCRIPT_IDENTIFIER);
			}
			// ע����
			else if (sc.Match('-', '-'))
			{
				// ע��
				if (sc.Match("--!"))  // Nice to have a different comment style
					sc.SetState(SCE_MAXSCRIPT_COMMENTLINEBANG);
				else
					sc.SetState(SCE_MAXSCRIPT_COMMENT);
			}
            else if( sc.Match('/', '/') )
			{
				if ((sc.Match("///") && !sc.Match("////")) || sc.Match("//!"))
					sc.SetState(SCE_MAXSCRIPT_COMMENTLINEBANG);
				else
					sc.SetState(SCE_MAXSCRIPT_COMMENT);
			}
			else if (sc.Match('/', '*'))
			{
				if (sc.Match("/**") || sc.Match("/*!"))
				{
					sc.SetState(SCE_MAXSCRIPT_COMMENTLINEBANG);
				}
				else
				{
					sc.SetState(SCE_MAXSCRIPT_COMMENT);
				}
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			}
			else if (sc.ch == '\"')
			{
				sc.SetState(SCE_MAXSCRIPT_STRING);
			}
			else if (sc.Match('@', '\"'))
			{
				sc.SetState(SCE_MAXSCRIPT_STRING);
				sc.Forward();
			}
			else if (isoperator(static_cast<char>(sc.ch))) 
			{
				sc.SetState(SCE_MAXSCRIPT_OPERATOR);
			}
		}
	}
	sc.Complete();
}

//=============================================================================
// �Ƿ���ע���У�-- // /**/
static bool IsCommentLine(int line, Accessor &styler)
{
	int pos = styler.LineStart(line);
	int eol_pos = styler.LineStart(line + 1) - 1;	
	for (int i = pos; i < eol_pos; i++)
	{
		char ch = styler[i];
		char chNext = styler[i+1];
		if ((ch == '-') && (chNext == '-'))
			return true;
		else if((ch == '/') && (chNext == '/'))
			return true;
		else if((ch == '/') && (chNext == '*'))
		{
			// �����Ƿ���Ҫ�ж�һ������ǲ���"*/"
			return true;
		}	
		else if (ch != ' ' && ch != '\t')
			return false;
	}
	return false;
}

//=============================================================================
// �۵�����
static void FoldNoBoxMAXScriptDoc(
  unsigned int startPos,
  int length,
  int,
  Accessor &styler)
{
	char words[] =
		"begin case else if end undefined unsupplied then local global level with"
		"function fn when while at in to time animate exit do on off for about as"
		"where continue try catch set undo and or not true false for by";
	WordList keywords;
	keywords.Set(words);

	bool foldComment      = styler.GetPropertyInt("fold.comment", 1) != 0;
	bool foldCompact      = styler.GetPropertyInt("fold.compact", 1) != 0;
	bool foldAtElse       = styler.GetPropertyInt("fold.at.else", 1) != 0;
	bool foldAtBegin      = styler.GetPropertyInt("fold.at.Begin", 1) != 0;
	bool foldAtParenthese = styler.GetPropertyInt("fold.at.Parenthese", 1) != 0;
  
	int  visibleChars     = 0;
	unsigned int endPos   = startPos + length;

	int lineCurrent       = styler.GetLine(startPos);
	int levelCurrent      = SC_FOLDLEVELBASE;
	if(lineCurrent > 0)
		levelCurrent        = styler.LevelAt(lineCurrent-1) >> 16;
	//int levelMinCurrent   = levelCurrent;
	int levelMinCurrentElse = levelCurrent;   //< Used for folding at 'else'
	int levelMinCurrentBegin = levelCurrent;  //< Used for folding at 'begin'
	int levelNext         = levelCurrent;
  
    /***************************************/
	int lastStart         = 0;
	char prevWord[32]     = "";
	/***************************************/
	// Find prev word
	// The logic for going up or down a level depends on a the previous keyword
	// This code could be cleaned up.
	int end = 0;
	unsigned int j;
	for(j = startPos; j>0; j--)
	{
		char ch       = styler.SafeGetCharAt(j);
		char chPrev   = styler.SafeGetCharAt(j-1);
		int style     = styler.StyleAt(j);
		int stylePrev = styler.StyleAt(j-1);
		if ((stylePrev != SCE_MAXSCRIPT_COMMENT) && (stylePrev != SCE_MAXSCRIPT_STRING))
		{
			if(IsAWordChar(chPrev) && !IsAWordChar(ch))
			{
				end = j-1;
			}
		}
		if ((style != SCE_MAXSCRIPT_COMMENT) && (style != SCE_MAXSCRIPT_STRING))
		{
			if(!IsAWordChar(chPrev) && IsAWordStart(ch) && (end != 0))
			{
				char s[32];
				unsigned int k;
				for(k=0; (k<31 ) && (k<end-j+1 ); k++)
				{
					s[k] = static_cast<char>(tolower(styler[j+k]));
				}
				s[k] = '\0';

				if(keywords.InList(s))
				{
					strcpy(prevWord, s);
					break;
				}
			}
		}
	}
	for(j=j+static_cast<unsigned int>(strlen(prevWord)); j<endPos; j++)
	{
		char ch       = styler.SafeGetCharAt(j);
		int style     = styler.StyleAt(j);
		if ((style != SCE_VHDL_COMMENT) && (style != SCE_VHDL_STRING))
		{
			if((ch == ';') && (strcmp(prevWord, "end") == 0))
			{
				strcpy(prevWord, ";");
			}
		}
	}

	
	char  chNext          = styler[startPos];
	char  chPrev          = '\0';
	char  chNextNonBlank;
	int   styleNext       = styler.StyleAt(startPos);
	
  /***************************************/
	for (unsigned int i = startPos; i < endPos; i++)
	{
		char ch         = chNext;
		chNext          = styler.SafeGetCharAt(i + 1);
		chPrev          = styler.SafeGetCharAt(i - 1);
		chNextNonBlank  = chNext;
		unsigned int j  = i+1;
		while(IsABlank(chNextNonBlank) && j<endPos)
		{
			j ++ ;
			chNextNonBlank = styler.SafeGetCharAt(j);
		}
		int style           = styleNext;
		styleNext       = styler.StyleAt(i + 1);
		bool atEOL      = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (foldComment && atEOL && IsCommentLine(lineCurrent, styler))
		{
			if(!IsCommentLine(lineCurrent-1, styler) && IsCommentLine(lineCurrent+1, styler))
			{		
				levelNext++;
			}
			else if(IsCommentLine(lineCurrent-1, styler) && !IsCommentLine(lineCurrent+1, styler))
			{
				levelNext--;
			}
		}

		if ((style == SCE_VHDL_OPERATOR) && foldAtParenthese)
		{
			if(ch == '(')
			{
				levelNext++;
			}
			else if (ch == ')')
			{
				levelNext--;
			}
		}

		if ((style != SCE_VHDL_COMMENT) && (style != SCE_VHDL_STRING))
		{
			if((ch == ';') && (strcmp(prevWord, "end") == 0))
			{
				strcpy(prevWord, ";");
			}

			if(!IsAWordChar(chPrev) && IsAWordStart(ch))
			{
				lastStart = i;
			}

		  if(iswordchar(ch) && !iswordchar(chNext))
		  {
			char s[32];
			unsigned int k;
			for(k=0; (k<31 ) && (k<i-lastStart+1 ); k++)
			{
				s[k] = static_cast<char>(tolower(styler[lastStart+k]));
			}
			s[k] = '\0';

			if(keywords.InList(s))
			{
				if (
					strcmp(s, "architecture") == 0  ||
					strcmp(s, "case") == 0          ||
					strcmp(s, "component") == 0     ||
					strcmp(s, "entity") == 0        ||
					strcmp(s, "generate") == 0      ||
					strcmp(s, "loop") == 0          ||
					strcmp(s, "package") ==0        ||
					strcmp(s, "process") == 0       ||
					strcmp(s, "record") == 0        ||
					strcmp(s, "then") == 0)
				{
					if (strcmp(prevWord, "end") != 0)
					{
						if (levelMinCurrentElse > levelNext)
						{
							levelMinCurrentElse = levelNext;
						}
						levelNext++;
					}
				}
				else if (strcmp(s, "procedure") == 0 || strcmp(s, "function") == 0)
				{		
					if (strcmp(prevWord, "end") != 0) // check for "end procedure" etc.
					{ 	// This code checks to see if the procedure / function is a definition within a "package"
						// rather than the actual code in the body.
						int BracketLevel = 0;
						for(int j=i+1; j<styler.Length(); j++)
						{
							int LocalStyle = styler.StyleAt(j);
							char LocalCh = styler.SafeGetCharAt(j);
							if(LocalCh == '(') BracketLevel++;
							if(LocalCh == ')') BracketLevel--;
							if( (BracketLevel == 0) &&
								(LocalStyle != SCE_VHDL_COMMENT) &&
								(LocalStyle != SCE_VHDL_STRING) &&
								!iswordchar(styler.SafeGetCharAt(j-1)) &&
								styler.Match(j, "is") &&
								!iswordchar(styler.SafeGetCharAt(j+2)))
							{	
								if (levelMinCurrentElse > levelNext)
								{
									levelMinCurrentElse = levelNext;
								}
								levelNext++;
								break;
							}
							if((BracketLevel == 0) && (LocalCh == ';'))
							{
								break;
							}
						}
					}

				}
				else if (strcmp(s, "end") == 0)
				{
					levelNext--;
				}
				else if(strcmp(s, "elsif") == 0)
				{ // elsif is followed by then so folding occurs correctly
					levelNext--;
				}
				else if (strcmp(s, "else") == 0)
				{
					if(strcmp(prevWord, "when") != 0)  // ignore a <= x when y else z;
					{
					  levelMinCurrentElse = levelNext - 1;  // VHDL else is all on its own so just dec. the min level
					}
				 }
				 else if(
					((strcmp(s, "begin") == 0) && (strcmp(prevWord, "architecture") == 0)) ||
					((strcmp(s, "begin") == 0) && (strcmp(prevWord, "function") == 0)) ||
					((strcmp(s, "begin") == 0) && (strcmp(prevWord, "procedure") == 0)))
				{
					levelMinCurrentBegin = levelNext - 1;
				}
				  //Platform::DebugPrintf("Line[%04d] Prev[%20s] Cur[%20s] Level[%x]\n", lineCurrent+1, prevWord, s, levelCurrent);
				  strcpy(prevWord, s);
				}
			}
		}
		if (atEOL)
		{
			int levelUse = levelCurrent;

			if (foldAtElse && (levelMinCurrentElse < levelUse))
			{
				levelUse = levelMinCurrentElse;
			}
			if (foldAtBegin && (levelMinCurrentBegin < levelUse))
			{
				levelUse = levelMinCurrentBegin;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && foldCompact)
			lev |= SC_FOLDLEVELWHITEFLAG;

			if (levelUse < levelNext)
			lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent))
			{
				styler.SetLevel(lineCurrent, lev);
			}
			//Platform::DebugPrintf("Line[%04d] ---------------------------------------------------- Level[%x]\n", lineCurrent+1, levelCurrent);
			lineCurrent++;
			levelCurrent = levelNext;
			//levelMinCurrent = levelCurrent;
			levelMinCurrentElse = levelCurrent;
			levelMinCurrentBegin = levelCurrent;
			visibleChars = 0;
		}
		/***************************************/
		if (!isspacechar(ch)) visibleChars++;
	} 	// end for
}





//=============================================================================
static void FoldMAXScriptDoc(unsigned int startPos, int length, int initStyle, WordList *[], Accessor &styler)
{
    FoldNoBoxMAXScriptDoc(startPos, length, initStyle, styler);
}

//=============================================================================
static const char * const MAXScriptWordLists[] = {
            "Keywords",
            "Operators",
            "Attributes",
            "Standard Functions",
            "Standard Packages",
            "Standard Types",
            "User Words",
            0,
        };

LexerModule lmMAXScript(SCLEX_MAXSCRIPT, ColouriseMAXScriptDoc, "maxscript", FoldMAXScriptDoc, MAXScriptWordLists);
