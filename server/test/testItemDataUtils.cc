#include <cppcutter.h>

#include "StringUtils.h"
using namespace mlpl;

#include "ItemDataUtils.h"

namespace testItemDataUtils {

void test_createAsNumberInt(void)
{
	int number = 50;
	string numberStr = StringUtils::sprintf("%d", number);
	ItemDataPtr dataPtr = ItemDataUtils::createAsNumber(numberStr);
	cppcut_assert_equal(true, dataPtr.hasData());
	const ItemInt *itemInt = dynamic_cast<const ItemInt *>(&*dataPtr);
	cppcut_assert_not_null(itemInt);
	cppcut_assert_equal(number, itemInt->get());
}

void test_createAsNumberInvalid(void)
{
	string word = "ABC";
	ItemDataPtr dataPtr = ItemDataUtils::createAsNumber(word);
	cppcut_assert_equal(false, dataPtr.hasData());
}

} // namespace testItemDataUtils


