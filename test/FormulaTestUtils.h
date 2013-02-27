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

#define assertFormulaParenthesis(X) \
cut_trace(assertFormulaElementType<FormulaParenthesis>(X))

#define assertFormulaComparatorEqual(X) \
cut_trace(assertFormulaElementType<FormulaComparatorEqual>(X))

#define assertFormulaGreaterThan(X) \
cut_trace(assertFormulaElementType<FormulaGreaterThan>(X))

#define assertFormulaOperatorPlus(X) \
cut_trace(assertFormulaElementType<FormulaOperatorPlus>(X))

#define assertFormulaOperatorAnd(X) \
cut_trace(assertFormulaElementType<FormulaOperatorAnd>(X))

#define assertFormulaOperatorOr(X) \
cut_trace(assertFormulaElementType<FormulaOperatorOr>(X))

#define assertTypeFormulaVariable(X) \
cut_trace(assertFormulaElementType<FormulaVariable>(X))

#define assertTypeFormulaValue(X) \
cut_trace(assertFormulaElementType<FormulaValue>(X))

#define assertTypeFormulaBetween(X) \
cut_trace(assertFormulaElementType<FormulaBetween>(X))

#define assertTypeFormulaIn(X) \
cut_trace(assertFormulaElementType<FormulaIn>(X))

#define assertFormulaFuncMax(X) \
cut_trace(assertFormulaElementType<FormulaFuncMax>(X))

#define assertFormulaFuncCount(X) \
cut_trace(assertFormulaElementType<FormulaFuncCount>(X))

void _assertFormulaVariable(FormulaElement *elem, const char *expected);
#define assertFormulaVariable(EL, EXP) \
cut_trace(_assertFormulaVariable(EL, EXP))

void _assertFormulaValue(FormulaElement *elem, int expected);
void _assertFormulaValue(FormulaElement *elem, const char *expected);
#define assertFormulaValue(EL, EXP) \
cut_trace(_assertFormulaValue(EL, EXP))

void _assertFormulaBetween(FormulaElement *elem, int v0, int v1);
#define assertFormulaBetween(X, V0, V1) \
cut_trace(_assertFormulaBetween(X, V0, V1))

void _assertFormulaBetweenWithVarName(FormulaElement *elem, int v0, int v1, const char *name);
#define assertFormulaBetweenWithVarName(X, V0, V1, N) \
cut_trace(_assertFormulaBetweenWithVarName(X, V0, V1, N))

void _assertFormulaInWithVarName(FormulaElement *elem,
                                 vector<int> &expectedValues, const char *name);
void _assertFormulaInWithVarName(FormulaElement *elem,
                                 StringVector &expectedValues,
                                 const char *name);
#define assertFormulaInWithVarName(X, E, N) \
cut_trace(_assertFormulaInWithVarName(X, E, N))

template<class ElementType, class LeftElementType, typename LeftValueType,
         class RightElementType, typename RightValueType>
void _assertBinomialFormula(FormulaElement *formula,
                            LeftValueType expectedLeft,
                            RightValueType expectedRight)
{
	assertFormulaElementType<ElementType>(formula);

	assertFormulaVariable(formula->getLeftHand(), expectedLeft);

	const type_info &typeR = typeid(RightElementType);
	if (typeR == typeid(FormulaVariable)) {
		// 'reinterpret_cast' is somewhat ad-hoc.
		// We want to call the proper method gracefully.
		assertFormulaVariable(formula->getRightHand(),
		                      reinterpret_cast<const char *>
		                        (expectedRight));
	} else if (typeR == typeid(FormulaValue)) {
		assertFormulaValue(formula->getRightHand(), expectedRight);
	} else {
		cut_fail("unexpected right hand type: %s.", typeR.name());
	}
}

#define assertBinomialFormula(ET, EV, LET, LVT, LV, RET, RVT, RV) \
cut_trace((_assertBinomialFormula<ET, LET, LVT, RET, RVT>(EV, LV, RV)))

void showTreeInfo(FormulaElement *formulaElement);

#endif // testFormulaCommon_h
