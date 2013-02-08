#ifndef testFormulaCommon_h
#define testFormulaCommon_h

#include <cutter.h>
#include "FormulaElement.h"
#include "FormulaOperator.h"
#include "Utils.h"

template<typename T>
static void assertFormulaElementType(FormulaElement *obj)
{
	cut_assert_not_null(obj);
	cppcut_assert_equal
	  (true, typeid(T) == typeid(*obj),
	   cut_message("type: *obj: %s (%s)",
	               DEMANGLED_TYPE_NAME(*obj), TYPE_NAME(*obj)));
}

#define assertFormulaComparatorEqual(X) \
cut_trace(assertFormulaElementType<FormulaComparatorEqual>(X))

#define assertFormulaOperatorAnd(X) \
cut_trace(assertFormulaElementType<FormulaOperatorAnd>(X))

#define assertTypeFormulaVariable(X) \
cut_trace(assertFormulaElementType<FormulaVariable>(X))

#define assertTypeFormulaValue(X) \
cut_trace(assertFormulaElementType<FormulaValue>(X))

#define assertTypeFormulaBetween(X) \
cut_trace(assertFormulaElementType<FormulaBetween>(X))

#define assertFormulaFuncMax(X) \
cut_trace(assertFormulaElementType<FormulaFuncMax>(X))

void _assertFormulaVariable(FormulaElement *elem, const char *expected);
#define assertFormulaVariable(EL, EXP) \
cut_trace(_assertFormulaVariable(EL, EXP))

void _assertFormulaValue(FormulaElement *elem, int expected);
void _assertFormulaValue(FormulaElement *elem, const char *expected);
#define assertFormulaValue(EL, EXP) \
cut_trace(_assertFormulaValue(EL, EXP))

void _assertFormulaBetween(FormulaElement *elem, string name, int v0, int v1);
#define assertFormulaBetween(X, N, V0, V1) \
cut_trace(_assertFormulaBetween(X, N, V0, V1))


void showTreeInfo(FormulaElement *formulaElement);

#endif // testFormulaCommon_h
