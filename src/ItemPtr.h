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

	ItemPtr(const ItemPtr<T> &itemPtr)
	: m_data(itemPtr.m_data) {
		if (m_data)
			m_data->ref();
	}

	ItemPtr(T *data, bool doRef = true)
	: m_data(data) {
		MLPL_INFO("[1] %s: doRef: %d, ref: %d\n", __PRETTY_FUNCTION__, doRef, data->getUsedCount());
		if (!data) {
			MLPL_WARN("data: NULL\n");
			return;
		}
		if (doRef)
			data->ref();
	}

	virtual ~ItemPtr() {
		if (m_data) {
			const_cast<T *>(m_data)->unref();
		}
	}

	T *operator->() const {
		return m_data;
	}

	operator T *() const {
		return m_data;
	}
	
	bool hasData(void) const {
		return (m_data != NULL);
	}

private:
	T *m_data;
};

#endif // #define ItemPtr_h


