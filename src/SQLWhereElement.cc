#include <cstdio>
#include "SQLWhereElement.h"

// ---------------------------------------------------------------------------
// Public methods (SQLWhereOperator)
// ---------------------------------------------------------------------------
SQLWhereOperator::SQLWhereOperator(SQLWhereOperatorType type)
: m_type(type)
{
}

SQLWhereOperator::~SQLWhereOperator()
{
}

// ---------------------------------------------------------------------------
// Public methods (SQLWhereElement)
// ---------------------------------------------------------------------------
SQLWhereElement::SQLWhereElement(void)
: m_lefthand(NULL),
  m_operator(NULL),
  m_righthand(NULL),
  m_parent(NULL)
{
}

SQLWhereElement::~SQLWhereElement()
{
	if (m_lefthand)
		delete m_lefthand;
	if (m_operator)
		delete m_operator;
	if (m_righthand)
		delete m_righthand;
}
