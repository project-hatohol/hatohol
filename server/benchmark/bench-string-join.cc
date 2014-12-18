/*
 * Copyright (C) 2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <glib.h>
#include <iostream>
#include <cstring>
#include <StringUtils.h>
#include <SeparatorInjector.h>
#include <Params.h>

using namespace std;
using namespace mlpl;

struct BenchmarkItem {
	string m_label;
	int m_n;

	BenchmarkItem(const string &label, const int &n)
	: m_label(label),
	  m_n(n)
	{
	}

	virtual ~BenchmarkItem() {
	}

	virtual void setup(void) {
	}
	virtual void run(void) {
	}
	virtual void teardown(void) {
	}
};

class BenchmarkReporter {
public:
	BenchmarkReporter()
	: m_items(),
	  m_maxLabelLength(0)
	{
	}

	void registerItem(BenchmarkItem &item) {
		m_items.push_back(&item);
		if (item.m_label.size() > m_maxLabelLength) {
			m_maxLabelLength = item.m_label.size();
		}
	}

	void run() {
		reportHeader();

		for (list<BenchmarkItem *>::iterator it = m_items.begin();
		     it != m_items.end();
		     ++it) {
			BenchmarkItem *item = *it;
			runItem(item);
		}
	}
private:
	list<BenchmarkItem *> m_items;
	unsigned int m_maxLabelLength;

	void reportHeader(void) {
		using StringUtils::sprintf;
		cout << sprintf("%*s: ", m_maxLabelLength, "Label");
		cout << "    Total";
		cout << " ";
		cout << "  Average";
		cout << " ";
		cout << "   Median";
		cout << endl;
	}

	void runItem(BenchmarkItem *item) {
		reportLabel(item->m_label);

		list<double> elapsedTimes;
		GTimer *timer = g_timer_new();
		for (int i = 0; i < item->m_n; i++) {
			item->setup();
			g_timer_start(timer);
			item->run();
			g_timer_stop(timer);
			elapsedTimes.push_back(g_timer_elapsed(timer, NULL));
			item->teardown();
		}
		g_timer_destroy(timer);
		reportElapsedTimeStatistics(elapsedTimes);
		cout << endl;
	}

	void reportLabel(const string &label) {
		using StringUtils::sprintf;
		cout << sprintf("%*s: ", m_maxLabelLength, label.c_str());
	}

	void reportElapsedTimeStatistics(list<double> &elapsedTimes) {
		reportElapsedTimeTotal(elapsedTimes);
		cout << " ";
		reportElapsedTimeAverage(elapsedTimes);
		cout << " ";
		reportElapsedTimeMedian(elapsedTimes);
	}

	void reportElapsedTimeTotal(list<double> &elapsedTimes) {
		reportElapsedTime(computeTotalElapsedTime(elapsedTimes));
	}

	double computeTotalElapsedTime(list<double> &elapsedTimes) {
		double total = 0.0;

		for (list<double>::iterator it = elapsedTimes.begin();
		     it != elapsedTimes.end();
		     ++it) {
			double &elapsedTime = *it;
			total += elapsedTime;
		}

		return total;
	}

	void reportElapsedTimeAverage(list<double> &elapsedTimes) {
		reportElapsedTime(computeAverageElapsedTime(elapsedTimes));
	}

	double computeAverageElapsedTime(list<double> &elapsedTimes) {
		double total = computeTotalElapsedTime(elapsedTimes);
		return total / elapsedTimes.size();
	}

	void reportElapsedTimeMedian(list<double> &elapsedTimes) {
		reportElapsedTime(computeMedianElapsedTime(elapsedTimes));
	}

	static bool compareElapsedTime(const double &elapsedTime1,
				const double &elapsedTime2)
	{
		return elapsedTime1 > elapsedTime2;
	}

	double computeMedianElapsedTime(list<double> &elapsedTimes) {
		elapsedTimes.sort(compareElapsedTime);

		int i = 0;
		int median = elapsedTimes.size() / 2;
		for (list<double>::iterator it = elapsedTimes.begin();
		     it != elapsedTimes.end();
		     ++it, i++) {
			if (i < median) {
				continue;
			}
			double &elapsedTime = *it;
			return elapsedTime;
		}

		return 0.0;
	}

	void reportElapsedTime(const double &elapsedTime) {
		using StringUtils::sprintf;

		double oneSecond = 1.0;
		double oneMillisecond = oneSecond / 1000.0;
		double oneMicrosecond = oneMillisecond / 1000.0;

		if (elapsedTime < oneMicrosecond) {
			cout << sprintf("(%.3fus)",
					elapsedTime * 1000.0 * 1000.0);
		} else if (elapsedTime < oneMillisecond) {
			cout << sprintf("(%.3fms)", elapsedTime * 1000.0);
		} else {
			cout << sprintf("(%.3fs) ", elapsedTime);
		}
	}
};

int
main(int argc, char **argv)
{
	BenchmarkReporter reporter;
	int n = 10000;
	StringList elements;
	elements.push_back("A");
	elements.push_back("B");
	elements.push_back("C");
	elements.push_back("D");
	elements.push_back("E");

	struct StringJoinBenchmarkItem : public BenchmarkItem {
		StringJoinBenchmarkItem(int n, StringList &elements)
		: BenchmarkItem("string::join", n),
		  m_elements(elements)
		{
		}

		virtual void run(void) {
			string all;
			all += join(", ", m_elements);
			all += join(", ", m_elements);
			all += join(", ", m_elements);
		}

		string join(const char *separator, StringList &elements) {
			string joined;
			bool first = false;
			for (StringListIterator it = elements.begin();
			     it != elements.end();
			     ++it) {
				if (first) {
					first = false;
				} else {
					joined += separator;
				}
				std::string &element = *it;
				joined += element;
			}
			return joined;
		}

		StringList &m_elements;
	} stringJoinBenchmarkItem(n, elements);
	reporter.registerItem(stringJoinBenchmarkItem);

	struct StringFastJoinBenchmarkItem : public BenchmarkItem {
		StringFastJoinBenchmarkItem(int n, StringList &elements)
		: BenchmarkItem("string::fast_join", n),
		  m_elements(elements)
		{
		}

		virtual void run(void) {
			string all;
			all += join(", ", m_elements);
			all += join(", ", m_elements);
			all += join(", ", m_elements);
		}

		size_t lookupTotalLength(const char *separator, StringList &elements) {
			size_t length = 0;
			for (StringListIterator it = elements.begin();
			     it != elements.end();
			     ++it) {
				length += it->size();
			}
			length += (elements.size() - 1) * strlen(separator) * 2;
			return length;
		}

		string join(const char *separator, StringList &elements) {
			string joined;
			joined.reserve(lookupTotalLength(separator, elements));
			bool first = false;
			for (StringListIterator it = elements.begin();
			     it != elements.end();
			     ++it) {
				if (first) {
					first = false;
				} else {
					joined += separator;
				}
				std::string &element = *it;
				joined += element;
			}
			return joined;
		}

		StringList &m_elements;
	} stringFastJoinBenchmarkItem(n, elements);
	reporter.registerItem(stringFastJoinBenchmarkItem);

	struct SeparatorInjectorBenchmarkItem : public BenchmarkItem {
		SeparatorInjectorBenchmarkItem(int n, StringList &elements)
		: BenchmarkItem("SeparatorInjector", n),
		  m_elements(elements)
		{
		}

		virtual void run(void) {
			string all;
			SeparatorInjector injector(", ");
			for (StringListIterator it = m_elements.begin();
			     it != m_elements.end();
			     ++it) {
				injector(all);
				std::string &element = *it;
				all += element;
			}
			injector.clear();
			for (StringListIterator it = m_elements.begin();
			     it != m_elements.end();
			     ++it) {
				injector(all);
				std::string &element = *it;
				all += element;
			}
			injector.clear();
			for (StringListIterator it = m_elements.begin();
			     it != m_elements.end();
			     ++it) {
				injector(all);
				std::string &element = *it;
				all += element;
			}
		}

		StringList &m_elements;
	} separatorInjectorBenchmarkItem(n, elements);
	reporter.registerItem(separatorInjectorBenchmarkItem);

	reporter.run();

	return EXIT_SUCCESS;
}
