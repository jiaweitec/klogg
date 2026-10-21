/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
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

#ifndef KLOGG_REGULAR_EXPRESSION_PATTERN_H
#define KLOGG_REGULAR_EXPRESSION_PATTERN_H

#include <QRegularExpression>
#include <algorithm>
#include <QString>
#include <cstdint>
#include <qregularexpression.h>
#include <string>

#include "containers.h"
#include "uuid.h"

struct SearchCondition {
    QString pattern;
    bool isExclude = false;
    bool isEnabled = true;

    bool operator==( const SearchCondition& other ) const
    {
        return std::tie( pattern, isExclude, isEnabled )
               == std::tie( other.pattern, other.isExclude, other.isEnabled );
    }
};

struct RegularExpressionPattern {

    QString pattern;
    bool isCaseSensitive = true;
    bool isExclude = false;
    bool isBoolean = false;
    bool isPlainText = false;
    bool isPrefilter = false;
    klogg::vector<SearchCondition> conditions;

    RegularExpressionPattern() = default;

    explicit RegularExpressionPattern( const QString& expression )
        : RegularExpressionPattern( expression, true, false, false, false )
    {
    }

    RegularExpressionPattern( const QString& expression, bool caseSensitive, bool inverse,
                              bool boolean, bool plainText )
        : pattern( expression )
        , isCaseSensitive( caseSensitive )
        , isExclude( inverse )
        , isBoolean( boolean )
        , isPlainText( plainText )
        , patternId_( nextId() )
    {
    }

    bool hasActiveConditions() const
    {
        return std::any_of( conditions.begin(), conditions.end(), []( const auto& c ) {
            return c.isEnabled && !c.pattern.isEmpty();
        } );
    }

    bool isEmpty() const
    {
        return pattern.isEmpty() && !hasActiveConditions();
    }

    klogg::vector<RegularExpressionPattern> includePatterns() const
    {
        klogg::vector<RegularExpressionPattern> includes;
        if ( !pattern.isEmpty() && !isBoolean && !isExclude ) {
            includes.emplace_back( pattern, isCaseSensitive, false, false, isPlainText );
        }
        for ( const auto& c : conditions ) {
            if ( c.isEnabled && !c.isExclude && !c.pattern.isEmpty() ) {
                includes.emplace_back( c.pattern, isCaseSensitive, false, false, isPlainText );
            }
        }
        return includes;
    }

    std::string id() const
    {
        return patternId_;
    }

    explicit operator QRegularExpression() const
    {
        auto patternOptions = QRegularExpression::UseUnicodePropertiesOption
                              | QRegularExpression::DontCaptureOption;

        if ( !isCaseSensitive ) {
            patternOptions |= QRegularExpression::CaseInsensitiveOption;
        }

        auto finalPattern = pattern;
        if ( isPlainText ) {
            finalPattern = QRegularExpression::escape( pattern );
        }

        return QRegularExpression( finalPattern, patternOptions );
    }

    bool operator==( const RegularExpressionPattern& other ) const
    {
        return std::tie( pattern, isCaseSensitive, isExclude, isBoolean, isPlainText )
                   == std::tie( other.pattern, other.isCaseSensitive, isExclude, isBoolean,
                                isPlainText )
               && conditions == other.conditions;
    }

  private:
    static std::string nextId()
    {
        static std::atomic<uint> counter_ = 0;
        return std::string{ "p_" } + std::to_string( counter_++ );
    }

    std::string patternId_;
};

#endif