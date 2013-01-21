#ifndef SQLWhereElement_h
#define SQLWhereElement_h

enum SQLWhereOperatorType {
	SQL_WHERE_OP_EQ,
	SQL_WHERE_OP_GT,
	SQL_WHERE_OP_GE,
	SQL_WHERE_OP_LT,
	SQL_WHERE_OP_LE,
	SQL_WHERE_OP_BETWEEN,
	SQL_WHERE_OP_AND,
	SQL_WHERE_OP_OR,
};

// ---------------------------------------------------------------------------
// class: SQLWhereOperator
// ---------------------------------------------------------------------------
class SQLWhereOperator {
public:
	SQLWhereOperator(SQLWhereOperatorType type);
	virtual ~SQLWhereOperator();
	const SQLWhereOperatorType getType(void) const;
private:
	const SQLWhereOperatorType m_type;
};

// ---------------------------------------------------------------------------
// class: SQLWhereOperatorEqual
// ---------------------------------------------------------------------------
class SQLWhereOperatorEqual : public SQLWhereOperator {
public:
	SQLWhereOperatorEqual();
	virtual ~SQLWhereOperatorEqual();
};

// ---------------------------------------------------------------------------
// class: SQLWhereELement
// ---------------------------------------------------------------------------
enum SQLWhereElementType {
	SQL_WHERE_ELEM_ELEMENT,
	SQL_WHERE_ELEM_COLUMN,
	SQL_WHERE_ELEM_NUMBER,
	SQL_WHERE_ELEM_STRING,
	SQL_WHERE_ELEM_PAIRED_NUMBER,
};

class SQLWhereElement {
public:
	SQLWhereElement(SQLWhereElementType elemType = SQL_WHERE_ELEM_ELEMENT);
	virtual ~SQLWhereElement();

	SQLWhereElement  *getLeftHand(void) const;
	SQLWhereElement  *getRightHand(void) const;
	SQLWhereOperator *getOperator(void) const;
	void setLeftHand(SQLWhereElement *whereElem);
	void setRightHand(SQLWhereElement *whereElem);
	void setOperator(SQLWhereOperator *whereOp);

private:
	SQLWhereElementType m_type;
	SQLWhereElement  *m_leftHand;
	SQLWhereOperator *m_operator;
	SQLWhereElement  *m_rightHand;

	SQLWhereElement  *m_parent;
};

// ---------------------------------------------------------------------------
// class: SQLWhereColumn
// ---------------------------------------------------------------------------
class SQLWhereColumn : public SQLWhereElement {
public:
	SQLWhereColumn(string &columnName);
	virtual ~SQLWhereColumn();
	const string &getValue(void);
private:
	string m_columnName;
};

// ---------------------------------------------------------------------------
// class: SQLWhereNumber
// ---------------------------------------------------------------------------
class SQLWhereNumber : public SQLWhereElement {
public:
	SQLWhereNumber(double value);
	virtual ~SQLWhereNumber();
	double getValue(void);
private:
	double m_value;
};

// ---------------------------------------------------------------------------
// class: SQLWhereString
// ---------------------------------------------------------------------------
class SQLWhereString : public SQLWhereElement {
public:
	SQLWhereString(string &str);
	virtual ~SQLWhereString();
	const string &getValue(void);
private:
	string m_str;
};

#endif // SQLWhereElement_h
