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


/// Integration tests for the repograph dot emitter. Builds a real
/// libdnf5::Base loaded with a tiny libsolv testcase repo and asserts
/// on the dot output of `repograph::emit_graph`.
class RepographTest : public BaseTestCase {
    CPPUNIT_TEST_SUITE(RepographTest);
    CPPUNIT_TEST(test_emits_graph_document);
    CPPUNIT_TEST(test_nodes_are_name_merged);
    CPPUNIT_TEST(test_strong_required_edges_present);
    CPPUNIT_TEST(test_multiple_providers_are_deterministic);
    CPPUNIT_TEST(test_recommends_excluded);
    CPPUNIT_TEST(test_recommends_included);
    CPPUNIT_TEST(test_suggests_excluded);
    CPPUNIT_TEST(test_no_self_edges);
    CPPUNIT_TEST(test_self_provider_does_not_hide_alternative);
    CPPUNIT_TEST(test_no_edges_to_missing_providers);
    CPPUNIT_TEST(test_output_is_deterministic);
    CPPUNIT_TEST(test_emits_empty_graph);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_emits_graph_document();
    void test_nodes_are_name_merged();
    void test_strong_required_edges_present();
    void test_multiple_providers_are_deterministic();
    void test_recommends_excluded();
    void test_recommends_included();
    void test_suggests_excluded();
    void test_no_self_edges();
    void test_self_provider_does_not_hide_alternative();
    void test_no_edges_to_missing_providers();
    void test_output_is_deterministic();
    void test_emits_empty_graph();
};


#endif  // DNF5_TEST_REPOGRAPH_HPP
