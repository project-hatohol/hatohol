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

void teardown()
{
	for (int i = 0; i < NUM_ELEM_POOL; i++) {
		if (g_elem[i]) {
			delete g_elem[i];
			g_elem[i] = NULL;
		}
	}
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

void test_setGetParent(void)
{
	x_elem = new SQLWhereElement();
	cppcut_assert_equal((SQLWhereElement *)NULL, x_elem->getParent());

	y_elem = new SQLWhereElement();
	x_elem->setParent(y_elem);
	cppcut_assert_equal(y_elem, x_elem->getParent());
	y_elem = NULL; // to avoid teardown() from calling destructor directly
}


} // namespace testSQLWhereElement
