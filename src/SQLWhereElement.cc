#include <cstdio>
#include "SQLWhereElement.h"

SQLWhereElement::SQLWhereElement(void)
: m_lefthand(NULL),
  m_operator(NULL),
  m_righthand(NULL)
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
