#ifndef SQLWhereElement_h
#define SQLWhereElement_h

class SQLWhereOperator {
};

class SQLWhereElement {
public:
	SQLWhereElement(void);
	virtual ~SQLWhereElement();

private:
	SQLWhereElement  *m_lefthand;
	SQLWhereOperator *m_operator;
	SQLWhereElement  *m_righthand;
};

#endif // SQLWhereElement_h
