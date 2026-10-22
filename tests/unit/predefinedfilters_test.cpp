/*
 * Copyright (C) 2026 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <catch2/catch.hpp>

#include "predefinedfilterscombobox.h"

SCENARIO( "Predefined filters selection survives search pattern round trip",
          "[predefinedfilters]" )
{
    const QList<PredefinedFilter> filters = { { "Errors", "error|fatal", true },
                                              { "Dots", "a.b", false },
                                              { "Quoted", "say \"hi\"", false },
                                              { "Warnings", "warn[0-9]+", true } };

    const auto useLogicalCombining = GENERATE( false, true );
    const auto useRegexp = GENERATE( true, false );

    GIVEN( "Several selected filters" )
    {
        const auto selection = GENERATE( QList<int>{ 0, 1 }, QList<int>{ 0, 3 },
                                         QList<int>{ 1, 2, 3 }, QList<int>{ 0, 1, 2, 3 },
                                         QList<int>{ 2 } );
        QList<PredefinedFilter> selected;
        for ( auto index : selection ) {
            selected.append( filters.at( index ) );
        }

        const auto searchPattern = PredefinedFiltersComboBox::combineFilters(
            selected, useRegexp, useLogicalCombining );

        THEN( "All of them are checked again" )
        {
            REQUIRE( PredefinedFiltersComboBox::matchingFilters(
                         searchPattern, filters, useRegexp, useLogicalCombining )
                     == selection );
        }
    }

    GIVEN( "Search pattern edited by user" )
    {
        REQUIRE( PredefinedFiltersComboBox::matchingFilters( "something else", filters, useRegexp,
                                                             useLogicalCombining )
                     .isEmpty() );
        REQUIRE( PredefinedFiltersComboBox::matchingFilters( {}, filters, useRegexp,
                                                             useLogicalCombining )
                     .isEmpty() );
    }
}

SCENARIO( "Predefined filters search pattern format", "[predefinedfilters]" )
{
    const QList<PredefinedFilter> filters
        = { { "A", "a.b", false }, { "B", "say \"hi\"", false }, { "C", "c|d", true } };

    REQUIRE( PredefinedFiltersComboBox::combineFilters( filters, true, false )
             == R"(a\.b|say\ \"hi\"|c|d)" );
    REQUIRE( PredefinedFiltersComboBox::combineFilters( filters, true, true )
             == R"("a\.b" or "say\ \\"hi\\"" or "c|d")" );
    REQUIRE( PredefinedFiltersComboBox::combineFilters( filters, false, true )
             == R"("a.b" or "say \"hi\"" or "c|d")" );
}
