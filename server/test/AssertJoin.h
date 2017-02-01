/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "Helpers.h"

template <typename RefDataType0, typename RefDataType1>
class AssertJoin
{
protected:
	typedef void (*AssertJoinRunner)
	               (const ItemGroup *itemGroup,
	                RefDataType0 *refData0, RefDataType1 *refData1,
	                size_t data0Index, size_t data1Index);

	RefDataType0 *m_refTable0;
	RefDataType1 *m_refTable1;
	ItemTable    *m_itemTable;
	size_t        m_numRowsRefTable0;
	size_t        m_numRowsRefTable1;
	size_t        m_refTable0Index;
	size_t        m_refTable1Index;
	void (*m_assertRunner)(const ItemGroup *itemGroup,
	                       RefDataType0 *refData0, RefDataType1 *refData1,
	                       size_t data0Index, size_t data1Index);
public:
	AssertJoin(ItemTable *itemTable,
	           RefDataType0 *refTable0, RefDataType1 *refTable1,
	           size_t numRowsRefTable0, size_t numRowsRefTable1)
	: m_refTable0(refTable0),
	  m_refTable1(refTable1),
	  m_itemTable(itemTable),
	  m_numRowsRefTable0(numRowsRefTable0),
	  m_numRowsRefTable1(numRowsRefTable1),
	  m_refTable0Index(0),
	  m_refTable1Index(0),
	  m_assertRunner(NULL)
	{
	}

	virtual ~AssertJoin()
	{
	}

	virtual void run(AssertJoinRunner runner)
	{
		m_assertRunner = runner;
		m_itemTable->foreach<AssertJoin *>(_assertForeach, this);
	}

	static bool _assertForeach(const ItemGroup *itemGroup, AssertJoin *obj)
	{
		return obj->assertForeach(itemGroup);
	}

	virtual void assertPreForeach(void)
	{
	}

	virtual void assertPostForeach(void)
	{
	}

	virtual bool assertForeach(const ItemGroup *itemGroup)
	{
		assertPreForeach();
		(*m_assertRunner)(itemGroup,
		                  &m_refTable0[m_refTable0Index],
		                  &m_refTable1[m_refTable1Index],
		                  m_refTable0Index, m_refTable1Index);
		assertPostForeach();
		return true;
	}
};

#define BASE AssertJoin<RefDataType0, RefDataType1>

template <typename RefDataType0, typename RefDataType1>
class AssertCrossJoin : public AssertJoin<RefDataType0, RefDataType1>
{
public:
	AssertCrossJoin(ItemTable *itemTable,
	                RefDataType0 *refTable0, RefDataType1 *refTable1,
	                size_t numRowsRefTable0, size_t numRowsRefTable1)
	: AssertJoin<RefDataType0, RefDataType1>
	    (itemTable, refTable0, refTable1,
	     numRowsRefTable0, numRowsRefTable1)
	{
	}

	virtual void assertPostForeach(void)
	{
		AssertJoin<RefDataType0, RefDataType1>::m_refTable1Index++;
		if (BASE::m_refTable1Index == BASE::m_numRowsRefTable1) {
			BASE::m_refTable1Index = 0;
			BASE::m_refTable0Index++;
		}
	}

};

template <typename RefDataType0, typename RefDataType1, class JoinedRowsChecker>
class AssertInnerJoin : public AssertJoin<RefDataType0, RefDataType1>
{
	IntIntPairVector  m_joinedRowsIndexVector;
	size_t            m_vectorIndex;

	void fillInnerJoinedRowsIndexVector(void)
	{
		JoinedRowsChecker checker;
		for (size_t i = 0; i <BASE::m_numRowsRefTable0; i++) {
			RefDataType0 &row0 = BASE::m_refTable0[i];
			for (size_t j = 0; j < BASE::m_numRowsRefTable1; j++) {
				RefDataType1 &row1 = BASE::m_refTable1[j];
				if (!checker(row0, row1))
					continue;
				IntIntPair pair = IntIntPair(i,j);
				m_joinedRowsIndexVector.push_back(pair);
			}
		}
	}

public:
	AssertInnerJoin(ItemTable *itemTable,
	                RefDataType0 *refTable0, RefDataType1 *refTable1,
	                size_t numRowsRefTable0, size_t numRowsRefTable1)
	: AssertJoin<RefDataType0, RefDataType1>
	    (itemTable, refTable0, refTable1,
	     numRowsRefTable0, numRowsRefTable1),
	  m_vectorIndex(0)
	{
		fillInnerJoinedRowsIndexVector();
		cppcut_assert_equal(m_joinedRowsIndexVector.size(),
		                    itemTable->getNumberOfRows());
	}

	virtual void assertPreForeach(void)
	{
		IntIntPair &idxVector =
		  m_joinedRowsIndexVector[m_vectorIndex++];
		BASE::m_refTable0Index = idxVector.first;
		BASE::m_refTable1Index = idxVector.second;
	}
};

#undef BASE

