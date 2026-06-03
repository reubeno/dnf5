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

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class RepographTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(RepographTest);
    CPPUNIT_TEST(test_all_options_registered);
    CPPUNIT_TEST(test_use_system_option);
    CPPUNIT_TEST(test_closed_option);
    CPPUNIT_TEST(test_include_reverse_weak_option);
    CPPUNIT_TEST(test_resolver_option);
    CPPUNIT_TEST(test_node_label_option);
    CPPUNIT_TEST(test_edge_label_option);
    CPPUNIT_TEST(test_format_option);
    CPPUNIT_TEST(test_output_option);
    CPPUNIT_TEST(test_json_option);
    CPPUNIT_TEST(test_specs_positional);
    CPPUNIT_TEST(test_default_markers_in_help);
    CPPUNIT_TEST(test_edge_kind_strings);
    CPPUNIT_TEST(test_dot_emit_empty);
    CPPUNIT_TEST(test_dot_emit_simple);
    CPPUNIT_TEST(test_dot_emit_quoting);
    CPPUNIT_TEST(test_dot_emit_annotations_modes);
    CPPUNIT_TEST(test_json_emit_empty);
    CPPUNIT_TEST(test_json_emit_simple);
    CPPUNIT_TEST(test_json_emit_includes_members);
    CPPUNIT_TEST(test_json_emit_includes_unresolved);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void test_all_options_registered();
    void test_use_system_option();
    void test_closed_option();
    void test_include_reverse_weak_option();
    void test_resolver_option();
    void test_node_label_option();
    void test_edge_label_option();
    void test_format_option();
    void test_output_option();
    void test_json_option();
    void test_specs_positional();
    void test_default_markers_in_help();
    void test_edge_kind_strings();
    void test_dot_emit_empty();
    void test_dot_emit_simple();
    void test_dot_emit_quoting();
    void test_dot_emit_annotations_modes();
    void test_json_emit_empty();
    void test_json_emit_simple();
    void test_json_emit_includes_members();
    void test_json_emit_includes_unresolved();
};

#endif  // DNF5_TEST_REPOGRAPH_HPP
