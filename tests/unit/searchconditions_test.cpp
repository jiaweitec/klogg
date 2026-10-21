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

#include "highlighterset.h"
#include "regularexpression.h"

namespace {
RegularExpressionPattern conditionsPattern( const klogg::vector<SearchCondition>& conditions,
                                            const QString& mainPattern = {} )
{
    RegularExpressionPattern pattern( mainPattern, true, false, false, false );
    pattern.conditions = conditions;
    return pattern;
}

bool matches( const RegularExpressionPattern& pattern, std::string_view line )
{
    RegularExpression expression( pattern );
    REQUIRE( expression.isValid() );
    return expression.createMatcher()->hasMatch( line );
}
} // namespace

SCENARIO( "Search conditions combine includes and excludes", "[searchconditions]" )
{
    GIVEN( "A or B, excluding C" )
    {
        const auto pattern = conditionsPattern(
            { { "err[0-9]+", false, true }, { "warn", false, true }, { "time.ut", true, true } } );

        REQUIRE( matches( pattern, "err42 happened" ) );
        REQUIRE( matches( pattern, "warn: disk" ) );
        REQUIRE_FALSE( matches( pattern, "err42 timeout" ) );
        REQUIRE_FALSE( matches( pattern, "warn timeout" ) );
        REQUIRE_FALSE( matches( pattern, "info line" ) );
    }

    GIVEN( "Only exclude conditions" )
    {
        const auto pattern = conditionsPattern( { { "debug", true, true }, { "trace", true, true } } );

        REQUIRE( matches( pattern, "info line" ) );
        REQUIRE_FALSE( matches( pattern, "debug line" ) );
        REQUIRE_FALSE( matches( pattern, "trace line" ) );
    }

    GIVEN( "Disabled conditions" )
    {
        const auto pattern = conditionsPattern(
            { { "alpha", false, true }, { "beta", false, false }, { "alpha2", true, false } } );

        REQUIRE( matches( pattern, "alpha2" ) );
        REQUIRE_FALSE( matches( pattern, "beta" ) );
    }

    GIVEN( "Only disabled conditions" )
    {
        const auto pattern = conditionsPattern( { { "alpha", false, false } } );
        REQUIRE( pattern.isEmpty() );
        REQUIRE_FALSE( pattern.hasActiveConditions() );
    }

    GIVEN( "Main search text combined with conditions" )
    {
        const auto pattern = conditionsPattern( { { "beta", false, true }, { "gamma", true, true } },
                                                "alpha" );

        REQUIRE( matches( pattern, "alpha" ) );
        REQUIRE( matches( pattern, "beta" ) );
        REQUIRE_FALSE( matches( pattern, "alpha gamma" ) );
        REQUIRE_FALSE( matches( pattern, "delta" ) );
    }

    GIVEN( "Inverse main search text acts as exclude" )
    {
        auto pattern = conditionsPattern( { { "beta", false, true } }, "gamma" );
        pattern.isExclude = true;

        REQUIRE( matches( pattern, "beta" ) );
        REQUIRE_FALSE( matches( pattern, "beta gamma" ) );
    }

    GIVEN( "Conditions with quotes and special characters" )
    {
        const auto pattern = conditionsPattern( { { "\"id\": \"[0-9]+\"", false, true },
                                                  { "path\\\\tmp", false, true },
                                                  { "a and not\\(b\\) or \"", true, true } } );

        REQUIRE( matches( pattern, R"({"id": "123"})" ) );
        REQUIRE( matches( pattern, R"(C:\path\tmp)" ) );
        REQUIRE_FALSE( matches( pattern, R"({"id": "abc"})" ) );
        REQUIRE_FALSE( matches( pattern, R"({"id": "1"} a and not(b) or ")" ) );
    }

    GIVEN( "Invalid regex in a condition" )
    {
        RegularExpression expression( conditionsPattern( { { "foo(", false, true } } ) );
        REQUIRE_FALSE( expression.isValid() );
    }

    GIVEN( "Same text with different conditions" )
    {
        REQUIRE_FALSE( conditionsPattern( { { "a", false, true } } )
                       == conditionsPattern( { { "a", true, true } } ) );
    }
}

SCENARIO( "Search conditions highlight matched parts", "[searchconditions]" )
{
    const QColor mainColor( Qt::yellow );
    const auto pattern = conditionsPattern(
        { { "err[0-9]+", false, true }, { "disk", false, true }, { "timeout", true, true },
          { "unused", false, false } },
        "main" );

    const auto highlighters = searchPatternHighlighters( pattern, mainColor, false );
    REQUIRE( highlighters.size() == 3 );
    REQUIRE( highlighters[ 0 ].backColor() == mainColor );
    REQUIRE( highlighters[ 1 ].backColor() != highlighters[ 0 ].backColor() );
    REQUIRE( highlighters[ 2 ].backColor() != highlighters[ 1 ].backColor() );

    const QString line = "main err42 on disk, err7";

    klogg::vector<HighlightedMatch> matches;
    REQUIRE( highlighters[ 0 ].matchLine( line, matches ) );
    REQUIRE( matches.size() == 1 );
    REQUIRE( matches[ 0 ].startColumn().get() == 0 );
    REQUIRE( matches[ 0 ].size().get() == 4 );

    REQUIRE( highlighters[ 1 ].matchLine( line, matches ) );
    REQUIRE( matches.size() == 2 );
    REQUIRE( matches[ 0 ].startColumn().get() == 5 );
    REQUIRE( matches[ 0 ].size().get() == 5 );
    REQUIRE( matches[ 1 ].startColumn().get() == 20 );
    REQUIRE( matches[ 1 ].size().get() == 4 );
    REQUIRE( matches[ 0 ].backColor() == highlighters[ 1 ].backColor() );

    REQUIRE( highlighters[ 2 ].matchLine( line, matches ) );
    REQUIRE( matches.size() == 1 );
    REQUIRE( matches[ 0 ].startColumn().get() == 14 );

    GIVEN( "Boolean main pattern" )
    {
        auto booleanPattern = conditionsPattern( { { "disk", false, true } }, "\"main\"" );
        booleanPattern.isBoolean = true;
        const auto booleanHighlighters = searchPatternHighlighters( booleanPattern, mainColor, false );
        REQUIRE( booleanHighlighters.size() == 1 );
        REQUIRE( booleanHighlighters[ 0 ].pattern() == "disk" );
    }
}
