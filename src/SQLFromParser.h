/* Asura
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SQLFromParser_h
#define SQLFromParser_h

#include <string>
using namespace std;

#include <ParsableString.h>
using namespace mlpl;

#include "SQLTableFormula.h"
#include "FormulaElement.h"

class SQLFromParser
{
public:
	static void init(void);
	SQLFromParser(void);
	virtual ~SQLFromParser();
	SQLTableFormula *getTableFormula(void) const;
	SQLTableElementList &getTableElementList(void) const;
	SeparatorCheckerWithCallback *getSeparatorChecker(void);
	void setColumnIndexResolver(SQLColumnIndexResoveler *resolver);
	ItemTablePtr doJoin(FormulaElement *whereFormula, bool existsMode);

	virtual void add(const string& word, const string &wordLower);
	virtual void flush(void);
	virtual void close(void);

protected:
	typedef void (SQLFromParser::*SubParser)
	               (const string &word, const string &wordLower);
	enum ParsingState {
		PARSING_STAT_EXPECT_TABLE_NAME,
		PARSING_STAT_POST_TABLE_NAME,
		PARSING_STAT_CREATED_TABLE,
		PARSING_STAT_GOT_INNER,
		PARSING_STAT_EXPECT_ON,
		PARSING_STAT_EXPECT_INNER_JOIN_LEFT_FIELD,
		PARSING_STAT_EXPECT_INNER_JOIN_EQUAL,
		PARSING_STAT_EXPECT_INNER_JOIN_RIGHT_FIELD,
		NUM_PARSING_STAT,
	};

	//
	// Sub parsers
	//
	void subParserExpectTableName
	       (const string &word, const string &wordLower);
	void subParserPostTableName
	       (const string &word, const string &wordLower);
	void subParserCreatedTable(const string &word, const string &wordLower);
	void subParserGotInner(const string &word, const string &wordLower);
	void subParserExpectOn(const string &word, const string &wordLower);
	void subParserExpectLeftField
	       (const string &word, const string &wordLower);
	void subParserExpectJoinEqual
	       (const string &word, const string &wordLower);
	void subParserExpectRightField
	       (const string &word, const string &wordLower);

	//
	// general sub routines
	//
	void goNextStateIfWordIsExpected(const string &expectedWord,
	                                 const string &actualWord,
	                                 ParsingState nextState);
	void insertTableFormula(SQLTableFormula *tableFormula);
	void makeTableElement(
	       const string &tableName,
	       const string &varName = StringUtils::EMPTY_STRING);
	void makeCrossJoin(void);
	void makeInnerJoin(void);
	void parseInnerJoinLeftField(const string &fieldName);
	void parseInnerJoinRightField(const string &fieldName);
	void doJoineOneRow(FormulaElement *whereFormula);
	void IterateTableRowForJoin(SQLTableElementListIterator tableItr,
	                            FormulaElement *whereFormula);

	//
	// SeparatorChecker callbacks
	//
	static void _separatorCbEqual(const char separator,
	                              SQLFromParser *fromParsesr);
	void separatorCbEqual(const char separator);

	static void _separatorCbComma(const char separator,
	                              SQLFromParser *fromParsesr);
	void separatorCbComma(const char separator);

private:
	struct PrivateContext;
	static SubParser             m_subParsers[];
	static size_t                m_numSubParsers;
	PrivateContext              *m_ctx;
	SeparatorCheckerWithCallback m_separator;
};

#endif // SQLFromParser_h


