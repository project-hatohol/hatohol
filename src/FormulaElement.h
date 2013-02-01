#ifndef FormulaElement_h 
#define FormulaElement_h

#include <vector>
using namespace std;

#include "ItemDataPtr.h"
#include "FormulaOperator.h"

// ---------------------------------------------------------------------------
// class: FormulaElement
// ---------------------------------------------------------------------------
class FormulaElement {
public:
	FormulaElement(void);
	virtual ~FormulaElement();
	void setLeftHand(FormulaElement *elem);
	void setRightHand(FormulaElement *elem);
	void setOperator(FormulaOperator *op);

	FormulaElement *getLeftHand(void) const;
	FormulaElement *getRightHand(void) const;

	virtual ItemDataPtr evaluate(void);

private:
	FormulaElement  *m_leftHand;
	FormulaElement  *m_rightHand;
	FormulaOperator *m_operator;
	FormulaElement  *m_parent;
};

typedef vector<FormulaElement *>       FormulaElementVector;
typedef FormulaElementVector::iterator FormulaElementVectorIterator;

// ---------------------------------------------------------------------------
// class: FormulaVariable
// ---------------------------------------------------------------------------
class FormulaVariable;
class FormulaVariableDataGetter {
public:
	virtual ~FormulaVariableDataGetter() {}
	virtual ItemDataPtr getData(void) = 0;
};

typedef FormulaVariableDataGetter *
(*FormulaVariableDataGetterFactory)(string &name, void *priv);

class FormulaVariable : public FormulaElement {
public:
	FormulaVariable(string &name,
	              FormulaVariableDataGetter *variableDataGetter);
	virtual ~FormulaVariable();
	virtual ItemDataPtr evaluate(void);

	const string &getName(void) const;
	FormulaVariableDataGetter *getFormulaVariableGetter(void) const;

private:
	string                     m_name;
	FormulaVariableDataGetter *m_variableGetter;
};

#endif // FormulaElement_h
