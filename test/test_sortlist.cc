#include "database/sortlist.h"
#include <iostream>
#include "gtest/gtest.h"

using namespace toydb;

TEST(testSortList, test0){
    SortList<int, std::string> sortlist;
    sortlist.insert_element(1, "hello");
    sortlist.insert_element(1, ",");
    sortlist.insert_element(1, "sortlist");
}