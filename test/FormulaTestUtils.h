#ifndef testFormulaCommon_h
#define testFormulaCommon_h

#include <cutter.h>
#include "FormulaElement.h"
#include "Utils.h"

template<typename T>
static void assertFormulaElementType(FormulaElement *obj)
{
	cppcut_assert_equal
	  (true, typeid(T) == typeid(*obj),
	   cut_message("type: *obj: %s (%s)",
	               DEMANGLED_TYPE_NAME(*obj), TYPE_NAME(*obj)));
}

#define assertFormulaCompareEqual(X) \
cut_trace(assertFormulaElementType<FormulaCompareEqual>(X))

#define assertTypeFormulaVariable(X) \
cut_trace(assertFormulaElementType<FormulaVariable>(X))

#define assertTypeFormulaNumber(X) \
cut_trace(assertFormulaElementType<FormulaNumber>(X))

#define assertFormulaFuncMax(X) \
cut_trace(assertFormulaElementType<FormulaFuncMax>(X))

void _assertFormulaVariable(FormulaElement *elem, const char *expected);
#define assertFormulaVariable(EL, EXP) \
cut_trace(_assertFormulaVariable(EL, EXP))

void _assertFormulaNumber(FormulaElement *elem, int expected);
#define assertFormulaNumber(EL, EXP) \
cut_trace(_assertFormulaNumber(EL, EXP))

#endif // testFormulaCommon_h
