// Copyright Contributors to the DNF5 project.
// Copyright Contributors to the libdnf project.
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of libdnf: https://github.com/rpm-software-management/libdnf/
//
// Libdnf is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Libdnf is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libdnf.  If not, see <https://www.gnu.org/licenses/>.

#ifndef DNF5_TEST_REPOGRAPH_HPP
#define DNF5_TEST_REPOGRAPH_HPP

#include "base_test_case.hpp"

#include <cppunit/extensions/HelperMacros.h>


/// Unit tests for the repograph dot emitter. Builds a real
/// libdnf5::Base loaded with a tiny libsolv testcase repo and asserts
/// on the dot output of `repograph::emit_graph`.
class RepographTest : public BaseTestCase {
    CPPUNIT_TEST_SUITE(RepographTest);
    CPPUNIT_TEST(test_name_merging_and_dependency_types);
    CPPUNIT_TEST(test_provider_selection_is_deterministic);
    CPPUNIT_TEST(test_self_provides_are_checked_per_package);
    CPPUNIT_TEST(test_versioned_requirements);
    CPPUNIT_TEST(test_provider_cache_reuses_missing_providers);
    CPPUNIT_TEST(test_provider_cache_is_per_graph);
    CPPUNIT_TEST(test_rich_and_requires_are_reported);
    CPPUNIT_TEST(test_rich_conditional_requires_are_reported);
    CPPUNIT_TEST(test_rich_dependency_cache_reuse);
    CPPUNIT_TEST(test_rich_recommends_follow_configuration);
    CPPUNIT_TEST(test_rich_suggests_are_not_reported);
    CPPUNIT_TEST(test_rich_dependencies_stay_in_node_set);
    CPPUNIT_TEST(test_emits_empty_graph);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_name_merging_and_dependency_types();
    void test_provider_selection_is_deterministic();
    void test_self_provides_are_checked_per_package();
    void test_versioned_requirements();
    void test_provider_cache_reuses_missing_providers();
    void test_provider_cache_is_per_graph();
    void test_rich_and_requires_are_reported();
    void test_rich_conditional_requires_are_reported();
    void test_rich_dependency_cache_reuse();
    void test_rich_recommends_follow_configuration();
    void test_rich_suggests_are_not_reported();
    void test_rich_dependencies_stay_in_node_set();
    void test_emits_empty_graph();
};


#endif  // DNF5_TEST_REPOGRAPH_HPP
