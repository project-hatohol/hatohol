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
// class: FormulaColumn
// ---------------------------------------------------------------------------
class FormulaColumn;
class FormulaColumnDataGetter {
public:
	virtual ~FormulaColumnDataGetter() {}
	virtual ItemDataPtr getData(void) = 0;
	//virtual void
	//  setFormulaColumnInstance(FormulaColumn *formulaColumn) = 0;
};

typedef FormulaColumnDataGetter *
(*FormulaColumnDataGetterFactory)(string &name, void *priv);

class FormulaColumn : public FormulaElement {
public:
	FormulaColumn(string &name,
	              FormulaColumnDataGetter *columnDataGetter);
	virtual ~FormulaColumn();
	virtual ItemDataPtr evaluate(void);

	const string &getName(void) const;
	FormulaColumnDataGetter *getFormulaColumnGetter(void) const;

private:
	string                   m_name;
	FormulaColumnDataGetter *m_columnGetter;
};

#endif // FormulaElement_h
