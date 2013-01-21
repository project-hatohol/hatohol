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

class SQLWhereOperator {
public:
	SQLWhereOperator(SQLWhereOperatorType type);
	virtual ~SQLWhereOperator();
private:
	const SQLWhereOperatorType m_type;
};

enum SQLWhereElementType {
	SQL_WHERE_ELEM_ELEMENT,
	SQL_WHERE_ELEM_COLUMN,
	SQL_WHERE_ELEM_NUMBER,
	SQL_WHERE_ELEM_PAIRED_NUMBER,
};

class SQLWhereElement {
public:
	SQLWhereElement(void);
	virtual ~SQLWhereElement();

private:
	SQLWhereElement  *m_lefthand;
	SQLWhereOperator *m_operator;
	SQLWhereElement  *m_righthand;

	SQLWhereElement  *m_parent;
};

#endif // SQLWhereElement_h
