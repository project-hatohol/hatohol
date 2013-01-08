#ifndef ItemPtr_h
#define ItemPtr_h

#include <cstdio>

template<class T>
class ItemPtr {
public:
	ItemPtr(void) : m_data(NULL) {
	}

	ItemPtr(const T *data) : m_data(data) {
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

	bool hasData(void) const {
		return (m_data != NULL);
	}

private:
	const T *m_data;
};

#endif // #define ItemPtr_h


