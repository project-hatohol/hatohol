#include <stdexcept>
using namespace std;

#include "SQLProcessor.h"
#include <cutter.h>
#include <cppcutter.h>

namespace testSQLWhereElement {

const static int NUM_ELEM_POOL = 10;
static SQLWhereElement *g_elem[NUM_ELEM_POOL];
static SQLWhereElement *&x_elem = g_elem[0];
static SQLWhereElement *&y_elem = g_elem[1];
static SQLWhereElement *&z_elem = g_elem[2];

void teardown()
{
	for (int i = 0; i < NUM_ELEM_POOL; i++) {
		if (g_elem[i]) {
			delete g_elem[i];
			g_elem[i] = NULL;
		}
	}
}

static void whereColumnTestDestructor(SQLWhereColumn *wehreColumn, void *priv)
{
	bool *called = static_cast<bool *>(priv);
	*called = true;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_constructor(void)
{
	SQLWhereOperatorEqual op;
	cppcut_assert_equal(SQL_WHERE_OP_EQ, op.getType());
}

void test_isFullEmpty(void)
{
	SQLWhereElement elem;
	cppcut_assert_equal(false, elem.isFull());
}

void test_isFullHasLeft(void)
{
	SQLWhereElement elem;
	elem.setLeftHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isFull());
}

void test_isFullHasRight(void)
{
	SQLWhereElement elem;
	elem.setRightHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isFull());
}

void test_isFullHasLeftRight(void)
{
	SQLWhereElement elem;
	elem.setLeftHand(new SQLWhereElement());
	elem.setRightHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isFull());
}

void test_isFullHasOp(void)
{
	SQLWhereElement elem;
	elem.setOperator(new SQLWhereOperatorEqual());
	cppcut_assert_equal(false, elem.isFull());
}

void test_isFullHasOpLeft(void)
{
	SQLWhereElement elem;
	elem.setOperator(new SQLWhereOperatorEqual());
	elem.setLeftHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isFull());
}

void test_isFullHasOpRight(void)
{
	SQLWhereElement elem;
	elem.setOperator(new SQLWhereOperatorEqual());
	elem.setRightHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isFull());
}

void test_isFullHasOpLeftRight(void)
{
	SQLWhereElement elem;
	elem.setOperator(new SQLWhereOperatorEqual());
	elem.setLeftHand(new SQLWhereElement());
	elem.setRightHand(new SQLWhereElement());
	cppcut_assert_equal(true, elem.isFull());
}


void test_isEmptyEmpty(void)
{
	SQLWhereElement elem;
	cppcut_assert_equal(true, elem.isEmpty());
}

void test_isEmptyHasLeft(void)
{
	SQLWhereElement elem;
	elem.setLeftHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isEmpty());
}

void test_isEmptyHasRight(void)
{
	SQLWhereElement elem;
	elem.setRightHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isEmpty());
}

void test_isEmptyHasLeftRight(void)
{
	SQLWhereElement elem;
	elem.setLeftHand(new SQLWhereElement());
	elem.setRightHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isEmpty());
}

void test_isEmptyHasOp(void)
{
	SQLWhereElement elem;
	elem.setOperator(new SQLWhereOperatorEqual());
	cppcut_assert_equal(false, elem.isEmpty());
}

void test_isEmptyHasOpLeft(void)
{
	SQLWhereElement elem;
	elem.setOperator(new SQLWhereOperatorEqual());
	elem.setLeftHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isEmpty());
}

void test_isEmptyHasOpRight(void)
{
	SQLWhereElement elem;
	elem.setOperator(new SQLWhereOperatorEqual());
	elem.setRightHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isEmpty());
}

void test_isEmptyHasOpLeftRight(void)
{
	SQLWhereElement elem;
	elem.setOperator(new SQLWhereOperatorEqual());
	elem.setLeftHand(new SQLWhereElement());
	elem.setRightHand(new SQLWhereElement());
	cppcut_assert_equal(false, elem.isEmpty());
}

void test_setGetParent(void)
{
	x_elem = new SQLWhereElement();
	SQLWhereElement *elem0 = new SQLWhereElement();
	SQLWhereElement *elem1 = new SQLWhereElement();
	x_elem->setLeftHand(elem0);
	x_elem->setRightHand(elem1);
	cppcut_assert_equal((SQLWhereElement *)NULL, x_elem->getParent());
	cppcut_assert_equal(x_elem, elem0->getParent());
	cppcut_assert_equal(x_elem, elem1->getParent());
}

void test_whereColumnConstructor(void)
{
	int index = 8;
	string columnName = "foo";
	SQLWhereColumn elem(columnName, NULL, &index);
	cppcut_assert_equal(columnName, elem.getColumnName());
	cppcut_assert_equal(&index, static_cast<int *>(elem.getPrivateData()));
}

void test_whereColumnDestructor(void)
{
	bool called = false;
	{
		string columnName = "foo";
		SQLWhereColumn elem(columnName, NULL,
		                    &called, whereColumnTestDestructor);
		cppcut_assert_equal(false, called);
	}
	cppcut_assert_equal(true, called);
}

} // namespace testSQLWhereElement
