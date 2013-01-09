#ifndef ItemPtr_h
#define ItemPtr_h

#include <cstdio>
#include "Logger.h"
using namespace mlpl;

template<class T>
class ItemPtr {
public:
	ItemPtr(void) : m_data(NULL) {
	}

	ItemPtr(const ItemPtr<T> &itemPtr) : m_data(itemPtr.m_data) {
		if (m_data)
			m_data->ref();
	}

	ItemPtr(const T *data) : m_data(data) {
		if (!data) {
			MLPL_WARN("data: NULL\n");
			return;
		}
		data->ref();
	}

	virtual ~ItemPtr() {
		if (m_data) {
			const_cast<T *>(m_data)->unref();
		}
	}

	const T *operator->() const {
		return m_data;
	}

	operator const T *() const {
		return m_data;
	}

	bool hasData(void) const {
		return (m_data != NULL);
	}

private:
	const T *m_data;
};

#endif // #define ItemPtr_h


